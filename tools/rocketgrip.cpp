// The game's own hand on the launcher's case, from its own reload: build\reloadrec.bin (weapon.txt "allrec", the game's bones before the
// mod moves them), the gloves' mesh dump and part A's. The user, 2026-11-04: "you have to look at how they're gripping it and how their hands look
// when they're changing the rocket behind the launcher, and use that."
//   rocketgrip <build folder> list [every]             each so many milliseconds: the left palm on the case, the case on the launcher
//   rocketgrip <build folder> draw <out.bmp> ms ms ...  a row for each time: the case and the left glove from four sides, in the case's own space
//   rocketgrip <build folder> table ms                  the hold and the bones at that time, as tables for src/rocketposes.h
// On the case's own bone: x, y across it, z towards the muzzle; its axis at x 0, y -0.0935.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "meshdump.h"

using namespace meshdump;

namespace {

constexpr float kAxisY = -0.0935f;
constexpr float kLeftPalm[3] = {0.537f, 1.098f, 0.207f};  // the left hand's centre in the gloves' bind space
constexpr float kRightPalm[3] = {-0.537f, 1.098f, 0.207f};  // the right's: its mirror image
constexpr int kRightRoot = 37;

struct Frame {
    int ms = 0;
    int serial = 0;
    float gloves[60][12] = {};
    float partA[3][12] = {};
};

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

void InvertRigid(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

// Each frame's first gloves' draw and the part A draw nearest before or after it in the same frame: both from one view.
std::vector<Frame> LoadRecording(const std::string& path) {
    std::vector<Frame> frames;
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return frames;
    std::vector<float> bones;
    int32_t head[8];
    struct Seen {
        bool haveGloves = false, haveA = false;
        Frame frame;
    };
    Seen now;
    int serial = -1;
    auto flush = [&] {
        if (now.haveGloves && now.haveA)
            frames.push_back(now.frame);
        now = Seen{};
    };
    while (fread(head, sizeof(head), 1, f) == 1) {
        const int kind = head[0], frame = head[1], start = head[3], ms = head[6], count = head[7];
        bones.resize(static_cast<size_t>(count) * 12);
        if (count > 0 && fread(bones.data(), 48, count, f) != static_cast<size_t>(count))
            break;
        if (frame != serial) {
            flush();
            serial = frame;
        }
        if (kind == 13755 && start == 15051 && !now.haveGloves && count >= 45) {
            std::memcpy(now.frame.gloves, bones.data(), std::min(count, 60) * 48);
            now.frame.ms = ms, now.frame.serial = frame;
            now.haveGloves = true;
        } else if (kind == 3414 && start == 0 && !now.haveA && count >= 3) {
            std::memcpy(now.frame.partA, bones.data(), 3 * 48);
            now.haveA = true;
        }
    }
    flush();
    fclose(f);
    return frames;
}

struct CaseMesh {
    std::vector<float> pos;  // on its own bone
    std::vector<int> tris;
};

bool LoadCase(const std::string& dir, CaseMesh& mesh) {
    auto vb = ReadAll(dir + "\\mesh_3414_0_vb0.bin"), ib = ReadAll(dir + "\\mesh_3414_0_ib.bin");
    if (vb.empty() || ib.size() < 3414 * 2u)
        return false;
    std::map<int, int> index;
    for (int i = 0; i + 2 < 3414; i += 3) {
        int corner[3];
        bool all = true;
        for (int c = 0; c < 3; ++c) {
            uint16_t v;
            std::memcpy(&v, &ib[2 * (i + c)], 2);
            const uint8_t* o = &vb[v * size_t{48}];
            int main = -1, weight = -1;
            for (int s = 0; s < 4; ++s)
                if (o[12 + s] > weight)
                    weight = o[12 + s], main = o[8 + s];
            all = all && main == 1;
            auto it = index.find(v);
            if (it == index.end()) {
                uint16_t h[3];
                std::memcpy(h, o, 6);
                it = index.emplace(v, static_cast<int>(mesh.pos.size() / 3)).first;
                mesh.pos.push_back(Half(h[0])), mesh.pos.push_back(Half(h[1])), mesh.pos.push_back(Half(h[2]));
            }
            corner[c] = it->second;
        }
        if (all)
            mesh.tris.insert(mesh.tris.end(), corner, corner + 3);
    }
    return true;
}

// The right glove, as meshdump's LoadGlove the left: its vertices with its bones, 23-44, by their numbers in the draw.
bool LoadRightGlove(const std::string& dir, std::vector<GloveVertex>& verts, std::vector<int>& tris) {
    auto vb = ReadAll(dir + "/mesh_13755_15051_vb0.bin"), ib = ReadAll(dir + "/mesh_13755_15051_ib.bin");
    if (vb.size() < kGloveOffset || ib.size() < (kGloveFirst + kGloveCount) * 2u)
        return false;
    std::vector<int> index(65536, -1);
    for (int j = kGloveFirst; j + 2 < kGloveFirst + kGloveCount; j += 3) {
        int corner[3];
        bool right = true;
        for (int c = 0; c < 3; ++c) {
            uint16_t v;
            std::memcpy(&v, &ib[2 * (j + c)], 2);
            const uint8_t* o = &vb[kGloveOffset + v * kStride];
            if (index[v] < 0) {
                GloveVertex x{};
                uint16_t h[3];
                std::memcpy(h, o, 6);
                int best = 0;
                for (int k = 0; k < 3; ++k)
                    x.pos[k] = Half(h[k]);
                for (int s = 0; s < 4; ++s) {
                    x.bone[s] = o[8 + s], x.weight[s] = o[12 + s] / 255.0f;
                    if (x.weight[s] > x.weight[best])
                        best = s;
                }
                const int b = x.bone[best];
                x.finger = b >= handpose::kRightFirst && b < handpose::kRightFirst + handpose::kRightBones ? handpose::kRightFinger[b - handpose::kRightFirst] : -2;
                index[v] = static_cast<int>(verts.size());
                verts.push_back(x);
            }
            if (verts[index[v]].finger == -2)
                right = false;
            corner[c] = index[v];
        }
        if (right)
            tris.insert(tris.end(), corner, corner + 3);
    }
    return true;
}

const Frame* FrameAt(const std::vector<Frame>& frames, int ms) {
    const Frame* best = nullptr;
    int gap = 1 << 30;
    for (const Frame& f : frames)
        if (std::abs(f.ms - ms) < gap)
            gap = std::abs(f.ms - ms), best = &f;
    return best;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: rocketgrip <build folder> list [every] | draw <out.bmp> ms ... | table ms\n");
        return 1;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1], what = argv[2];
    const std::vector<Frame> frames = LoadRecording(dir + "\\reloadrec.bin");
    if (frames.empty()) {
        std::fprintf(stderr, "nothing of the gloves and the launcher in %s\\reloadrec.bin\n", dir.c_str());
        return 1;
    }
    std::printf("%zu frames with the gloves and part A, %d ms to %d ms\n", frames.size(), frames.front().ms, frames.back().ms);

    if (what == "list") {
        const int every = argc > 3 ? std::atoi(argv[3]) : 100;
        int next = 0;
        std::printf("    ms | the case on the launcher: at (x y z), its turn | the left palm on the case: x y z, off the axis, way round | its turn on the case\n");
        float lastTurn[9] = {};
        for (const Frame& f : frames) {
            if (f.ms < next)
                continue;
            next = f.ms + every;
            float frameInverse[12], onFrame[12], caseInverse[12], palmOnCase[12], at[3];
            InvertRigid(f.partA[0], frameInverse);
            Compose(frameInverse, f.partA[1], onFrame);
            InvertRigid(f.partA[1], caseInverse);
            Compose(caseInverse, f.gloves[1], palmOnCase);
            Apply(palmOnCase, kLeftPalm, at);
            float rightOnCase[12], rightAt[3];
            Compose(caseInverse, f.gloves[kRightRoot], rightOnCase);
            Apply(rightOnCase, kRightPalm, rightAt);
            const float dr = rightAt[1] - kAxisY;
            std::printf("R palm %6.3f %6.3f %6.3f off %5.3f round %6.1f | ", rightAt[0], rightAt[1], rightAt[2], std::sqrt(rightAt[0] * rightAt[0] + dr * dr),
                        std::atan2(dr, rightAt[0]) * 57.29578f);
            const float dy = at[1] - kAxisY;
            float change = 0.0f;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    change += std::fabs(palmOnCase[i * 4 + j] - lastTurn[i * 3 + j]);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    lastTurn[i * 3 + j] = palmOnCase[i * 4 + j];
            std::printf("%6d | at (%6.3f %6.3f %6.3f) z axis (%5.2f %5.2f %5.2f) | palm %6.3f %6.3f %6.3f, off the axis %5.3f, round %6.1f | changed %.3f\n", f.ms,
                        onFrame[3], onFrame[7], onFrame[11], onFrame[2], onFrame[6], onFrame[10], at[0], at[1], at[2], std::sqrt(at[0] * at[0] + dy * dy),
                        std::atan2(dy, at[0]) * 57.29578f, change);
        }
        return 0;
    }

    if (what == "table") {
        const Frame* f = FrameAt(frames, argc > 3 ? std::atoi(argv[3]) : 0);
        float caseInverse[12], palmInverse[12], hold[12];
        InvertRigid(f->partA[1], caseInverse);
        InvertRigid(f->gloves[1], palmInverse);
        // The case in the glove as the palm bone has it: case -> the hand's frame (bone 1's frame).
        Compose(palmInverse, f->partA[1], hold);
        std::printf("// at %d ms\nconstexpr float kCaseOnPalm[12] = {", f->ms);
        for (int k = 0; k < 12; ++k)
            std::printf("%.5ff%s", hold[k], k == 11 ? "};\n" : ", ");
        std::printf("constexpr float kFingers[handpose::kBones][12] = {\n");
        for (int b = 0; b < handpose::kBones; ++b) {
            float rel[12];
            Compose(palmInverse, f->gloves[b], rel);
            std::printf("    {");
            for (int k = 0; k < 12; ++k)
                std::printf("%.5ff%s", rel[k], k == 11 ? "},\n" : ", ");
        }
        std::printf("};\n");
        return 0;
    }

    if (what == "draw" && argc > 4) {
        std::vector<GloveVertex> glove;
        std::vector<int> gloveTris;
        CaseMesh mesh;
        if (!LoadRightGlove(dir, glove, gloveTris) || !LoadCase(dir, mesh)) {
            std::fprintf(stderr, "the gloves' or the launcher's mesh dump isn't in %s\n", dir.c_str());
            return 1;
        }
        const int rows = argc - 4;
        Image img(kTile * 4, kTile * rows);
        constexpr float kCaseColour[3] = {0.55f, 0.55f, 0.6f};
        constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f},
                                               {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
        const float scale = kTile / 0.34f;
        for (int row = 0; row < rows; ++row) {
            const Frame* f = FrameAt(frames, std::atoi(argv[4 + row]));
            float caseInverse[12], rel[60][12];
            InvertRigid(f->partA[1], caseInverse);
            for (int b = 0; b < 60; ++b)
                Compose(caseInverse, f->gloves[b], rel[b]);
            std::vector<float> placed(glove.size() * 3);
            for (size_t i = 0; i < glove.size(); ++i) {
                float sum[3] = {};
                for (int s = 0; s < 4; ++s) {
                    if (glove[i].weight[s] <= 0 || glove[i].bone[s] >= 60)
                        continue;
                    float p[3];
                    Apply(rel[glove[i].bone[s]], glove[i].pos, p);
                    for (int k = 0; k < 3; ++k)
                        sum[k] += glove[i].weight[s] * p[k];
                }
                std::memcpy(&placed[i * 3], sum, sizeof(sum));
            }
            float palm[3];
            Apply(rel[kRightRoot], kRightPalm, palm);
            std::printf("row %d: %d ms, the palm at (%.3f %.3f %.3f) on the case\n", row, f->ms, palm[0], palm[1], palm[2]);
            const float centre[3] = {0.0f, kAxisY, palm[2]};
            // Round the case: from its +x, its -y side, its -x, its +y; its z towards the top of the tile.
            const View views[4] = {MakeView(-1, 0, 0, 0, 0, 1), MakeView(0, 1, 0, 0, 0, 1), MakeView(1, 0, 0, 0, 0, 1), MakeView(0, -1, 0, 0, 0, 1)};
            for (int v = 0; v < 4; ++v) {
                for (size_t t = 0; t + 2 < mesh.tris.size(); t += 3)
                    DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, &mesh.pos[mesh.tris[t] * 3], &mesh.pos[mesh.tris[t + 1] * 3],
                                 &mesh.pos[mesh.tris[t + 2] * 3], kCaseColour);
                for (size_t t = 0; t + 2 < gloveTris.size(); t += 3)
                    DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, &placed[gloveTris[t] * 3], &placed[gloveTris[t + 1] * 3],
                                 &placed[gloveTris[t + 2] * 3], kFingerColour[glove[gloveTris[t]].finger + 1]);
            }
        }
        img.Save(argv[3]);
        std::printf("saved %s\n", argv[3]);
        return 0;
    }
    // The left hand as the mirror image of the game's right on the case (the mirror through the case's axis, x to -x on its bone and in the gloves'
    // bind space): each left bone from the right bone at the same place on the hand.
    std::vector<GloveVertex> leftGlove, rightGlove;
    std::vector<int> leftTris, rightTris;
    CaseMesh mesh;
    if (!LoadGlove(dir, leftGlove, leftTris) || !LoadRightGlove(dir, rightGlove, rightTris) || !LoadCase(dir, mesh)) {
        std::fprintf(stderr, "the gloves' or the launcher's mesh dump isn't in %s\n", dir.c_str());
        return 1;
    }
    // Where each bone's own vertices lie.
    float centre[60][3] = {};
    int centreCount[60] = {};
    auto addCentres = [&](const std::vector<GloveVertex>& verts, float flip) {
        for (const GloveVertex& v : verts) {
            for (int s = 0; s < 4; ++s) {
                const int b = v.bone[s];
                if (b < 0 || b >= 60 || v.weight[s] <= 0.0f)
                    continue;
                centre[b][0] += v.pos[0] * flip, centre[b][1] += v.pos[1], centre[b][2] += v.pos[2];
                ++centreCount[b];
            }
        }
    };
    addCentres(leftGlove, 1.0f);
    addCentres(rightGlove, -1.0f);
    for (int b = 0; b < 60; ++b)
        if (centreCount[b])
            for (float& c : centre[b])
                c /= centreCount[b];
    int rightOf[handpose::kBones] = {};
    for (int b = 0; b < handpose::kBones; ++b) {
        float nearest = 1e9f;
        for (int r = handpose::kRightFirst; r < handpose::kRightFirst + handpose::kRightBones; ++r) {
            if (!centreCount[r] || !centreCount[b])
                continue;
            const float dx = centre[b][0] - centre[r][0], dy = centre[b][1] - centre[r][1], dz = centre[b][2] - centre[r][2];
            const float d = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (d < nearest)
                nearest = d, rightOf[b] = r;
        }
    }
    // The palm and the cuff, as skin.cpp's kGlovesForearm has them: the cuff's bones 0, 4 and 5 are the forearm's twist bones, as the right's 37 and 44.
    rightOf[0] = 37, rightOf[1] = 32, rightOf[4] = 44, rightOf[5] = 44;
    auto leftBones = [&](const Frame& f, float (*rel)[12]) {
        float caseInverse[12];
        InvertRigid(f.partA[1], caseInverse);
        for (int b = 0; b < handpose::kBones; ++b) {
            float m[12];
            Compose(caseInverse, f.gloves[rightOf[b]], m);
            for (int i = 0; i < 3; ++i) {
                const float si = i == 0 ? -1.0f : 1.0f;
                rel[b][i * 4 + 0] = -si * m[i * 4 + 0];
                rel[b][i * 4 + 1] = si * m[i * 4 + 1];
                rel[b][i * 4 + 2] = si * m[i * 4 + 2];
                rel[b][i * 4 + 3] = si * m[i * 4 + 3];
            }
        }
    };
    auto placeVerts = [&](const float (*rel)[12], std::vector<float>& placed) {
        placed.resize(leftGlove.size() * 3);
        for (size_t i = 0; i < leftGlove.size(); ++i) {
            float sum[3] = {};
            for (int s = 0; s < 4; ++s) {
                if (leftGlove[i].weight[s] <= 0 || leftGlove[i].bone[s] >= handpose::kBones)
                    continue;
                float q[3];
                Apply(rel[leftGlove[i].bone[s]], leftGlove[i].pos, q);
                for (int k = 0; k < 3; ++k)
                    sum[k] += leftGlove[i].weight[s] * q[k];
            }
            std::memcpy(&placed[i * 3], sum, sizeof(sum));
        }
    };

    if (what == "wrist") {
        // The wrist's and palm's bones of each hand: their vertices (by the main bone and by any weight), and how they sit on each other at rest.
        auto report = [&](const std::vector<GloveVertex>& verts, int bone, float flip) {
            float main[3] = {}, any[3] = {}, anyWeight = 0.0f;
            int mainCount = 0, anyCount = 0;
            for (const GloveVertex& v : verts) {
                int best = 0;
                for (int s = 1; s < 4; ++s)
                    if (v.weight[s] > v.weight[best])
                        best = s;
                if (v.bone[best] == bone) {
                    main[0] += v.pos[0] * flip, main[1] += v.pos[1], main[2] += v.pos[2];
                    ++mainCount;
                }
                for (int s = 0; s < 4; ++s)
                    if (v.bone[s] == bone && v.weight[s] > 0.0f) {
                        any[0] += v.pos[0] * flip * v.weight[s], any[1] += v.pos[1] * v.weight[s], any[2] += v.pos[2] * v.weight[s];
                        anyWeight += v.weight[s];
                        ++anyCount;
                    }
            }
            std::printf("bone %2d: %4d vertices by the main bone, centre (%.3f %.3f %.3f); %4d by any weight, centre (%.3f %.3f %.3f)\n", bone, mainCount,
                        mainCount ? main[0] / mainCount : 0.0f, mainCount ? main[1] / mainCount : 0.0f, mainCount ? main[2] / mainCount : 0.0f, anyCount,
                        anyWeight > 0 ? any[0] / anyWeight : 0.0f, anyWeight > 0 ? any[1] / anyWeight : 0.0f, anyWeight > 0 ? any[2] / anyWeight : 0.0f);
        };
        for (int b : {0, 1, 4, 5})
            report(leftGlove, b, 1.0f);
        for (int b : {32, 37, 44})
            report(rightGlove, b, -1.0f);
        const Frame& rest = frames[10];
        auto relation = [&](int a, int b) {
            float inv[12], m[12];
            InvertRigid(rest.gloves[a], inv);
            Compose(inv, rest.gloves[b], m);
            std::printf("%2d on %2d: x axis (%.3f %.3f %.3f) y axis (%.3f %.3f %.3f) moved (%.3f %.3f %.3f)\n", b, a, m[0], m[4], m[8], m[1], m[5], m[9], m[3], m[7], m[11]);
        };
        relation(1, 0), relation(1, 4), relation(1, 5);
        relation(32, 37), relation(32, 44), relation(37, 44);
        return 0;
    }
    if (what == "radius") {
        // The case's real shape, its vertices as rings: z and radius to the millimetre, how many at each, and how far round they go.
        {
            std::map<std::pair<int, int>, int> rings;
            for (size_t i = 0; i + 2 < mesh.pos.size(); i += 3) {
                const float dy = mesh.pos[i + 1] - kAxisY;
                const float r = std::sqrt(mesh.pos[i] * mesh.pos[i] + dy * dy);
                ++rings[{static_cast<int>(std::lround(mesh.pos[i + 2] * 1000.0f)), static_cast<int>(std::lround(r * 1000.0f))}];
            }
            for (const auto& [where, n] : rings)
                if (n >= 6)
                    std::printf("ring z %5d mm radius %3d mm: %d vertices\n", where.first, where.second, n);
        }
        // The outer surface: where the triangles' edges cross each z, the farthest from the axis there (and the nearest).
        for (float z = -0.46f; z <= 0.10f; z += 0.04f) {
            float most = 0.0f, least = 1e9f;
            for (size_t t = 0; t + 2 < mesh.tris.size(); t += 3)
                for (int e = 0; e < 3; ++e) {
                    const float* a = &mesh.pos[mesh.tris[t + e] * 3];
                    const float* b = &mesh.pos[mesh.tris[t + (e + 1) % 3] * 3];
                    if ((a[2] < z) == (b[2] < z))
                        continue;
                    const float s = (z - a[2]) / (b[2] - a[2]);
                    const float x = a[0] + (b[0] - a[0]) * s, y = a[1] + (b[1] - a[1]) * s - kAxisY;
                    const float r = std::sqrt(x * x + y * y);
                    most = std::max(most, r), least = std::min(least, r);
                }
            std::printf("surface at z %5.2f: outer radius %.4f, inner %.4f\n", z, most, least);
        }
        // The case's real shape: how far its surface is from the axis, along it; and the game's hand's turn on it.
        constexpr float kStep = 0.02f;
        for (float z = -0.50f; z < 0.18f; z += kStep) {
            float most = 0.0f, least = 1e9f, sum = 0.0f;
            int n = 0;
            for (size_t i = 0; i + 2 < mesh.pos.size(); i += 3) {
                if (mesh.pos[i + 2] < z || mesh.pos[i + 2] >= z + kStep)
                    continue;
                const float dy = mesh.pos[i + 1] - kAxisY;
                const float r = std::sqrt(mesh.pos[i] * mesh.pos[i] + dy * dy);
                most = std::max(most, r), least = std::min(least, r), sum += r, ++n;
            }
            if (n)
                std::printf("z %6.2f: %3d vertices, radius %.4f to %.4f, mean %.4f\n", z, n, least, most, sum / n);
        }
        const Frame* f = FrameAt(frames, argc > 3 ? std::atoi(argv[3]) : 5641);
        float rel[handpose::kBones][12];
        leftBones(*f, rel);
        constexpr float kAlongHand[3] = {0.437f, -0.544f, 0.716f}, kAcrossHand[3] = {0.633f, -0.380f, -0.675f};
        float along[3], across[3];
        for (int i = 0; i < 3; ++i) {
            along[i] = rel[1][i * 4] * kAlongHand[0] + rel[1][i * 4 + 1] * kAlongHand[1] + rel[1][i * 4 + 2] * kAlongHand[2];
            across[i] = rel[1][i * 4] * kAcrossHand[0] + rel[1][i * 4 + 1] * kAcrossHand[1] + rel[1][i * 4 + 2] * kAcrossHand[2];
        }
        std::printf("the game's hand at %d ms: along the fingers (%.3f %.3f %.3f), across (%.3f %.3f %.3f): the tube's axis %.0f degrees off the across\n", f->ms, along[0], along[1],
                    along[2], across[0], across[1], across[2], std::acos(std::min(std::fabs(across[2]), 1.0f)) * 57.29578f);
        return 0;
    }
    if (what == "map") {
        for (int b = 0; b < handpose::kBones; ++b)
            std::printf("left %2d <- right %2d: %.1f mm apart, mirrored\n", b, rightOf[b],
                        1000.0f * std::sqrt(std::pow(centre[b][0] - centre[rightOf[b]][0], 2.0f) + std::pow(centre[b][1] - centre[rightOf[b]][1], 2.0f) +
                                            std::pow(centre[b][2] - centre[rightOf[b]][2], 2.0f)));
        return 0;
    }

    if (what == "score") {
        const int from = argc > 3 ? std::atoi(argv[3]) : 2800, to = argc > 4 ? std::atoi(argv[4]) : 6100;
        constexpr float kRadius = 0.0445f;  // the body's (the flange at its back is 6.8 cm: "radius")
        std::printf("    ms | palm: off the axis, z | deepest in the case (mm): palm thumb index middle ring little | touching (within 4 mm), each\n");
        int next = from;
        for (const Frame& f : frames) {
            if (f.ms < next || f.ms > to)
                continue;
            next = f.ms + 60;
            float rel[handpose::kBones][12];
            leftBones(f, rel);
            std::vector<float> placed;
            placeVerts(rel, placed);
            float deepest[6] = {};
            int touching[6] = {};
            for (size_t i = 0; i < leftGlove.size(); ++i) {
                if (leftGlove[i].finger < -1)
                    continue;
                const float* q = &placed[i * 3];
                const float dy = q[1] - kAxisY;
                const float d = std::sqrt(q[0] * q[0] + dy * dy) - kRadius;
                const int w = leftGlove[i].finger + 1;
                deepest[w] = std::max(deepest[w], -d);
                touching[w] += std::fabs(d) < 0.004f;
            }
            float palm[3];
            Apply(rel[1], kLeftPalm, palm);
            const float dy = palm[1] - kAxisY;
            std::printf("%6d | %.3f, %6.3f | %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f | %4d %4d %4d %4d %4d %4d\n", f.ms, std::sqrt(palm[0] * palm[0] + dy * dy), palm[2],
                        deepest[0] * 1000, deepest[1] * 1000, deepest[2] * 1000, deepest[3] * 1000, deepest[4] * 1000, deepest[5] * 1000, touching[0], touching[1],
                        touching[2], touching[3], touching[4], touching[5]);
        }
        return 0;
    }

    if (what == "left" && argc > 4) {
        const int rows = argc - 4;
        Image img(kTile * 4, kTile * rows);
        constexpr float kCaseColour[3] = {0.55f, 0.55f, 0.6f};
        constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f},
                                               {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
        const float scale = kTile / 0.26f;
        for (int row = 0; row < rows; ++row) {
            const Frame* f = FrameAt(frames, std::atoi(argv[4 + row]));
            float rel[handpose::kBones][12];
            leftBones(*f, rel);
            std::vector<float> placed;
            placeVerts(rel, placed);
            float palm[3];
            Apply(rel[1], kLeftPalm, palm);
            const float middle[3] = {0.0f, kAxisY, palm[2]};
            const View views[4] = {MakeView(0, 0, 1, 0, 1, 0), MakeView(0, 0, -1, 0, 1, 0), MakeView(1, 0, 0, 0, 0, 1), MakeView(-1, 0, 0, 0, 0, 1)};
            for (int v = 0; v < 4; ++v) {
                for (size_t t = 0; t + 2 < mesh.tris.size(); t += 3) {
                    // Looking down its length: a slice of the case at the hand, or it hides what's round it.
                    if (v < 2 && std::fabs(mesh.pos[mesh.tris[t] * 3 + 2] - palm[2]) > 0.012f)
                        continue;
                    DrawTriangle(img, v * kTile, row * kTile, views[v], middle, scale, &mesh.pos[mesh.tris[t] * 3], &mesh.pos[mesh.tris[t + 1] * 3],
                                 &mesh.pos[mesh.tris[t + 2] * 3], kCaseColour);
                }
                for (size_t t = 0; t + 2 < leftTris.size(); t += 3)
                    DrawTriangle(img, v * kTile, row * kTile, views[v], middle, scale, &placed[leftTris[t] * 3], &placed[leftTris[t + 1] * 3],
                                 &placed[leftTris[t + 2] * 3], kFingerColour[leftGlove[leftTris[t]].finger + 1]);
            }
        }
        img.Save(argv[3]);
        std::printf("saved %s\n", argv[3]);
        return 0;
    }

    // The tables for src/rocketposes.h: how the case sits in the hand (case -> the gloves' bind space, the palm bone where the hand is) and each left
    // bone from the bind space relative to the palm bone, as handpose::Pose gives them.
    if (what == "header") {
        const Frame* f = FrameAt(frames, argc > 3 ? std::atoi(argv[3]) : 0);
        float rel[handpose::kBones][12], palmInverse[12];
        leftBones(*f, rel);
        InvertRigid(rel[1], palmInverse);
        std::printf("// The game's own hand on the case at %d ms of its reload, its right hand mirrored.\n", f->ms);
        std::printf("constexpr float kCaseInHand[12] = {");
        for (int k = 0; k < 12; ++k)
            std::printf("%.5ff%s", palmInverse[k], k == 11 ? "};\n" : ", ");
        std::printf("constexpr float kFingers[handpose::kBones][12] = {\n");
        for (int b = 0; b < handpose::kBones; ++b) {
            float m[12];
            Compose(palmInverse, rel[b], m);
            std::printf("    {");
            for (int k = 0; k < 12; ++k)
                std::printf("%.5ff%s", m[k], k == 11 ? "},\n" : ", ");
        }
        std::printf("};\n");
        return 0;
    }
    std::fprintf(stderr, "what's that: %s\n", what.c_str());
    return 1;
}
