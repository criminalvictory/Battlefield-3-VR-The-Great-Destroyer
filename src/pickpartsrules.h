#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
// The rules of a dropped gun's attachments where it lies that need no game: tools/pickpartstest.cpp runs them on memory of its own. Every read goes
// through a Reader, so the game's memory and made-up memory are read alike; a read that fails refuses what it was for. Every address in the comments
// is a VA in bf3_mem.exe. Matrices are 16 floats, four rows (right, up, forward, translation), as the game keeps them.
namespace pickparts {

// ---- Layouts, as read in the game's code ----

// A client pickup's content (pickup +0xA8: its data, then the entries' begin and end), its entries of 0xC0 (011BB9D0).
constexpr uint32_t kContentList = 0xA8, kEntrySize = 0xC0, kEntrySlot = 0xA8, kPreferredSlot = 0x84;
// An entry's unlocks: the tree at entry+4 (008559B0 inserts, 00855AA0 copies). The anchor at entry+8: its +0 the node reached by +0 links from the
// root, its +4 the node reached by +4 links (the insert's begin, 008559ED), its +8 the root with a flag in bit 0; the size at entry+0x14. A node:
// +0 and +4 its two children (a key below the node's goes to +4: setb at 008559D5), +8 its parent and colour, +0xC the value (an unlock).
constexpr uint32_t kSetAnchor = 8, kSetBegin = 0xC, kSetRoot = 0x10, kSetSize = 0x14;
constexpr uint32_t kNodeUpper = 0, kNodeLower = 4, kNodeValue = 0xC;
// An unlock: its name (a pointer to letters) and its kind (011F1411).
constexpr uint32_t kUnlockName = 8, kUnlockKind = 0x1C;
// A blueprint's gun data (01235800), its WeaponStates (inline, 0x5C each, the count at -4: 00854B8D) and Sockets (pointers, the count at -4); a
// state's Mesh3p, Mesh3pTransforms (64-byte matrices) and Mesh3pRigidMeshSocketObjectTransforms (0x50 each: a matrix, then +0x40 its object).
constexpr uint32_t kBlueprintData = 0x20, kDataStates = 0x60, kDataSockets = 0x134;
constexpr uint32_t kStateMesh3p = 0x10, kStateTransforms = 0x44, kStateRigid = 0x48, kRigidSize = 0x50, kRigidObject = 0x40;
// A SocketData: UnlockAsset, BoneName (a pointer to letters), AvailableObjects (pointers, the count at -4), UsesDefaultObject, Excluded.
constexpr uint32_t kSocketUnlock = 0x90, kSocketBone = 0x98, kSocketObjects = 0x9C, kSocketUsesDefault = 0xA8, kSocketExcluded = 0xA9;
// A WeaponRegularSocketObjectData: Asset3p, its own Transform. An asset's name.
constexpr uint32_t kObjectAsset3p = 0x14, kObjectTransform = 0x20, kAssetName = 8;
// A type's class id (u16; the range of a class's ids at +0x14 and +0x16 of its type, 01039146-0103915E).
constexpr uint32_t kTypeClassId = 0x14;

// What is read at most.
constexpr int kMostEntries = 64, kMostUnlocks = 16, kMostSteps = 64, kMostSockets = 64, kMostObjects = 8, kMostRigid = 64, kMostParts = 32;
constexpr int kText = 64;

struct Reader {
    bool (*read)(void* context, uint32_t at, void* out, uint32_t size) = nullptr;
    void* context = nullptr;
    bool Bytes(uint32_t at, void* out, uint32_t size) const { return at && read && read(context, at, out, size); }
    bool U32(uint32_t at, uint32_t& out) const { return Bytes(at, &out, 4); }
    bool U16(uint32_t at, uint16_t& out) const { return Bytes(at, &out, 2); }
    bool U8(uint32_t at, uint8_t& out) const { return Bytes(at, &out, 1); }
    // The letters a pointer at `at` points to, cut to fit, anything unprintable as '?'; "" when there's none. False when the pointer can't be read.
    bool Text(uint32_t at, char* out, int size) const {
        out[0] = 0;
        uint32_t text = 0;
        if (!U32(at, text))
            return false;
        if (!text)
            return true;
        int i = 0;
        for (; i < size - 1; ++i) {
            char c = 0;
            if (!Bytes(text + i, &c, 1) || !c)
                break;
            out[i] = static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7E ? '?' : c;
        }
        out[i] = 0;
        return true;
    }
};

// The image's code (.text, and ctr: the type getters the vtables name are there, 01F21440 and 01FAD150) and read-only data, from its section headers.
struct Image {
    uint32_t codeFrom = 0, codeTo = 0, rdataFrom = 0, rdataTo = 0, moreCodeFrom = 0, moreCodeTo = 0;
    bool Code(uint32_t at) const { return (at >= codeFrom && at < codeTo) || (at >= moreCodeFrom && at < moreCodeTo); }
    bool Rdata(uint32_t at) const { return at >= rdataFrom && at + 4 <= rdataTo; }
};

// ---- The content entry the game draws (011BB9D0, both passes) ----

// The first entry whose slot is the data's PreferredSlot and whose blueprint isn't null (pass 1); else the entry with a blueprint and the lowest slot
// (unsigned), the first of equal ones (pass 2); else none. list: the pickup + kContentList. 0: none, or a read refused (pass then 0).
inline uint32_t PickEntry(const Reader& r, uint32_t list, int* pass = nullptr) {
    if (pass)
        *pass = 0;
    uint32_t data = 0, begin = 0, end = 0, preferred = 0;
    if (!r.U32(list, data) || !r.U32(list + 4, begin) || !r.U32(list + 8, end) || !data || !r.U32(data + kPreferredSlot, preferred))
        return 0;
    if (!begin || end < begin || end - begin > kEntrySize * kMostEntries)
        return 0;
    const uint32_t count = (end - begin) / kEntrySize;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t e = begin + kEntrySize * i;
        uint32_t slot = 0, blueprint = 0;
        if (!r.U32(e + kEntrySlot, slot) || !r.U32(e, blueprint))
            return 0;
        if (slot == preferred && blueprint) {
            if (pass)
                *pass = 1;
            return e;
        }
    }
    uint32_t best = 0, bestSlot = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t e = begin + kEntrySize * i;
        uint32_t slot = 0, blueprint = 0;
        if (!r.U32(e + kEntrySlot, slot) || !r.U32(e, blueprint))
            return 0;
        if ((!best || slot < bestSlot) && blueprint)
            best = e, bestSlot = slot;
    }
    if (best && pass)
        *pass = 2;
    return best;
}

// ---- An entry's unlocks ----

// Every value of the tree at entry+4, lowest key first (the +4 child before the node before the +0 child), at most `most`. Refused (-1, why said):
// a read refused, a size over kMostUnlocks, more nodes than the size or fewer, over kMostSteps steps, a child that is the anchor, a null value, or
// a walk whose first or last node isn't the one the anchor names. An empty tree (size 0, root 0, both ends the anchor) gives 0.
inline int WalkSet(const Reader& r, uint32_t entry, uint32_t* out, int most, const char** why = nullptr) {
    const char* dummy = nullptr;
    const char*& w = why ? *why : dummy;
    w = "";
    const uint32_t anchor = entry + kSetAnchor;
    uint32_t upperEnd = 0, lowerEnd = 0, rootWord = 0, size = 0;
    if (!r.U32(anchor, upperEnd) || !r.U32(entry + kSetBegin, lowerEnd) || !r.U32(entry + kSetRoot, rootWord) || !r.U32(entry + kSetSize, size)) {
        w = "its set can't be read";
        return -1;
    }
    const uint32_t root = rootWord & ~1u;
    if (size > static_cast<uint32_t>(kMostUnlocks)) {
        w = "its set says more than 16";
        return -1;
    }
    if (!root) {
        if (size != 0 || upperEnd != anchor || lowerEnd != anchor) {
            w = "its set is empty but says otherwise";
            return -1;
        }
        return 0;
    }
    uint32_t stack[kMostSteps];
    int depth = 0, n = 0, steps = 0;
    uint32_t node = root, first = 0, last = 0;
    while (node || depth) {
        if (++steps > kMostSteps) {
            w = "its set's walk went over 64 steps";
            return -1;
        }
        if (node) {
            if (node == anchor) {
                w = "a node of its set points at the anchor";
                return -1;
            }
            if (depth >= kMostSteps) {
                w = "its set is deeper than 64";
                return -1;
            }
            stack[depth++] = node;
            uint32_t lower = 0;
            if (!r.U32(node + kNodeLower, lower)) {
                w = "a node of its set can't be read";
                return -1;
            }
            node = lower;
            continue;
        }
        node = stack[--depth];
        uint32_t value = 0, upper = 0;
        if (!r.U32(node + kNodeValue, value) || !r.U32(node + kNodeUpper, upper)) {
            w = "a node of its set can't be read";
            return -1;
        }
        if (!value) {
            w = "a node of its set holds nothing";
            return -1;
        }
        if (static_cast<uint32_t>(n) >= size) {
            w = "its set has more nodes than it says";
            return -1;
        }
        if (!first)
            first = node;
        last = node;
        if (n < most)
            out[n] = value;
        ++n;
        node = upper;
    }
    if (static_cast<uint32_t>(n) != size) {
        w = "its set has fewer nodes than it says";
        return -1;
    }
    if (first != lowerEnd || last != upperEnd) {
        w = "its set's ends aren't the anchor's";
        return -1;
    }
    return n < most ? n : most;
}

// The same unlocks in any order give the same key (FNV-1a 64 over them sorted).
inline uint64_t SetKey(const uint32_t* unlocks, int n) {
    uint32_t sorted[kMostUnlocks];
    const int m = n < kMostUnlocks ? n : kMostUnlocks;
    for (int i = 0; i < m; ++i)
        sorted[i] = unlocks[i];
    for (int i = 1; i < m; ++i)
        for (int j = i; j > 0 && sorted[j - 1] > sorted[j]; --j) {
            const uint32_t t = sorted[j];
            sorted[j] = sorted[j - 1];
            sorted[j - 1] = t;
        }
    uint64_t h = 14695981039346656037ull;
    for (int i = 0; i < m; ++i)
        for (int b = 0; b < 4; ++b) {
            h ^= (sorted[i] >> (8 * b)) & 0xFF;
            h *= 1099511628211ull;
        }
    return h;
}

// ---- What an unlock is, by its name ----

enum Kind : uint8_t { kSight = 0, kBarrel, kRail, kMagazine, kNoPicture, kOther };
inline const char* KindName(Kind k) {
    switch (k) {
    case kSight: return "sight";
    case kBarrel: return "barrel";
    case kRail: return "rail";
    case kMagazine: return "magazine";
    case kNoPicture: return "no picture";
    default: return "other";
    }
}
// By its name's letters and digits (any case): "Weapons/AN94/U_AN94_PKS-07" a sight, "Weapons/Common/NoOptics" none; a name not in the table "other".
inline Kind KindOf(const char* name) {
    char flat[kText * 2] = {};
    int n = 0;
    for (const char* p = name ? name : ""; *p && n < static_cast<int>(sizeof(flat)) - 1; ++p) {
        const char c = *p;
        if (c >= 'a' && c <= 'z')
            flat[n++] = static_cast<char>(c - 'a' + 'A');
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            flat[n++] = c;
        else if (c == '/' || c == '\\')
            n = 0;  // the last part of the path only
    }
    flat[n] = 0;
    struct Row {
        const char* part;
        Kind kind;
    };
    static const Row kRows[] = {
        {"NOOPTICS", kNoPicture}, {"AIUNLOCK", kNoPicture}, {"FLECHETTE", kNoPicture}, {"FRAG", kNoPicture}, {"SLUG", kNoPicture},
        {"BUCKSHOT", kNoPicture}, {"DEPLOYTIME", kNoPicture}, {"HEAVYBARREL", kNoPicture},
        {"EXTENDEDMAG", kMagazine}, {"EXTENDED", kMagazine}, {"MAGAZINE", kMagazine},
        {"KOBRA", kSight}, {"RX01", kSight}, {"EOTECH", kSight}, {"HOLO", kSight}, {"PKAS", kSight}, {"PKA", kSight}, {"M145", kSight},
        {"ACOG", kSight}, {"PSO1", kSight}, {"PKS07", kSight}, {"SCOPE", kSight}, {"BALLISTIC", kSight}, {"IRNV", kSight}, {"FLIR", kSight},
        {"SILENCER", kBarrel}, {"SUPPRESSOR", kBarrel}, {"FLASHSUPP", kBarrel}, {"FLASHHIDER", kBarrel},
        {"FOREGRIP", kRail}, {"BIPOD", kRail}, {"FLASHLIGHT", kRail}, {"TORCH", kRail}, {"LASER", kRail}, {"TACTICAL", kRail}, {"40MM", kRail},
        {"M320", kRail}, {"M26", kRail}, {"GP30", kRail},
    };
    for (const Row& row : kRows)
        if (std::strstr(flat, row.part))
            return row.kind;
    return kOther;
}

// ---- Placement ----

// A part's place in the world: its matrix in the body's model space (rel) times the body's pose (base), as the game moves the 3P bone matrices
// onto the pose (008549D0-00854AD6): out row i = rel[i][0] base row 0 + rel[i][1] base row 1 + rel[i][2] base row 2, and the translation row adds
// base row 3. rel's fourth column isn't read.
inline void Compose(const float* rel, const float* base, float* out) {
    float r[16];
    for (int i = 0; i < 4; ++i)
        for (int k = 0; k < 4; ++k) {
            float v = rel[4 * i + 0] * base[0 + k] + rel[4 * i + 1] * base[4 + k] + rel[4 * i + 2] * base[8 + k];
            if (i == 3)
                v += base[12 + k];
            r[4 * i + k] = v;
        }
    std::memcpy(out, r, sizeof(r));
}

// Whether the game's builder made its body again: there was none, it is another instance, or the same address names another MeshSet (the 0x2C bytes
// freed just before an allocation of the same size may come back).
inline bool Rebuilt(uint32_t pre, uint32_t preSet, uint32_t post, uint32_t postSet) {
    return post && (!pre || post != pre || postSet != preSet);
}

// ---- The gun's third-person data ----

// The class of an object by its type getter, read and never called: its vtable in the image's read-only data, the vtable's first function in its
// code and exactly "mov eax, type; ret" (B8 imm32 C3, the form of the getters at 01FAD150 and 01F21440), then the type's class id against the range
// in the two u16 at rangeCell (as 01039146-0103915E and 010391C1-010391D9 compare). False: not that class, or not readable so.
inline bool ClassIn(const Reader& r, const Image& image, uint32_t object, uint32_t rangeCell, uint32_t* vtableOut = nullptr, uint16_t* idOut = nullptr) {
    uint32_t vtable = 0, getter = 0, type = 0;
    uint8_t code[6] = {};
    uint16_t id = 0, from = 0, to = 0;
    if (!object || !r.U32(object, vtable) || !image.Rdata(vtable) || !r.U32(vtable, getter) || !image.Code(getter) || !r.Bytes(getter, code, 6))
        return false;
    if (vtableOut)
        *vtableOut = vtable;
    if (code[0] != 0xB8 || code[5] != 0xC3)
        return false;
    std::memcpy(&type, code + 1, 4);
    if (!r.U16(type + kTypeClassId, id) || !r.U16(rangeCell, from) || !r.U16(rangeCell + 2, to))
        return false;
    if (idOut)
        *idOut = id;
    return static_cast<uint16_t>(id - from) <= static_cast<uint16_t>(to - from);
}

// A part's matrix on the 3P body: the state's rigid entry whose object is this one (01039490), else the object's own Transform (01039228-01039252).
// fromState says which. False: neither readable, or the state's list over kMostRigid.
inline bool RigidFor(const Reader& r, uint32_t state, uint32_t object, float* out16, bool* fromState) {
    *fromState = false;
    uint32_t list = 0, count = 0;
    if (state && r.U32(state + kStateRigid, list) && list && r.U32(list - 4, count)) {
        if (count > static_cast<uint32_t>(kMostRigid))
            return false;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t at = 0;
            if (!r.U32(list + kRigidSize * i + kRigidObject, at))
                return false;
            if (at == object) {
                *fromState = true;
                return r.Bytes(list + kRigidSize * i, out16, 64);
            }
        }
    }
    return r.Bytes(object + kObjectTransform, out16, 64);
}

// One socket object the 3P data has for the drop: a socket whose UnlockAsset is in the drop's set, or one with none (the game always loads those,
// 01036F08), and one of its AvailableObjects.
struct Part {
    uint32_t unlock = 0, socket = 0, object = 0, asset = 0;
    int socketIndex = -1, objectIndex = -1;
    uint32_t unlockKind = 0;            // its unlock's +0x1C (0 when it has none)
    Kind kind = kOther;                 // by its unlock's name (none: by its asset's)
    bool rootBone = false;              // its socket's BoneName empty or Wep_Root
    bool excluded = false, usesDefault = false;
    bool objectOk = false, assetOk = false;  // the class tests (WeaponRegularSocketObjectData; the mesh class)
    bool fromState = false, haveRel = false;
    uint32_t objectVt = 0, assetVt = 0;
    uint16_t assetClass = 0;
    float rel[16] = {};
    char bone[24] = {};
    char unlockName[kText] = {}, assetName[kText] = {};
};

struct Gun {
    uint32_t data = 0, states = 0, mesh3p = 0, mesh3pVt = 0;
    uint16_t mesh3pClass = 0;
    int stateCount = 0, socketCount = 0;
    bool rootIdentity = false;          // Mesh3pTransforms[0] of state 0 is the identity
    bool haveRoot = false;              // ... and that matrix, read (the census says where it is when it isn't the identity)
    float root[16] = {};
    bool socketsCut = false;            // more than kMostSockets: refused
    int parts = 0;
    Part part[kMostParts];
    int otherBones = 0, absentDefaults = 0;  // sockets of its unlocks on another bone; UsesDefaultObject with their unlock absent
};

// The class ranges the game compares with (two u16 each: the first id and the last).
struct Classes {
    uint32_t socketObject = 0;  // 023F5098: WeaponRegularSocketObjectData
    uint32_t mesh = 0;          // 023D1B04: the mesh assets
};

inline bool InSet(const uint32_t* unlocks, int n, uint32_t unlock) {
    for (int i = 0; i < n; ++i)
        if (unlocks[i] == unlock)
            return true;
    return false;
}

// The gun's data and its parts for these unlocks. False (why said): its data or states can't be read, more than kMostSockets sockets. A state count
// of 0 reads no state (stateCount 0, true).
inline bool ReadGun(const Reader& r, const Image& image, const Classes& classes, uint32_t blueprint, const uint32_t* unlocks, int n, Gun& g,
                    const char** why = nullptr) {
    const char* dummy = nullptr;
    const char*& w = why ? *why : dummy;
    w = "";
    g = Gun{};
    uint32_t states = 0, stateCount = 0, sockets = 0, socketCount = 0;
    if (!r.U32(blueprint + kBlueprintData, g.data) || !g.data || !r.U32(g.data + kDataStates, states) || !states || !r.U32(states - 4, stateCount)) {
        w = "its gun data can't be read";
        return false;
    }
    g.states = states;
    g.stateCount = static_cast<int>(stateCount > 64 ? 64 : stateCount);
    if (stateCount) {
        r.U32(states + kStateMesh3p, g.mesh3p);
        if (g.mesh3p)
            ClassIn(r, image, g.mesh3p, classes.mesh, &g.mesh3pVt, &g.mesh3pClass);
        uint32_t transforms = 0, count = 0;
        float m[16];
        if (r.U32(states + kStateTransforms, transforms) && transforms && r.U32(transforms - 4, count) && count && r.Bytes(transforms, m, 64)) {
            static const float kId[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            bool id = true;
            for (int i = 0; i < 16; ++i)
                id = id && m[i] > kId[i] - 1e-4f && m[i] < kId[i] + 1e-4f;
            g.rootIdentity = id;
            g.haveRoot = true;
            std::memcpy(g.root, m, sizeof(m));
        }
    }
    if (!r.U32(g.data + kDataSockets, sockets) || !sockets || !r.U32(sockets - 4, socketCount)) {
        w = "its sockets can't be read";
        return false;
    }
    if (socketCount > static_cast<uint32_t>(kMostSockets)) {
        g.socketsCut = true;
        w = "its gun has more than 64 sockets";
        return false;
    }
    g.socketCount = static_cast<int>(socketCount);
    for (int i = 0; i < g.socketCount; ++i) {
        uint32_t socket = 0, unlock = 0, objects = 0, objectCount = 0;
        uint8_t usesDefault = 0, excluded = 0;
        if (!r.U32(sockets + 4 * i, socket) || !socket || !r.U32(socket + kSocketUnlock, unlock))
            continue;
        r.U8(socket + kSocketUsesDefault, usesDefault);
        r.U8(socket + kSocketExcluded, excluded);
        if (unlock && !InSet(unlocks, n, unlock)) {
            if (usesDefault)
                ++g.absentDefaults;
            continue;
        }
        char bone[24] = {};
        r.Text(socket + kSocketBone, bone, sizeof(bone));
        const bool root = !bone[0] || std::strcmp(bone, "Wep_Root") == 0;
        if (!root && unlock)
            ++g.otherBones;
        if (!r.U32(socket + kSocketObjects, objects) || !objects || !r.U32(objects - 4, objectCount))
            continue;
        if (objectCount > static_cast<uint32_t>(kMostObjects))
            objectCount = kMostObjects;
        for (uint32_t k = 0; k < objectCount && g.parts < kMostParts; ++k) {
            uint32_t object = 0;
            if (!r.U32(objects + 4 * k, object) || !object)
                continue;
            Part& p = g.part[g.parts++];
            p = Part{};
            p.unlock = unlock, p.socket = socket, p.object = object;
            p.socketIndex = i, p.objectIndex = static_cast<int>(k);
            p.rootBone = root, p.excluded = excluded != 0, p.usesDefault = usesDefault != 0;
            std::memcpy(p.bone, bone, sizeof(bone));
            if (unlock) {
                r.U32(unlock + kUnlockKind, p.unlockKind);
                r.Text(unlock + kUnlockName, p.unlockName, kText);
            }
            p.objectOk = ClassIn(r, image, object, classes.socketObject, &p.objectVt);
            if (p.objectOk) {
                r.U32(object + kObjectAsset3p, p.asset);
                if (p.asset) {
                    p.assetOk = ClassIn(r, image, p.asset, classes.mesh, &p.assetVt, &p.assetClass);
                    r.Text(p.asset + kAssetName, p.assetName, kText);
                }
                p.haveRel = RigidFor(r, g.stateCount ? states : 0, object, p.rel, &p.fromState);
            }
            p.kind = KindOf(unlock ? p.unlockName : p.assetName);
        }
    }
    return true;
}

// ---- What is drawn ----

// The switch's values: 0 off, 1 the census and the hooks counting only, 2 one sight on one pickup, 3 the sights on every pickup, 4 every part with a
// picture but magazines.
constexpr int kPartsOff = 0, kPartsCensus = 1, kPartsOne = 2, kPartsSights = 3, kPartsAll = 4;

// Why a part isn't drawn ("" it may be).
inline const char* NotDrawn(const Part& p, int level) {
    if (level < kPartsOne)
        return "the switch draws none";
    if (!p.objectOk)
        return "its object isn't a regular socket object";
    if (!p.asset)
        return "no 3P asset";
    if (!p.assetOk)
        return "its 3P asset isn't a mesh";
    if (!p.haveRel)
        return "no place on the body";
    if (p.excluded)
        return "its socket is excluded";
    if (!p.rootBone)
        return "its socket is on another bone";
    if (p.unlock && p.unlockKind != 0)
        return "its unlock's kind isn't 0";
    if (p.kind == kMagazine)
        return "a magazine (not drawn until filmed)";
    if (p.kind == kNoPicture)
        return "no picture";
    if (level < kPartsAll && (p.kind != kSight || !p.unlock))
        return "not a sight (parts 4 draws it)";
    return "";
}

// The parts to make for one pickup, sights first, then the others in the data's order: their indexes into g.part, at most `most`. level kPartsOne:
// the first sight only, and only when `firstAllowed` (no other pickup has a part).
inline int Wanted(const Gun& g, int level, bool firstAllowed, int* out, int most) {
    int n = 0;
    if (level < kPartsOne || (level == kPartsOne && !firstAllowed))
        return 0;
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < g.parts && n < most; ++i) {
            const Part& p = g.part[i];
            if ((pass == 0) != (p.kind == kSight) || NotDrawn(p, level)[0])
                continue;
            bool same = false;  // one part per object
            for (int k = 0; k < n; ++k)
                same = same || g.part[out[k]].object == p.object;
            if (same)
                continue;
            out[n++] = i;
            if (level == kPartsOne)
                return n;
        }
    return n;
}

// The key of what is wanted for a pickup: the same objects and assets of the same gun (its blueprint) give the same key.
inline uint64_t WantKey(const Gun& g, const int* wanted, int n, uint32_t blueprint) {
    uint64_t h = 14695981039346656037ull;
    auto mix = [&h](uint32_t v) {
        for (int b = 0; b < 4; ++b) {
            h ^= (v >> (8 * b)) & 0xFF;
            h *= 1099511628211ull;
        }
    };
    mix(blueprint);
    mix(static_cast<uint32_t>(n));
    for (int i = 0; i < n; ++i) {
        mix(g.part[wanted[i]].object);
        mix(g.part[wanted[i]].asset);
    }
    return h;
}

// ---- The pickup collector and the stand-ins handed to it ----
//
// A dropped gun's body is drawn by the ClientPickupEntity renderer's collector (00797FF0, the renderer's slot 9: 020AE204), every frame: for each
// element it's handed (16 bytes, the entity at +0, its views' mask at +4: 0179BD0D) it asks the entity for its one instance (slot 47 at +0xBC,
// 00844560) and that instance's box (slot 23 at +0x5C, 008444C0), then copies the entity's transform (64 bytes at [+0x1C], read with movaps),
// looks at its flags (+8) and its handle in [023B3F94] (the word +0x166, 01744260) into a draw record of the frame's own memory. A part is drawn the
// same way: one stand-in entity per part, handed to the same collector right after the game's own pass. All that the collector and what it calls
// for a mesh that isn't skinned read of an entity (00797FF0-00798936, 00844560, 008444C0, 01015950, 01744260, 017A1FA0, 0179BD00):
constexpr uint32_t kStandinSize = 0x180;      // (the reads end at +0x168)
constexpr uint32_t kStandinVtable = 0;        // the client pickup's own: slot 47 and slot 23 are then the game's
constexpr uint32_t kStandinFlags = 8;         // 0: bit 0x40000 clear (the plain transform; 01015950 copies no transforms of parts)
constexpr uint32_t kStandinTransform = 0x1C;  // the address of a 16-byte aligned matrix
constexpr uint32_t kStandinBody = 0x110;      // the part's instance
constexpr uint32_t kStandinShown = 0x164, kStandinHiding = 0x165;  // both 0: slot 47 hands over +0x110
constexpr uint32_t kStandinHandle = 0x166;    // the gun's own handle in [023B3F94] (its local light), or 0xFFFF for none
// An element: the entity, its views' mask, two words the collector doesn't read (the stand-in's three copied from the gun's).
constexpr uint32_t kElementWords = 4;
// The light handles' system [023B3F94] as 01744260 reads it: the byte at +0x144 says it's on, then the handle's block at [+4 * (handle >> 10)].
constexpr uint32_t kLightsOn = 0x144;
constexpr uint16_t kNoLight = 0xFFFF;

// Slot 47 (00844560): the body, or none while +0x165 is set and +0x164 isn't (a drop's fly-in hides it; its parts hide with it).
inline bool BodyShown(uint8_t at164, uint8_t at165) {
    return !(at165 && !at164);
}
// A MeshSet's kind (its +0, 00798099-007980B9): 0 and 2 are drawn from the stand-in alone; 1, skinned, goes through 00854920 and 00854960 (the
// pickup's own bones), so it's never handed over; the collector skips any other kind (00798916).
inline bool KindDrawn(uint32_t kind) {
    return kind == 0 || kind == 2;
}
// One stand-in: a zeroed block with the client pickup's vtable, the matrix's address, the part's instance and the gun's handle.
inline void FillStandin(uint8_t* block, uint32_t vtable, uint32_t transform, uint32_t instance, uint16_t handle) {
    std::memset(block, 0, kStandinSize);
    std::memcpy(block + kStandinVtable, &vtable, 4);
    std::memcpy(block + kStandinTransform, &transform, 4);
    std::memcpy(block + kStandinBody, &instance, 4);
    std::memcpy(block + kStandinHandle, &handle, 2);
}
// The handle a stand-in carries: the gun's own while 01744260 would find its block (with the system off it reads no block, so the gun's goes as it
// is), else none (01744260 gives no light for 0xFFFF: 0174427A).
inline uint16_t LightFor(const Reader& r, uint32_t system, uint16_t handle) {
    uint8_t on = 0;
    uint32_t block = 0;
    if (handle == kNoLight || !system || !r.U8(system + kLightsOn, on))
        return kNoLight;
    if (!on)
        return handle;
    return r.U32(system + 4u * (handle >> 10), block) && block ? handle : kNoLight;
}
// The entry the pose is taken from, guarded as 00854960 guards it before 0084A7C0 (00854971-00854998): the entry the game draws, its blueprint's
// gun data, a WeaponStates count above 0. 0: no pose (0084CC50 would hand 0084A7C0 a null entry: 0084CC66-0084CC73).
inline uint32_t PoseEntry(const Reader& r, uint32_t pickup) {
    const uint32_t entry = PickEntry(r, pickup + kContentList);
    uint32_t blueprint = 0, data = 0, states = 0, count = 0;
    if (!entry || !r.U32(entry, blueprint) || !blueprint || !r.U32(blueprint + kBlueprintData, data) || !data || !r.U32(data + kDataStates, states) ||
        !states || !r.U32(states - 4, count) || !count)
        return 0;
    return entry;
}

// ---- What the collector's threads read: one row per pickup's record, written by the game's thread alone, read with no lock (a seqlock) ----

constexpr int kRowParts = 8;
struct Row {
    uint32_t pickup = 0, body = 0, world = 0, scene = 0, count = 0;  // the pickup, its body, the client world and the scene its parts were made in
    uint32_t instance[kRowParts] = {}, kind[kRowParts] = {};
    float rel[kRowParts][16] = {};
};
constexpr int kRowWords = static_cast<int>(sizeof(Row) / 4);
static_assert(sizeof(Row) % 4 == 0 && offsetof(Row, pickup) == 0, "a row is words, its pickup first");
struct SharedRow {
    std::atomic<uint32_t> seq{0};  // odd while it's written
    std::atomic<uint32_t> word[kRowWords];
};
// The writer (one at a time: the game's thread, under its lock): the seq odd, the words, the seq even again.
inline void WriteRow(SharedRow& s, const Row& row) {
    uint32_t words[kRowWords];
    std::memcpy(words, &row, sizeof(words));
    const uint32_t seq = s.seq.load(std::memory_order_relaxed);
    s.seq.store(seq + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    for (int i = 0; i < kRowWords; ++i)
        s.word[i].store(words[i], std::memory_order_relaxed);
    s.seq.store(seq + 2, std::memory_order_release);
}
// A reader's copy, at most `tries` times over while the writer is at it. False: no steady copy (nothing of it is used). seqOut: the copy's seq.
inline bool ReadRow(const SharedRow& s, Row& out, uint32_t& seqOut, int tries = 4) {
    uint32_t words[kRowWords];
    for (int t = 0; t < tries; ++t) {
        const uint32_t before = s.seq.load(std::memory_order_acquire);
        if (before & 1)
            continue;
        for (int i = 0; i < kRowWords; ++i)
            words[i] = s.word[i].load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (s.seq.load(std::memory_order_relaxed) == before) {
            std::memcpy(&out, words, sizeof(words));
            seqOut = before;
            return true;
        }
    }
    return false;
}
inline uint32_t RowPickup(const SharedRow& s) {
    return s.word[0].load(std::memory_order_relaxed);
}

// ---- Who holds which instance: a collector's call holds those it hands the game until the call is back; a free waits until none does ----
//
// The reader holds its row's instances, then looks at the row's seq again: if the row was written since its copy, it hands none of them. The writer
// writes the row without the instance, then looks at every hold. Both with a full fence between: one of the two always sees the other's write, so an
// instance is never freed while a call that copied it hands it to the game.
// Stand-ins a call: the design's 32, since they're static memory and no job's stack bounds them (one call can carry every drop in view: with 8,
// five drops of two sights at parts 3 left the fifth bare, the parts of the drops past the cap missing for as long as they're in that call).
constexpr int kCallSlots = 8, kDrawEach = 32;  // calls at once; stand-ins a call
struct CallHolds {
    std::atomic<uint32_t> busy{0};
    std::atomic<uint32_t> instance[kDrawEach];
};
inline int ClaimCall(CallHolds* calls, int n) {
    for (int i = 0; i < n; ++i)
        if (!calls[i].busy.exchange(1, std::memory_order_seq_cst))
            return i;
    return -1;
}
inline void ReleaseCall(CallHolds& call) {
    for (std::atomic<uint32_t>& h : call.instance)
        h.store(0, std::memory_order_seq_cst);
    call.busy.store(0, std::memory_order_seq_cst);
}
// Holds instances[0..n) at from.. for the call, then the row must still be at seq; else the holds are let go of and false.
inline bool HoldRow(CallHolds& call, int from, const uint32_t* instances, int n, const SharedRow& row, uint32_t seq) {
    for (int k = 0; k < n; ++k)
        call.instance[from + k].store(instances[k], std::memory_order_seq_cst);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if (row.seq.load(std::memory_order_seq_cst) == seq)
        return true;
    for (int k = 0; k < n; ++k)
        call.instance[from + k].store(0, std::memory_order_seq_cst);
    return false;
}
// The writer, after the row is written without it: whether any call still holds the instance.
inline bool AnyHolds(const CallHolds* calls, int n, uint32_t instance) {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    for (int i = 0; i < n; ++i)
        for (const std::atomic<uint32_t>& h : calls[i].instance)
            if (h.load(std::memory_order_seq_cst) == instance)
                return true;
    return false;
}

}  // namespace pickparts
