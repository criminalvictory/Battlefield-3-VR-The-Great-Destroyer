// The way a box drawn apart from its gun (the M249's cloth pouch, 987/27099 on its own palette's bone 0) comes off and goes on in the gun's own reload,
// from that reload's recording (weapon.txt "allrec": build\survey\M249_empty.bin), as the tables of src/magposes/<gun>_way.h (handmag::BoxWay): the
// owner's rule, the game's own animation first, and the review of 2026-11-18 ("the box comes off a different way from the game's").
//   boxway <recording> <body count>@<start> <frame bone> <box count>@<start> <box bone> <grab x y z> <middle x y z> <off from ms> <off to ms>
//          <on from ms> <on to ms> <name> <out.h>
// grab: where the hand takes it, on the gun's frame with it seated (handmag::Gun::grab); middle: its own middle there. Each frame's box against its
// seat (the box on the frame in the recording's first frame), from the first eye's draws of the frame (the game draws the body before the box).
//   off: from the seat, each frame of that stretch that has gone further along the way than any before it (the game's own nudge the other way first,
//        7.6 mm to the right before it slides off, isn't kept: the way starts at the seat): {metres the grab point has come along the way, the box's
//        place on the frame from its seat (3x4)}; free: the furthest along it still within 1 cm of the seat's height (it slides off level, then
//        drops away; on past that the rows go on as the game's own way goes, so a pull that goes on with it still counts).
//   on: from the first frame of that stretch with its middle back within 1 cm of the seat's height to the seat: {seconds from that frame (its frames
//        evenly over the stretch's time: two frames can share a millisecond), its place from its seat}; the seat itself last, exactly.
// Prints each row and how far it goes, turns and drops.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] = a[i * 4] * b[3] + a[i * 4 + 1] * b[7] + a[i * 4 + 2] * b[11] + a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
void Invert(const float* m, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = -(r[i * 4] * m[3] + r[i * 4 + 1] * m[7] + r[i * 4 + 2] * m[11]);
    std::memcpy(out, r, sizeof(r));
}
void Apply(const float* m, const float* p, float* o) {
    float r[3];
    for (int k = 0; k < 3; ++k)
        r[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
    std::memcpy(o, r, sizeof(r));
}
float Turn(const float* m) {  // degrees from none
    const float c = std::clamp((m[0] + m[5] + m[10] - 1.0f) * 0.5f, -1.0f, 1.0f);
    return std::acos(c) * 57.29578f;
}
void ParseDraw(const char* text, int& count, int& start) {
    count = std::atoi(text);
    const char* at = std::strchr(text, '@');
    start = at ? std::atoi(at + 1) : 0;
}

struct Frame {
    int ms;
    float moved[12];  // the box on the frame from its seat
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 18) {
        std::fprintf(stderr, "usage: boxway <recording> <body count>@<start> <frame bone> <box count>@<start> <box bone> <grab x y z> <middle x y z> <off from ms> <off to ms> "
                             "<on from ms> <on to ms> <name> <out.h>\n");
        return 1;
    }
    int bodyCount = 0, bodyStart = 0, boxCount = 0, boxStart = 0;
    ParseDraw(argv[2], bodyCount, bodyStart);
    const int frameBone = std::atoi(argv[3]);
    ParseDraw(argv[4], boxCount, boxStart);
    const int boxBone = std::atoi(argv[5]);
    const float grab[3] = {static_cast<float>(std::atof(argv[6])), static_cast<float>(std::atof(argv[7])), static_cast<float>(std::atof(argv[8]))};
    const float middle[3] = {static_cast<float>(std::atof(argv[9])), static_cast<float>(std::atof(argv[10])), static_cast<float>(std::atof(argv[11]))};
    const int offFrom = std::atoi(argv[12]), offTo = std::atoi(argv[13]), onFrom = std::atoi(argv[14]), onTo = std::atoi(argv[15]);
    const char* name = argv[16];
    const char* outPath = argv[17];

    // The recording: records of 8 ints (the draw's count, the frame's serial, the eye, its start, the rounds loaded, the spare, ms, bones) and that many bones.
    FILE* f = nullptr;
    if (fopen_s(&f, argv[1], "rb") || !f) {
        std::fprintf(stderr, "can't read %s\n", argv[1]);
        return 1;
    }
    std::vector<Frame> frames;
    float seat[12] = {}, seatInverse[12] = {};
    bool haveSeat = false;
    int head[8], first = -1, serial = -1;
    bool haveBody = false, haveBox = false;
    float body[12] = {}, box[12] = {};
    int frameMs = 0;
    std::vector<float> bones;
    auto close = [&] {
        if (!haveBody || !haveBox)
            return;
        float inverse[12], on[12];
        Invert(body, inverse);
        Compose(inverse, box, on);
        if (!haveSeat) {
            std::memcpy(seat, on, sizeof(seat));
            Invert(seat, seatInverse);
            haveSeat = true;
        }
        Frame fr;
        fr.ms = frameMs;
        Compose(on, seatInverse, fr.moved);
        frames.push_back(fr);
    };
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        bones.resize(static_cast<size_t>(n) * 12);
        if (fread(bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        if (first < 0)
            first = head[6];
        if (head[1] != serial) {
            close();
            serial = head[1];
            frameMs = head[6] - first;
            haveBody = haveBox = false;
        }
        if (!haveBody && head[0] == bodyCount && head[3] == bodyStart && n > frameBone) {
            std::memcpy(body, &bones[static_cast<size_t>(frameBone) * 12], sizeof(body));
            haveBody = true;
        } else if (!haveBox && head[0] == boxCount && head[3] == boxStart && n > boxBone) {
            std::memcpy(box, &bones[static_cast<size_t>(boxBone) * 12], sizeof(box));
            haveBox = true;
        }
    }
    close();
    fclose(f);
    std::printf("%zu frames with the gun's body and its box\n", frames.size());
    if (frames.size() < 10)
        return 1;

    // The way off: the grab point's move from the seat; each frame further along the way's overall line than any before.
    float wayEnd[3] = {};
    for (const Frame& fr : frames)
        if (fr.ms <= offTo) {
            float at[3];
            Apply(fr.moved, grab, at);
            for (int k = 0; k < 3; ++k)
                wayEnd[k] = at[k] - grab[k];
        }
    const float endLength = std::sqrt(wayEnd[0] * wayEnd[0] + wayEnd[1] * wayEnd[1] + wayEnd[2] * wayEnd[2]);
    if (endLength < 0.02f) {
        std::printf("it hasn't gone 2 cm by %d ms\n", offTo);
        return 1;
    }
    struct Row {
        float at;
        float pose[12];
    };
    std::vector<Row> off;
    {
        Row seatRow{0.0f, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}};
        off.push_back(seatRow);
    }
    float lastAlong = 0.0f, lastMove[3] = {}, freeAt = 0.0f;
    bool dropped = false;
    for (const Frame& fr : frames) {
        if (fr.ms < offFrom || fr.ms > offTo)
            continue;
        float at[3], move[3], mid[3];
        Apply(fr.moved, grab, at);
        for (int k = 0; k < 3; ++k)
            move[k] = at[k] - grab[k];
        const float along = (move[0] * wayEnd[0] + move[1] * wayEnd[1] + move[2] * wayEnd[2]) / endLength;
        if (along <= lastAlong + 1e-4f)
            continue;
        const float step = std::sqrt((move[0] - lastMove[0]) * (move[0] - lastMove[0]) + (move[1] - lastMove[1]) * (move[1] - lastMove[1]) + (move[2] - lastMove[2]) * (move[2] - lastMove[2]));
        Row r;
        r.at = off.back().at + step;
        std::memcpy(r.pose, fr.moved, sizeof(r.pose));
        off.push_back(r);
        lastAlong = along;
        std::memcpy(lastMove, move, sizeof(move));
        Apply(fr.moved, middle, mid);
        const float drop = middle[1] - mid[1];
        if (!dropped && drop < 0.01f)
            freeAt = r.at;
        else
            dropped = true;
        std::printf("off %5d ms: %.4f m along; the grab point moved (%+.4f %+.4f %+.4f), the middle dropped %.1f mm, turned %.2f degrees\n", fr.ms, r.at, move[0], move[1], move[2],
                    drop * 1000.0f, Turn(fr.moved));
    }
    std::printf("free %.4f m along (the last within 1 cm of the seat's height)\n", freeAt);

    // The way on: from the first frame back at the seat's height to the seat.
    std::vector<Row> on;
    int startIndex = -1, endIndex = -1;
    for (size_t i = 0; i < frames.size(); ++i) {
        const Frame& fr = frames[i];
        if (fr.ms < onFrom || fr.ms > onTo)
            continue;
        float mid[3];
        Apply(fr.moved, middle, mid);
        if (startIndex < 0 && std::fabs(mid[1] - middle[1]) < 0.01f)
            startIndex = static_cast<int>(i);
        if (startIndex >= 0)
            endIndex = static_cast<int>(i);
    }
    if (startIndex < 0 || endIndex <= startIndex) {
        std::printf("no way on in %d to %d ms\n", onFrom, onTo);
        return 1;
    }
    // Home: the first frame from which it stays within 0.2 mm and 0.1 degrees of the seat.
    for (int i = startIndex; i <= endIndex; ++i) {
        float mid[3];
        Apply(frames[static_cast<size_t>(i)].moved, middle, mid);
        const float apart = std::sqrt((mid[0] - middle[0]) * (mid[0] - middle[0]) + (mid[1] - middle[1]) * (mid[1] - middle[1]) + (mid[2] - middle[2]) * (mid[2] - middle[2]));
        if (apart < 0.0002f && Turn(frames[static_cast<size_t>(i)].moved) < 0.1f) {
            endIndex = i;
            break;
        }
    }
    const float seconds = (frames[static_cast<size_t>(endIndex)].ms - frames[static_cast<size_t>(startIndex)].ms) / 1000.0f;
    const int steps = endIndex - startIndex;
    for (int i = startIndex; i <= endIndex; ++i) {
        Row r;
        r.at = seconds * static_cast<float>(i - startIndex) / static_cast<float>(steps);
        if (i == endIndex) {
            const float seatPose[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            std::memcpy(r.pose, seatPose, sizeof(r.pose));
        } else {
            std::memcpy(r.pose, frames[static_cast<size_t>(i)].moved, sizeof(r.pose));
        }
        on.push_back(r);
        float mid[3];
        Apply(r.pose, middle, mid);
        std::printf("on  %5d ms: %.3f s; the middle at (%+.4f %+.4f %+.4f) from its seat, turned %.2f degrees\n", frames[static_cast<size_t>(i)].ms, r.at, mid[0] - middle[0],
                    mid[1] - middle[1], mid[2] - middle[2], Turn(r.pose));
    }

    FILE* o = nullptr;
    if (fopen_s(&o, outPath, "w") || !o) {
        std::printf("can't write %s\n", outPath);
        return 1;
    }
    std::fprintf(o, "// Written by tools\\boxway.cpp from %s: its box's way off (%d to %d ms) and on (%d to %d ms) in the gun's own reload, each row {how far along it\n", argv[1], offFrom, offTo, onFrom, onTo);
    std::fprintf(o, "// (off: metres its grab point has come; on: seconds), the box on the gun's frame from its seat (3x4)}. Off: free %.1f cm along, the last within 1 cm of\n", freeAt * 100.0f);
    std::fprintf(o, "// the seat's height; on: from its middle back at the seat's height, %.2f s, to the seat.\n", seconds);
    auto write = [&](const char* table, const std::vector<Row>& rows) {
        std::fprintf(o, "inline constexpr float k%s%s[%zu][13] = {\n", name, table, rows.size());
        for (const Row& r : rows) {
            std::fprintf(o, "    {%.4ff", r.at);
            for (int k = 0; k < 12; ++k)
                std::fprintf(o, ", %.5ff", r.pose[k]);
            std::fprintf(o, "},\n");
        }
        std::fprintf(o, "};\n");
    };
    write("WayOff", off);
    std::fprintf(o, "inline constexpr float k%sWayFree = %.4ff;\n", name, freeAt);
    write("WayOn", on);
    fclose(o);
    std::printf("wrote %s: %zu rows off (%.1f cm), %zu on (%.2f s)\n", outPath, off.size(), off.back().at * 100.0f, on.size(), seconds);
    return 0;
}
