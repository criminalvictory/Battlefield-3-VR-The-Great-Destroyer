// The player's own key bindings (src/keybind.h, header only, as keybind.cpp reads the game's settings file and input.cpp's SendKey sends each key)
// without the game.
//   1  the game's default bindings as its settings file has them (the test PC's, 2026-11-25): every one of the mod's keys today's, nothing said
//   2  no file, a file with no key bindings, a file cut short: today's keys
//   3  a player's own keys followed: crouch C, prone V, use F, reload Q, jump the right Ctrl (extended), sprint Right Shift, the weapons' keys
//   4  the walking keys by their axes' direction: W/S and A/D, the arrows (extended), a slot of the other direction never taken
//   5  an action with no key the mod sends: on the mouse alone, the pad alone, unbound, or only on one of the mod's own keys (Insert, F9, Home,
//      Escape, Caps Lock, a Windows key): nothing pressed for it, said; a second slot's key taken over a first refused
//   6  the quick-time actions: the game's own keys without lines; a line for one followed
//   7  the keys the mod sends at all: the main block and the number pad yes; F1-F12, Escape, Home, End, Insert, Delete, Page Up/Down, Pause, Scroll,
//      Num and Caps Lock, Print Screen, the Windows, menu, media and power keys never; scan codes and extended flags
//   8  broken lines: CR LF, tabs, spaces, other contexts and settings, slots past eight, numbers not numbers, a field not known, a cut last line
//   9  the log's words: the keys moved, nothing to press, two of the mod's keys on one, Fire and Zoom off their mouse buttons; the key names
//   10 this PC's own settings file (read only; SKIP without it): read, its lines all understood, what the log would say
//   11 the time a whole file takes
// Usage: keybindtest
// The exit code is the number of failed checks.

#include <windows.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/keybind.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

// The game's default infantry bindings as its settings file has them (the test PC's PROF_SAVE_profile, 2026-11-25: the concepts the mod presses,
// a few it doesn't, another context's and other settings), CR LF as the game writes it.
const char kDefaultFile[] =
    "GstAudio.Volume 1.000000\r\n"
    "GstInput.MouseSensitivity 0.462209\r\n"
    "GstKeyBinding.default.ConceptMenu.0.axis 24\r\n"
    "GstKeyBinding.default.ConceptMenu.0.button 12\r\n"
    "GstKeyBinding.default.ConceptMenu.0.negate 0\r\n"
    "GstKeyBinding.default.ConceptMenu.0.type 2\r\n"
    "GstKeyBinding.default.ConceptMenu.2.button 1\r\n"
    "GstKeyBinding.default.ConceptMenu.2.type 0\r\n"
    "GstKeyBinding.infantry.ConceptBreathControl.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptBreathControl.0.button 42\r\n"
    "GstKeyBinding.infantry.ConceptBreathControl.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptBreathControl.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.0.button 11\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.1.button 255\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptCrawl.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouch.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouch.0.button 45\r\n"
    "GstKeyBinding.infantry.ConceptCrouch.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouch.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouchOnHold.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouchOnHold.0.button 29\r\n"
    "GstKeyBinding.infantry.ConceptCrouchOnHold.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptCrouchOnHold.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptFire.0.button 15\r\n"
    "GstKeyBinding.infantry.ConceptFire.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptFire.1.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptFire.1.button 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.1.type 1\r\n"
    "GstKeyBinding.infantry.ConceptFire.2.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.2.button 255\r\n"
    "GstKeyBinding.infantry.ConceptFire.2.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptFire.2.type 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.0.button 18\r\n"
    "GstKeyBinding.infantry.ConceptInteract.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.1.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptInteract.1.button 7\r\n"
    "GstKeyBinding.infantry.ConceptInteract.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.1.type 2\r\n"
    "GstKeyBinding.infantry.ConceptInteract.2.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptInteract.2.button 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.2.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptInteract.2.type 1\r\n"
    "GstKeyBinding.infantry.ConceptJump.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptJump.0.button 5\r\n"
    "GstKeyBinding.infantry.ConceptJump.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptJump.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptJump.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptJump.1.button 57\r\n"
    "GstKeyBinding.infantry.ConceptJump.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptJump.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.0.button 17\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.1.button 33\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMeleeAttack.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.0.axis 1\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.0.button 60\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.1.button 17\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.2.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.2.button 31\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.2.negate 1\r\n"
    "GstKeyBinding.infantry.ConceptMoveFB.2.type 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.0.button 60\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.1.button 30\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.1.negate 1\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.2.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.2.button 32\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.2.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptMoveLR.2.type 0\r\n"
    "GstKeyBinding.infantry.ConceptPitch.0.axis 7\r\n"
    "GstKeyBinding.infantry.ConceptPitch.0.button 60\r\n"
    "GstKeyBinding.infantry.ConceptPitch.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptPitch.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptPitch.1.axis 1\r\n"
    "GstKeyBinding.infantry.ConceptPitch.1.button 8\r\n"
    "GstKeyBinding.infantry.ConceptPitch.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptPitch.1.type 1\r\n"
    "GstKeyBinding.infantry.ConceptProne.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptProne.0.button 44\r\n"
    "GstKeyBinding.infantry.ConceptProne.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptProne.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptReload.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptReload.0.button 19\r\n"
    "GstKeyBinding.infantry.ConceptReload.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptReload.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptReload.1.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptReload.1.button 6\r\n"
    "GstKeyBinding.infantry.ConceptReload.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptReload.1.type 2\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.button 2\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem2.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem2.0.button 3\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem2.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem2.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem4.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem4.0.button 4\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem4.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem4.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.button 5\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptSprint.0.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptSprint.0.button 42\r\n"
    "GstKeyBinding.infantry.ConceptSprint.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSprint.0.type 0\r\n"
    "GstKeyBinding.infantry.ConceptSprint.1.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptSprint.1.button 10\r\n"
    "GstKeyBinding.infantry.ConceptSprint.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptSprint.1.type 2\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.0.button 16\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.0.type 2\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.1.axis 0\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.1.button 34\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptThrowGrenade.1.type 0\r\n"
    "GstKeyBinding.infantry.ConceptZoom.0.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptZoom.0.button 1\r\n"
    "GstKeyBinding.infantry.ConceptZoom.0.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptZoom.0.type 1\r\n"
    "GstKeyBinding.infantry.ConceptZoom.1.axis 24\r\n"
    "GstKeyBinding.infantry.ConceptZoom.1.button 14\r\n"
    "GstKeyBinding.infantry.ConceptZoom.1.negate 0\r\n"
    "GstKeyBinding.infantry.ConceptZoom.1.type 2\r\n"
    "GstRender.FieldOfView 90.000000\r\n"
    "GstRender.VSyncEnabled 1\r\n";

// input.cpp's kScan constants: the keys the mod sent before it read anything (each key's dik must be the same).
constexpr uint8_t kToday[keybind::kKeyCount] = {
    0x11, 0x1F, 0x1E, 0x20,  // W S A D
    0x39,                    // Space
    0x1D,                    // Left Ctrl
    0x2D, 0x2C,              // X, Z
    0x12, 0x13,              // E, R
    0x2A, 0x22,              // Left Shift, G
    0x02, 0x03, 0x05,        // 1, 2, 4
    0x12, 0x21, 0x39, 0x2C,  // the quick-time E, F, Space, Z
};

// The default file with some lines replaced: each pair (the line's start up to its value, the new value), and lines added at its end.
std::string Edited(const std::vector<std::pair<std::string, std::string>>& changes, const std::string& added = "") {
    std::string text(kDefaultFile);
    for (const auto& c : changes) {
        const size_t at = text.find(c.first + " ");
        if (at == std::string::npos) {
            std::printf("     (the test's own edit didn't find %s)\n", c.first.c_str());
            continue;
        }
        const size_t valueAt = at + c.first.size() + 1, end = text.find("\r\n", valueAt);
        text.replace(valueAt, end - valueAt, c.second);
    }
    return text + added;
}

keybind::Table TableOf(const std::string& text, bool read = true) {
    keybind::Table t;
    t.bindings = keybind::Parse(text.data(), text.size(), read);
    keybind::Fill(t);
    return t;
}

std::string Keys(const keybind::Table& t) {
    std::string s;
    char name[40];
    for (int k = 0; k < keybind::kKeyCount; ++k) {
        s += k ? " " : "";
        s += t.key[k].dik ? keybind::KeyName(t.key[k].dik, name, sizeof(name)) : "-";
    }
    return s;
}

int DiffersFromToday(const keybind::Table& t) {
    int n = 0;
    for (int k = 0; k < keybind::kKeyCount; ++k)
        n += t.key[k].dik != kToday[k];
    return n;
}

// ------------------------------------------------------------------ 1, 2 ------------------------------------------------------------------

void Defaults() {
    // 0: today's table (keybind.h's own defaults) is input.cpp's constants.
    {
        int same = 0;
        for (int k = 0; k < keybind::kKeyCount; ++k)
            same += keybind::kKeys[k].dik == kToday[k];
        Report("0", "keybind.h's defaults are input.cpp's keys as they were (W S A D, Space, Left Ctrl, X, Z, E, R, Left Shift, G, 1, 2, 4; E F Space Z)",
               same == keybind::kKeyCount, "%d of %d", same, keybind::kKeyCount);
    }
    const keybind::Table t = TableOf(kDefaultFile);
    char said[2048];
    const int n = keybind::Describe(t, said, sizeof(said));
    int players = 0, defaults = 0;
    for (int k = 0; k < keybind::kKeyCount; ++k)
        (t.key[k].source == keybind::Source::kPlayer ? players : defaults) += t.key[k].source != keybind::Source::kNone;
    Report("1a", "the game's default bindings (the settings file as the game writes it): every key today's", DiffersFromToday(t) == 0,
           "%d differ; keys: %s", DiffersFromToday(t), Keys(t).c_str());
    Report("1b", "... the play actions' from the file, the quick-time actions' the game's own (no lines for them)", players == 15 && defaults == 4,
           "%d from the file, %d the game's own; %d lines read, %d not understood", players, defaults, t.bindings.lines, t.bindings.bad);
    Report("1c", "... nothing for the log to say (no key moved, Fire and Zoom on their mouse buttons), no line not understood",
           n == 0 && t.bindings.bad == 0 && keybind::OnMouseButton(t.bindings, keybind::kFire, 0) && keybind::OnMouseButton(t.bindings, keybind::kZoom, 1),
           "%d said: %s", n, said);
    // 2: no file; a file with no key bindings; a file cut short in the middle of the bindings.
    const keybind::Table none = TableOf("", false);
    const keybind::Table bare = TableOf("GstRender.VSyncEnabled 1\r\nGstAudio.Volume 1.0\r\n");
    std::string cut(kDefaultFile);
    cut.resize(cut.find("GstKeyBinding.infantry.ConceptInteract.0.button"));  // (cut before use's key: use, jump and the rest not in it)
    const keybind::Table shortened = TableOf(cut);
    int noneDefault = 0;
    for (int k = 0; k < keybind::kKeyCount; ++k)
        noneDefault += none.key[k].source == keybind::Source::kDefault && bare.key[k].source == keybind::Source::kDefault;
    Report("2a", "no file, or a file with no key bindings: today's keys, all the game's own", DiffersFromToday(none) == 0 && DiffersFromToday(bare) == 0 &&
           noneDefault == keybind::kKeyCount, "%d and %d differ, %d of %d the game's own", DiffersFromToday(none), DiffersFromToday(bare), noneDefault,
           keybind::kKeyCount);
    // Cut at use's key line: use has its type... lines? (no: its slot 0's axis line only, no button) -> no key the mod sends: nothing. The rest after it
    // have no lines: the game's own.
    Report("2b", "a file cut short: the actions it has no lines for the game's own, today's keys; one cut in its slot presses nothing (said)",
           shortened.key[keybind::kCrouch].dik == 0x2D && shortened.key[keybind::kJump].source == keybind::Source::kDefault &&
               shortened.key[keybind::kJump].dik == 0x39 && shortened.key[keybind::kInteract].source == keybind::Source::kNone,
           "crouch %02X, jump %02X (%s), use %s", shortened.key[keybind::kCrouch].dik, shortened.key[keybind::kJump].dik,
           shortened.key[keybind::kJump].source == keybind::Source::kDefault ? "the game's own" : "?",
           shortened.key[keybind::kInteract].source == keybind::Source::kNone ? "nothing" : "a key");
}

// ------------------------------------------------------------------ 3, 4 ------------------------------------------------------------------

void PlayersKeys() {
    const keybind::Table t = TableOf(Edited({{"GstKeyBinding.infantry.ConceptCrouch.0.button", "46"},          // C
                                             {"GstKeyBinding.infantry.ConceptProne.0.button", "47"},           // V
                                             {"GstKeyBinding.infantry.ConceptInteract.0.button", "33"},        // F
                                             {"GstKeyBinding.infantry.ConceptReload.0.button", "16"},          // Q
                                             {"GstKeyBinding.infantry.ConceptJump.1.button", "157"},           // the right Ctrl
                                             {"GstKeyBinding.infantry.ConceptSprint.0.button", "54"},          // Right Shift
                                             {"GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.button", "79"},  // the number pad's 1
                                             {"GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.button", "6"},   // 5
                                             {"GstKeyBinding.infantry.ConceptThrowGrenade.1.button", "184"}}));     // the right Alt
    const bool keys = t.key[keybind::kCrouch].dik == 0x2E && t.key[keybind::kProne].dik == 0x2F && t.key[keybind::kInteract].dik == 0x21 &&
                      t.key[keybind::kReload].dik == 0x10 && t.key[keybind::kJump].dik == 0x9D && t.key[keybind::kSprint].dik == 0x36 &&
                      t.key[keybind::kSlot1].dik == 0x4F && t.key[keybind::kSlot5].dik == 0x06 && t.key[keybind::kGrenade].dik == 0xB8 &&
                      t.key[keybind::kSlot2].dik == 0x03;
    Report("3a", "a player's own keys followed: crouch C, prone V, use F, reload Q, jump the right Ctrl, sprint Right Shift, 1 on the number pad, "
                 "the launcher 5, the grenade the right Alt",
           keys, "%s", Keys(t).c_str());
    const bool sent = keybind::ScanOf(0x9D) == 0x1D && keybind::Extended(0x9D) && keybind::ScanOf(0xB8) == 0x38 && keybind::Extended(0xB8) &&
                      keybind::ScanOf(0x2E) == 0x2E && !keybind::Extended(0x2E) && keybind::ScanOf(0x4F) == 0x4F && !keybind::Extended(0x4F);
    Report("3b", "... sent as SendInput takes them: the right Ctrl and Alt as E0 1D and E0 38 (extended), C and the number pad's 1 as they are",
           sent, "right Ctrl %02X%s, right Alt %02X%s", keybind::ScanOf(0x9D), keybind::Extended(0x9D) ? " extended" : "", keybind::ScanOf(0xB8),
           keybind::Extended(0xB8) ? " extended" : "");
    // The quick-time actions keep the game's own keys though play's moved (use on F, prone on V: the quick-time E and Z stay E and Z).
    Report("3c", "... the quick-time actions' keys the game's own though use and prone moved: E, F, Space, Z",
           t.key[keybind::kQteInteractDrag].dik == 0x12 && t.key[keybind::kQteFastMelee].dik == 0x21 && t.key[keybind::kQteJumpClimb].dik == 0x39 &&
               t.key[keybind::kQteCrouchDuck].dik == 0x2C,
           "%02X %02X %02X %02X", t.key[keybind::kQteInteractDrag].dik, t.key[keybind::kQteFastMelee].dik, t.key[keybind::kQteJumpClimb].dik,
           t.key[keybind::kQteCrouchDuck].dik);
    // 4: the walking keys on the arrows: forward Up, back Down (negate 1), left Left (negate 1), right Right.
    const keybind::Table arrows = TableOf(Edited({{"GstKeyBinding.infantry.ConceptMoveFB.1.button", "200"},
                                                  {"GstKeyBinding.infantry.ConceptMoveFB.2.button", "208"},
                                                  {"GstKeyBinding.infantry.ConceptMoveLR.1.button", "203"},
                                                  {"GstKeyBinding.infantry.ConceptMoveLR.2.button", "205"}}));
    Report("4a", "the walking keys by their axes' direction: on the arrows, forward Up, back Down, left Left, right Right (extended)",
           arrows.key[keybind::kForward].dik == 0xC8 && arrows.key[keybind::kBack].dik == 0xD0 && arrows.key[keybind::kLeft].dik == 0xCB &&
               arrows.key[keybind::kRight].dik == 0xCD && keybind::Extended(0xC8) && keybind::ScanOf(0xC8) == 0x48,
           "%02X %02X %02X %02X", arrows.key[keybind::kForward].dik, arrows.key[keybind::kBack].dik, arrows.key[keybind::kLeft].dik,
           arrows.key[keybind::kRight].dik);
    // The back key in the first slot, forward in the second: each still by its direction, never by its slot.
    const keybind::Table swapped = TableOf(Edited({{"GstKeyBinding.infantry.ConceptMoveFB.1.button", "31"},
                                                   {"GstKeyBinding.infantry.ConceptMoveFB.1.negate", "1"},
                                                   {"GstKeyBinding.infantry.ConceptMoveFB.2.button", "17"},
                                                   {"GstKeyBinding.infantry.ConceptMoveFB.2.negate", "0"}}));
    Report("4b", "... the back key in the first slot and forward in the second: W forward, S back all the same",
           swapped.key[keybind::kForward].dik == 0x11 && swapped.key[keybind::kBack].dik == 0x1F, "forward %02X, back %02X",
           swapped.key[keybind::kForward].dik, swapped.key[keybind::kBack].dik);
    // No back key at all (its slot gone): back has lines (the axis's) and no key: nothing, while forward stays W.
    const keybind::Table noBack = TableOf(Edited({{"GstKeyBinding.infantry.ConceptMoveFB.2.button", "255"}}));
    Report("4c", "... no back key (unbound): nothing pressed for back, forward still W", noBack.key[keybind::kBack].source == keybind::Source::kNone &&
           noBack.key[keybind::kForward].dik == 0x11, "back %s, forward %02X", noBack.key[keybind::kBack].source == keybind::Source::kNone ? "nothing" : "a key",
           noBack.key[keybind::kForward].dik);
}

// ------------------------------------------------------------------ 5, 6 ------------------------------------------------------------------

void NoKey() {
    // Use on the mouse's fourth button alone; reload on the pad alone; crouch unbound; prone on Insert; jump on F9; sprint on Caps Lock; the
    // grenade on Escape; the first weapon on Home; the launcher on the left Windows key.
    const keybind::Table t = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.type", "1"},
                                             {"GstKeyBinding.infantry.ConceptInteract.0.axis", "24"},
                                             {"GstKeyBinding.infantry.ConceptInteract.0.button", "3"},
                                             {"GstKeyBinding.infantry.ConceptReload.0.button", "255"},
                                             {"GstKeyBinding.infantry.ConceptCrouch.0.button", "255"},
                                             {"GstKeyBinding.infantry.ConceptProne.0.button", "210"},
                                             {"GstKeyBinding.infantry.ConceptJump.1.button", "67"},
                                             {"GstKeyBinding.infantry.ConceptSprint.0.button", "58"},
                                             {"GstKeyBinding.infantry.ConceptThrowGrenade.1.button", "1"},
                                             {"GstKeyBinding.infantry.ConceptSelectInventoryItem1.0.button", "199"},
                                             {"GstKeyBinding.infantry.ConceptSelectInventoryItem5.0.button", "219"}}));
    const keybind::Out& use = t.key[keybind::kInteract];
    const keybind::Out& reload = t.key[keybind::kReload];
    const keybind::Out& crouch = t.key[keybind::kCrouch];
    Report("5a", "use on the mouse alone (and the pad), reload on the pad alone, crouch unbound: nothing pressed for them, why said",
           use.source == keybind::Source::kNone && !use.dik && use.mouse && reload.source == keybind::Source::kNone && reload.pad && !reload.mouse &&
               crouch.source == keybind::Source::kNone && !crouch.mouse && !crouch.pad && !crouch.refused,
           "use %s (mouse %d pad %d), reload %s (pad %d), crouch %s", use.dik ? "a key" : "nothing", use.mouse, use.pad, reload.dik ? "a key" : "nothing",
           reload.pad, crouch.dik ? "a key" : "nothing");
    int refused = 0;
    const int refusedKeys[] = {keybind::kProne, keybind::kJump, keybind::kSprint, keybind::kGrenade, keybind::kSlot1, keybind::kSlot5};
    const uint8_t refusedWhat[] = {0xD2, 0x43, 0x3A, 0x01, 0xC7, 0xDB};
    for (int i = 0; i < 6; ++i)
        refused += t.key[refusedKeys[i]].source == keybind::Source::kNone && !t.key[refusedKeys[i]].dik && t.key[refusedKeys[i]].refused == refusedWhat[i];
    Report("5b", "prone on Insert, jump on F9, sprint on Caps Lock, the grenade on Escape, the rifle on Home, the launcher on the Windows key: the mod "
                 "never sends those (its own keys, or the system's): nothing pressed, the key said",
           refused == 6, "%d of 6", refused);
    // A second slot's key taken over a first refused: use's first slot on Insert, its second (added) on G.
    const keybind::Table second = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.button", "210"}},
                                                 "GstKeyBinding.infantry.ConceptInteract.3.axis 0\r\nGstKeyBinding.infantry.ConceptInteract.3.button 34\r\n"
                                                 "GstKeyBinding.infantry.ConceptInteract.3.negate 0\r\nGstKeyBinding.infantry.ConceptInteract.3.type 0\r\n"));
    Report("5c", "use's first key Insert (refused), a later slot's G: G", second.key[keybind::kInteract].dik == 0x22 &&
           second.key[keybind::kInteract].source == keybind::Source::kPlayer, "use %02X", second.key[keybind::kInteract].dik);
    // 6: a line for a quick-time action followed (the game's options never write one; a player's own file might).
    const keybind::Table qte = TableOf(Edited({}, "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.axis 0\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.button 20\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.negate 0\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.type 0\r\n"));
    Report("6", "a line for the quick-time InteractDrag (T) followed; use stays E, the other quick-time keys the game's own",
           qte.key[keybind::kQteInteractDrag].dik == 0x14 && qte.key[keybind::kInteract].dik == 0x12 &&
               qte.key[keybind::kQteFastMelee].source == keybind::Source::kDefault,
           "InteractDrag %02X, use %02X", qte.key[keybind::kQteInteractDrag].dik, qte.key[keybind::kInteract].dik);
}

// ------------------------------------------------------------------ 7 ------------------------------------------------------------------

void Sendable() {
    int main = 0, pad = 0, never = 0, neverCount = 0;
    for (unsigned d = 0x02; d <= 0x39; ++d)
        main += keybind::Sendable(d);
    for (unsigned d = 0x47; d <= 0x53; ++d)
        pad += keybind::Sendable(d);
    // The mod's own keys (F1-F12, Home, End, Insert, Delete, Page Up/Down, Pause, Scroll Lock) and the system's (Escape, Caps and Num Lock, Print
    // Screen, the Windows and menu keys, power, sleep, wake, the media and web keys).
    const unsigned kNever[] = {0x00, 0x01, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x57, 0x58, 0xB7, 0xC5, 0xC7,
                               0xC9, 0xCF, 0xD1, 0xD2, 0xD3, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE3, 0x90, 0x99, 0xA0, 0xA1, 0xA2, 0xA4, 0xAE, 0xB0, 0xB2,
                               0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xFF};
    for (unsigned d : kNever) {
        ++neverCount;
        never += !keybind::Sendable(d);
    }
    int all = 0;
    for (unsigned d = 0; d < 256; ++d)
        all += keybind::Sendable(d);
    Report("7a", "sent: the main block (1 to the number pad's *, Space), the number pad", main == 0x39 - 0x02 + 1 && pad == 0x53 - 0x47 + 1,
           "%d of %d, %d of %d", main, 0x39 - 0x02 + 1, pad, 0x53 - 0x47 + 1);
    Report("7b", "never: the mod's own keys and the system's (F keys, Escape, Home, End, Insert, Delete, Page Up/Down, Pause, the Locks, Print Screen, "
                 "Windows, menu, power, media)",
           never == neverCount, "%d of %d never; %d codes sent in all", never, neverCount, all);
    int arrows = 0;
    for (unsigned d : {0xC8u, 0xCBu, 0xCDu, 0xD0u, 0x9Cu, 0x9Du, 0xB5u, 0xB8u})
        arrows += keybind::Sendable(d) && keybind::Extended(d) && keybind::ScanOf(d) == (d & 0x7F);
    Report("7c", "the arrows, the number pad's Enter and /, the right Ctrl and Alt: sent, extended", arrows == 8, "%d of 8", arrows);
}

// ------------------------------------------------------------------ 8 ------------------------------------------------------------------

void Broken() {
    // LF alone, tabs and spaces, a field not known, a value not a number, a negative slot, a slot past eight, the last line cut short.
    const std::string text =
        "  GstKeyBinding.infantry.ConceptCrouch.0.type\t0\n"
        "GstKeyBinding.infantry.ConceptCrouch.0.button   46   \n"
        "GstKeyBinding.infantry.ConceptCrouch.0.colour 3\n"
        "GstKeyBinding.infantry.ConceptProne.0.type 0\n"
        "GstKeyBinding.infantry.ConceptProne.0.button Z\n"
        "GstKeyBinding.infantry.ConceptJump.-1.button 57\n"
        "GstKeyBinding.infantry.ConceptJump.9.type 0\n"
        "GstKeyBinding.infantry.ConceptJump.9.button 48\n"
        "GstKeyBinding.vehicle.ConceptInteract.0.button 20\n"
        "GstKeyBinding.infantry.ConceptReload.0.type 0\n"
        "GstKeyBinding.infantry.ConceptReload.0.button 99999999999\n"
        "GstKeyBinding.infantry.ConceptInteract.0.type 0\n"
        "GstKeyBinding.infantry.ConceptInteract.0.butt";
    const keybind::Table t = TableOf(text);
    Report("8a", "LF lines, tabs and spaces: crouch's C read", t.key[keybind::kCrouch].dik == 0x2E, "crouch %02X", t.key[keybind::kCrouch].dik);
    Report("8b", "a field not known, a value not a number, a negative slot, a number too big, the last line cut: each counted, nothing taken from them",
           t.bindings.bad >= 4 && t.key[keybind::kProne].source == keybind::Source::kNone && t.key[keybind::kReload].source == keybind::Source::kNone,
           "%d lines not understood; prone %s, reload %s", t.bindings.bad, t.key[keybind::kProne].dik ? "a key" : "nothing",
           t.key[keybind::kReload].dik ? "a key" : "nothing");
    Report("8c", "a slot past the eighth: the action's lines there (it has a binding) but no key taken from it; another context's line not taken",
           t.key[keybind::kJump].source == keybind::Source::kNone && t.key[keybind::kInteract].source == keybind::Source::kNone,
           "jump %s, use %s", t.key[keybind::kJump].dik ? "a key" : "nothing", t.key[keybind::kInteract].dik ? "a key" : "nothing");
    // Nothing read from a file with no text, a null pointer.
    const keybind::Bindings b = keybind::Parse(nullptr, 100, true);
    Report("8d", "no text: no lines, the game's own keys", b.lines == 0 && keybind::Resolve(b, keybind::kJump).dik == 0x39, "%d lines", b.lines);
}

// ------------------------------------------------------------------ 9 ------------------------------------------------------------------

void Words() {
    const keybind::Table t = TableOf(Edited({{"GstKeyBinding.infantry.ConceptCrouch.0.button", "46"},
                                             {"GstKeyBinding.infantry.ConceptInteract.0.button", "19"},  // use on R: reload's too
                                             {"GstKeyBinding.infantry.ConceptProne.0.button", "210"},    // Insert
                                             {"GstKeyBinding.infantry.ConceptFire.1.button", "2"},       // the middle mouse button
                                             {"GstKeyBinding.infantry.ConceptZoom.0.type", "0"},         // Zoom on a key
                                             {"GstKeyBinding.infantry.ConceptZoom.0.button", "44"}}));
    char said[2048];
    const int n = keybind::Describe(t, said, sizeof(said));
    const bool words = std::strstr(said, "crouch C (was X)") && std::strstr(said, "use R (was E)") && std::strstr(said, "prone: nothing pressed") &&
                       std::strstr(said, "Insert") && std::strstr(said, "use and reload are both on R") && std::strstr(said, "Fire isn't on the left mouse") &&
                       std::strstr(said, "Zoom isn't on the right mouse");
    Report("9a", "the log's words: a key moved, one with nothing pressed (its key named), two on one key, Fire and Zoom off their buttons", words && n == 6,
           "%d said: %s", n, said);
    char a[40], b[40], c[40], d[40], e[40];
    Report("9b", "the keys' names", !std::strcmp(keybind::KeyName(0x1D, a, sizeof(a)), "Left Ctrl") && !std::strcmp(keybind::KeyName(0x0B, b, sizeof(b)), "0") &&
           !std::strcmp(keybind::KeyName(0x43, c, sizeof(c)), "F9") && !std::strcmp(keybind::KeyName(0x58, d, sizeof(d)), "F12") &&
           !std::strcmp(keybind::KeyName(0x99, e, sizeof(e)), "the key 0x99"),
           "%s, %s, %s, %s, %s", a, b, c, d, e);
    // A buffer too small for it all: cut, ended, never past it.
    char tiny[24];
    std::memset(tiny, 'x', sizeof(tiny));
    keybind::Describe(t, tiny, sizeof(tiny));
    Report("9c", "a buffer too small: cut and ended inside it", std::memchr(tiny, 0, sizeof(tiny)) != nullptr, "\"%s\"", tiny);
}

// ------------------------------------------------------------------ 10, 11 ------------------------------------------------------------------

void ThisPc() {
    wchar_t documents[MAX_PATH] = {};
    std::wstring path;
    if (GetEnvironmentVariableW(L"USERPROFILE", documents, MAX_PATH)) {
        for (const wchar_t* under : {L"\\OneDrive\\Documents", L"\\Documents"}) {
            const std::wstring p = std::wstring(documents) + under + L"\\Battlefield 3\\settings\\PROF_SAVE_profile";
            if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
                path = p;
                break;
            }
        }
    }
    std::string text;
    if (!path.empty()) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, path.c_str(), L"rb") == 0 && f) {
            char buf[65536];
            size_t got = 0;
            while ((got = std::fread(buf, 1, sizeof(buf), f)) > 0)
                text.append(buf, got);
            std::fclose(f);
        }
    }
    if (text.empty()) {
        Skip("10", "this PC's own settings file (Documents\\Battlefield 3\\settings\\PROF_SAVE_profile) isn't there");
        return;
    }
    const keybind::Table t = TableOf(text);
    char said[2048];
    const int n = keybind::Describe(t, said, sizeof(said));
    Report("10", "this PC's own settings file (read only): its key bindings read, every line understood", t.bindings.lines > 0 && t.bindings.bad == 0,
           "%d lines, %d not understood; keys: %s; the log would say %s", t.bindings.lines, t.bindings.bad, Keys(t).c_str(),
           n ? said : "nothing (the game's defaults)");
}

void Time() {
    std::string big;
    for (int i = 0; i < 4; ++i)
        big += kDefaultFile;
    const auto start = std::chrono::steady_clock::now();
    int lines = 0;
    for (int i = 0; i < 200; ++i) {
        keybind::Table t = TableOf(big);
        lines += t.bindings.lines;
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / 200.0;
    Report("11", "the time a file of the game's size takes to read and make a table of (a thread of its own, once a change)", ms < 5.0 && lines > 0,
           "%.3f ms for %zu bytes", ms, big.size());
}

}  // namespace

int main() {
    Defaults();
    PlayersKeys();
    NoKey();
    Sendable();
    Broken();
    Words();
    ThisPc();
    Time();
    std::printf("\n%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}
