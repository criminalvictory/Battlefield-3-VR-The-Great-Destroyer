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
//   10 this PC's own settings file (read only, shared with the game's saves; SKIP without it): read, its lines all understood, what the log would say
//   11 the time a whole file takes
//   12 the fights' guard by the key that goes (input.cpp's SendKey: qtefight::KeysOfQuickTime of keybind::QuickTimeOn): with the game's default keys
//      every key's as it always was; use moved onto F held up at an E prompt and jump moved onto E at a Space prompt (the review of 2026-11-25:
//      both went through, a wrong key that fails a fight), each still answering its own prompt; a key moved off every quick-time key never held up;
//      a quick-time action's own key moved by a line, the mod's quick-time press of it its prompt's answer
//   13 the watch's looks at the file (keybind::LookAt, as keybind.cpp runs it): a read that fails or finds the file empty while the player's keys
//      are in use keeps them, said once, read again at the next look until it reads; the file gone keeps them until it's back; no key bindings
//      found while the player's are in use believed only the second time; the run's first look as before
//   14 the reload key looked at on the keyboard (ammo.cpp): the scan codes Windows' MapVirtualKeyEx takes (the right Ctrl, the arrows extended)
//      and the key codes this PC gives them; the number pad's own key codes
//   15 the grips' quick-time press as keys, frame by frame as input.cpp runs it (keybind.h's QuickTimeKeyAs and QuickTimeMapped): each key the one
//      it went down as till it's let go, so a frame the fights couldn't be read in a prompt's tap never gives play's use or jump for it (the review
//      of 2026-11-25: with use moved to Q a press prompt's E went up and down again); a prompt closing into play's Interact; play's Interact held,
//      then InteractDrag beside it: the game's E; the train's climb beside its fight the game's Space, never the player's jump key; in play; with
//      the game's default keys every run as before
// Usage: keybindtest
// The exit code is the number of failed checks.

#include <windows.h>

#include <share.h>

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/keybind.h"
#include "../src/qtefight.h"

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
        // (Shared every way: the game may save its settings while this runs beside it. _wfopen_s's own sharing refused it any write meanwhile.)
        if (FILE* f = _wfsopen(path.c_str(), L"rb", _SH_DENYNO)) {
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

// ------------------------------------------------------------------ 12 ------------------------------------------------------------------

// input.cpp's SendKey for one of the mod's keys (its action's key in the table) going down while a fight's prompt awaiting answer is up: held up?
// The guard's keys as SendKey asks: qtefight::KeysOfQuickTime(keybind::QuickTimeOn(table, the key that goes)).
bool HeldUpAt(const keybind::Table& t, int action, unsigned answer) {
    qtefight::Guard g;
    qtefight::SetGuard(g, true, answer, 1000);
    return qtefight::GuardKey(g, qtefight::KeysOfQuickTime(keybind::QuickTimeOn(t, t.key[action].dik)), true, 1000).drop;
}

void Guard() {
    // 12a: the game's default keys: each of the mod's keys guarded as its name always was (input.cpp's QteKeyOfScan before 2026-11-25: use and the
    // quick-time E as E, jump and the quick-time Space as Space, prone and the quick-time Z as Z, the quick-time F as F, every other key as none).
    const keybind::Table d = TableOf(kDefaultFile);
    unsigned was[keybind::kKeyCount] = {};
    was[keybind::kJump] = was[keybind::kQteJumpClimb] = qte::kKeySpace;
    was[keybind::kInteract] = was[keybind::kQteInteractDrag] = qte::kKeyE;
    was[keybind::kProne] = was[keybind::kQteCrouchDuck] = qte::kKeyZ;
    was[keybind::kQteFastMelee] = qte::kKeyF;
    int same = 0;
    std::string differ;
    for (int k = 0; k < keybind::kKeyCount; ++k) {
        const unsigned now = qtefight::KeysOfQuickTime(keybind::QuickTimeOn(d, d.key[k].dik));
        same += now == was[k];
        if (now != was[k])
            differ += std::string(" ") + keybind::kKeys[k].action;
    }
    const keybind::Table none = TableOf("", false);
    int sameNone = 0;
    for (int k = 0; k < keybind::kKeyCount; ++k)
        sameNone += qtefight::KeysOfQuickTime(keybind::QuickTimeOn(none, none.key[k].dik)) == was[k];
    Report("12a", "the game's default keys (its file, or none): every one of the mod's keys guarded as it always was", same == keybind::kKeyCount &&
           sameNone == keybind::kKeyCount, "%d and %d of %d;%s", same, sameNone, keybind::kKeyCount, differ.c_str());
    // 12b: use on F (melee's own F moved to V), as the review's probe: Y at an E prompt held up (F is FastMelee's: it would fail the fight), at an
    // F prompt sent (F is its answer).
    const keybind::Table useOnF = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.button", "33"},
                                                  {"GstKeyBinding.infantry.ConceptMeleeAttack.1.button", "47"}}));
    const bool useHeld = HeldUpAt(useOnF, keybind::kInteract, qte::kKeyE), useSent = !HeldUpAt(useOnF, keybind::kInteract, qte::kKeyF);
    Report("12b", "use moved onto F: Y's F held up at an E prompt (InteractDrag: a wrong key, the fight failed), sent at an F prompt", useOnF.key[keybind::kInteract].dik == 0x21 &&
           useHeld && useSent, "use %02X; E prompt %s, F prompt %s", useOnF.key[keybind::kInteract].dik, useHeld ? "held up" : "SENT", useSent ? "sent" : "held up");
    // 12c: jump on E (use moved to R): A at a Space prompt held up (E is InteractDrag's), at an E prompt sent.
    const keybind::Table jumpOnE = TableOf(Edited({{"GstKeyBinding.infantry.ConceptJump.1.button", "18"},
                                                   {"GstKeyBinding.infantry.ConceptInteract.0.button", "19"}}));
    const bool jumpHeld = HeldUpAt(jumpOnE, keybind::kJump, qte::kKeySpace), jumpSent = !HeldUpAt(jumpOnE, keybind::kJump, qte::kKeyE);
    Report("12c", "jump moved onto E: A's E held up at a Space prompt (JumpClimb), sent at an E prompt", jumpOnE.key[keybind::kJump].dik == 0x12 && jumpHeld &&
           jumpSent, "jump %02X; Space prompt %s, E prompt %s", jumpOnE.key[keybind::kJump].dik, jumpHeld ? "held up" : "SENT", jumpSent ? "sent" : "held up");
    // 12d: use on K and prone on P (no quick-time key): never held up, whatever the prompt (no quick-time key: it can't fail a fight); the mod's own
    // quick-time presses still their prompts' answers and the others' wrong keys.
    const keybind::Table off = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.button", "37"},
                                               {"GstKeyBinding.infantry.ConceptProne.0.button", "25"}}));
    int never = 0;
    for (unsigned answer : {qte::kKeyE, qte::kKeyF, qte::kKeySpace, qte::kKeyZ, qte::kKeyLeftMouse})
        never += !HeldUpAt(off, keybind::kInteract, answer) && !HeldUpAt(off, keybind::kProne, answer);
    const bool qteOwn = !HeldUpAt(off, keybind::kQteInteractDrag, qte::kKeyE) && HeldUpAt(off, keybind::kQteInteractDrag, qte::kKeyF) &&
                        !HeldUpAt(off, keybind::kQteCrouchDuck, qte::kKeyZ) && HeldUpAt(off, keybind::kQteCrouchDuck, qte::kKeySpace);
    Report("12d", "use on K, prone on P: never held up at any prompt; the quick-time E and Z as ever (their prompts' answers, the others' wrong keys)",
           never == 5 && qteOwn, "%d of 5 prompts let both through; the quick-time keys %s", never, qteOwn ? "as ever" : "NOT as ever");
    // 12e: a line for the quick-time InteractDrag on T: the mod's quick-time E goes as T, its E prompt's answer; use's E is no quick-time key now.
    const keybind::Table qteT = TableOf(Edited({}, "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.axis 0\r\n"
                                                   "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.button 20\r\n"
                                                   "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.negate 0\r\n"
                                                   "GstKeyBinding.infantry.ConceptQuicktimeInteractDrag.0.type 0\r\n"));
    const bool tAnswers = qteT.key[keybind::kQteInteractDrag].dik == 0x14 && !HeldUpAt(qteT, keybind::kQteInteractDrag, qte::kKeyE) &&
                          HeldUpAt(qteT, keybind::kQteInteractDrag, qte::kKeySpace) && !HeldUpAt(qteT, keybind::kInteract, qte::kKeySpace);
    Report("12e", "the quick-time InteractDrag on T by a line: the quick-time E's T answers an E prompt, is held up at a Space prompt; use's E no "
                  "quick-time key",
           tAnswers, "InteractDrag %02X", qteT.key[keybind::kQteInteractDrag].dik);
    // 12f: the bits themselves: none for no key; each quick-time action's own bit; two on one key both.
    const keybind::Table two = TableOf(Edited({}, "GstKeyBinding.infantry.ConceptQuicktimeJumpClimb.0.axis 0\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeJumpClimb.0.button 18\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeJumpClimb.0.negate 0\r\n"
                                                  "GstKeyBinding.infantry.ConceptQuicktimeJumpClimb.0.type 0\r\n"));
    const bool bits = keybind::QuickTimeOn(d, 0) == 0 && keybind::QuickTimeOn(d, 0x12) == keybind::kQuickTimeDrag &&
                      keybind::QuickTimeOn(d, 0x21) == keybind::kQuickTimeMelee && keybind::QuickTimeOn(d, 0x39) == keybind::kQuickTimeClimb &&
                      keybind::QuickTimeOn(d, 0x2C) == keybind::kQuickTimeDuck && keybind::QuickTimeOn(d, 0x13) == 0 &&
                      keybind::QuickTimeOn(two, 0x12) == (keybind::kQuickTimeDrag | keybind::kQuickTimeClimb) &&
                      qtefight::KeysOfQuickTime(keybind::kQuickTimeDrag | keybind::kQuickTimeClimb) == (qte::kKeyE | qte::kKeySpace);
    Report("12f", "the quick-time actions on a key: none for no key or R; E, F, Space, Z each their own; two on one key both", bits, "E with JumpClimb on it too: %u",
           keybind::QuickTimeOn(two, 0x12));
}

// ------------------------------------------------------------------ 13 ------------------------------------------------------------------

// keybind.cpp's watch over a run of looks: each look (read whole or not, the file there, its key binding lines), the table in use after it (0 none
// yet, 'd' today's from no file or no key bindings, 'p' the player's), what's said, and the Kept each look leaves.
struct Seen {
    char table = 0;
    int said = 0, takes = 0;
    keybind::Kept kept = keybind::Kept::kNo;
};
struct Read {
    bool read, there;
    int lines;
};
Seen Watch(std::initializer_list<Read> looks, char start = 0) {
    Seen s;
    s.table = start;
    for (const Read& r : looks) {
        const keybind::Look l = keybind::LookAt(r.read, r.there, r.lines, s.table != 0, s.table == 'p', s.kept);
        if (l.take) {
            s.table = r.read && r.lines ? 'p' : 'd';
            ++s.takes;
        }
        s.said += l.say;
        s.kept = l.kept;
    }
    return s;
}

void Looks() {
    const Seen firstLocked = Watch({{false, true, 0}});
    const Seen firstLockedThen = Watch({{false, true, 0}, {false, true, 0}, {true, true, 80}});
    const Seen firstNone = Watch({{false, false, 0}});
    Report("13a", "the run's first look: no file, today's keys, settled; the file there but locked, today's keys and read again at each look (nothing "
                  "said again) until it reads",
           firstNone.table == 'd' && firstNone.kept == keybind::Kept::kNo && firstLocked.table == 'd' && firstLocked.kept == keybind::Kept::kUnread &&
               firstLockedThen.table == 'p' && firstLockedThen.takes == 2 && firstLockedThen.said == 0 && firstLockedThen.kept == keybind::Kept::kNo,
           "no file %c, locked %c then %c (%d said)", firstNone.table, firstLocked.table, firstLockedThen.table, firstLockedThen.said);
    // The review's case: the player's keys in use, the game writing the file as it's read (locked, or found empty: ReadText's 0 bytes).
    const Seen locked = Watch({{false, true, 0}}, 'p');
    const Seen lockedTwice = Watch({{false, true, 0}, {false, true, 0}}, 'p');
    const Seen lockedThenRead = Watch({{false, true, 0}, {false, true, 0}, {true, true, 80}}, 'p');
    Report("13b", "the player's keys in use, a read that fails or finds the file empty: kept, said once, read again at each look until it reads "
                  "(before: today's keys for the rest of the run)",
           locked.table == 'p' && locked.said == 1 && locked.kept == keybind::Kept::kUnread && lockedTwice.said == 1 && lockedTwice.table == 'p' &&
               lockedTwice.kept == keybind::Kept::kUnread && lockedThenRead.table == 'p' && lockedThenRead.takes == 1 &&
               lockedThenRead.kept == keybind::Kept::kNo,
           "kept %c (said %d), twice %c (said %d), then read: %c, %d taken", locked.table, locked.said, lockedTwice.table, lockedTwice.said,
           lockedThenRead.table, lockedThenRead.takes);
    const Seen gone = Watch({{false, false, 0}}, 'p');
    Report("13c", "the player's keys in use, the file gone: kept, said, settled (its coming back changes its stamp: the next look)", gone.table == 'p' &&
           gone.said == 1 && gone.kept == keybind::Kept::kNo && gone.takes == 0, "%c, said %d", gone.table, gone.said);
    const Seen empty = Watch({{true, true, 0}}, 'p');
    const Seen emptyTwice = Watch({{true, true, 0}, {true, true, 0}}, 'p');
    const Seen emptyThenKeys = Watch({{true, true, 0}, {true, true, 80}}, 'p');
    const Seen lockedThenEmpty = Watch({{false, true, 0}, {true, true, 0}}, 'p');
    Report("13d", "the player's keys in use, a file with no key bindings: kept and read again (said); the second such read believed (today's keys); "
                  "the bindings back at the second: theirs",
           empty.table == 'p' && empty.said == 1 && empty.kept == keybind::Kept::kNoKeys && emptyTwice.table == 'd' &&
               emptyTwice.kept == keybind::Kept::kNo && emptyThenKeys.table == 'p' && emptyThenKeys.takes == 1 && lockedThenEmpty.table == 'p' &&
               lockedThenEmpty.kept == keybind::Kept::kNoKeys,
           "once %c, twice %c, then the keys %c; locked then none %c", empty.table, emptyTwice.table, emptyThenKeys.table, lockedThenEmpty.table);
    const Seen todays = Watch({{true, true, 0}}, 'd');
    const Seen changed = Watch({{true, true, 90}}, 'p');
    Report("13e", "today's keys in use, a file with no key bindings: taken at once (as before); the player's changed: taken at once", todays.table == 'd' &&
           todays.takes == 1 && todays.said == 0 && changed.table == 'p' && changed.takes == 1 && changed.kept == keybind::Kept::kNo,
           "%c (%d taken), %c", todays.table, todays.takes, changed.table);
}

// ------------------------------------------------------------------ 14 ------------------------------------------------------------------

void ReloadKey() {
    // MapVirtualKeyEx's scan codes for the keys whose key code doesn't hang on the keyboard's layout, and the key codes this PC gives them.
    const HKL layout = GetKeyboardLayout(0);
    struct Want {
        uint8_t dik;
        unsigned vk;
        const char* name;
    };
    const Want wants[] = {{0x1D, VK_LCONTROL, "Left Ctrl"}, {0x9D, VK_RCONTROL, "the right Ctrl"}, {0x2A, VK_LSHIFT, "Left Shift"},
                          {0x36, VK_RSHIFT, "Right Shift"}, {0x39, VK_SPACE, "Space"},          {0xC8, VK_UP, "Up"},
                          {0xD0, VK_DOWN, "Down"},          {0xB8, VK_RMENU, "the right Alt"},  {0x0F, VK_TAB, "Tab"}};
    int right = 0;
    std::string wrong;
    for (const Want& w : wants) {
        const UINT vk = MapVirtualKeyExW(keybind::MapScanOf(w.dik), MAPVK_VSC_TO_VK_EX, layout);
        right += vk == w.vk;
        if (vk != w.vk) {
            char one[96];
            std::snprintf(one, sizeof(one), " %s: %02X, not %02X", w.name, vk, w.vk);
            wrong += one;
        }
    }
    Report("14a", "the reload key's Windows key code by its scan code (ammo.cpp): Ctrl, Shift, the right Alt, Space, the arrows, Tab (extended ones "
                  "with E0)",
           right == static_cast<int>(sizeof(wants) / sizeof(wants[0])) && keybind::MapScanOf(0x9D) == 0xE01D && keybind::MapScanOf(0x13) == 0x13,
           "%d of %d%s", right, static_cast<int>(sizeof(wants) / sizeof(wants[0])), wrong.c_str());
    // R by its scan code on this PC's layout: R on QWERTY, QWERTZ and AZERTY ones (said, not a failure: a Dvorak's is P, as the game reads it too).
    char layoutName[KL_NAMELENGTH] = "";
    GetKeyboardLayoutNameA(layoutName);
    const UINT r = MapVirtualKeyExW(keybind::MapScanOf(0x13), MAPVK_VSC_TO_VK_EX, layout);
    std::printf("     (this PC's keyboard layout %s gives the game's reload key, scan code 13, the key code %02X%s)\n", layoutName, r,
                r == 'R' ? ": R" : "");
    const bool pad = keybind::NumberPadVk(0x4F) == VK_NUMPAD1 && keybind::NumberPadVk(0x52) == VK_NUMPAD0 && keybind::NumberPadVk(0x53) == VK_DECIMAL &&
                     keybind::NumberPadVk(0x47) == VK_NUMPAD7 && keybind::NumberPadVk(0x4A) == VK_SUBTRACT && keybind::NumberPadVk(0x4E) == VK_ADD &&
                     keybind::NumberPadVk(0x13) == 0 && keybind::NumberPadVk(0x46) == 0 && keybind::NumberPadVk(0x54) == 0;
    Report("14b", "the number pad's keys: their own key codes too (Num Lock on), beside the one Windows gives their scan code", pad,
           "1 %02X, 0 %02X, . %02X; Windows' key code for the number pad's 1 by its scan code %02X (End's is 23)", keybind::NumberPadVk(0x4F),
           keybind::NumberPadVk(0x52), keybind::NumberPadVk(0x53), MapVirtualKeyExW(keybind::MapScanOf(0x4F), MAPVK_VSC_TO_VK_EX, layout));
}

// ------------------------------------------------------------------ 15 ------------------------------------------------------------------

// The grips' quick-time press as keys with a player's own bindings, frame by frame at 72 a second (14 ms), as input.cpp runs it: QteStep's version 2
// lines (the fights' guard; g_qteProper, the key each bit goes down as, from the frame's proper and left as it was over a frame the fights couldn't be
// read; qte::TapRule; qte::Step2), then Sync's QteMerged, QteKeys and the mappings of jump, crouch-and-hold and use (g_buttons' order), each key sent as
// SendKey sends it (the player's key for its action, a release to the key its press went to, the fights' guard asked by the key that goes). The
// decisions are keybind.h's (QuickTimeKeyAs, QuickTimeMapped). old: as the players lane had it before its review (each bit pressed as the key it is
// now, the routing by qtewatch's quick), to show the checks catch what the review found.
struct QteSim {
    static constexpr uint16_t kSpace = 0x39, kCtrl = 0x1D, kE = 0x12, kF = 0x21, kZ = 0x2C;  // input.cpp's kScan*
    static constexpr uint16_t kQteE = 0x100 | kE, kQteSpace = 0x100 | kSpace, kQteZ = 0x100 | kZ;  // ... kLogQte*
    static constexpr int kBits = 5;
    static constexpr unsigned kBit[kBits] = {qte::kKeyF, qte::kKeyZ, qte::kKeyE, qte::kKeySpace, qte::kKeyCtrl};  // (input.cpp's kQteOwnBit)
    struct Awaited {  // qtewatch::Frame's fields QteStep reads
        unsigned await = 0, hold = 0, answer = 0, quick = 0, proper = 0, gates = 0;
        bool prompt = false, mash = false, unknown = false;
    };
    struct Sent {
        uint8_t dik;
        bool down;
        unsigned long long ms;
    };
    struct As {
        bool down = false;
        uint8_t dik = 0;
    };
    keybind::Table table;
    bool old = false, scripted = true;
    unsigned long long now = 100000;
    qte::State2 st;
    qtefight::Guard guard;
    unsigned proper = 0;          // g_qteProper
    uint16_t held[kBits] = {};    // g_qteHeld
    bool mapped[3] = {};          // the jump, crouch-and-hold and use mappings' down
    As sentAs[0x200];             // g_sentAs
    bool physical[256] = {};
    std::vector<Sent> sent;
    int noKey = 0, heldUp = 0;

    QteSim(const keybind::Table& t, bool oldWay) : table(t), old(oldWay) {}
    static int BindingOf(uint16_t scan) {
        switch (scan) {
        case kSpace: return keybind::kJump;
        case kCtrl: return keybind::kCrouchHold;
        case kE: return keybind::kInteract;
        case kF: return keybind::kQteFastMelee;
        case kQteE: return keybind::kQteInteractDrag;
        case kQteSpace: return keybind::kQteJumpClimb;
        case kQteZ: return keybind::kQteCrouchDuck;
        default: return -1;
        }
    }
    uint8_t KeyNow(uint16_t scan) const {
        const int b = BindingOf(scan);
        return b < 0 ? static_cast<uint8_t>(scan & 0xFF) : table.key[b].dik;
    }
    bool SameKey(uint16_t a, uint16_t b) const {
        if (a == b)
            return true;
        const uint8_t ka = KeyNow(a), kb = KeyNow(b);
        return ka && ka == kb;
    }
    void SendKey(uint16_t scan, bool down) {
        As& s = sentAs[scan & 0x1FF];
        const uint8_t dik = !down && s.down ? s.dik : KeyNow(scan);
        if (qtefight::GuardKey(guard, qtefight::KeysOfQuickTime(keybind::QuickTimeOn(table, dik)), down, now).drop) {
            ++heldUp;
            return;
        }
        s.down = down;
        if (down)
            s.dik = dik;
        if (!dik) {
            noKey += down;
            return;
        }
        sent.push_back({dik, down, now - 100000});
        physical[dik] = down;
    }
    uint16_t Logical(unsigned bit) const {  // QteLogical
        switch (bit) {
        case qte::kKeyE: return (proper & qte::kKeyE) ? kQteE : kE;
        case qte::kKeySpace: return (proper & qte::kKeySpace) ? kQteSpace : kSpace;
        case qte::kKeyF: return kF;
        case qte::kKeyZ: return kQteZ;
        case qte::kKeyCtrl: return kCtrl;
        default: return 0;
        }
    }
    uint16_t ApartAs(unsigned bit) const {  // QteApartAs
        for (int i = 0; i < kBits; ++i)
            if (kBit[i] == bit)
                return held[i];
        return 0;
    }
    unsigned Merged(unsigned keys) const {  // QteMerged
        unsigned merged = keys & (qte::kKeyLeftMouse | qte::kKeyRightMouse);
        const unsigned bits[3] = {qte::kKeyE, qte::kKeySpace, qte::kKeyCtrl};
        const uint16_t mapping[3] = {kE, kSpace, kCtrl};
        for (int b = 0; b < 3; ++b) {
            const bool same = SameKey(Logical(bits[b]), mapping[b]);
            const uint16_t apart = ApartAs(bits[b]);
            if (old ? (keys & bits[b]) && same
                    : keybind::QuickTimeMapped((keys & bits[b]) != 0, same, apart != 0, apart && SameKey(apart, mapping[b])))
                merged |= bits[b];
        }
        return merged;
    }
    void Keys(unsigned keys) {  // QteKeys
        for (int i = 0; i < kBits; ++i) {
            uint16_t& h = held[i];
            const bool wanted = (keys & kBit[i]) != 0;
            const uint16_t want = old ? (wanted ? Logical(kBit[i]) : uint16_t{0}) : keybind::QuickTimeKeyAs(wanted, h, Logical(kBit[i]));
            if (h == want)
                continue;
            if (old && h && want && SameKey(h, want)) {
                h = want;
                continue;
            }
            if (h)
                SendKey(h, false);
            if (want)
                SendKey(want, true);
            h = want;
        }
    }
    unsigned Seen() const {  // the game's input: its quick-time actions down by their own keys (E, F, Space, Z)
        return (physical[0x12] ? qte::kKeyE : 0u) | (physical[0x21] ? qte::kKeyF : 0u) | (physical[0x39] ? qte::kKeySpace : 0u) |
               (physical[0x2C] ? qte::kKeyZ : 0u);
    }
    void Frame(const Awaited& a, float left, float right) {
        qte::In in;
        in.nowMs = now;
        in.on = in.front = true;
        in.cutscene = scripted;
        in.read[0] = in.read[1] = true;
        in.squeeze[0] = left, in.squeeze[1] = right;
        qtefight::GuardFrame(guard, a.unknown, a.prompt, a.answer, now);
        if (old)
            proper = a.prompt ? qte::kKeysV2 : a.quick;
        else if (!a.unknown)
            proper = a.prompt ? qte::kKeysV2 : a.proper;
        in.await = a.await;
        in.hold = a.hold;
        in.seen = Seen();
        in.seenRead = !a.unknown;
        qte::TapRule(true, a.prompt, a.mash, in.mash, in.once);
        in.prompt = a.prompt;
        in.unknown = a.unknown;
        in.gates = a.gates;
        in.quick = a.quick;
        const qte::Out o = qte::Step2(st, in);
        const unsigned merged = Merged(o.keys);
        Keys(o.keys & ~merged);
        const unsigned bits[3] = {qte::kKeySpace, qte::kKeyCtrl, qte::kKeyE};
        const uint16_t scans[3] = {kSpace, kCtrl, kE};
        for (int m = 0; m < 3; ++m) {
            const bool down = (merged & bits[m]) != 0;
            if (down != mapped[m]) {
                SendKey(scans[m], down);
                mapped[m] = down;
            }
        }
        now += 14;
    }
    int Downs(uint8_t dik) const {
        int n = 0;
        for (const Sent& s : sent)
            n += s.dik == dik && s.down;
        return n;
    }
    int AllDowns() const {
        int n = 0;
        for (const Sent& s : sent)
            n += s.down;
        return n;
    }
    bool Up() const {  // every key let go at the end
        for (bool d : physical)
            if (d)
                return false;
        return true;
    }
    std::string Text() const {
        std::string s;
        char name[40], one[64];
        for (const Sent& e : sent) {
            std::snprintf(one, sizeof(one), "%s%s %s %llu", s.empty() ? "" : ", ", keybind::KeyName(e.dik, name, sizeof(name)), e.down ? "down" : "up", e.ms);
            s += one;
        }
        return s.empty() ? "nothing" : s;
    }
};

using Awaited = QteSim::Awaited;

// A fight's prompt of one key (a press, or a mash) as the fights' reader gives it.
Awaited PromptOf(unsigned key, bool mash = false) {
    Awaited a;
    a.prompt = true, a.answer = key, a.await = key, a.gates = key, a.mash = mash;
    return a;
}
Awaited Unread() {
    Awaited a;
    a.unknown = true;
    return a;
}

// The review's case: a fight's press prompt (or mash), the right grip squeezed as it opens (a grip squeezed already answers it at once), a frame the
// fights couldn't be read at each of unreadAt (frame numbers in the prompt), then the prompt over and the grip let go.
QteSim PromptRun(const keybind::Table& t, bool old, unsigned key, bool mash, std::initializer_list<int> unreadAt, int frames = 12) {
    QteSim s(t, old);
    s.Frame(Awaited{}, 0.0f, 0.0f);
    const Awaited prompt = PromptOf(key, mash), unread = Unread();
    for (int i = 0; i < frames; ++i) {
        bool torn = false;
        for (int u : unreadAt)
            torn = torn || u == i;
        s.Frame(torn ? unread : prompt, 0.0f, 1.0f);
    }
    for (int i = 0; i < 4; ++i)
        s.Frame(Awaited{}, 0.0f, 1.0f);
    for (int i = 0; i < 2; ++i)
        s.Frame(Awaited{}, 0.0f, 0.0f);
    return s;
}

void QuickTimeKeys() {
    const keybind::Table d = TableOf(kDefaultFile), none = TableOf("", false);
    const keybind::Table useQ = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.button", "16"}}));
    const keybind::Table jumpC = TableOf(Edited({{"GstKeyBinding.infantry.ConceptJump.1.button", "46"}}));
    const keybind::Table useMouse = TableOf(Edited({{"GstKeyBinding.infantry.ConceptInteract.0.type", "1"},
                                                    {"GstKeyBinding.infantry.ConceptInteract.0.button", "3"}}));
    constexpr uint8_t kE = 0x12, kQ = 0x10, kSpace = 0x39, kC = 0x2E;
    // 15a the pure decisions: a bit is the key it went down as till it's let go; through its mapping while its key now is the mapping's, unless it's
    // down apart as another key (down apart as the mapping's very key, the test's grip then the headset on: to the mapping, as before).
    const bool keyAs = keybind::QuickTimeKeyAs(false, 0x112, 0x12) == 0 && keybind::QuickTimeKeyAs(true, 0, 0x12) == 0x12 &&
                       keybind::QuickTimeKeyAs(true, 0x112, 0x12) == 0x112 && keybind::QuickTimeKeyAs(true, 0x12, 0x112) == 0x12;
    const bool mappedRule = keybind::QuickTimeMapped(true, true, false, false) && !keybind::QuickTimeMapped(false, true, false, false) &&
                            !keybind::QuickTimeMapped(true, false, false, false) && !keybind::QuickTimeMapped(true, true, true, false) &&
                            keybind::QuickTimeMapped(true, true, true, true) && !keybind::QuickTimeMapped(true, false, true, true);
    Report("15a", "a quick-time bit is the key it went down as till it's let go; through its button's mapping only while its key is the mapping's, and "
                  "not while it's down apart as another key (apart as the mapping's very key: to the mapping, as before)",
           keyAs && mappedRule, "key as %d, mapped %d", keyAs, mappedRule);
    // 15b-15e the review's case (2026-11-25): a frame the fights couldn't be read 14 ms into a prompt's tap gave play's use (Q) for its E, and the E
    // went up, Q down, E down again: a press prompt's one press made two.
    const QteSim pd = PromptRun(d, false, qte::kKeyE, false, {1}), pq = PromptRun(useQ, false, qte::kKeyE, false, {1}),
                 pqOld = PromptRun(useQ, true, qte::kKeyE, false, {1});
    Report("15b", "use moved to Q, a fight's press prompt E with a frame the fights couldn't be read in its tap: E down once and up after its 70 ms, Q "
                  "never (as with the game's default keys); before: E, Q, E again",
           pq.Downs(kE) == 1 && pq.Downs(kQ) == 0 && pq.Text() == pd.Text() && pq.Up() && pqOld.Downs(kE) == 2 && pqOld.Downs(kQ) == 1,
           "now: %s; the default keys: %s; before: %s", pq.Text().c_str(), pd.Text().c_str(), pqOld.Text().c_str());
    const QteSim jc = PromptRun(jumpC, false, qte::kKeySpace, false, {1}), jcOld = PromptRun(jumpC, true, qte::kKeySpace, false, {1}),
                 jd = PromptRun(d, false, qte::kKeySpace, false, {1});
    Report("15c", "jump moved to C, a Space press prompt (JumpClimb) with the frame not read: Space once, C never; before: Space twice and C",
           jc.Downs(kSpace) == 1 && jc.Downs(kC) == 0 && jc.Text() == jd.Text() && jcOld.Downs(kSpace) == 2 && jcOld.Downs(kC) == 1,
           "now: %s; before: %s", jc.Text().c_str(), jcOld.Text().c_str());
    const QteSim um = PromptRun(useMouse, false, qte::kKeyE, false, {1}), umOld = PromptRun(useMouse, true, qte::kKeyE, false, {1});
    Report("15d", "use on the mouse alone (no key for it), an E prompt with the frame not read: E once, nothing said of use; before: E twice and \"use "
                  "not pressed\" with no Y pressed",
           um.Downs(kE) == 1 && um.AllDowns() == 1 && um.noKey == 0 && umOld.Downs(kE) == 2 && umOld.noKey == 1,
           "now: %s (%d with no key); before: %s (%d with no key)", um.Text().c_str(), um.noKey, umOld.Text().c_str(), umOld.noKey);
    const QteSim md = PromptRun(d, false, qte::kKeyE, true, {1, 11}, 24), mq = PromptRun(useQ, false, qte::kKeyE, true, {1, 11}, 24),
                 mqOld = PromptRun(useQ, true, qte::kKeyE, true, {1, 11}, 24);
    Report("15e", "use moved to Q, a mash prompt E with frames not read in its first and second taps: as many E presses as the default keys', Q never; "
                  "before: one more E and a Q for each",
           md.Downs(kE) == 3 && mq.Downs(kE) == md.Downs(kE) && mq.Downs(kQ) == 0 && mq.Text() == md.Text() && mqOld.Downs(kE) == 5 && mqOld.Downs(kQ) == 2,
           "default %d presses; Q's now: %s; before: %s", md.Downs(kE), mq.Text().c_str(), mqOld.Text().c_str());
    // 15f a fight's press prompt E (a fresh squeeze) closing 28 ms into its tap, play's Interact awaited beside the fight (a single: proper none): E kept
    // its 70 ms, Q never; before: E cut at 28 ms and Q for the rest.
    const auto closing = [](const keybind::Table& t, bool old) {
        QteSim s(t, old);
        Awaited beside;
        beside.await = qte::kKeyE;  // (play's Interact 0x23: neither quick nor proper)
        s.Frame(Awaited{}, 0.0f, 0.0f);
        s.Frame(PromptOf(qte::kKeyE), 0.0f, 0.0f);  // (the grip seen let go in the prompt: a fresh squeeze next)
        s.Frame(PromptOf(qte::kKeyE), 0.0f, 1.0f);
        s.Frame(PromptOf(qte::kKeyE), 0.0f, 1.0f);
        for (int i = 0; i < 8; ++i)
            s.Frame(beside, 0.0f, 1.0f);
        for (int i = 0; i < 3; ++i)
            s.Frame(Awaited{}, 0.0f, 0.0f);
        return s;
    };
    const QteSim cq = closing(useQ, false), cqOld = closing(useQ, true), cd = closing(d, false);
    Report("15f", "use moved to Q, a fight's E prompt closing 28 ms into its tap with play's Interact awaited beside it: E its 70 ms, Q never (as the "
                  "default keys' E); before: E cut, Q down",
           cq.Downs(kE) == 1 && cq.Downs(kQ) == 0 && cq.Text() == cd.Text() && cqOld.Downs(kQ) == 1,
           "now: %s; the default keys: %s; before: %s", cq.Text().c_str(), cd.Text().c_str(), cqOld.Text().c_str());
    // 15g play's Interact held in a scripted moment (a trigger read by its level), InteractDrag then awaited beside it with the grip still squeezed:
    // Q (the player's use) goes up and the game's E down once (its key, the quick-time action's), let go with the grip.
    const auto upgrade = [](const keybind::Table& t) {
        QteSim s(t, false);
        Awaited play, drag;
        play.await = play.hold = qte::kKeyE;  // (play's Interact alone)
        drag.await = drag.hold = drag.quick = drag.proper = qte::kKeyE;  // (InteractDrag beside it)
        s.Frame(Awaited{}, 0.0f, 0.0f);
        s.Frame(play, 0.0f, 0.0f);
        for (int i = 0; i < 4; ++i)
            s.Frame(play, 1.0f, 0.0f);
        for (int i = 0; i < 6; ++i)
            s.Frame(drag, 1.0f, 0.0f);
        for (int i = 0; i < 3; ++i)
            s.Frame(drag, 0.0f, 0.0f);
        return s;
    };
    const QteSim uq = upgrade(useQ), ud = upgrade(d);
    Report("15g", "use moved to Q, play's Interact held, InteractDrag then awaited beside it: Q up, the game's E down once; the default keys: E held "
                  "throughout, one press",
           uq.Downs(kQ) == 1 && uq.Downs(kE) == 1 && uq.Up() && ud.Downs(kE) == 1 && ud.AllDowns() == 1 && ud.Up(),
           "use on Q: %s; the default keys: %s", uq.Text().c_str(), ud.Text().c_str());
    // 15h The Great Destroyer's train: a climb's JumpClimb beside its fight (quick none: a fight's triggers live; proper Space), fresh squeezes: Space,
    // never C; before: C (play's jump), the climb never answered.
    const auto climb = [](const keybind::Table& t, bool old) {
        QteSim s(t, old);
        Awaited climbing;
        climbing.await = climbing.proper = qte::kKeySpace;  // (quick 0: the fight's triggers live)
        for (int round = 0; round < 3; ++round) {
            for (int i = 0; i < 4; ++i)
                s.Frame(climbing, 0.0f, 0.0f);
            for (int i = 0; i < 6; ++i)
                s.Frame(climbing, 0.0f, 1.0f);
        }
        s.Frame(Awaited{}, 0.0f, 0.0f);
        return s;
    };
    const QteSim kc = climb(jumpC, false), kcOld = climb(jumpC, true), kd = climb(d, false);
    Report("15h", "jump moved to C, the train's climb beside its fight (a fight's triggers live): Space for each fresh squeeze, C never (as the default "
                  "keys); before: C, the climb never answered",
           kc.Downs(kSpace) == 3 && kc.Downs(kC) == 0 && kc.Text() == kd.Text() && kcOld.Downs(kC) == 3 && kcOld.Downs(kSpace) == 0,
           "now: %s; before: %s", kc.Text().c_str(), kcOld.Text().c_str());
    // 15i in play, a single InteractDrag awaited (use moved to Q), the gun's grip squeezed afresh: the game's E, never Q.
    QteSim inPlay(useQ, false);
    inPlay.scripted = false;
    {
        Awaited single;
        single.await = single.quick = single.proper = qte::kKeyE;
        inPlay.Frame(single, 0.0f, 0.0f);
        for (int i = 0; i < 10; ++i)
            inPlay.Frame(single, 0.0f, 1.0f);
        for (int i = 0; i < 3; ++i)
            inPlay.Frame(Awaited{}, 0.0f, 0.0f);
    }
    Report("15i", "in play, a single's InteractDrag with use moved to Q, the gun's grip squeezed afresh: the game's E, never Q",
           inPlay.Downs(kE) >= 1 && inPlay.Downs(kQ) == 0 && inPlay.Up(), "%s", inPlay.Text().c_str());
    // 15j the game's default keys (its file, or none): every one of the runs above sends the same now as before.
    int same = 0, runs = 0;
    for (const keybind::Table* t : {&d, &none}) {
        const auto check = [&](const QteSim& a, const QteSim& b) {
            ++runs;
            same += a.Text() == b.Text();
        };
        check(PromptRun(*t, false, qte::kKeyE, false, {1}), PromptRun(*t, true, qte::kKeyE, false, {1}));
        check(PromptRun(*t, false, qte::kKeySpace, false, {1}), PromptRun(*t, true, qte::kKeySpace, false, {1}));
        check(PromptRun(*t, false, qte::kKeyE, true, {1, 11}, 24), PromptRun(*t, true, qte::kKeyE, true, {1, 11}, 24));
        check(closing(*t, false), closing(*t, true));
        check(climb(*t, false), climb(*t, true));
    }
    Report("15j", "the game's default keys (its file, or none): every run above sends just as before", same == runs, "%d of %d the same", same, runs);
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
    Guard();
    Looks();
    ReloadKey();
    QuickTimeKeys();
    std::printf("\n%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}
