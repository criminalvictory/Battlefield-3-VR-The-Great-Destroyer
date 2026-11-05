#include "gunshape.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <vector>

#include "log.h"

namespace {

struct Field {
    int n[3] = {};
    float origin[3] = {}, cell = 0.0f;
    std::vector<int16_t> tenths;  // signed tenths of a millimetre, x fastest
};
std::mutex g_mutex;
std::map<std::string, std::unique_ptr<Field>> g_fields;  // each gun's, null when it has none
std::atomic<const Field*> g_current{nullptr};

// The field from the file at path; null when it can't.
std::unique_ptr<Field> LoadFile(const wchar_t* path, const char* name) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"rb") || !f) {
        Log("Gun shape: guns\\%s.bin isn't there (tools/gunshape.exe makes it); the hand goes through this gun", name);
        return nullptr;
    }
    char magic[4] = {};
    int32_t version = 0;
    auto loaded = std::make_unique<Field>();
    Field& field = *loaded;
    // Version 2: tenths of a millimetre in an int16 a cell; version 1 (whole millimetres in a byte)
    // was too coarse: the glove pushed out of the gun by it jittered.
    bool ok = fread(magic, 1, 4, f) == 4 && !std::memcmp(magic, "GSDF", 4) && fread(&version, 4, 1, f) == 1 &&
              version == 2 && fread(field.n, 4, 3, f) == 3 && fread(field.origin, 4, 3, f) == 3 &&
              fread(&field.cell, 4, 1, f) == 1 && field.cell > 0.0005f && field.n[0] > 1 && field.n[1] > 1 &&
              field.n[2] > 1 && field.n[0] < 2000 && field.n[1] < 2000 && field.n[2] < 2000;
    if (ok) {
        const uint64_t cells = static_cast<uint64_t>(field.n[0]) * field.n[1] * field.n[2];
        try {
            ok = cells <= field.tenths.max_size();
            if (ok) {
                field.tenths.resize(static_cast<size_t>(cells));
                ok = fread(field.tenths.data(), 2, field.tenths.size(), f) == field.tenths.size();
            }
        } catch (const std::bad_alloc&) {  // a damaged file, or no room left in 32 bits: no shape, not the game gone
            ok = false;
        }
    }
    fclose(f);
    if (!ok) {
        Log("Gun shape: guns\\%s.bin can't be read (version %d; tools/gunshape.exe writes version 2)", name, version);
        return nullptr;
    }
    Log("Gun shape: %s's shape loaded, %d x %d x %d cells of %.0f mm", name, field.n[0], field.n[1], field.n[2], field.cell * 1000.0f);
    return loaded;
}

// guns\<name>.bin next to the module (the mod's DLL).
std::unique_ptr<Field> LoadBesideModule(const char* name) {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&LoadBesideModule), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        *(slash + 1) = 0;
    wchar_t file[64];
    swprintf_s(file, L"guns\\%S.bin", name);
    wcscat_s(path, file);
    return LoadFile(path, name);
}

// The field at p by trilinear interpolation (metres); false off the grid.
bool Sample(const Field& field, const float* p, float& out) {
    float f[3];
    int i[3];
    for (int k = 0; k < 3; ++k) {
        const float c = (p[k] - field.origin[k]) / field.cell;
        i[k] = static_cast<int>(std::floor(c));
        f[k] = c - static_cast<float>(i[k]);
        if (i[k] < 0 || i[k] >= field.n[k] - 1)
            return false;
    }
    auto at = [&](int x, int y, int z) {
        return static_cast<float>(field.tenths[(static_cast<size_t>(z) * field.n[1] + y) * field.n[0] + x]);
    };
    float v = 0.0f;
    for (int dz = 0; dz < 2; ++dz)
        for (int dy = 0; dy < 2; ++dy)
            for (int dx = 0; dx < 2; ++dx) {
                const float w = (dx ? f[0] : 1 - f[0]) * (dy ? f[1] : 1 - f[1]) * (dz ? f[2] : 1 - f[2]);
                v += w * at(i[0] + dx, i[1] + dy, i[2] + dz);
            }
    out = v * 0.0001f;
    return true;
}

}

namespace gunshape {

bool Use(const char* name) {
    std::lock_guard lock(g_mutex);
    auto it = g_fields.find(name);
    if (it == g_fields.end())
        it = g_fields.emplace(name, LoadBesideModule(name)).first;
    g_current = it->second.get();
    return g_current.load() != nullptr;
}

bool Load(const wchar_t* path) {
    std::lock_guard lock(g_mutex);
    auto field = LoadFile(path, "the file's");
    if (!field)
        return false;
    g_current = field.get();
    g_fields["(file)"] = std::move(field);
    return true;
}

// As asked for: none is none, and not the M416's for a gun with no shape.
static const Field* Current() {
    return g_current.load();
}

bool Distance(const float* p, float& distance) {
    const Field* field = Current();
    return field && Sample(*field, p, distance);
}

bool Distance(const float* p, float& distance, float* normal) {
    const Field* field = Current();
    if (!field || !Sample(*field, p, distance))
        return false;
    // The way out: the field's slope over half a cell each way.
    const float h = field->cell * 0.5f;
    float slope[3];
    for (int k = 0; k < 3; ++k) {
        float a[3] = {p[0], p[1], p[2]}, b[3] = {p[0], p[1], p[2]};
        a[k] += h, b[k] -= h;
        float va, vb;
        if (!Sample(*field, a, va) || !Sample(*field, b, vb))
            return false;
        slope[k] = va - vb;
    }
    const float length = std::sqrt(slope[0] * slope[0] + slope[1] * slope[1] + slope[2] * slope[2]);
    if (length < 1e-6f)
        return false;
    for (int k = 0; k < 3; ++k)
        normal[k] = slope[k] / length;
    return true;
}

}
