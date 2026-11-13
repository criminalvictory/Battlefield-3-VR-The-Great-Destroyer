#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
// The rules of the game's dropped guns that need no game: tools/picktest.cpp runs them. Matrices are three rows, rotation | translation, unless said.
namespace pickrules {

// The gun's drawn pose (its third-person mesh's model -> the world) from the pickup's transform (16 floats: rows right, up, forward, translation) and its
// gun's PickupSettings, as the client draws it at rest (0084A7C0): turned in its own axes by y, then x, then z (degrees), then moved by the offset in
// the world. A number not finite: false.
bool DrawnPose(const float* gameTransform16, const float* offset3, const float* rotationDegrees3, float* out12);

// A pickup's countdown by the policy. seconds: 0 the game's own, -1 never, N > 0 that long. own: it holds a gun that left the player's slots; then
// ownSeconds is the policy unless it's 0 (-1 never, N). unseen: kept while in view. rate: seconds of its countdown a second as last measured between
// two looks (1 if not known; taken within 1 to 60). raisedOnce: the mod has already moved this pickup's countdown for its present content (the caller
// resets it when the first blueprint changes or +0x10C jumps back up to TimeToLive). True with out when +0x10C should be written.
bool CountdownFor(int seconds, bool own, int ownSeconds, bool unseen, float ttl, float left, float rate, bool inView, bool& raisedOnce, float& out);

// Within 70 degrees of the look and 80 m of the eye.
bool InView(const float* eye3, const float* look3, const float* at3);

// A slot's weapon, its firing state, its blueprint. A new object at a freed one's address with another blueprint is another gun.
struct Triple {
    uint32_t weapon = 0, firing = 0, blueprint = 0;
    bool operator==(const Triple& o) const { return weapon == o.weapon && firing == o.firing && blueprint == o.blueprint; }
};
// The slots whose triple changed (indexes into changed; how many).
int Diff(const Triple* before, const Triple* after, int slots, int* changed);
// A take has landed: any slot's triple changed, or the counts in the taken entry's slot aren't the ones there right before it (loadedBefore as written by
// the hand-over, spareBefore as read then). Only the same triples with both counts the same is "the game swapped nothing".
bool Landed(const Triple* before, const Triple* after, int slots, int slot, int loadedBefore, int spareBefore, int loadedAfter, int spareAfter);
// The one slot in the game's hand by its firing state's +0xA8 word: a weapon there, the word neither 0 (never in hand) nor 3 (put away). None, or more
// than one: -1.
int ActiveSlot(const Triple* slots, const int* stateA8, int count);

// A request's life on the game's thread: 500 ms from when it was asked (GetTickCount64's).
bool Fresh(unsigned long long askedAt, unsigned long long now);

// A pose, or a point, of the world (absolute: the arms' space plus its origin in whole metres) into the room: armsToRoom the inverse of loose's
// roomToArms, origin its Frame::origin. The origin comes off first, so the floats stay small.
void WorldToRoom(const float* armsToRoom12, const int* origin3, const float* world12, float* room12);
void WorldPointToRoom(const float* armsToRoom12, const int* origin3, const float* world3, float* room3);

// An arrival's start when the two meshes' frames aren't known to agree: the gun turned as the hand will hold it (handGunRoom's rotation), the middle of
// its box (model) on centreRoom. A number not finite, or a box with no size: false.
bool StartAtCentre(const float* handGunRoom12, const float* boxMin3, const float* boxMax3, const float* centreRoom3, float* out12);

// ---- What the render thread makes of a swap (hands.cpp's NoteSwaps, ammo.cpp's Swapped): pure, so picktest runs it. ----

// A gun of the player's is forgotten by ammo at a swap only when its own slot is among the swap's, or its firing state is among those that went. A
// swap that leaves a slot alone leaves that slot's gun as it was (its magazine out, the grenade launcher's count).
bool Forgets(const int* slot, const uint32_t* wentFiring, int changed, int ownSlot, uint32_t ownFiring);

// One swap as hands.cpp sees it.
struct SwapSeen {
    bool lies = false;                  // the player's gun lies: a loose body of it, the grasp in the world
    int lyingSlot = -1;                 // ... the slot it's in, as the game has it
    unsigned long long releasedAt = 0;  // ... when it was let go of (GetTickCount64)
    bool retired = false;               // ... retired already by an earlier swap
    unsigned long long at = 0;          // the swap: when it was made
    int changed = 0;                    // ... its slots, and what each held before
    const int* slot = nullptr;
    const uint32_t* wentBlueprint = nullptr;
    uint32_t takenBlueprint = 0;        // ... its taken entry's blueprint, and the slot that entry goes into
    int takenSlot = -1;
    bool forceSlot = false;             // ... the pickup makes the game select that slot
    bool byHand = false;                // ... a hand's take, the request it answers, the hand that asked
    uint32_t request = 0;
    int side = -1;
    uint32_t waiting = 0;               // the take hands.cpp waits on (0 none)
    uint32_t knownId = 0;               // the profile id free hands will have for the taken gun (0: none)
    bool noRow = false;                 // the guns' table has no row for its name: what it is can't be told before it's drawn
};
// What follows from it.
struct SwapVerdict {
    bool retire = false;        // the gun that lies went into the pickup: retired from now (false if it already was)
    bool answered = false;      // it answers the take waiting: that request is over
    int landedSide = -1;        // ... and its gun is due in that hand,
    uint32_t landedId = 0;      // ... as that profile,
    bool landedAny = false;     // ... or as any profile other than the one in force now (no row for its name)
    bool sameKind = false;      // ... the same gun as the one that lay: no change of gun will come
    bool refused = false;       // the grasp's wait ends now, with nothing in a hand
    bool freedAtOnce = false;   // ... because the game won't select the taken gun (another slot, not forced); its arrival stays due should it come
    bool unknown = false;       // ... because free hands don't know the gun
    bool swappedAway = false;   // the game's own pickup of the same gun as the one that lay: back in the right hand now
};
SwapVerdict OnSwap(const SwapSeen& s);

// The net, each frame after the swaps: a landed take whose gun isn't in the hand kNetMs after it landed is given up; a gun retired kNetMs ago with no
// take due goes back to the right hand. swappedAway is never true unless the gun still lies (raised with a gun in a hand, it would clear the grasp
// every frame).
constexpr unsigned long long kNetMs = 1500;
struct NetVerdict {
    bool landedOver = false;    // the landed take is given up
    bool refused = false;       // the grasp's wait ends
    bool swappedAway = false;   // back in the right hand, as the game has it
    bool retiredOver = false;   // the retired copy went some other way: nothing to give back
};
NetVerdict Net(bool lies, bool retired, unsigned long long retiredAt, int landedSide, unsigned long long landedAt, unsigned long long now);

// The seqlock pair (the writer bumps to odd, writes, bumps to even; the reader copies and compares), templated on the payload.
template <class T> struct SeqLock {
    std::atomic<uint32_t> seq{0};
    T value{};
    void Write(const T& v);
    bool Read(T& out, int tries = 4) const;
};

}  // namespace pickrules

// ---- The bodies of the above (inline: this header is all of it). ----
namespace pickrules {

namespace detail {
inline bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}
// a times b, both 3x3 (rows), into out (not a or b).
inline void Mul3(const double* a, const double* b, double* out) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            out[3 * r + c] = a[3 * r] * b[c] + a[3 * r + 1] * b[3 + c] + a[3 * r + 2] * b[6 + c];
}
}  // namespace detail

// The client's three turns as it builds them (rows, a point times them; cos and sin of degrees x pi / 180): y (00428D30) rows (c 0 -s) (0 1 0)
// (s 0 c), x (00557AA0) rows (1 0 0) (0 c s) (0 -s c), z (00557B50) rows (c s 0) (-s c 0) (0 0 1). Each is put before the transform in turn, so
// the drawn rows are Rz Rx Ry M; the offset is then added to the translation.
inline bool DrawnPose(const float* gameTransform16, const float* offset3, const float* rotationDegrees3, float* out12) {
    if (!gameTransform16 || !offset3 || !rotationDegrees3 || !out12 || !detail::Finite(gameTransform16, 16) || !detail::Finite(offset3, 3) ||
        !detail::Finite(rotationDegrees3, 3))
        return false;
    const double toRadians = 3.14159274101257 / 180.0;  // the game's pi (0206EBE0) over its 180 (0206EBD8)
    const double ax = rotationDegrees3[0] * toRadians, ay = rotationDegrees3[1] * toRadians, az = rotationDegrees3[2] * toRadians;
    const double cx = std::cos(ax), sx = std::sin(ax), cy = std::cos(ay), sy = std::sin(ay), cz = std::cos(az), sz = std::sin(az);
    const double ry[9] = {cy, 0.0, -sy, 0.0, 1.0, 0.0, sy, 0.0, cy};
    const double rx[9] = {1.0, 0.0, 0.0, 0.0, cx, sx, 0.0, -sx, cx};
    const double rz[9] = {cz, sz, 0.0, -sz, cz, 0.0, 0.0, 0.0, 1.0};
    double m[9];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            m[3 * r + c] = gameTransform16[4 * r + c];
    double a[9], b[9];
    detail::Mul3(ry, m, a);
    detail::Mul3(rx, a, b);
    detail::Mul3(rz, b, a);
    // Its rows are the model's axes in the world: here they're the columns.
    float out[12];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = static_cast<float>(a[3 * c + r]);
        out[4 * r + 3] = gameTransform16[12 + r] + offset3[r];
    }
    if (!detail::Finite(out, 12))
        return false;
    std::memcpy(out12, out, sizeof(out));
    return true;
}

inline bool CountdownFor(int seconds, bool own, int ownSeconds, bool unseen, float ttl, float left, float rate, bool inView, bool& raisedOnce,
                         float& out) {
    if (!std::isfinite(ttl) || !std::isfinite(left) || !(ttl > 0.0f))
        return false;
    const float r = std::isfinite(rate) ? std::fmin(std::fmax(rate, 1.0f), 60.0f) : 1.0f;
    const int policy = own && ownSeconds != 0 ? ownSeconds : seconds;
    bool write = false;
    float v = left;
    if (policy > 0 && !raisedOnce) {
        v = std::fmax(0.5f, left + (static_cast<float>(policy) - ttl));
        raisedOnce = true;
        write = true;
    } else if (policy == -1 && left < std::fmax(30.0f, 3.0f * r)) {
        v = ttl;
        write = true;
    }
    const float hold = std::fmax(2.0f, 3.0f * r);
    if (unseen && inView && v < hold) {
        v = hold;
        write = true;
    }
    if (write)
        out = v;
    return write;
}

inline bool InView(const float* eye3, const float* look3, const float* at3) {
    if (!eye3 || !look3 || !at3 || !detail::Finite(eye3, 3) || !detail::Finite(look3, 3) || !detail::Finite(at3, 3))
        return false;
    const double d[3] = {static_cast<double>(at3[0]) - eye3[0], static_cast<double>(at3[1]) - eye3[1], static_cast<double>(at3[2]) - eye3[2]};
    const double distance = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    const double lookLength = std::sqrt(static_cast<double>(look3[0]) * look3[0] + static_cast<double>(look3[1]) * look3[1] +
                                        static_cast<double>(look3[2]) * look3[2]);
    if (distance > 80.0 || lookLength < 1e-6)
        return false;
    if (distance < 1e-6)
        return true;  // at the eye
    const double cosine = (d[0] * look3[0] + d[1] * look3[1] + d[2] * look3[2]) / (distance * lookLength);
    return cosine >= std::cos(70.0 * 3.14159265358979323846 / 180.0);
}

inline int Diff(const Triple* before, const Triple* after, int slots, int* changed) {
    if (!before || !after)
        return 0;
    int n = 0;
    for (int i = 0; i < slots; ++i)
        if (!(before[i] == after[i])) {
            if (changed)
                changed[n] = i;
            ++n;
        }
    return n;
}

inline bool Landed(const Triple* before, const Triple* after, int slots, int slot, int loadedBefore, int spareBefore, int loadedAfter, int spareAfter) {
    if (Diff(before, after, slots, nullptr) > 0)
        return true;
    return slot >= 0 && slot < slots && (loadedAfter != loadedBefore || spareAfter != spareBefore);
}

// (A word not read, below 0, says nothing either way.)
inline int ActiveSlot(const Triple* slots, const int* stateA8, int count) {
    if (!slots || !stateA8)
        return -1;
    int found = -1;
    for (int i = 0; i < count; ++i) {
        if (!slots[i].weapon || stateA8[i] <= 0 || stateA8[i] == 3)
            continue;
        if (found >= 0)
            return -1;
        found = i;
    }
    return found;
}

inline bool Fresh(unsigned long long askedAt, unsigned long long now) {
    return now >= askedAt && now - askedAt <= 500;
}

inline void WorldPointToRoom(const float* armsToRoom12, const int* origin3, const float* world3, float* room3) {
    const double a[3] = {static_cast<double>(world3[0]) - origin3[0], static_cast<double>(world3[1]) - origin3[1],
                         static_cast<double>(world3[2]) - origin3[2]};
    float out[3];
    for (int r = 0; r < 3; ++r)
        out[r] = static_cast<float>(armsToRoom12[4 * r] * a[0] + armsToRoom12[4 * r + 1] * a[1] + armsToRoom12[4 * r + 2] * a[2] + armsToRoom12[4 * r + 3]);
    std::memcpy(room3, out, sizeof(out));
}

inline void WorldToRoom(const float* armsToRoom12, const int* origin3, const float* world12, float* room12) {
    const float t[3] = {world12[3], world12[7], world12[11]};
    float out[12], moved[3];
    WorldPointToRoom(armsToRoom12, origin3, t, moved);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = static_cast<float>(static_cast<double>(armsToRoom12[4 * r]) * world12[c] +
                                                static_cast<double>(armsToRoom12[4 * r + 1]) * world12[4 + c] +
                                                static_cast<double>(armsToRoom12[4 * r + 2]) * world12[8 + c]);
        out[4 * r + 3] = moved[r];
    }
    std::memcpy(room12, out, sizeof(out));
}

inline bool StartAtCentre(const float* handGunRoom12, const float* boxMin3, const float* boxMax3, const float* centreRoom3, float* out12) {
    if (!handGunRoom12 || !boxMin3 || !boxMax3 || !centreRoom3 || !out12 || !detail::Finite(handGunRoom12, 12) || !detail::Finite(boxMin3, 3) ||
        !detail::Finite(boxMax3, 3) || !detail::Finite(centreRoom3, 3))
        return false;
    float mid[3];
    for (int i = 0; i < 3; ++i) {
        if (!(boxMax3[i] - boxMin3[i] > 0.0f))
            return false;
        mid[i] = 0.5f * (boxMin3[i] + boxMax3[i]);
    }
    float out[12];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = handGunRoom12[4 * r + c];
        out[4 * r + 3] = centreRoom3[r] - (handGunRoom12[4 * r] * mid[0] + handGunRoom12[4 * r + 1] * mid[1] + handGunRoom12[4 * r + 2] * mid[2]);
    }
    if (!detail::Finite(out, 12))
        return false;
    std::memcpy(out12, out, sizeof(out));
    return true;
}

inline bool Forgets(const int* slot, const uint32_t* wentFiring, int changed, int ownSlot, uint32_t ownFiring) {
    for (int i = 0; i < changed; ++i) {
        if (slot && slot[i] == ownSlot)
            return true;
        if (ownFiring && wentFiring && wentFiring[i] == ownFiring)
            return true;
    }
    return false;
}

inline SwapVerdict OnSwap(const SwapSeen& s) {
    SwapVerdict v;
    int at = -1;
    if (s.lies && s.lyingSlot >= 0 && s.slot)
        for (int i = 0; i < s.changed; ++i)
            if (s.slot[i] == s.lyingSlot) {
                at = i;
                break;
            }
    const bool took = s.lies && at >= 0 && s.at >= s.releasedAt;
    v.retire = took && !s.retired;
    v.sameKind = took && s.takenBlueprint != 0 && s.wentBlueprint && s.takenBlueprint == s.wentBlueprint[at];
    v.answered = s.byHand && s.waiting != 0 && s.request == s.waiting;
    if (v.answered) {
        if (s.side != 0 && s.side != 1) {
            v.refused = true;
        } else if (s.knownId) {
            v.landedSide = s.side;
            v.landedId = s.knownId;
        } else if (s.noRow) {
            v.landedSide = s.side;
            v.landedAny = true;
        } else {
            v.refused = true;
            v.unknown = true;
        }
        if (v.landedSide >= 0 && !took && !s.forceSlot) {
            v.refused = true;
            v.freedAtOnce = true;
        }
    } else if (!s.byHand && took && v.sameKind) {
        v.swappedAway = true;
    }
    return v;
}

inline NetVerdict Net(bool lies, bool retired, unsigned long long retiredAt, int landedSide, unsigned long long landedAt, unsigned long long now) {
    NetVerdict v;
    if (landedSide >= 0 && now > landedAt && now - landedAt > kNetMs) {
        v.landedOver = true;
        v.refused = true;
        if (retired && lies)
            v.swappedAway = true;
    }
    if (retired && !lies)
        v.retiredOver = true;
    else if (retired && (landedSide < 0 || v.landedOver) && now > retiredAt && now - retiredAt > kNetMs)
        v.swappedAway = true;
    return v;
}

// One writer thread; readers on any, never holding it up. Read is false while it writes (after the tries), and before anything was written.
template <class T> inline void SeqLock<T>::Write(const T& v) {
    const uint32_t s = seq.load(std::memory_order_relaxed);
    seq.store(s + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(static_cast<void*>(&value), &v, sizeof(T));
    std::atomic_thread_fence(std::memory_order_release);
    seq.store(s + 2, std::memory_order_release);
}

template <class T> inline bool SeqLock<T>::Read(T& out, int tries) const {
    for (int i = 0; i < tries; ++i) {
        const uint32_t before = seq.load(std::memory_order_acquire);
        if (before == 0)
            return false;
        if (before & 1)
            continue;
        std::memcpy(static_cast<void*>(&out), &value, sizeof(T));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq.load(std::memory_order_relaxed) == before)
            return true;
    }
    return false;
}

}  // namespace pickrules
