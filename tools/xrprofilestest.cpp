// The other controllers' OpenXR profiles (src/xrprofiles.h, header only, as input.cpp's SuggestOthers suggests them) without a runtime.
//   1  every path of every profile (its bindings, its grips, the grip poses, the haptics and the aim poses input.cpp adds) is one the OpenXR
//      specification (or its controller's extension) lists for that profile and that hand
//   2  each action bound to an input of its kind: the aim to a pose, the triggers and the grips to a value or a click (a click to a float action is
//      0 or 1), the sticks to a 2D input's parent, the buttons to a click
//   3  every control of the Touch's on each controller: aim, fire, aim down sights, walk, turn, the magazine, use, sprint, recenter, both grips, a
//      jump where it has a button for one; nothing on a D-pad or a View button (the Quest has neither); no path twice
//   4  the controllers with the Touch's buttons have its layout bit for bit as input.cpp suggests the Touch's
//   5  the extensions: each profile that needs one names it; OpenXR 1.0's own need none; a profile left out without its extension
// Usage: xrprofilestest
// The exit code is the number of failed checks.

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../src/xrprofiles.h"

namespace {

int g_failed = 0, g_counted = 0;

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

// The specification's input and output paths of each profile (OpenXR 1.0, 6.4 Interaction Profiles, and the extensions XR_EXT_hp_mixed_reality_
// controller, XR_HTC_vive_cosmos_controller_interaction, XR_HTC_vive_focus3_controller_interaction (revision 2: squeeze/value), XR_BD_controller_
// interaction): those on both hands, the left's own, the right's own.
struct SpecProfile {
    const char* path;
    std::vector<const char*> both, left, right;
};

const std::vector<SpecProfile>& Spec() {
    static const std::vector<SpecProfile> spec = {
        {"/interaction_profiles/hp/mixed_reality_controller",
         {"input/menu/click", "input/squeeze/value", "input/trigger/value", "input/thumbstick/x", "input/thumbstick/y", "input/thumbstick/click",
          "input/grip/pose", "input/aim/pose", "output/haptic"},
         {"input/x/click", "input/y/click"},
         {"input/a/click", "input/b/click"}},
        {"/interaction_profiles/microsoft/motion_controller",
         {"input/menu/click", "input/squeeze/click", "input/trigger/value", "input/thumbstick/x", "input/thumbstick/y", "input/thumbstick/click",
          "input/trackpad/x", "input/trackpad/y", "input/trackpad/click", "input/trackpad/touch", "input/grip/pose", "input/aim/pose", "output/haptic"},
         {},
         {}},
        {"/interaction_profiles/htc/vive_controller",
         {"input/system/click", "input/squeeze/click", "input/menu/click", "input/trigger/click", "input/trigger/value", "input/trackpad/x",
          "input/trackpad/y", "input/trackpad/click", "input/trackpad/touch", "input/grip/pose", "input/aim/pose", "output/haptic"},
         {},
         {}},
        {"/interaction_profiles/htc/vive_cosmos_controller",
         {"input/shoulder/click", "input/squeeze/click", "input/trigger/click", "input/trigger/value", "input/thumbstick/x", "input/thumbstick/y",
          "input/thumbstick/click", "input/thumbstick/touch", "input/grip/pose", "input/aim/pose", "output/haptic"},
         {"input/x/click", "input/y/click", "input/menu/click"},
         {"input/a/click", "input/b/click", "input/system/click"}},
        {"/interaction_profiles/htc/vive_focus3_controller",
         {"input/squeeze/click", "input/squeeze/touch", "input/squeeze/value", "input/trigger/click", "input/trigger/touch", "input/trigger/value",
          "input/thumbstick/x", "input/thumbstick/y", "input/thumbstick/click", "input/thumbstick/touch", "input/thumbrest/touch", "input/grip/pose",
          "input/aim/pose", "output/haptic"},
         {"input/x/click", "input/y/click", "input/menu/click"},
         {"input/a/click", "input/b/click", "input/system/click"}},
        {"/interaction_profiles/bytedance/pico4_controller",
         {"input/system/click", "input/trigger/click", "input/trigger/value", "input/trigger/touch", "input/thumbstick/x", "input/thumbstick/y",
          "input/thumbstick/click", "input/thumbstick/touch", "input/squeeze/click", "input/squeeze/value", "input/grip/pose", "input/aim/pose",
          "output/haptic"},
         {"input/x/click", "input/x/touch", "input/y/click", "input/y/touch", "input/menu/click"},
         {"input/a/click", "input/a/touch", "input/b/click", "input/b/touch"}},
    };
    return spec;
}

const SpecProfile* SpecOf(const char* path) {
    for (const SpecProfile& s : Spec())
        if (std::strcmp(s.path, path) == 0)
            return &s;
    return nullptr;
}

// A path's hand (0 left, 1 right; -1 neither) and the rest after "/user/hand/<hand>/".
int HandOf(const std::string& path, std::string& rest) {
    for (int h = 0; h < 2; ++h) {
        const std::string prefix = h ? "/user/hand/right/" : "/user/hand/left/";
        if (path.compare(0, prefix.size(), prefix) == 0) {
            rest = path.substr(prefix.size());
            return h;
        }
    }
    return -1;
}

// Whether the specification lists it (a 2D input's parent: its x and y listed).
bool Listed(const SpecProfile& s, const std::string& path) {
    std::string rest;
    const int hand = HandOf(path, rest);
    if (hand < 0)
        return false;
    const auto has = [&](const std::string& r) {
        for (const char* p : s.both)
            if (r == p)
                return true;
        for (const char* p : hand ? s.right : s.left)
            if (r == p)
                return true;
        return false;
    };
    return has(rest) || (has(rest + "/x") && has(rest + "/y"));
}

// The kind of input each action takes: 'p' a pose, 'f' a float (a value or a click), 'v' a 2D input, 'b' a boolean (a click).
char KindOf(xrprofiles::Action a) {
    switch (a) {
    case xrprofiles::kAim: return 'p';
    case xrprofiles::kFire: case xrprofiles::kAds: return 'f';
    case xrprofiles::kMove: case xrprofiles::kTurn: return 'v';
    default: return 'b';
    }
}

bool Fits(char kind, const std::string& path) {
    const auto ends = [&](const char* t) { return path.size() >= std::strlen(t) && path.compare(path.size() - std::strlen(t), std::strlen(t), t) == 0; };
    switch (kind) {
    case 'p': return ends("/pose");
    case 'f': return ends("/value") || ends("/click");
    case 'v': return ends("/thumbstick") || ends("/trackpad");
    case 't': return ends("/touch");
    default: return ends("/click");
    }
}

// Every path input.cpp suggests for a profile, with its kind: the bindings, a grips' way, the grip poses, the haptics, the hands' aim poses.
std::vector<std::pair<std::string, char>> Suggested(const xrprofiles::Profile& p, const xrprofiles::Grips& g) {
    std::vector<std::pair<std::string, char>> out;
    for (int k = 0; k < p.count; ++k)
        out.push_back({p.binding[k].path, KindOf(p.binding[k].action)});
    out.push_back({g.leftSqueeze, 'f'});
    out.push_back({g.rightSqueeze, 'f'});
    if (g.leftTouch)
        out.push_back({g.leftTouch, 't'});
    if (g.rightTouch)
        out.push_back({g.rightTouch, 't'});
    out.push_back({"/user/hand/left/input/grip/pose", 'p'});
    out.push_back({"/user/hand/right/input/grip/pose", 'p'});
    out.push_back({"/user/hand/left/output/haptic", 'h'});
    out.push_back({"/user/hand/right/output/haptic", 'h'});
    for (const char* aim : xrprofiles::kAimPose)
        out.push_back({aim, 'p'});
    return out;
}

void Paths() {
    int checked = 0, unlisted = 0, misfit = 0;
    std::string wrong;
    for (int i = 0; i < xrprofiles::kProfileCount; ++i) {
        const xrprofiles::Profile& p = xrprofiles::kProfiles[i];
        const SpecProfile* s = SpecOf(p.path);
        if (!s) {
            wrong += std::string(" no specification list for ") + p.path;
            ++unlisted;
            continue;
        }
        for (const xrprofiles::Grips& g : p.grips) {
            if (!g.leftSqueeze)
                break;
            for (const auto& [path, kind] : Suggested(p, g)) {
                ++checked;
                if (!Listed(*s, path)) {
                    ++unlisted;
                    wrong += " " + std::string(p.name) + ": " + path;
                }
                if (kind != 'h' && !Fits(kind, path)) {
                    ++misfit;
                    wrong += " " + std::string(p.name) + " (kind " + kind + "): " + path;
                }
            }
        }
    }
    Report("1", "every path suggested (bindings, grips, grip poses, haptics, aim poses) is the specification's for its profile and hand", unlisted == 0,
           "%d paths, %d not listed%s", checked, unlisted, wrong.c_str());
    Report("2", "each action bound to an input of its kind (a pose, a value or a click, a 2D input's parent, a click, a touch)", misfit == 0,
           "%d not of their kind", misfit);
}

void Controls() {
    int complete = 0, dpad = 0, twice = 0, jumps = 0, gripsBoth = 0;
    std::string missing;
    for (int i = 0; i < xrprofiles::kProfileCount; ++i) {
        const xrprofiles::Profile& p = xrprofiles::kProfiles[i];
        bool has[xrprofiles::kActionCount] = {};
        std::set<std::string> paths;
        for (int k = 0; k < p.count; ++k) {
            has[p.binding[k].action] = true;
            if (!paths.insert(p.binding[k].path).second)
                ++twice;
            if (std::strstr(p.binding[k].path, "dpad") || std::strstr(p.binding[k].path, "/view/"))
                ++dpad;
        }
        const xrprofiles::Action needed[] = {xrprofiles::kAim, xrprofiles::kFire, xrprofiles::kAds, xrprofiles::kMove, xrprofiles::kTurn,
                                             xrprofiles::kReload, xrprofiles::kUse, xrprofiles::kSprint, xrprofiles::kRecenter};
        bool all = true;
        for (xrprofiles::Action a : needed)
            if (!has[a]) {
                all = false;
                missing += std::string(" ") + p.name + " lacks action " + std::to_string(a);
            }
        complete += all;
        // A jump: A, a trackpad's click, or (no button for one) the recenter's short click, which jumps on every controller.
        jumps += has[xrprofiles::kJump] || has[xrprofiles::kRecenter];
        bool grips = true;
        for (const xrprofiles::Grips& g : p.grips) {
            if (!g.leftSqueeze)
                break;
            grips = grips && g.rightSqueeze && std::strstr(g.leftSqueeze, "/left/") && std::strstr(g.rightSqueeze, "/right/");
        }
        gripsBoth += grips && p.grips[0].leftSqueeze;
    }
    Report("3a", "every controller has the Touch's controls: aim, fire, aim down sights, walk, turn, the magazine, use, sprint, recenter",
           complete == xrprofiles::kProfileCount, "%d of %d%s", complete, xrprofiles::kProfileCount, missing.c_str());
    Report("3b", "... both grips on each (a squeeze's value or a button's click), a jump on each (A, a trackpad's click, or the right stick's click)",
           gripsBoth == xrprofiles::kProfileCount && jumps == xrprofiles::kProfileCount, "grips %d, jumps %d of %d", gripsBoth, jumps,
           xrprofiles::kProfileCount);
    Report("3c", "nothing on a D-pad or a View button (the Quest has neither), no path bound twice", dpad == 0 && twice == 0, "%d on a D-pad or View, %d twice",
           dpad, twice);
}

// input.cpp's own Touch suggestion (SuggestBindings), copied: the layout the controllers with A/B and X/Y must have bit for bit.
const std::vector<std::pair<xrprofiles::Action, std::string>> kTouch = {
    {xrprofiles::kAim, "/user/hand/right/input/aim/pose"},          {xrprofiles::kFire, "/user/hand/right/input/trigger/value"},
    {xrprofiles::kAds, "/user/hand/left/input/trigger/value"},      {xrprofiles::kMove, "/user/hand/left/input/thumbstick"},
    {xrprofiles::kTurn, "/user/hand/right/input/thumbstick"},       {xrprofiles::kJump, "/user/hand/right/input/a/click"},
    {xrprofiles::kCrouch, "/user/hand/right/input/b/click"},        {xrprofiles::kReload, "/user/hand/left/input/x/click"},
    {xrprofiles::kUse, "/user/hand/left/input/y/click"},            {xrprofiles::kSelectSwap, "/user/hand/left/input/menu/click"},
    {xrprofiles::kSprint, "/user/hand/left/input/thumbstick/click"}, {xrprofiles::kRecenter, "/user/hand/right/input/thumbstick/click"},
};

void TouchLayout() {
    int touchLike = 0, same = 0;
    std::string names;
    for (int i = 0; i < xrprofiles::kProfileCount; ++i) {
        const xrprofiles::Profile& p = xrprofiles::kProfiles[i];
        if (std::strcmp(p.layout, "as the Touch's") != 0)
            continue;
        ++touchLike;
        bool all = p.count == static_cast<int>(kTouch.size());
        for (const auto& [action, path] : kTouch) {
            bool found = false;
            for (int k = 0; k < p.count; ++k)
                found = found || (p.binding[k].action == action && path == p.binding[k].path);
            all = all && found;
        }
        same += all;
        names += std::string(names.empty() ? "" : ", ") + p.name;
    }
    Report("4", "the controllers with the Touch's buttons have its layout bit for bit (input.cpp's Touch suggestion)", touchLike == 4 && same == touchLike,
           "%d of %d (%s)", same, touchLike, names.c_str());
}

void Extensions() {
    int named = 0, core = 0, needs = 0;
    for (int i = 0; i < xrprofiles::kProfileCount; ++i) {
        const xrprofiles::Profile& p = xrprofiles::kProfiles[i];
        const bool isCore = std::strstr(p.path, "/microsoft/motion_controller") || std::strstr(p.path, "/htc/vive_controller");
        if (isCore)
            core += p.extension == -1;
        else {
            ++needs;
            named += p.extension >= 0 && p.extension < xrprofiles::kExtensionCount;
        }
    }
    const bool hp = std::strcmp(xrprofiles::kExtensions[xrprofiles::kProfiles[0].extension], "XR_EXT_hp_mixed_reality_controller") == 0;
    Report("5a", "each profile that needs an extension names one (the HP's XR_EXT_hp_mixed_reality_controller...), OpenXR 1.0's own none",
           named == needs && core == 2 && hp, "%d of %d named, %d of 2 core", named, needs, core);
    int usableNone = 0, usableAll = 0;
    for (int i = 0; i < xrprofiles::kProfileCount; ++i) {
        usableNone += xrprofiles::Usable(xrprofiles::kProfiles[i], 0);
        usableAll += xrprofiles::Usable(xrprofiles::kProfiles[i], (1u << xrprofiles::kExtensionCount) - 1);
    }
    Report("5b", "with no extension enabled only Windows Mixed Reality and the Vive's wands are suggested; with all, every one", usableNone == 2 &&
           usableAll == xrprofiles::kProfileCount, "%d and %d", usableNone, usableAll);
}

}  // namespace

int main() {
    Paths();
    Controls();
    TouchLayout();
    Extensions();
    std::printf("\n%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}
