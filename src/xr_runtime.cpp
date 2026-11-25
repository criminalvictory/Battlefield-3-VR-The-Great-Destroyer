// OpenXR side of the mod. Two output modes:
//
// Theater: the game's finished frame is shown as a large quad floating in front of the player.
//
// Stereo (milestone 2): alternate-eye rendering. Each game frame is assigned to one eye in
// turn and submitted as that eye's projection view, together with the headset pose it was
// shown for. The other eye keeps showing its last image. Until the camera write lands
// (see camera.cpp) both eyes see the same viewpoint, so this exercises the pipeline only.

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11_4.h>
#include <dxgi.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "aim.h"
#include "ammo.h"
#include "footsteps.h"
#include "ground.h"
#include "camera.h"
#include "config.h"
#include "depth3d.h"
#include "edgefill.h"
#include "effects.h"
#include "finder.h"
#include "fov.h"
#include "bodies.h"
#include "god.h"
#include "gputime.h"
#include "hands.h"
#include "headmove.h"
#include "headtest.h"
#include "input.h"
#include "launchreport.h"
#include "log.h"
#include "pad.h"
#include "perfinfo.h"
#include "pickups.h"
#include "skin.h"
#include "overlay.h"
#include "streamview.h"
#include "scopeview.h"
#include "views.h"
#include "window.h"
#include "xr_runtime.h"
#include "xrpick.h"
#include "xrprofiles.h"
#include "spacechange.h"

#include <atomic>

namespace {

constexpr float kPi = 3.14159265f;

struct Swapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageD3D11KHR> images;
};

// Index 0/1 are the eyes in stereo mode, 2 is the theater screen.
constexpr int kLeftEye = 0, kRightEye = 1, kTheater = 2;

XrInstance g_instance = XR_NULL_HANDLE;
XrSystemId g_system = XR_NULL_SYSTEM_ID;
XrSession g_session = XR_NULL_HANDLE;
XrSpace g_localSpace = XR_NULL_HANDLE;  // runtime's LOCAL origin, never moved
XrSpace g_space = XR_NULL_HANDLE;       // LOCAL shifted by recenter; all layers live here
XrSpace g_viewSpace = XR_NULL_HANDLE;   // the headset itself
Swapchain g_swapchains[3];
// Per-eye views: the picture with its edge carried outwards (edgefill.h), bigger than the back buffer; made once the headset's field is known.
Swapchain g_fillChain;
edgefill::Layout g_fillLayout;
bool g_fillActive = false;  // this frame's image is in it
bool g_fillBroken = false;  // it can't be done here: as before

ID3D11Device* g_device = nullptr;
UINT g_scWidth = 0, g_scHeight = 0;
DXGI_FORMAT g_scFormat = DXGI_FORMAT_UNKNOWN;
DXGI_FORMAT g_scSourceFormat = DXGI_FORMAT_UNKNOWN;

bool g_failed = false;
bool g_running = false;
bool g_resizePending = false;
bool g_recenterPending = true;
// The session's state as last told (HandleSessionState): focused, somebody is wearing the headset (the head replay never acts then).
XrSessionState g_sessionState = XR_SESSION_STATE_UNKNOWN;

// Alternate-eye state: which eye the next game frame goes to, and the pose each eye was last shown for.
int g_nextEye = kLeftEye;
XrPosef g_eyePose[2] = {};
bool g_eyeValid[2] = {};
// The desktop window shows every game frame, so in stereo it would alternate between the eyes
// and fine detail flickers. The last left-eye frame is kept here and shown in place of each
// right-eye frame, once the right eye has gone to the headset.
ID3D11Texture2D* g_mirror = nullptr;
bool g_mirrorValid = false;
// Depth-3D eyes made while the headset is idle, for the desktop (PreviewDepth3D).
ID3D11Texture2D* g_previewEyes[2] = {};

// The eye poses located at each of the last few frames, the newest at g_poseNewest. The game draws
// each frame with the head pose its camera was handed some frames earlier; submitting the image
// with that pose lets the compositor correct for the latency. One frame was assumed; the picture
// doubling on head turns, more so at a lower frame rate, suggests more ([stereo] pose_lag, F11).
constexpr int kPoseHistory = 4;
XrPosef g_poseHistory[kPoseHistory][2] = {};
int g_poseNewest = 0;
int g_posesKept = 0;
// The same by their ids, as each is published with its turn (camera.h NoteDrawnView): the picture's own pose, found by its main view, not guessed.
constexpr uint32_t kPoseRing = 16;
XrPosef g_poseById[kPoseRing][2] = {};
uint32_t g_poseSeq = 0;  // the newest published
uint32_t g_drawnAge = 2;  // how many poses old the last named picture's was
// Pictures since the last log by how many poses old theirs was (1 to 4, more), and not named; how far pose_lag's guess was from the pose really drawn with.
uint32_t g_poseAges[6] = {}, g_poseUnnamed = 0, g_poseAgeChanges = 0;
double g_poseGuessOff = 0.0;
float g_poseGuessWorst = 0.0f;
ULONGLONG g_poseLogged = 0;

// Degrees between two orientations.
float AngleBetween(const XrQuaternionf& a, const XrQuaternionf& b) {
    const float dot = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
    return 2.0f * std::acos(std::min(dot, 1.0f)) * 57.29578f;
}

bool g_aimingWithHand = false;
// The look yaw could be written last frame, so the stick's turn goes in with it (input.cpp).
bool g_turnDirect = false;
// The picture's body (BuildStereo) kept from before a recenter or a mode switch is stale: started again.
bool g_bodyStale = true;

// Yaw (turning left positive) and pitch (looking up positive) of where q points: (0, 0, -1) rotated by q.
void LookAngles(const XrQuaternionf& q, float& yaw, float& pitch) {
    float fx = -2.0f * (q.x * q.z + q.w * q.y);
    float fy = -2.0f * (q.y * q.z - q.w * q.x);
    float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
    yaw = std::atan2(-fx, -fz);
    pitch = std::asin(std::clamp(fy, -1.0f, 1.0f));
}

bool g_poseFilterFresh = true;  // FilterViews starts afresh

// Also restarts head tracking, so a mode switch or recenter doesn't jerk the view.
void ResetEyes() {
    g_poseFilterFresh = true;
    g_eyeValid[0] = g_eyeValid[1] = false;
    g_posesKept = 0;
    aim::Reset();
    g_bodyStale = true;
    streamview::Reset();
}
// F7: the game may render a frame later than we think, which swaps the eyes and turns depth inside out.
bool g_swapEyes = false;
// How the two eyes are made (Scroll Lock cycles): from each frame's picture and depth (both eyes
// every frame, depth3d.cpp); alternate eyes (each game frame for one eye: the picture doubled on
// head turns, more so at lower frame rates); mono (every frame to both eyes, flat: no doubling,
// which showed the alternating caused it); frozen (nothing new shown: the compositor held the
// picture still like a wall, so it handles each eye's pose); hybrid (each frame is rendered from
// one eye, alternately, and goes to that eye as it is; the other eye is made from it and its
// depth, so both eyes get every frame and only one of them has filled gaps at a time).
// Per-eye views (views.cpp): BF3 draws both eyes each frame, side by side; depth 3D and the
// others are the earlier ways, kept on Scroll Lock.
enum StereoTest { kTestViews, kTestDepth, kTestHybrid, kTestNone, kTestMono, kTestFreeze, kStereoTests };
int g_stereoTest = kTestViews;
// This frame's image holds both eyes, side by side, in the left eye's swapchain.
bool g_sideBySide = false;

// The field of view (radians) the game really drew the last frame with: with per-eye views what
// views.cpp built them with (the render camera's pointer chain is lost then), otherwise what the
// camera module reads from the render view.
float DrawnFov() {
    float eyes = views::RenderedFov();
    return eyes > 0.0f ? eyes : camera::RenderedFov();
}
// The picture's turn each frame (BuildStereo): with the look yaw written or steered by the mouse, or following
// the game's camera from the yaw the last picture took out of it.
enum ViewMode { kViewLook, kViewFollow };

// A picture the game drew without the head's turn in its camera (camera::TakeTurned none: the
// cutscenes of The Great Destroyer, as Kovac walks in; the viewlog showed no camera turned there
// for 20 s at a time) was still handed over as seen from the head, so the headset moved it the wrong
// way as the head turned (the user, 2026-10-31: "if I move it to the left, it goes to the right").
// It's handed over as seen from where it was drawn instead: the room yaw the game's camera stands
// for (the hand's yaw the view turn would have taken out, g_viewTurnYaw), level. The headset then
// keeps it still in the room and the head looks about it, the right way round.
bool g_viewTurnOn = false;
float g_viewTurnYaw = 0.0f;
bool g_unturnedLogged = false;
// A scripted camera (a cutscene's or a ride's, its rows' w lanes filled: the opening drive of Comrades) is
// turned to the head about its own yaw (camera.cpp), so its picture is handed over seen from the head like the
// player's own: no edges show as the head turns (the user, 2026-11-01: "without having to ruin the resolution").
// Turned or not, the game's camera is still the game's: g_cutscene, either kind of picture three in a row, keeps
// the look angles alone and the free hands off, as the unturned picture alone did.
bool g_scriptedLogged = false;
bool g_cutscene = false;

bool Check(XrResult result, const char* what) {
    if (XR_SUCCEEDED(result))
        return true;
    char name[XR_MAX_RESULT_STRING_SIZE] = "?";
    if (g_instance)
        xrResultToString(g_instance, result, name);
    Log("%s failed: %s (%d)", what, name, static_cast<int>(result));
    return false;
}

// The Steam Frame's controllers have a profile of their own (X and Y on the right hand, a D-pad on
// the left); without it SteamVR guesses from the Touch bindings and put crouch on X.
bool RuntimeHasExtension(const char* name) {
    uint32_t count = 0;
    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr)) || !count)
        return false;
    std::vector<XrExtensionProperties> props(count, {XR_TYPE_EXTENSION_PROPERTIES});
    if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, props.data())))
        return false;
    for (const XrExtensionProperties& p : props)
        if (std::strcmp(p.extensionName, name) == 0)
            return true;
    return false;
}

bool CreateInstance() {
    std::vector<const char*> extensions = {XR_KHR_D3D11_ENABLE_EXTENSION_NAME};
    bool frame = RuntimeHasExtension(input::kFrameControllerExtension);
    if (frame)
        extensions.push_back(input::kFrameControllerExtension);
    // The other controllers' profiles that need an extension (xrprofiles.h: the HP Reverb G2's, the Vive Cosmos', the Vive Focus 3's, the Pico 4's),
    // each enabled where the runtime offers it.
    const size_t ownExtensions = extensions.size();
    unsigned profileExtensions = 0;
    std::string offered;
    for (int i = 0; i < xrprofiles::kExtensionCount; ++i)
        if (RuntimeHasExtension(xrprofiles::kExtensions[i])) {
            extensions.push_back(xrprofiles::kExtensions[i]);
            profileExtensions |= 1u << i;
            offered += offered.empty() ? "" : ", ";
            offered += xrprofiles::kExtensions[i];
        }
    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(ci.applicationInfo.applicationName, "BF3 VR");
    ci.applicationInfo.applicationVersion = 2;
    strcpy_s(ci.applicationInfo.engineName, "bf3vr");
    ci.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.enabledExtensionNames = extensions.data();
    Log("Controllers: Steam Frame controller profile %s", frame ? "available" : "not offered by this runtime");
    if (profileExtensions)
        Log("Controllers: other controllers' extensions offered by this runtime, enabled: %s", offered.c_str());
    XrResult made = xrCreateInstance(&ci, &g_instance);
    if (XR_FAILED(made) && profileExtensions) {
        // (A runtime offering one and then refusing the instance with it: never VR lost for another controller's profile. Again without them.)
        Check(made, "xrCreateInstance with the other controllers' extensions");
        extensions.resize(ownExtensions);
        profileExtensions = 0;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        g_instance = XR_NULL_HANDLE;
        made = xrCreateInstance(&ci, &g_instance);
        Log("Controllers: the instance made again without the other controllers' extensions: %s", XR_SUCCEEDED(made) ? "made" : "refused too");
    }
    input::SetProfileExtensions(profileExtensions);
    if (!Check(made, "xrCreateInstance")) {
        launchreport::Note(launchreport::kRuntime, launchreport::IsVdxr()
                               ? "Virtual Desktop's OpenXR runtime (VDXR) wouldn't start (xrCreateInstance failed: is the Virtual Desktop Streamer running?)"
                               : "the OpenXR runtime wouldn't start (xrCreateInstance failed: is SteamVR installed and running?)");
        return false;
    }

    XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};
    const bool named = XR_SUCCEEDED(xrGetInstanceProperties(g_instance, &props));
    if (named)
        Log("OpenXR runtime: %s %u.%u.%u", props.runtimeName, XR_VERSION_MAJOR(props.runtimeVersion),
            XR_VERSION_MINOR(props.runtimeVersion), XR_VERSION_PATCH(props.runtimeVersion));

    XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};
    si.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!Check(xrGetSystem(g_instance, &si, &g_system), "xrGetSystem (is the headset connected?)")) {
        launchreport::Note(launchreport::kRuntime, launchreport::IsVdxr()
                               ? "the OpenXR runtime %s started but sees no headset (xrGetSystem failed: is Virtual Desktop on the headset connected to this PC?)"
                               : "the OpenXR runtime %s started but sees no headset (xrGetSystem failed: is the headset connected and shown in SteamVR?)",
                           named ? props.runtimeName : "(unnamed)");
        return false;
    }

    XrSystemProperties sys{XR_TYPE_SYSTEM_PROPERTIES};
    const bool described = XR_SUCCEEDED(xrGetSystemProperties(g_instance, g_system, &sys));
    if (described)
        Log("Headset: %s", sys.systemName);
    launchreport::Ok(launchreport::kRuntime, "the VR runtime %s found the headset %s", named ? props.runtimeName : "(unnamed)",
                     described ? sys.systemName : "(unnamed)");
    return true;
}

// The runtime must composite on the same GPU the game renders on.
bool CheckGraphicsRequirements(ID3D11Device* device) {
    PFN_xrGetD3D11GraphicsRequirementsKHR getRequirements = nullptr;
    if (!Check(xrGetInstanceProcAddr(g_instance, "xrGetD3D11GraphicsRequirementsKHR",
                                     reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements)),
               "xrGetInstanceProcAddr"))
        return false;
    XrGraphicsRequirementsD3D11KHR reqs{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    if (!Check(getRequirements(g_instance, g_system, &reqs), "xrGetD3D11GraphicsRequirementsKHR"))
        return false;

    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    DXGI_ADAPTER_DESC desc{};
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
        if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
            adapter->GetDesc(&desc);
            adapter->Release();
        }
        dxgiDevice->Release();
    }
    Log("Game GPU: %ls", desc.Description);
    // Retrying can't fix either of these.
    if (std::memcmp(&desc.AdapterLuid, &reqs.adapterLuid, sizeof(LUID)) != 0) {
        Log("Game is rendering on a different GPU than the headset runtime wants; VR inactive");
        g_failed = true;
        return false;
    }
    if (device->GetFeatureLevel() < reqs.minFeatureLevel) {
        Log("Game D3D feature level is below the runtime minimum; VR inactive");
        g_failed = true;
        return false;
    }
    return true;
}

bool CreateReferenceSpace(XrReferenceSpaceType type, const XrPosef& pose, XrSpace& out) {
    XrReferenceSpaceCreateInfo rs{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rs.referenceSpaceType = type;
    rs.poseInReferenceSpace = pose;
    return Check(xrCreateReferenceSpace(g_session, &rs, &out), "xrCreateReferenceSpace");
}

bool CreateSession(ID3D11Device* device) {
    XrGraphicsBindingD3D11KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    binding.device = device;
    XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};
    ci.next = &binding;
    ci.systemId = g_system;
    if (!Check(xrCreateSession(g_instance, &ci, &g_session), "xrCreateSession"))
        return false;
    // What the headset asks for per eye (SteamVR's resolution setting), against [window] size.
    uint32_t viewCount = 0;
    XrViewConfigurationView configViews[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    if (XR_SUCCEEDED(xrEnumerateViewConfigurationViews(g_instance, g_system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                       2, &viewCount, configViews)) &&
        viewCount > 0)
        Log("Headset: %ux%u per eye recommended (at most %ux%u); the game draws %d per eye ([window] size)",
            configViews[0].recommendedImageRectWidth, configViews[0].recommendedImageRectHeight,
            configViews[0].maxImageRectWidth, configViews[0].maxImageRectHeight, config::Get().windowSize);

    const XrPosef identity{{0, 0, 0, 1}, {0, 0, 0}};
    return CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL, identity, g_localSpace) &&
           CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL, identity, g_space) &&
           CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_VIEW, identity, g_viewSpace);
}

bool Init(ID3D11Device* device) {
    if (!g_device) {
        g_device = device;
        g_device->AddRef();
    }
    return CreateInstance() && CheckGraphicsRequirements(device) && CreateSession(device);
}

// The OpenXR loader explains why it can't find or load a runtime, but only on stderr.
// Send that to bf3vr_xr.log, and note any OpenXR/VR settings the game inherited from its launcher.
void EnableLoaderDiagnostics() {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&EnableLoaderDiagnostics), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"bf3vr_xr.log");
    FILE* f = nullptr;
    if (_wfreopen_s(&f, path, L"w", stderr) == 0)
        SetEnvironmentVariableA("XR_LOADER_DEBUG", "all");

    if (wchar_t* env = GetEnvironmentStringsW()) {
        for (const wchar_t* var = env; *var; var += wcslen(var) + 1) {
            if (_wcsnicmp(var, L"XR_", 3) == 0 || _wcsnicmp(var, L"VR_", 3) == 0 || wcsstr(var, L"OPENXR") ||
                wcsstr(var, L"OpenXR") || _wcsnicmp(var, L"STEAMVR", 7) == 0)
                Log("Inherited env: %ls", var);
        }
        FreeEnvironmentStringsW(env);
    }
}

// Steam hands games it launches while SteamVR is running an XR_RUNTIME_JSON that names the
// 64-bit runtime (steamxr_win64.json). A 32-bit game like BF3 can't load it, and the loader
// gives up with "runtime unavailable". Point it at the 32-bit runtime instead.
void FixRuntimeOverride() {
    if (sizeof(void*) != 4)
        return;
    wchar_t value[MAX_PATH] = {};
    if (!GetEnvironmentVariableW(L"XR_RUNTIME_JSON", value, MAX_PATH))
        return;
    std::wstring path(value);
    size_t pos = path.rfind(L"win64");
    if (pos == std::wstring::npos)
        return;
    path.replace(pos, 5, L"win32");
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SetEnvironmentVariableW(L"XR_RUNTIME_JSON", path.c_str());
        Log("XR_RUNTIME_JSON pointed at a 64-bit runtime; using %ls", path.c_str());
    } else {
        // No 32-bit file next to it: fall back to the registered 32-bit runtime.
        SetEnvironmentVariableW(L"XR_RUNTIME_JSON", nullptr);
        Log("XR_RUNTIME_JSON pointed at a 64-bit runtime; cleared it to use the registered 32-bit one");
    }
}

// A registry string under HKLM in the 32-bit view (the one a 32-bit OpenXR loader reads), or empty.
std::wstring Registry32(const wchar_t* key, const wchar_t* name) {
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ | KEY_WOW64_32KEY, &k) != ERROR_SUCCESS)
        return {};
    wchar_t value[MAX_PATH * 2] = {};
    DWORD size = sizeof(value) - sizeof(wchar_t), type = 0;
    const bool ok = RegQueryValueExW(k, name, nullptr, &type, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS &&
                    (type == REG_SZ || type == REG_EXPAND_SZ);
    RegCloseKey(k);
    return ok ? value : std::wstring();
}

bool FileThere(const std::wstring& path) {
    const DWORD a = path.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// SteamVR's 32-bit OpenXR runtime (steamxr_win32.json) where this PC has one: among the 32-bit runtimes registered as
// available, else in SteamVR's own folder as SteamVR last wrote it (%LOCALAPPDATA%\openvr\openvrpaths.vrpath), else in
// Steam's folder. Empty: none found.
std::wstring SteamVr32Manifest() {
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes", 0, KEY_READ | KEY_WOW64_32KEY, &k) ==
        ERROR_SUCCESS) {
        std::wstring found;
        for (DWORD i = 0; found.empty(); ++i) {
            wchar_t name[MAX_PATH * 2];
            DWORD length = static_cast<DWORD>(std::size(name));
            if (RegEnumValueW(k, i, name, &length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;
            if (xrpick::IsSteamVr32(name) && FileThere(name))
                found = name;
        }
        RegCloseKey(k);
        if (!found.empty())
            return found;
    }
    wchar_t local[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH)) {
        // {"runtime": ["C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR"], ...}: its folders, JSON escapes undone.
        FILE* f = nullptr;
        if (_wfopen_s(&f, (std::wstring(local) + L"\\openvr\\openvrpaths.vrpath").c_str(), L"rb") == 0 && f) {
            std::string text(64 * 1024, '\0');
            text.resize(fread(text.data(), 1, text.size(), f));
            fclose(f);
            const size_t runtime = text.find("\"runtime\"");
            const size_t open = runtime == std::string::npos ? std::string::npos : text.find('[', runtime);
            const size_t close = open == std::string::npos ? std::string::npos : text.find(']', open);
            for (size_t q = open; close != std::string::npos && (q = text.find('"', q + 1)) < close;) {
                std::wstring folder;
                size_t e = q + 1;
                for (; e < close && text[e] != '"'; ++e) {
                    if (text[e] == '\\' && e + 1 < close)
                        ++e;
                    folder += static_cast<unsigned char>(text[e]);  // (a folder's name outside ASCII isn't found this way)
                }
                q = e;
                if (FileThere(folder + L"\\steamxr_win32.json"))
                    return folder + L"\\steamxr_win32.json";
            }
        }
    }
    HKEY steam = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_READ, &steam) == ERROR_SUCCESS) {
        wchar_t path[MAX_PATH] = {};
        DWORD size = sizeof(path) - sizeof(wchar_t), type = 0;
        const bool ok = RegQueryValueExW(steam, L"SteamPath", nullptr, &type, reinterpret_cast<BYTE*>(path), &size) == ERROR_SUCCESS && type == REG_SZ;
        RegCloseKey(steam);
        const std::wstring manifest = std::wstring(path) + L"\\steamapps\\common\\SteamVR\\steamxr_win32.json";
        if (ok && FileThere(manifest))
            return manifest;
    }
    return {};
}

// Virtual Desktop's own 32-bit OpenXR runtime (VDXR: virtualdesktop-openxr-32.json, part of the Virtual Desktop Streamer) where
// this PC has one: among the 32-bit runtimes registered as available, else the 32-bit active one, else beside the 64-bit one
// registered as available, else in the Streamer's usual folder. Empty: none found.
std::wstring Vdxr32Manifest() {
    auto search = [](REGSAM view, bool beside) {
        std::wstring found;
        HKEY k = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes", 0, KEY_READ | view, &k) != ERROR_SUCCESS)
            return found;
        for (DWORD i = 0; found.empty(); ++i) {
            wchar_t name[MAX_PATH * 2];
            DWORD length = static_cast<DWORD>(std::size(name));
            if (RegEnumValueW(k, i, name, &length, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;
            std::wstring path = name;
            if (beside && xrpick::FileName(path) == L"virtualdesktop-openxr.json")
                path = path.substr(0, path.find_last_of(L"\\/") + 1) + L"virtualdesktop-openxr-32.json";
            if (xrpick::IsVdxr32(path) && FileThere(path))
                found = path;
        }
        RegCloseKey(k);
        return found;
    };
    std::wstring found = search(KEY_WOW64_32KEY, false);
    if (found.empty()) {
        const std::wstring active = Registry32(L"SOFTWARE\\Khronos\\OpenXR\\1", L"ActiveRuntime");
        if (xrpick::IsVdxr32(active) && FileThere(active))
            found = active;
    }
    if (found.empty())
        found = search(KEY_WOW64_64KEY, true);
    wchar_t programs[MAX_PATH] = {};
    if (found.empty() && GetEnvironmentVariableW(L"ProgramW6432", programs, MAX_PATH)) {
        const std::wstring path = std::wstring(programs) + L"\\Virtual Desktop Streamer\\OpenXR\\virtualdesktop-openxr-32.json";
        if (FileThere(path))
            found = path;
    }
    return found;
}

// The runtime the launcher asked for: build\runtime.txt, written at each start ("vdxr" by Start BF3 VR (VDXR).bat, "steamvr" by
// Start BF3 VR.bat): its first word that isn't a comment, lowered. Empty: no file, or one from another start (the game started
// some other way since: a "vdxr" left over mustn't send a later start to VDXR; review of 2026-11-18).
std::string RuntimeAsked() {
    wchar_t path[MAX_PATH] = {};
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&RuntimeAsked), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring file = path;
    file.resize(file.find_last_of(L'\\') + 1);
    file += L"runtime.txt";
    // This start's only: written before the game started, within 20 minutes (the launcher waits up to 4 for SteamVR, then up to
    // 10 for the game).
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &info))
        return {};
    FILETIME created, exited, kernel, user;
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        const auto ticks = [](const FILETIME& t) {
            return static_cast<long long>((static_cast<unsigned long long>(t.dwHighDateTime) << 32) | t.dwLowDateTime);
        };
        constexpr long long kMinute = 600000000LL;  // in FILETIME's 100 ns
        const long long age = ticks(created) - ticks(info.ftLastWriteTime);
        if (age > 20 * kMinute || age < -kMinute) {
            Log("build\\runtime.txt is from another start (written %.1f minutes before the game started): not used",
                static_cast<double>(age) / kMinute);
            return {};
        }
    }
    FILE* f = nullptr;
    if (_wfopen_s(&f, file.c_str(), L"rb") || !f)
        return {};
    std::string text(4096, '\0');
    text.resize(fread(text.data(), 1, text.size(), f));
    fclose(f);
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF)
        text.erase(0, 3);
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos)
            end = text.size();
        size_t i = at;
        while (i < end && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r'))
            ++i;
        if (i < end && text[i] != '#') {
            std::string word;
            for (; i < end && std::isalnum(static_cast<unsigned char>(text[i])); ++i)
                word += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
            if (!word.empty())
                return word;
        }
        at = end + 1;
    }
    return {};
}

// SteamVR's 32-bit runtime preferred over another (xrpick.h), in the game's own environment, before the loader starts; VDXR
// when the launcher asked for it.
void PreferSteamVr() {
    if (sizeof(void*) != 4)
        return;
    wchar_t value[MAX_PATH * 2] = {};
    const std::wstring override = GetEnvironmentVariableW(L"XR_RUNTIME_JSON", value, MAX_PATH * 2) ? value : L"";
    const std::wstring active = Registry32(L"SOFTWARE\\Khronos\\OpenXR\\1", L"ActiveRuntime");
    const std::wstring steamVr = SteamVr32Manifest();
    const std::string asked = RuntimeAsked();
    const bool vdxr = asked == "vdxr";
    const std::wstring vd = vdxr ? Vdxr32Manifest() : std::wstring();
    if (!asked.empty())
        Log("OpenXR runtime asked for by the launcher: %s (build\\runtime.txt)%s%ls", asked.c_str(), vdxr ? "; VDXR's 32-bit manifest: " : "",
            vdxr ? (vd.empty() ? L"not found" : vd.c_str()) : L"");
    const xrpick::Choice choice = xrpick::Pick(override, active, steamVr, config::Get().preferSteamVr, vdxr, vd);
    if (choice.vdxrMissing)
        launchreport::Warn("VDXR was asked for (Start BF3 VR (VDXR).bat), but Virtual Desktop's 32-bit OpenXR runtime "
                           "(virtualdesktop-openxr-32.json, part of the Virtual Desktop Streamer) isn't on this PC: update the Virtual "
                           "Desktop Streamer, or start with Start BF3 VR.bat to play through SteamVR. %s",
                           choice.set || xrpick::IsSteamVr32(override.empty() ? active : override) ? "SteamVR's runtime is used now"
                                                                                                    : "The PC's own OpenXR runtime is used now");
    if (choice.vdxr) {
        launchreport::Vdxr();
        launchreport::Line("VDXR: the game runs on Virtual Desktop's own OpenXR runtime, not SteamVR (keep SteamVR closed)");
    }
    if (choice.set) {
        SetEnvironmentVariableW(L"XR_RUNTIME_JSON", choice.manifest.c_str());
        Log("OpenXR runtime choice: %s (%ls) for this game: %s (was %ls)", choice.vdxr ? "Virtual Desktop's VDXR" : "SteamVR's 32-bit one",
            choice.manifest.c_str(), choice.why, override.empty() ? (active.empty() ? L"none" : active.c_str()) : override.c_str());
    } else {
        Log("OpenXR runtime choice: %s (XR_RUNTIME_JSON %ls; the 32-bit active runtime %ls)", choice.why, override.empty() ? L"not set" : override.c_str(),
            active.empty() ? L"none" : active.c_str());
    }
}

// Brings up OpenXR on the game's device. If SteamVR isn't running yet, tries again every
// few seconds, so it can be started after the game.
bool TryInit(IDXGISwapChain* swap) {
    if (g_session)
        return true;
    static bool diagnosticsOn = false;
    if (!diagnosticsOn) {
        EnableLoaderDiagnostics();
        FixRuntimeOverride();
        PreferSteamVr();
        diagnosticsOn = true;
    }
    static ULONGLONG nextAttempt = 0;
    ULONGLONG now = GetTickCount64();
    if (now < nextAttempt)
        return false;
    nextAttempt = now + 5000;

    ID3D11Device* device = nullptr;
    if (FAILED(swap->GetDevice(IID_PPV_ARGS(&device)))) {
        Log("Swap chain is not D3D11; VR inactive");
        g_failed = true;
        launchreport::Problem(launchreport::kSession, "the game's picture isn't Direct3D 11, so VR can't start");
        return false;
    }
    bool ok = Init(device);
    device->Release();
    if (ok) {
        Log("OpenXR session created");
        launchreport::Ok(launchreport::kSession, "the VR session is made");
        input::Init(g_instance, g_session);  // the mod still works without controllers
        return true;
    }
    if (g_failed)
        launchreport::Problem(launchreport::kSession, "VR can't start in this game (the lines above say why)");
    else
        launchreport::Note(launchreport::kSession, "the VR session couldn't be made yet (the OpenXR lines above say why)");

    // Destroying the instance destroys everything created from it.
    if (g_instance)
        xrDestroyInstance(g_instance);
    g_instance = XR_NULL_HANDLE;
    g_system = XR_NULL_SYSTEM_ID;
    g_session = XR_NULL_HANDLE;
    g_localSpace = g_space = g_viewSpace = XR_NULL_HANDLE;
    if (!g_failed)
        Log("OpenXR not available yet (is SteamVR running and the headset on?); retrying in 5 s");
    return false;
}

// Pick a runtime swapchain format that CopyResource accepts from the game's back buffer.
// The sRGB variant is preferred: the game writes gamma-encoded values, so the
// compositor should read them as sRGB.
DXGI_FORMAT PickSwapchainFormat(DXGI_FORMAT source) {
    DXGI_FORMAT wanted[2] = {};
    switch (source) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        wanted[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        wanted[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        wanted[0] = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        wanted[1] = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }

    uint32_t count = 0;
    xrEnumerateSwapchainFormats(g_session, 0, &count, nullptr);
    std::vector<int64_t> formats(count);
    xrEnumerateSwapchainFormats(g_session, count, &count, formats.data());
    for (DXGI_FORMAT f : wanted)
        if (std::find(formats.begin(), formats.end(), static_cast<int64_t>(f)) != formats.end())
            return f;
    return DXGI_FORMAT_UNKNOWN;
}

void WaitHandover();

void DestroySwapchains() {
    WaitHandover();  // a frame still being handed over uses them
    for (Swapchain& sc : g_swapchains) {
        if (sc.handle)
            xrDestroySwapchain(sc.handle);
        sc = {};
    }
    if (g_fillChain.handle)
        xrDestroySwapchain(g_fillChain.handle);
    g_fillChain = {};
    g_fillActive = false;
    edgefill::Reset();
    g_scWidth = g_scHeight = 0;
    g_scSourceFormat = DXGI_FORMAT_UNKNOWN;
    depth3d::Reset();
    ResetEyes();
    if (g_mirror)
        g_mirror->Release();
    g_mirror = nullptr;
    g_mirrorValid = false;
    for (ID3D11Texture2D*& eye : g_previewEyes) {
        if (eye)
            eye->Release();
        eye = nullptr;
    }
}

void UpdateMirror(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, int eye) {
    if (!config::Get().mirrorOneEye)
        return;
    if (!g_mirror) {
        D3D11_TEXTURE2D_DESC copy = desc;
        copy.Usage = D3D11_USAGE_DEFAULT;
        copy.BindFlags = 0;
        copy.CPUAccessFlags = 0;
        copy.MiscFlags = 0;
        if (FAILED(g_device->CreateTexture2D(&copy, nullptr, &g_mirror))) {
            g_mirror = nullptr;
            return;
        }
    }
    ID3D11DeviceContext* context = nullptr;
    g_device->GetImmediateContext(&context);
    if (eye == kLeftEye) {
        context->CopyResource(g_mirror, back);
        g_mirrorValid = true;
    } else if (g_mirrorValid) {
        context->CopyResource(back, g_mirror);
    }
    context->Release();
}

bool CreateSwapchain(UINT width, UINT height, Swapchain& sc) {
    XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = g_scFormat;
    ci.sampleCount = 1;
    ci.width = width;
    ci.height = height;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (!Check(xrCreateSwapchain(g_session, &ci, &sc.handle), "xrCreateSwapchain"))
        return false;

    uint32_t count = 0;
    xrEnumerateSwapchainImages(sc.handle, 0, &count, nullptr);
    sc.images.assign(count, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    return Check(xrEnumerateSwapchainImages(sc.handle, count, &count,
                                            reinterpret_cast<XrSwapchainImageBaseHeader*>(sc.images.data())),
                 "xrEnumerateSwapchainImages");
}

// One swapchain per eye plus one for the theater screen, all the size of the game's back buffer.
bool CreateSwapchains(const D3D11_TEXTURE2D_DESC& back) {
    g_scFormat = PickSwapchainFormat(back.Format);
    if (g_scFormat == DXGI_FORMAT_UNKNOWN) {
        Log("No compatible swapchain format for back buffer format %d", back.Format);
        return false;
    }
    for (Swapchain& sc : g_swapchains)
        if (!CreateSwapchain(back.Width, back.Height, sc))
            return false;

    g_scWidth = back.Width;
    g_scHeight = back.Height;
    g_scSourceFormat = back.Format;
    depth3d::SetPictureSize(back.Width, back.Height);
    Log("Swapchains %ux%u format %d (back buffer format %d, %u samples)", back.Width, back.Height, g_scFormat,
        back.Format, back.SampleDesc.Count);
    return true;
}

void HandleSessionState(XrSessionState state) {
    Log("Session state -> %d", state);
    g_sessionState = state;
    if (state == XR_SESSION_STATE_FOCUSED)
        launchreport::Ok(launchreport::kFocused, "the headset shows the game and gives it the controllers (focused)");
    switch (state) {
    case XR_SESSION_STATE_READY: {
        XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};
        bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        g_running = Check(xrBeginSession(g_session, &bi), "xrBeginSession");
        if (g_running)
            launchreport::Ok(launchreport::kRunning, "the VR runtime started the session: it's ready for pictures");
        else
            launchreport::Problem(launchreport::kRunning, "the VR runtime wouldn't start the session (xrBeginSession failed)");
        break;
    }
    case XR_SESSION_STATE_STOPPING:
        WaitHandover();
        xrEndSession(g_session);
        g_running = false;
        break;
    case XR_SESSION_STATE_EXITING:
    case XR_SESSION_STATE_LOSS_PENDING:
        // Headset went away (SteamVR closed, etc). Stay out of the way for the rest of this run.
        g_running = false;
        g_failed = true;
        break;
    default:
        break;
    }
}

// The runtime's own recenter (spacechange.h): the mod's recenters too once it has taken (RenderFrame), said in the log.
spacechange::State g_spaceChange;

void PollEvents() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(g_instance, &event) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            HandleSessionState(reinterpret_cast<XrEventDataSessionStateChanged*>(&event)->state);
        } else if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            g_running = false, g_failed = true;
        } else if (event.type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) {
            const auto* change = reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(&event);
            if (change->session == g_session) {
                const bool flooded = g_spaceChange.flooded;
                const bool ours = spacechange::Note(g_spaceChange, static_cast<int32_t>(change->referenceSpaceType), change->changeTime,
                                                    static_cast<int64_t>(GetTickCount64()));
                if (ours)
                    Log("Recenter: the VR runtime recentered its own space (its recenter: a long press of the headset's menu button, SteamVR's "
                        "Reset seated position or the like; %d so far): the view recenters with it once that has taken",
                        g_spaceChange.count);
                else if (g_spaceChange.flooded && !flooded)
                    Log("Recenter: the VR runtime moved its own space more than %d times in %d s: not followed any more this run (the right stick's "
                        "held click and F9 still recenter)",
                        spacechange::kFloodCount, static_cast<int>(spacechange::kFloodMs / 1000));
                else if (!g_spaceChange.flooded && g_spaceChange.ignored <= 3)
                    Log("Recenter: the VR runtime moved a space the mod doesn't use (type %d: the play area?): nothing to do",
                        static_cast<int>(change->referenceSpaceType));
            }
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

bool KeyPressed(int vk) {
    static bool wasDown[256] = {};
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool pressed = down && !wasDown[vk];
    wasDown[vk] = down;
    return pressed;
}

bool GameInFront() {
    DWORD process = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &process);
    return process == GetCurrentProcessId();
}

// Whether a pressed key of the mod's counts. GetAsyncKeyState sees every key on the PC, whatever window has it, so on 2026-11-18 a
// keyboard knocked off the desk (the headset off, the game in front) ran the camera finder (F10: 28 s of beeps) and saved another
// pose lag (F11, five times); F12 is Steam's screenshot key as well, and it started the skeleton finder's scan. Now the player's keys
// (the README's: F8, F9, End, Home, Insert) count only with the game's window in front, as the game's own keys do, and the developer's
// also only with bf3vr.ini [general] dev_keys=1 (config::DevKeys, read at the press: no restart; off by default, on the test PC too).
// A key left alone is said once a launch. F13-F19 (keys no real keyboard has, sent by the test scripts) are not asked here.
enum class KeyKind { Player, Developer };
bool KeyCounts(int vk, const char* name, KeyKind kind) {
    static bool said[256] = {};
    const char* why = nullptr;
    if (!GameInFront())
        why = "another window is in front (the mod's keys count with the game's window in front)";
    else if (kind == KeyKind::Developer && !config::DevKeys())
        why = "a developer's key, off (bf3vr.ini [general] dev_keys=1 turns them on)";
    if (!why)
        return true;
    if (!said[vk & 0xFF]) {
        said[vk & 0xFF] = true;
        Log("Keys: %s left alone: %s", name, why);
    }
    return false;
}

void PollHotkeys() {
    // Into the headset's view by itself the moment the player's soldier exists (once a launch; F8 still
    // toggles it): pressed after the menu by a script, the game's own hands showed some 5 s more after
    // every load (the user, 2026-10-31: "it's not right away"). Not in the menu: there the doubled window
    // threw the menu's clicks off.
    static bool s_autoStereo = false;
    if (!s_autoStereo && god::PlayerClientSoldier()) {
        s_autoStereo = true;
        config::Settings& s = config::Mutable();
        if (s.mode != config::Mode::Stereo) {
            s.mode = config::Mode::Stereo;
            ResetEyes();
            Log("Mode -> stereo (the player's soldier is in: the headset's view at once)");
        }
    }
    const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    if (KeyPressed(VK_F8) && KeyCounts(VK_F8, "F8", KeyKind::Player)) {
        config::Settings& s = config::Mutable();
        s.mode = s.mode == config::Mode::Stereo ? config::Mode::Theater : config::Mode::Stereo;
        ResetEyes();
        Log("Mode -> %s", s.mode == config::Mode::Stereo ? "stereo" : "theater");
    }
    // F9 or the right stick's click held (input.cpp): recenter. The stick's is taken every frame, F9 or not.
    const bool f9 = KeyPressed(VK_F9) && KeyCounts(VK_F9, "F9", KeyKind::Player);
    if (input::TakeRecenter() || f9)
        g_recenterPending = true;
    if (KeyPressed(VK_F10) && KeyCounts(VK_F10, "F10", KeyKind::Developer))
        finder::Start();
    if (KeyPressed(VK_F7) && KeyCounts(VK_F7, "F7", KeyKind::Developer)) {
        g_swapEyes = !g_swapEyes;
        Log("Eyes %s", g_swapEyes ? "swapped" : "normal");
    }
    // F11: how many frames old the head pose each eye image is submitted with (0-3). The view
    // turn no longer needs its own lag (camera.cpp reads it from each camera), so F11 moved here.
    if (KeyPressed(VK_F11) && KeyCounts(VK_F11, "F11", KeyKind::Developer)) {
        config::Settings& s = config::Mutable();
        s.poseLag = (s.poseLag + 1) % kPoseHistory;
        config::SavePoseLag();
        Log("Pose lag %d frame%s", s.poseLag, s.poseLag == 1 ? "" : "s");
    }
    // F6: hunt for the cull camera (after F10); F5: save the copy being tried.
    if (KeyPressed(VK_F6) && KeyCounts(VK_F6, "F6", KeyKind::Developer))
        camera::ToggleHunt();
    if (KeyPressed(VK_F5) && KeyCounts(VK_F5, "F5", KeyKind::Developer))
        finder::SaveCull();
    camera::UpdateHunt();
    // F1: swing the gun model the other way, if it moves away from the hand. Saved to bf3vr.ini.
    if (KeyPressed(VK_F1) && KeyCounts(VK_F1, "F1", KeyKind::Developer)) {
        config::Mutable().gunFlip = !config::Get().gunFlip;
        config::SaveGunFlip();
        Log("Gun direction %s", config::Get().gunFlip ? "flipped" : "normal");
    }
    camera::SetGunFlip(config::Get().gunFlip);
    // Delete: make the game render a wider field of view, or its own again. Saved to bf3vr.ini.
    if (KeyPressed(VK_DELETE) && KeyCounts(VK_DELETE, "Delete", KeyKind::Developer)) {
        config::Settings& s = config::Mutable();
        s.renderFovDeg = s.renderFovDeg > 0.0f ? 0.0f : s.renderFovLastDeg;
        config::SaveRenderFov();
        Log("Render FOV %s", s.renderFovDeg > 0.0f ? "on" : "off (the game's own)");
    }
    // Page Up / Page Down: with the wider view on, change it (the headset image follows, so the
    // world stays life-size). Otherwise tune the FOV the eye images are shown with until the
    // world looks life-size: showing an image over a wider angle than BF3 drew it stretches it,
    // so a smaller value = less zoomed. Saved to bf3vr.ini.
    bool moreZoom = KeyPressed(VK_PRIOR) && KeyCounts(VK_PRIOR, "Page Up", KeyKind::Developer);
    bool lessZoom = KeyPressed(VK_NEXT) && KeyCounts(VK_NEXT, "Page Down", KeyKind::Developer);
    if (moreZoom || lessZoom) {
        config::Settings& s = config::Mutable();
        float step = lessZoom ? -5.0f : 5.0f;
        if (s.renderFovDeg > 0.0f) {
            s.renderFovDeg = s.renderFovLastDeg = std::clamp(s.renderFovDeg + step, 40.0f, 140.0f);
            config::SaveRenderFov();
            Log("Render FOV %.0f deg", s.renderFovDeg);
        } else {
            s.gameVerticalFovDeg = std::clamp(s.gameVerticalFovDeg + step, 20.0f, 150.0f);
            config::SaveFov();
            Log("Display FOV %.0f deg", s.gameVerticalFovDeg);
        }
    }
    float wideFov = config::Get().renderFovDeg * kPi / 180.0f;
    // weapon.txt "renderfov" (a test): another angle while its line is there, only on top of the wider view (with that off the game draws its own).
    if (const float test = camera::TestFov(); test > 0.0f && wideFov > 0.0f)
        wideFov = test * kPi / 180.0f;
    camera::SetRenderFov(wideFov);
    views::SetDetailBase(wideFov > 0.0f ? config::Get().detailFovDeg : 0.0f);
    scopeview::SetEyesShadowBase(config::Get().shadowMapSize);
    scopeview::SetEyesBlurBase(config::Get().motionBlurPasses);
    // The gun and arms are rescaled only while the world really is drawn wide; with the render
    // camera lost the game draws its own FOV, and the gun has to match it.
    bool wide = wideFov > 0.0f && std::fabs(DrawnFov() - wideFov) < 0.035f;
    skin::SetProjectionFix(wide ? camera::SourceFov() : 0.0f, wideFov);
    // End: god mode on/off.
    if (KeyPressed(VK_END) && KeyCounts(VK_END, "End", KeyKind::Player))
        god::Toggle();
    // Home: the stream view on or off (the window and OBS: the left eye alone, or both eyes side by side). Saved to
    // bf3vr.ini. Only with the game in front: Home is pressed in other programs' text too.
    if (KeyPressed(VK_HOME) && KeyCounts(VK_HOME, "Home", KeyKind::Player)) {
        config::Mutable().streamView = !streamview::On();
        streamview::SetOn(config::Get().streamView);
        config::SaveStreamView();
    }
    // F16 (sent by tools/drawhunt.ps1): the X button's magazine eject / insert. It inserts even with
    // pouch.txt "xinsert 0", so a scripted run can always put the magazine back.
    if (KeyPressed(VK_F16))
        input::MagazineTap(true);
    // F17 (tools/drawhunt.ps1): 3 s of turning as the stick does, to measure how even it is.
    if (KeyPressed(VK_F17))
        input::StartTurnTest();
    // F18 (tools/drawhunt.ps1): a census of every draw, to find the gun's effects (fire after 2 s).
    if (KeyPressed(VK_F18))
        effects::StartCensus();
    // F19: every draw with something placed at the game's own gun, logged once by its kind.
    if (KeyPressed(VK_F19))
        effects::StartFlashProbe();
    effects::Update();
    // Draw hunt, sent by tools/drawhunt.ps1 on keys no real keyboard has.
    if (KeyPressed(VK_F15))
        skin::HuntStart();
    if (KeyPressed(VK_F13))
        skin::HuntAnswer(true);
    if (KeyPressed(VK_F14))
        skin::HuntAnswer(false);
    // Ctrl + F3: the old look-angle finder, which also sets the yaw's direction (saved): not by accident.
    if (KeyPressed(VK_F3) && ctrl && KeyCounts(VK_F3, "Ctrl+F3", KeyKind::Developer))
        finder::StartAngles();
    // F12: the skeleton finder (F12 is Steam's screenshot key too).
    if (KeyPressed(VK_F12) && KeyCounts(VK_F12, "F12", KeyKind::Developer))
        finder::StartSkeleton();
    // Insert: free hands (the gun in the right hand, the left hand on its own) or the game's own.
    if (KeyPressed(VK_INSERT) && KeyCounts(VK_INSERT, "Insert", KeyKind::Player))
        hands::Toggle();
    // Pause: the skinning research hooks on/off (off by default: they cost frame rate).
    if (KeyPressed(VK_PAUSE) && KeyCounts(VK_PAUSE, "Pause", KeyKind::Developer))
        skin::ToggleResearch();
    // Ctrl + Scroll Lock: the next stereo test mode. Scroll Lock alone once switched the user's session
    // to depth 3D in the headset (halos round the arms, the arms big: 2026-10-31 08:02).
    if (KeyPressed(VK_SCROLL) && ctrl && KeyCounts(VK_SCROLL, "Ctrl+Scroll Lock", KeyKind::Developer)) {
        static const char* kNames[] = {"per-eye views (BF3 draws both eyes each frame)",
                                       "depth 3D (both eyes from each frame and its depth)",
                                       "hybrid (each frame from one eye, the other eye from its depth)",
                                       "alternate eyes", "mono (the same picture to both eyes)", "frozen"};
        g_stereoTest = (g_stereoTest + 1) % kStereoTests;
        Log("Stereo: %s", kNames[g_stereoTest]);
    }
    // The weapon camera sits at a fixed place in bf3.exe, unlike the render cameras, which move
    // in memory and can be stale for a moment.
    {
        uintptr_t address = 0;
        camera::Transform cam;
        if (camera::Resolve(config::GetAim().gameCamera, address) && camera::SafeRead(address, &cam, sizeof(cam)))
        {
            const float forward[3] = {-cam.forward[0], -cam.forward[1], -cam.forward[2]};  // true view direction
            skin::SetCamera(cam.trans, cam.right, cam.up, forward);
        }
    }
    // Ctrl + F4 / Ctrl + F2: the look yaw's / pitch's direction flipped (saved to bf3vr.ini). F4 alone
    // once flipped it in the user's session unnoticed: the game aimed opposite the gun and the view
    // swung twice the gun's turn ("moving the right controller moves my camera", 2026-10-31 08:15).
    if (KeyPressed(VK_F4) && ctrl && KeyCounts(VK_F4, "Ctrl+F4", KeyKind::Developer))
        aim::FlipYaw();
    if (KeyPressed(VK_F2) && ctrl && KeyCounts(VK_F2, "Ctrl+F2", KeyKind::Developer))
        aim::FlipPitch();
    // Stereo mode sets the offset and view turn per frame; in theater it's the shift test or nothing.
    if (config::Get().mode == config::Mode::Theater) {
        camera::SetEyeOffset(0.0f);
        camera::SetViewTurn(nullptr);
    }
}

// Moves the layer space so its origin is under the headset, facing where the player looks (yaw only).
void Recenter(XrTime time) {
    XrSpaceLocation head{XR_TYPE_SPACE_LOCATION};
    if (XR_FAILED(xrLocateSpace(g_viewSpace, g_localSpace, time, &head)) ||
        !(head.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) ||
        !(head.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
        return;

    const XrQuaternionf& q = head.pose.orientation;
    float yaw = std::atan2(2.0f * (q.w * q.y + q.x * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
    XrPosef origin{{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)}, head.pose.position};

    XrSpace recentered = XR_NULL_HANDLE;
    if (!CreateReferenceSpace(XR_REFERENCE_SPACE_TYPE_LOCAL, origin, recentered))
        return;
    xrDestroySpace(g_space);
    g_space = recentered;
    ResetEyes();
    headmove::NoteRecentered();
    g_recenterPending = false;
    Log("Recentered (yaw %.0f deg)", yaw * 180.0f / kPi);
}

// Copies the game's back buffer into the next image of a swapchain.
// One eye every other frame (weapon.txt "halfeye 1": a test for now, of whether that eye at half rate is bearable: the scope's picture would take its frames): on the frames between, that eye's half of the picture is the one kept from the frame before, and it keeps that frame's pose, so the compositor turns it to where the head is.
std::atomic<bool> g_halfEye{false};
// weapon.txt "forceheadless 1" (a test): frames worked headless with the stand-in on, even while the headset asks for them (SetForceHeadless).
std::atomic<bool> g_forceHeadless{false};
ID3D11Texture2D* g_heldHalf = nullptr;
bool g_heldHalfThere = false, g_heldTurn = false;

// True: this frame's left half is the one before, put back.
bool HoldLeftEye(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc) {
    if (!g_halfEye.load(std::memory_order_relaxed) || desc.SampleDesc.Count > 1) {
        g_heldHalfThere = false;
        return false;
    }
    D3D11_TEXTURE2D_DESC half = desc;
    half.Width = desc.Width / 2;
    half.Usage = D3D11_USAGE_DEFAULT;
    half.BindFlags = 0;
    half.CPUAccessFlags = 0;
    half.MiscFlags = 0;
    half.MipLevels = 1;
    if (g_heldHalf) {
        D3D11_TEXTURE2D_DESC have{};
        g_heldHalf->GetDesc(&have);
        if (have.Width != half.Width || have.Height != half.Height || have.Format != half.Format) {
            g_heldHalf->Release();
            g_heldHalf = nullptr;
            g_heldHalfThere = false;
        }
    }
    if (!g_heldHalf && FAILED(g_device->CreateTexture2D(&half, nullptr, &g_heldHalf)))
        return false;
    g_heldTurn = !g_heldTurn;
    ID3D11DeviceContext* context = nullptr;
    g_device->GetImmediateContext(&context);
    const bool put = g_heldTurn && g_heldHalfThere;
    if (put) {
        context->CopySubresourceRegion(back, 0, 0, 0, 0, g_heldHalf, 0, nullptr);
    } else {
        const D3D11_BOX box{0, 0, 0, half.Width, half.Height, 1};
        context->CopySubresourceRegion(g_heldHalf, 0, 0, 0, 0, back, 0, &box);
        g_heldHalfThere = true;
    }
    context->Release();
    return put;
}

bool SubmitImage(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, Swapchain& sc) {
    uint32_t index = 0;
    XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wait.timeout = XR_INFINITE_DURATION;
    XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    if (!Check(xrAcquireSwapchainImage(sc.handle, &acquire, &index), "xrAcquireSwapchainImage"))
        return false;
    if (!Check(xrWaitSwapchainImage(sc.handle, &wait), "xrWaitSwapchainImage")) {
        xrReleaseSwapchainImage(sc.handle, &release);
        return false;
    }

    ID3D11DeviceContext* context = nullptr;
    g_device->GetImmediateContext(&context);
    if (desc.SampleDesc.Count > 1)
        context->ResolveSubresource(sc.images[index].texture, 0, back, 0, g_scFormat);
    else
        context->CopyResource(sc.images[index].texture, back);
    context->Release();

    return Check(xrReleaseSwapchainImage(sc.handle, &release), "xrReleaseSwapchainImage");
}

XrFovf GameFov();

// Depth 3D: both eyes from this frame's picture and BF3's depth (depth3d.cpp). Until a depth
// buffer has been found, the picture goes to both eyes as it is (flat).
// bias: as depth3d::DepthModel's (outside -1..1, depth3d.txt decides).
bool SubmitDepth3D(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, float ipd, float bias = 2.0f) {
    uint32_t index[2] = {};
    XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wait.timeout = XR_INFINITE_DURATION;
    XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    int acquired = 0;
    for (; acquired < 2; ++acquired) {
        Swapchain& sc = g_swapchains[acquired];
        if (!Check(xrAcquireSwapchainImage(sc.handle, &acquire, &index[acquired]), "xrAcquireSwapchainImage"))
            break;
        if (!Check(xrWaitSwapchainImage(sc.handle, &wait), "xrWaitSwapchainImage")) {
            xrReleaseSwapchainImage(sc.handle, &release);
            break;
        }
    }
    bool ok = acquired == 2;
    if (ok) {
        // A point at distance d moves sideways by (eye offset) * scale / d in clip space, half
        // that in picture widths; the left eye sees it moved right. scale = 1 / tan(half width).
        float scale = 1.0f / std::tan(GameFov().angleRight);
        float shift = 0.25f * ipd * scale * (g_swapEyes ? -1.0f : 1.0f);
        depth3d::Eye eyes[2] = {{g_swapchains[kLeftEye].images[index[kLeftEye]].texture, shift},
                                {g_swapchains[kRightEye].images[index[kRightEye]].texture, -shift}};
        depth3d::DepthModel model;
        camera::ProjectionDepth(model.nearPlane, model.depthScale);
        model.bias = bias;
        ID3D11DeviceContext* context = nullptr;
        g_device->GetImmediateContext(&context);
        if (depth3d::Render(g_device, back, g_scFormat, eyes, model)) {
            // The desktop window shows the left eye as the headset gets it.
            if (config::Get().mirrorOneEye && desc.SampleDesc.Count == 1)
                context->CopyResource(back, eyes[depth3d::MirrorEye()].target);
        } else {
            for (const depth3d::Eye& e : eyes) {
                if (desc.SampleDesc.Count > 1)
                    context->ResolveSubresource(e.target, 0, back, 0, g_scFormat);
                else
                    context->CopyResource(e.target, back);
            }
        }
        context->Release();
    }
    for (int e = 0; e < acquired; ++e)
        Check(xrReleaseSwapchainImage(g_swapchains[e].handle, &release), "xrReleaseSwapchainImage");
    return ok;
}

// With the headset idle SteamVR asks for no frames, so nothing is drawn for it. The depth-3D eyes
// are then still made, into textures of our own (g_previewEyes), and one is shown on the desktop:
// the 3D can be checked by screenshot with nobody wearing the headset.
void PreviewDepth3D(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc) {
    if (desc.SampleDesc.Count > 1)
        return;
    for (ID3D11Texture2D*& eye : g_previewEyes) {
        if (eye)
            continue;
        D3D11_TEXTURE2D_DESC td = desc;
        td.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        td.BindFlags = D3D11_BIND_RENDER_TARGET;
        td.MiscFlags = 0;
        td.CPUAccessFlags = 0;
        td.Usage = D3D11_USAGE_DEFAULT;
        if (desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM || FAILED(g_device->CreateTexture2D(&td, nullptr, &eye))) {
            eye = nullptr;
            return;
        }
    }
    float scale = 1.0f / std::tan(GameFov().angleRight);
    float shift = 0.25f * 0.064f * scale * (g_swapEyes ? -1.0f : 1.0f);  // a typical eye distance
    depth3d::Eye eyes[2] = {{g_previewEyes[kLeftEye], shift}, {g_previewEyes[kRightEye], -shift}};
    depth3d::DepthModel model;
    camera::ProjectionDepth(model.nearPlane, model.depthScale);
    if (!depth3d::Render(g_device, back, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, eyes, model))
        return;
    ID3D11DeviceContext* context = nullptr;
    g_device->GetImmediateContext(&context);
    context->CopyResource(back, g_previewEyes[depth3d::MirrorEye()]);
    context->Release();
}

XrSwapchainSubImage FullImage(const Swapchain& sc) {
    XrSwapchainSubImage sub{};
    sub.swapchain = sc.handle;
    sub.imageRect = {{0, 0}, {static_cast<int32_t>(g_scWidth), static_cast<int32_t>(g_scHeight)}};
    return sub;
}

// One eye's half of a side-by-side image.
XrSwapchainSubImage HalfImage(const Swapchain& sc, int eye) {
    XrSwapchainSubImage sub{};
    sub.swapchain = sc.handle;
    const int32_t half = static_cast<int32_t>(g_scWidth / 2);
    sub.imageRect = {{eye == kRightEye ? half : 0, 0}, {half, static_cast<int32_t>(g_scHeight)}};
    return sub;
}

// The game renders a symmetric frustum from its own FOV setting, not the headset's.
// Telling the compositor exactly what was rendered lets it place the image correctly.
// With the wider view on, that's the angle the game really drew the last frame with: when the
// render camera was lost (2026-10-30, after the opening ride) the game drew its own 55 degrees,
// and showing that over the wide angle magnified everything 1.8x (blurry, gun and arms big).
// sideBySide: the picture holds both eyes side by side, each half of it.
XrFovf GameFov(bool sideBySide) {
    const config::Settings& s = config::Get();
    float fullV = s.gameVerticalFovDeg * kPi / 180.0f;
    if (s.renderFovDeg > 0.0f) {
        float rendered = DrawnFov();
        fullV = rendered > 0.0f ? rendered : s.renderFovDeg * kPi / 180.0f;
    }
    float halfV = fullV * 0.5f;
    float width = sideBySide ? g_scWidth / 2.0f : static_cast<float>(g_scWidth);  // one eye's part
    float halfH = std::atan(std::tan(halfV) * width / g_scHeight);
    return {-halfH, halfH, halfV, -halfV};
}
XrFovf GameFov() {
    return GameFov(g_sideBySide);
}

// One eye's field as the layer is told it, given the centred one (GameFov's). A picture drawn centred has that field, the same for both eyes. Drawn off-centre (weapon.txt "offcentre":
// views.cpp moves each eye's window in its own plane by a share of its width and height, the left eye's to the left, the right eye's to the right), each eye has four angles of its
// own, made here from the very numbers the game's projection was made from: told the centred field, the headset would show the whole world that share of the picture away from where
// it is.
XrFovf EyeFov(int eye, const XrFovf& centred) {
    float right = 0.0f, up = 0.0f;
    if (!g_sideBySide || !views::OffCentre(eye, right, up))
        return centred;
    const float w = std::tan(centred.angleRight), h = std::tan(centred.angleUp);
    return {std::atan(w * (2.0f * right - 1.0f)), std::atan(w * (2.0f * right + 1.0f)), std::atan(h * (2.0f * up + 1.0f)), std::atan(h * (2.0f * up - 1.0f))};
}

struct FrameLayers {
    XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    XrCompositionLayerProjection projection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    XrCompositionLayerProjectionView views[2] = {{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},
                                                 {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};
    const XrCompositionLayerBaseHeader* list[1] = {};
    uint32_t count = 0;
};

void BuildTheater(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, FrameLayers& out) {
    Swapchain& sc = g_swapchains[kTheater];
    if (!SubmitImage(back, desc, sc))
        return;
    const config::Settings& s = config::Get();
    out.quad.space = g_space;
    out.quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    out.quad.subImage = FullImage(sc);
    out.quad.pose = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -s.screenDistance}};
    out.quad.size = {s.screenWidth, s.screenWidth * g_scHeight / g_scWidth};
    out.list[0] = reinterpret_cast<XrCompositionLayerBaseHeader*>(&out.quad);
    out.count = 1;
}

// The edge carried this much further than the headset's own field (degrees): what's left of the picture's lag on the head still turns it away from the view.
constexpr float kFillSpare = 6.0f;

// Where the picture goes in its eye's part of the bigger image: as far from each side as the headset's field and the spare reach past the picture's own edge, the picture's sharpness kept.
edgefill::Layout FillLayout(const D3D11_TEXTURE2D_DESC& desc, const XrFovf* field) {
    edgefill::Layout l;
    l.pictureWidth = static_cast<int>(desc.Width / 2);
    l.pictureHeight = static_cast<int>(desc.Height);
    const XrFovf drawn = GameFov(true);
    const float perX = l.pictureWidth * 0.5f / std::tan(drawn.angleRight), perY = l.pictureHeight * 0.5f / std::tan(drawn.angleUp);
    auto past = [](float angle, float per, int half) {
        const float reach = std::min(std::fabs(angle) + kFillSpare * kPi / 180.0f, 75.0f * kPi / 180.0f);
        return std::max(0, static_cast<int>(std::tan(reach) * per) - half) + 8;
    };
    int width = 0, height = 0;
    for (int e = 0; e < 2; ++e) {
        l.pictureX[e] = past(field[e].angleLeft, perX, l.pictureWidth / 2);
        l.pictureY[e] = past(field[e].angleUp, perY, l.pictureHeight / 2);
        width = std::max(width, l.pictureX[e] + past(field[e].angleRight, perX, l.pictureWidth / 2));
        height = std::max(height, l.pictureY[e] + past(field[e].angleDown, perY, l.pictureHeight / 2));
    }
    l.eyeWidth = (l.pictureWidth + width + 1) & ~1;
    l.eyeHeight = (l.pictureHeight + height + 1) & ~1;
    return l;
}

// That image's swapchain, once.
bool PrepareFill(const D3D11_TEXTURE2D_DESC& desc, const XrView* views) {
    if (g_fillBroken)
        return false;
    if (g_fillChain.handle)
        return true;
    const XrFovf field[2] = {views[kLeftEye].fov, views[kRightEye].fov};
    const edgefill::Layout l = FillLayout(desc, field);
    if (!CreateSwapchain(static_cast<UINT>(l.eyeWidth * 2), static_cast<UINT>(l.eyeHeight), g_fillChain)) {
        if (g_fillChain.handle)
            xrDestroySwapchain(g_fillChain.handle);
        g_fillChain = {};
        g_fillBroken = true;
        Log("Edge fill: no swapchain of %dx%d: black round the picture", l.eyeWidth * 2, l.eyeHeight);
        return false;
    }
    g_fillLayout = l;
    Log("Edge fill: %dx%d for both eyes, the picture %dx%d in each at (%d, %d) and (%d, %d)", l.eyeWidth * 2, l.eyeHeight, l.pictureWidth, l.pictureHeight, l.pictureX[0], l.pictureY[0], l.pictureX[1], l.pictureY[1]);
    return true;
}

// The picture and its filled edge into the next image.
bool SubmitFilled(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc) {
    uint32_t index = 0;
    XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wait.timeout = XR_INFINITE_DURATION;
    XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    if (!Check(xrAcquireSwapchainImage(g_fillChain.handle, &acquire, &index), "xrAcquireSwapchainImage"))
        return false;
    if (!Check(xrWaitSwapchainImage(g_fillChain.handle, &wait), "xrWaitSwapchainImage")) {
        xrReleaseSwapchainImage(g_fillChain.handle, &release);
        return false;
    }
    if (!edgefill::Draw(g_device, back, desc, g_fillChain.images[index].texture, g_fillLayout))
        g_fillBroken = true;  // (the image goes out as it is this once)
    return Check(xrReleaseSwapchainImage(g_fillChain.handle, &release), "xrReleaseSwapchainImage");
}

// One eye's part of that image, as angles from the head: the picture's own scale carried out to its sides.
XrFovf FilledFov(int eye) {
    const edgefill::Layout& l = g_fillLayout;
    const XrFovf drawn = GameFov(true);
    const float perX = l.pictureWidth * 0.5f / std::tan(drawn.angleRight), perY = l.pictureHeight * 0.5f / std::tan(drawn.angleUp);
    const float cx = l.pictureX[eye] + l.pictureWidth * 0.5f, cy = l.pictureY[eye] + l.pictureHeight * 0.5f;
    return {std::atan(-cx / perX), std::atan((l.eyeWidth - cx) / perX), std::atan(cy / perY), std::atan(-(l.eyeHeight - cy) / perY)};
}

// weapon.txt "fillshot n", the headset idle: the filled image made into a texture of our own, and written to eye_fill.bmp, to look at. With the field the headset reports (or, never having been
// told, this headset's).
void FilmFill(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, const XrView* views) {
    constexpr float kDeg = kPi / 180.0f;
    XrFovf field[2] = {{-59.3f * kDeg, 50.1f * kDeg, 49.1f * kDeg, -60.4f * kDeg}, {-50.4f * kDeg, 59.1f * kDeg, 49.9f * kDeg, -59.6f * kDeg}};
    if (std::fabs(views[kLeftEye].fov.angleLeft) > 52.0f * kDeg) {
        field[0] = views[kLeftEye].fov;
        field[1] = views[kRightEye].fov;
    }
    const edgefill::Layout l = FillLayout(desc, field);
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(l.eyeWidth * 2);
    td.Height = static_cast<UINT>(l.eyeHeight);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = desc.Format;
    td.SampleDesc = {1, 0};
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* target = nullptr;
    if (FAILED(g_device->CreateTexture2D(&td, nullptr, &target)))
        return;
    const bool drawn = edgefill::Draw(g_device, back, desc, target, l);
    const bool written = drawn && depth3d::SaveNow(g_device, target, L"eye_fill.bmp");
    Log("Edge fill: filmed %ux%u: %s", td.Width, td.Height, written ? "eye_fill.bmp" : "failed");
    target->Release();
    edgefill::Reset();  // (holds the target otherwise)
}

// The projection layer: each eye's latest image with the pose it was rendered for, and the field it was drawn with: one centred field for both, or each eye's own (EyeFov).
void BuildProjection(FrameLayers& out) {
    XrFovf fov = GameFov();
    const XrFovf eyeFov[2] = {EyeFov(kLeftEye, fov), EyeFov(kRightEye, fov)};
    // Said whenever the eyes are told fields of their own, those change, or they go back to the one centred field: the four half-angles of each eye as the headset is told them.
    {
        static XrFovf s_told[2] = {};
        static bool s_own = false;
        const auto same = [](const XrFovf& a, const XrFovf& b) { return a.angleLeft == b.angleLeft && a.angleRight == b.angleRight && a.angleUp == b.angleUp && a.angleDown == b.angleDown; };
        const bool own = !same(eyeFov[0], fov) || !same(eyeFov[1], fov);
        if (own != s_own || (own && !(same(eyeFov[0], s_told[0]) && same(eyeFov[1], s_told[1])))) {
            constexpr float kToDegrees = 180.0f / kPi;
            if (own)
                Log("Headset: each eye's picture handed over with its own field (drawn off-centre): left eye %.2f/%.2f/%.2f/%.2f, right eye %.2f/%.2f/%.2f/%.2f (left/right/up/down, degrees)",
                    eyeFov[0].angleLeft * kToDegrees, eyeFov[0].angleRight * kToDegrees, eyeFov[0].angleUp * kToDegrees, eyeFov[0].angleDown * kToDegrees, eyeFov[1].angleLeft * kToDegrees, eyeFov[1].angleRight * kToDegrees,
                    eyeFov[1].angleUp * kToDegrees, eyeFov[1].angleDown * kToDegrees);
            else
                Log("Headset: both eyes' pictures handed over with the one centred field again: %.2f/%.2f/%.2f/%.2f (left/right/up/down, degrees)", fov.angleLeft * kToDegrees,
                    fov.angleRight * kToDegrees, fov.angleUp * kToDegrees, fov.angleDown * kToDegrees);
            s_own = own;
            s_told[0] = eyeFov[0], s_told[1] = eyeFov[1];
        }
    }
    for (int i = 0; i < 2; ++i) {
        out.views[i].pose = g_eyePose[i];
        out.views[i].fov = eyeFov[i];
        out.views[i].subImage = g_sideBySide ? HalfImage(g_swapchains[kLeftEye], i) : FullImage(g_swapchains[i]);
        if (g_fillActive) {
            out.views[i].fov = FilledFov(i);
            out.views[i].subImage = {};
            out.views[i].subImage.swapchain = g_fillChain.handle;
            out.views[i].subImage.imageRect = {{i * g_fillLayout.eyeWidth, 0}, {g_fillLayout.eyeWidth, g_fillLayout.eyeHeight}};
        }
    }
    out.projection.space = g_space;
    out.projection.viewCount = 2;
    out.projection.views = out.views;
    out.list[0] = reinterpret_cast<XrCompositionLayerBaseHeader*>(&out.projection);
    out.count = 1;
}

// The eyes as last located, for the headless frames below.
XrView g_lastViews[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
bool g_haveLastViews = false;

// ---- The head replay (build\headtest.txt; headtest.h) ----
// With nobody in the headset, a made-up head in place of the located one (or of the headless frames' eyes), just before the filter, so everything after it is the code as played: the filter and
// its lead, the poses published, the look angles, the game's cameras, the rows turned at draw, the pose by draw, the layer. The file is looked at every 300 ms and read when it changes; with no
// file, or "off" in it, nothing here runs but that look and a flag tested. It never acts while the session is focused (somebody is wearing the headset): said once, and nothing replayed.
struct HeadReplay {
    headtest::Config config, next;  // the motion asked for or running; one that waits for the running one to ease out
    bool wanted = false, haveNext = false;
    bool running = false;    // its base taken: the head is being replaced
    XrTime start = 0;        // the display time it began at
    headtest::Pose base;     // the head as it was then: its turn (the left eye's, as the filter takes it) and the middle between the eyes
    double eyeAt[2][3] = {};  // each eye from that middle, in the head's own frame: their own spacing kept
    headtest::Turn eyeTurn[2];  // and each eye's turn from the head's
    uint32_t frame = 0;
    bool thisFrame = false;  // this frame's head is the made-up one: its line is owed
    double ms = 0.0, msWas = 0.0;  // this frame's display time from the start, and the last frame's
    headtest::Pose truth, truthWas, located;
    double locatedAt = 0.0;      // the time this frame's head is located for, seconds from the start, and which frame it is: the made-up head's turn rate is asked for the same
    uint32_t locatedFrame = 0;
    bool haveWas = false;
    headtest::Second second;
    ULONGLONG secondAt = 0;
    ULONGLONG lastAt = 0;  // (GetTickCount64) when the head was last replaced: "Head rough" is the made-up head's meanwhile
} g_replay;
XrTime g_frameShownAt = 0;  // this frame's display time as the runtime gave it (the head is located for it, plus "poseahead")

// The head's own turn rate this frame as it is given (BuildStereo: the runtime's, for the headset itself in the room's space; under a head replay the made-up head's), for
// weapon.txt "poserate" (FilterViews): radians a second, a vector along the turn's axis. Not given: a headless frame's stood-in eyes have none, or the runtime had none.
struct HeadRate {
    bool given = false;
    XrVector3f rate{};
} g_headRate;
// What FilterViews made of this frame beside the picture's pose. The head as the aim's limit and the walk's direction take it while a pose candidate is on (g_facingSplit:
// "poseadapt" or "poserate"): the located turn through the fixed filter's two low-passes and no lead, so nothing a candidate does to the picture's pose shakes the game's look
// yaw or where the soldier walks. With neither on they take the picture's own pose, as before. And how the picture's pose was made this frame, with the size of the rate it was
// carried by (degrees a second), for the head replay's line.
XrQuaternionf g_facing{0.0f, 0.0f, 0.0f, 1.0f};
bool g_facingSplit = false;
headtest::By g_poseBy = headtest::kByFixed;
double g_poseRate = 0.0;
// The two meters that run whenever a picture is handed over, with somebody in the headset too (headtest.h; BuildStereo), a line a second each: the world's turn (the
// pose the headset is told against the cameras the picture's own uploads carried) and the edge (the picture's pose against the head as located for the frame it is
// shown in).
headtest::WorldSecond g_worldSecond;
headtest::EdgeSecond g_edgeSecond;
ULONGLONG g_metersLogged = 0;
// One picture's world turn, degrees: by the view build most of its main-view uploads carried, by its last before the first eye's picture went out, by its first
// (under 0: none to go by); whether they carried more than one build.
struct WorldTurn {
    double most = -1.0, last = -1.0, first = -1.0;
    bool mixed = false;
};

// The head of a pair of eyes: the left eye's turn, the middle between them.
headtest::Pose HeadOf(const XrPosef& left, const XrPosef& right) {
    headtest::Pose p;
    p.turn = {left.orientation.x, left.orientation.y, left.orientation.z, left.orientation.w};
    p.place[0] = (static_cast<double>(left.position.x) + right.position.x) / 2.0;
    p.place[1] = (static_cast<double>(left.position.y) + right.position.y) / 2.0;
    p.place[2] = (static_cast<double>(left.position.z) + right.position.z) / 2.0;
    return p;
}

// A file beside the DLL, or at a full path.
std::string BesideDll(const char* name) {
    if (name[0] == '\\' || name[0] == '/' || (name[0] && name[1] == ':'))
        return name;
    char path[MAX_PATH] = {};
    HMODULE self = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&HeadOf), &self);
    GetModuleFileNameA(self, path, MAX_PATH);
    std::string folder(path);
    folder.erase(folder.find_last_of("\\/") + 1);
    return folder + name;
}

// A file's text, whole. False: not there, or still being written (the open isn't shared).
bool ReadWhole(const std::string& path, std::string& out) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return false;
    out.clear();
    char block[4096];
    for (size_t n = 0; (n = fread(block, 1, sizeof(block), f)) > 0;)
        out.append(block, n);
    fclose(f);
    if (out.size() >= 3 && out.compare(0, 3, "\xEF\xBB\xBF") == 0)
        out.erase(0, 3);  // (a byte order mark)
    return true;
}

// The motion running eased back to the head as it was, from now: asked to stop, or another waits.
void EaseReplayOut() {
    const double now = g_replay.ms * 1e-3;
    if (now < g_replay.config.seconds)
        g_replay.config.seconds = now;
}

// headtest.txt looked at every 300 ms, read again whole when it changes (as recoilrun.cpp reads recoil.txt).
void PollHeadTest() {
    static FILETIME s_written{};
    static ULONGLONG s_looked = 0;
    static bool s_read = false, s_there = false;
    const ULONGLONG now = GetTickCount64();
    if (s_read && now - s_looked < 300)
        return;
    s_looked = now;
    static const std::string path = BesideDll("headtest.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool there = GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info) != 0;
    if (s_read && there == s_there && (!there || CompareFileTime(&info.ftLastWriteTime, &s_written) == 0))
        return;
    std::string text;
    if (there && !ReadWhole(path, text))
        return;  // still being written: read at the next look
    const bool first = !s_read;
    s_read = true, s_there = there, s_written = info.ftLastWriteTime;
    if (first) {
        // One that was there before the game is a run's leftover (its script killed before it could delete it): somebody may be about to put the headset on.
        if (there)
            Log("Head test: build\\headtest.txt was there before the game started (a run's leftover): not acted on");
        return;
    }
    headtest::Config c = headtest::Parse(text.c_str());
    if (c.kind == headtest::Kind::kTrack) {
        std::string rows;
        if (!ReadWhole(BesideDll(c.trackFile.c_str()), rows) || !headtest::ParseTrack(rows.c_str(), c)) {
            Log("Head test: no track of two rows or more in %s: nothing replayed", c.trackFile.c_str());
            c = headtest::Config{};
        } else if (!c.locatedFile.empty() && (!ReadWhole(BesideDll(c.locatedFile.c_str()), rows) || !headtest::ParseLocated(rows.c_str(), c))) {
            // (The located head's own rows beside the track: without them it is the true head, with "predict" and "noise" as the file says.)
            Log("Head test: no located rows of two or more in %s: the located head is made from the track itself", c.locatedFile.c_str());
        }
    }
    if (!c.Moves()) {
        g_replay.haveNext = false;
        if (g_replay.running) {
            EaseReplayOut();
            Log("Head test: off (headtest.txt %s): the head eases back to where it was", there ? "says so" : "deleted");
        } else if (g_replay.wanted) {
            g_replay.wanted = false;
            camera::SetDrawnDetail(false);
            Log("Head test: off before it began (headtest.txt %s)", there ? "says so" : "deleted");
        }
        return;
    }
    camera::SetDrawnDetail(true);  // (each view build keeps its camera for the lines from here on)
    if (g_replay.running) {
        // The one running eases out first, so the head never jumps; this one starts from the head as it is then.
        g_replay.next = c;
        g_replay.haveNext = true;
        EaseReplayOut();
        Log("Head test: another motion asked for: the one running eases out first");
    } else {
        g_replay.config = c;
        g_replay.wanted = true;
    }
}

// The made-up head into this frame's eyes, in place of what was located (or the headless frames' eyes), before the filter. locatedFor: the time the eyes are located for.
void ReplayHead(XrView* views, XrTime locatedFor, bool headless) {
    g_replay.thisFrame = false;
    // Somebody in the headset: never. Said once; the file has to change before anything is replayed again.
    if (g_sessionState == XR_SESSION_STATE_FOCUSED) {
        Log("Head test: the headset's session is focused (state 5: somebody is wearing it): %s",
            g_replay.running ? "the replay stopped at once, the head is the headset's own again" : "nothing is replayed");
        if (g_replay.running)
            g_poseFilterFresh = true;  // (the real head taken at once, not glided to)
        g_replay.wanted = g_replay.running = g_replay.haveNext = false;
        camera::SetDrawnDetail(false);
        return;
    }
    if (!g_replay.running) {
        // The head as it is now: where the motion starts from and goes back to. Each eye as it sits on it.
        g_replay.base = HeadOf(views[kLeftEye].pose, views[kRightEye].pose);
        const headtest::Turn inverse = headtest::Inverse(g_replay.base.turn);
        for (int e = 0; e < 2; ++e) {
            const XrPosef& p = views[e].pose;
            const double from[3] = {p.position.x - g_replay.base.place[0], p.position.y - g_replay.base.place[1], p.position.z - g_replay.base.place[2]};
            headtest::Rotate(inverse, from, g_replay.eyeAt[e]);
            g_replay.eyeTurn[e] = headtest::Mul(inverse, {p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w});
        }
        g_replay.running = true;
        g_replay.start = g_frameShownAt;
        g_replay.frame = 0;
        g_replay.ms = 0.0;
        g_replay.second.Clear();
        g_replay.secondAt = GetTickCount64();
        const headtest::Angles a = headtest::ToAngles(g_replay.base.turn);
        Log("Head test: started (%s): %s; from the head at yaw %.2f pitch %.2f roll %.2f, %.0f %.0f %.0f mm", headless ? "frames worked headless, nothing handed to the headset" : "frames handed to the headset",
            headtest::Describe(g_replay.config).c_str(), a.yaw, a.pitch, a.roll, g_replay.base.place[0] * 1000.0, g_replay.base.place[1] * 1000.0, g_replay.base.place[2] * 1000.0);
    }
    // Handed to the headset (not headless), the eyes given are the headset's own: a hand's width from where it lay as the replay began, somebody has picked it up. Stopped at once, as when
    // the session focuses, which only comes once it is on the head. (Headless, nothing is handed to the headset: whoever puts it on sees no picture of the game's.)
    if (!headless) {
        const headtest::Pose own = HeadOf(views[kLeftEye].pose, views[kRightEye].pose);
        const double moved = std::sqrt((own.place[0] - g_replay.base.place[0]) * (own.place[0] - g_replay.base.place[0]) + (own.place[1] - g_replay.base.place[1]) * (own.place[1] - g_replay.base.place[1]) +
                                       (own.place[2] - g_replay.base.place[2]) * (own.place[2] - g_replay.base.place[2]));
        if (moved > 0.05) {
            Log("Head test: the headset itself has moved %.0f cm since the replay began (picked up?): the replay stopped at once, the head is the headset's own again", moved * 100.0);
            g_poseFilterFresh = true;
            g_replay.wanted = g_replay.running = g_replay.haveNext = false;
            camera::SetDrawnDetail(false);
            return;
        }
    }
    const double shown = static_cast<double>(g_frameShownAt - g_replay.start) * 1e-9, located = static_cast<double>(locatedFor - g_replay.start) * 1e-9;
    if (headtest::Over(g_replay.config, shown)) {
        char line[512];
        if (g_replay.second.frames > 0) {
            g_replay.second.Line(line, sizeof(line));
            Log("%s", line);
        }
        Log("Head test: ended after %.1f s: the head is back where it was", g_replay.config.seconds);
        g_replay.running = false;
        g_replay.wanted = g_replay.haveNext;
        if (g_replay.haveNext) {
            g_replay.config = g_replay.next;  // (it starts at the next frame, from the head as it is then)
            g_replay.haveNext = false;
        } else {
            camera::SetDrawnDetail(false);
        }
        return;
    }
    g_replay.haveWas = g_replay.frame > 0;
    g_replay.msWas = g_replay.ms;
    g_replay.truthWas = g_replay.truth;
    g_replay.ms = shown * 1000.0;
    g_replay.truth = headtest::HeadAt(g_replay.config, g_replay.base, shown);
    g_replay.locatedAt = located;
    g_replay.locatedFrame = g_replay.frame;
    g_replay.located = headtest::LocatedAt(g_replay.config, g_replay.base, located, g_replay.frame++);
    for (int e = 0; e < 2; ++e) {
        double at[3];
        headtest::Rotate(g_replay.located.turn, g_replay.eyeAt[e], at);
        const headtest::Turn turn = headtest::Mul(g_replay.located.turn, g_replay.eyeTurn[e]);
        views[e].pose.orientation = {static_cast<float>(turn.x), static_cast<float>(turn.y), static_cast<float>(turn.z), static_cast<float>(turn.w)};
        views[e].pose.position = {static_cast<float>(g_replay.located.place[0] + at[0]), static_cast<float>(g_replay.located.place[1] + at[1]),
                                  static_cast<float>(g_replay.located.place[2] + at[2])};
    }
    g_replay.thisFrame = true;
    g_replay.lastAt = GetTickCount64();
}

// This frame's line and the second's sums (headtest.h). filtered: this frame's eyes after the filter. shown: the pose the picture handed over this frame was drawn with (the layer's; headless, as
// it would be), age poses old, told how; named: by the picture's own main view, and then the camera is that very picture's; else poseId is the pose it is taken to have, and the camera the
// newest build turned by that pose. world: how far that pose stands from the cameras the picture's own uploads carried.
void NoteReplayFrame(const XrView* filtered, const XrPosef* shown, uint32_t age, headtest::How how, bool named, uint32_t poseId, const WorldTurn& world) {
    headtest::Frame f;
    f.world = world.most, f.worldLast = world.last, f.worldFirst = world.first;
    f.mixed = world.mixed;
    f.ms = g_replay.ms;
    f.truth = g_replay.truth;
    f.located = g_replay.located;
    f.filtered = HeadOf(filtered[kLeftEye].pose, filtered[kRightEye].pose);
    f.picture = HeadOf(shown[kLeftEye], shown[kRightEye]);
    f.age = age;
    f.how = how;
    f.pose = poseId;
    // The head the aim's limit and the walk took this frame, and how the filter made this frame's pose.
    f.smooth = g_facingSplit ? headtest::Turn{g_facing.x, g_facing.y, g_facing.z, g_facing.w} : f.filtered.turn;
    f.by = g_poseBy;
    f.rate = g_poseRate;
    camera::DrawnCamera drawn;
    f.cameraOwn = named && camera::DrawnCameraNamed(drawn);
    f.haveCamera = f.cameraOwn || (!named && camera::DrawnCameraOfPose(poseId, drawn));
    if (f.haveCamera) {
        f.camera = headtest::RowsAngles(drawn.rows);
        f.game = headtest::RowsAngles(drawn.gameRows);
        for (int i = 0; i < 3; ++i) {
            f.gamePlace[i] = drawn.gamePlace[i];
            f.cameraPlace[i] = drawn.gamePlace[i] + drawn.move[i];  // (in floats, as views.cpp adds it)
            f.moveMm[i] = drawn.move[i] * 1000.0;
        }
        f.look = drawn.look;
        f.lookYaw = drawn.lookYaw * (180.0 / headtest::kPi);
        f.handYaw = drawn.handYaw * (180.0 / headtest::kPi);
        f.pose = drawn.poseId;
        f.build = drawn.build;
    }
    const double seconds = (g_replay.ms - g_replay.msWas) * 1e-3;
    const double speed = g_replay.haveWas && seconds > 0.0 ? headtest::Between(g_replay.truthWas.turn, g_replay.truth.turn) / seconds : 0.0;
    g_replay.second.Add(headtest::Slide(f.picture.turn, f.truth.turn), speed, age, named);
    char line[900];
    if (g_replay.ms * 1e-3 < g_replay.config.logSeconds) {
        headtest::FrameLine(f, line, sizeof(line));
        Log("%s", line);
    }
    if (const ULONGLONG now = GetTickCount64(); now - g_replay.secondAt >= 1000) {
        g_replay.secondAt = now;
        g_replay.second.Line(line, sizeof(line));
        Log("%s", line);
        g_replay.second.Clear();
    }
}

// weapon.txt "posefilter hz", "placefilter hz" (camera.h): the head as located, its turn and its place each through two one-pole low-passes, both eyes alike, before anything's made from it:
// the picture, the layer's pose and the hands' reference all go by the same, so the compositor still takes the world to where the head really is. A jump (a recenter, tracking found again) starts afresh.
void FilterViews(XrView* views, XrTime time) {
    static bool s_have = false;
    static XrQuaternionf s_turn[2] = {};
    static XrVector3f s_place[2] = {};
    static XrTime s_time = 0;
    const float turnHz = camera::PoseTry(0), placeHz = camera::PoseTry(1);
    const XrQuaternionf q = views[kLeftEye].pose.orientation;
    const XrVector3f a = views[kLeftEye].pose.position, b = views[kRightEye].pose.position;
    const XrVector3f centre{(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
    const float seconds = s_have ? std::clamp(static_cast<float>(time - s_time) / 1e9f, 0.0f, 0.1f) : 0.0f;
    s_time = time;
    auto apart = [](const XrVector3f& p, const XrVector3f& r) { return std::sqrt((p.x - r.x) * (p.x - r.x) + (p.y - r.y) * (p.y - r.y) + (p.z - r.z) * (p.z - r.z)); };
    const bool fresh = !s_have || g_poseFilterFresh;
    g_poseFilterFresh = false;
    static XrQuaternionf s_turnWas{};
    static XrVector3f s_speed{};  // the filtered turn's, radians a second about the room's axes, smoothed
    static XrQuaternionf s_rawWas{};
    static XrVector3f s_rawRate{};  // the located head's own, the same way, lightly smoothed ("poseadapt", below)
    if (fresh || AngleBetween(q, s_turn[1]) > 40.0f || turnHz <= 0.0f) {
        s_turn[0] = s_turn[1] = s_turnWas = q;
        s_speed = {};
    }
    if (fresh) {
        s_rawWas = q;
        s_rawRate = {};
    }
    if (fresh || apart(centre, s_place[1]) > 0.25f || placeHz <= 0.0f)
        s_place[0] = s_place[1] = centre;
    s_have = true;
    auto towards = [](const XrQuaternionf& from, XrQuaternionf to, float share) {
        if (from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w < 0.0f)
            to = {-to.x, -to.y, -to.z, -to.w};
        XrQuaternionf r{from.x + (to.x - from.x) * share, from.y + (to.y - from.y) * share, from.z + (to.z - from.z) * share, from.w + (to.w - from.w) * share};
        const float n = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
        return n > 0.0f ? XrQuaternionf{r.x / n, r.y / n, r.z / n, r.w / n} : from;
    };
    // weapon.txt "poseadapt" (camera.h). The fixed filter holds a fast head's picture back by its own delay (two low-passes at 6 Hz: 53 ms, 26 degrees at 500 a second) and the fixed lead, by a
    // speed that's slow to learn, makes that up only in a steady turn: in a quick swing the picture trails the head and then runs past it, and its edge comes far into view (the user's black corners
    // on a head shaken left and right, 2026-11-12). So the cut-off rises with the located head's own speed: the same filter on a still head, all but none on a fast one.
    const float adaptMs = camera::PoseTry(camera::kPoseAdapt);
    static float s_period = 1.0f / 72.0f, s_age = 1.0f;  // a frame's length, and how many frames old the pictures' poses have been lately
    if (seconds > 0.004f && seconds < 0.02f)
        s_period += (seconds - s_period) * 0.1f;
    s_age += (std::clamp(static_cast<float>(g_drawnAge), 1.0f, 3.0f) - s_age) * 0.1f;
    if (seconds > 0.0f) {
        const XrQuaternionf w{-s_rawWas.x, -s_rawWas.y, -s_rawWas.z, s_rawWas.w};
        XrQuaternionf step{q.w * w.x + q.x * w.w + q.y * w.z - q.z * w.y, q.w * w.y - q.x * w.z + q.y * w.w + q.z * w.x, q.w * w.z + q.x * w.y - q.y * w.x + q.z * w.w,
                           q.w * w.w - q.x * w.x - q.y * w.y - q.z * w.z};
        if (step.w < 0.0f)
            step = {-step.x, -step.y, -step.z, -step.w};
        const float share = 1.0f - std::exp(-2.0f * kPi * 12.0f * seconds), toRate = 2.0f / seconds;
        s_rawRate.x += (step.x * toRate - s_rawRate.x) * share;
        s_rawRate.y += (step.y * toRate - s_rawRate.y) * share;
        s_rawRate.z += (step.z * toRate - s_rawRate.z) * share;
    }
    s_rawWas = q;
    const float rawSpeed = std::sqrt(s_rawRate.x * s_rawRate.x + s_rawRate.y * s_rawRate.y + s_rawRate.z * s_rawRate.z) * (180.0f / kPi);  // degrees a second
    const float hzNow = adaptMs > 0.0f ? turnHz * (1.0f + rawSpeed / 15.0f) : turnHz;  // as much again for every 15 degrees a second
    if (turnHz > 0.0f) {
        const float share = 1.0f - std::exp(-2.0f * kPi * hzNow * seconds);
        s_turn[0] = towards(s_turn[0], q, share);
        s_turn[1] = towards(s_turn[1], s_turn[0], share);
    }
    // The head for the aim's limit and the walk beside a pose candidate (g_facing, above): the located turn through the fixed filter's own two low-passes, whatever "poseadapt"
    // opens or "poserate" carries, and no lead. Kept up every frame, so a candidate switched on has it at once.
    static XrQuaternionf s_facing[2] = {};
    if (fresh || AngleBetween(q, s_facing[1]) > 40.0f || turnHz <= 0.0f)
        s_facing[0] = s_facing[1] = q;
    if (turnHz > 0.0f) {
        const float share = 1.0f - std::exp(-2.0f * kPi * turnHz * seconds);
        s_facing[0] = towards(s_facing[0], q, share);
        s_facing[1] = towards(s_facing[1], s_facing[0], share);
    }
    const float rateTry = camera::PoseTry(camera::kPoseRate);
    g_facing = s_facing[1];
    g_facingSplit = adaptMs > 0.0f || rateTry != 0.0f;
    g_poseBy = adaptMs > 0.0f ? headtest::kByAdapt : headtest::kByFixed;
    g_poseRate = 0.0;
    if (placeHz > 0.0f) {
        const float share = 1.0f - std::exp(-2.0f * kPi * placeHz * seconds);
        auto step = [share](XrVector3f& p, const XrVector3f& to) { p.x += (to.x - p.x) * share, p.y += (to.y - p.y) * share, p.z += (to.z - p.z) * share; };
        step(s_place[0], centre);
        step(s_place[1], s_place[0]);
    }
    if (turnHz <= 0.0f && placeHz <= 0.0f)
        return;
    auto mul = [](const XrQuaternionf& l, const XrQuaternionf& r) {
        return XrQuaternionf{l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y, l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x, l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w, l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z};
    };
    // The filter's lag and the picture's age turn the picture away from where the head is by the time it's seen, so the compositor swings it further and its edge shows: carried on by the filtered
    // turn's own speed, itself smoothed, or it brings the noise back. It needn't be exact: the compositor still takes the world to where the head is.
    XrQuaternionf f = s_turn[1];
    if (turnHz > 0.0f && seconds > 0.0f) {
        XrQuaternionf step = mul(f, XrQuaternionf{-s_turnWas.x, -s_turnWas.y, -s_turnWas.z, s_turnWas.w});
        if (step.w < 0.0f)
            step = {-step.x, -step.y, -step.z, -step.w};
        const float share = 1.0f - std::exp(-2.0f * kPi * 3.0f * seconds);
        const float toSpeed = 2.0f / seconds;
        s_speed.x += (step.x * toSpeed - s_speed.x) * share;
        s_speed.y += (step.y * toSpeed - s_speed.y) * share;
        s_speed.z += (step.z * toSpeed - s_speed.z) * share;
        s_turnWas = f;
        // "poseadapt": carried on by the located head's own speed for as long as the picture will be old when it's shown (measured; or the ms given) plus what the opened filter still holds back;
        // not at all while the head's all but still, or its noise would come back. Else the fixed lead by the filtered turn's speed.
        XrVector3f r;
        if (adaptMs > 0.0f) {
            const float held = hzNow > 0.0f ? 1.0f / (kPi * hzNow) : 0.0f;  // two one-pole low-passes' delay
            const float lead = ((adaptMs > 1.0f ? adaptMs * 1e-3f : s_age * s_period) + held) * std::clamp((rawSpeed - 10.0f) / 30.0f, 0.0f, 1.0f);
            r = {s_rawRate.x * lead, s_rawRate.y * lead, s_rawRate.z * lead};
        } else {
            const float lead = camera::PoseTry(camera::kPoseLead) * 1e-3f;
            r = {s_speed.x * lead, s_speed.y * lead, s_speed.z * lead};
        }
        const float angle = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
        if (angle > 1e-5f) {
            const float kept = std::min(angle, 0.6f);  // (35 degrees at the most)
            const float s = std::sin(kept / 2) / angle, c = std::cos(kept / 2);
            f = mul(XrQuaternionf{r.x * s, r.y * s, r.z * s, c}, f);
        }
    }
    // weapon.txt "poserate" (camera.h; headtest.h RatePose). The fixed filter's turn is late and its lead late again; "poseadapt" follows the located head closely and hands on
    // its unevenness three times over (the user, 2026-11-12: less black, but "too jittery ... really bad"). Here the turn is carried from frame to frame by the head's own turn
    // rate as it is given (g_headRate) and only pulled towards the located turn, twice, at "posefilter", then carried on by the same rate for the pictures' age: nothing is held
    // back however fast the head turns, and no more of the located head's unevenness comes through than two low-passes let through. A frame without a rate, or a second in which
    // the rate given turned out not to be the head's, keeps the turn the filter beside it made (f, as it stands here), and the carried turn goes on from that. 1: the rate as it
    // comes. 2: carried on by its own change for as late as it is given (as its fit against the located head's turning says). Negative: the rate taken as the head itself has
    // it, not as the room has it (for a runtime that gives it so: the fit's line tells).
    static headtest::RatePose s_rated;
    static XrTime s_ratedAt = 0, s_ratedFrom = 0;
    static ULONGLONG s_ratedLogged = 0;
    if (rateTry != 0.0f) {
        const bool own = rateTry < 0.0f, makeUp = std::fabs(rateTry) >= 2.0f;
        // Started again with the filter (the first frame, a recenter) and after a gap in the frames (the headset off and on again, a load's hitch): no turn is carried across it.
        const bool again = fresh || s_ratedAt == 0 || time - s_ratedAt > 100000000 || time < s_ratedAt;
        if (s_ratedAt == 0)
            s_ratedFrom = time;
        s_ratedAt = time;
        const double rate[3] = {g_headRate.rate.x, g_headRate.rate.y, g_headRate.rate.z};
        // (The runtime's rate is used only once a third of a second of turning has shown it to be the head's. A head replay's is the made-up head's own.)
        if (g_replay.thisFrame && g_headRate.given)
            s_rated.TakeAsShown();
        headtest::Turn carried;
        if (s_rated.Step({q.x, q.y, q.z, q.w}, g_headRate.given ? rate : nullptr, own, makeUp, static_cast<double>(time - s_ratedFrom) * 1e-9, seconds, turnHz, s_period,
                         camera::PoseTry(camera::kPoseAhead), g_drawnAge, again, {f.x, f.y, f.z, f.w}, carried))
            f = {static_cast<float>(carried.x), static_cast<float>(carried.y), static_cast<float>(carried.z), static_cast<float>(carried.w)};
        g_poseBy = s_rated.back ? headtest::kByFallback : headtest::kByRate;
        g_poseRate = s_rated.size;
        if (const ULONGLONG now = GetTickCount64(); now - s_ratedLogged >= 1000) {
            s_ratedLogged = now;
            char line[700], fit[700];
            if (s_rated.Line(line, sizeof(line), fit, sizeof(fit), own, makeUp) > 0)
                Log("%s", line);
            if (fit[0])
                Log("%s", fit);
        }
    } else if (s_ratedAt != 0) {
        s_rated = headtest::RatePose{};  // switched off: switched on again, it starts afresh
        s_ratedAt = 0;
    }
    // The eyes: the same turn and move on both, about the head's middle.
    const XrQuaternionf inverse{-q.x, -q.y, -q.z, q.w};
    const XrQuaternionf by = mul(f, inverse);
    for (int e = 0; e < 2; ++e) {
        const XrVector3f v{views[e].pose.position.x - centre.x, views[e].pose.position.y - centre.y, views[e].pose.position.z - centre.z};
        const XrVector3f t{2 * (by.y * v.z - by.z * v.y), 2 * (by.z * v.x - by.x * v.z), 2 * (by.x * v.y - by.y * v.x)};
        views[e].pose.position = {s_place[1].x + v.x + by.w * t.x + by.y * t.z - by.z * t.y, s_place[1].y + v.y + by.w * t.y + by.z * t.x - by.x * t.z, s_place[1].z + v.z + by.w * t.z + by.x * t.y - by.y * t.x};
        views[e].pose.orientation = mul(by, views[e].pose.orientation);
    }
}

// headless: the headset isn't asking for frames (taken off, asleep: the session never got past
// synchronized), but the stand-in hand is on (weapon.txt "standin 1"): the frame's work is done as
// usual from the eyes as last seen (or a default standing head), so the aim, the hands and the
// self-test (tools/selftest.ps1) run overnight; nothing is handed to the headset.
void BuildStereo(ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& desc, XrTime displayTime, FrameLayers& out,
                 bool headless = false) {
    XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};
    locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate.displayTime = displayTime;
    locate.space = g_space;
    XrViewState viewState{XR_TYPE_VIEW_STATE};
    XrView views[2] = {{XR_TYPE_VIEW}, {XR_TYPE_VIEW}};
    uint32_t viewCount = 0;
    if (headless) {
        if (g_haveLastViews) {
            views[0] = g_lastViews[0], views[1] = g_lastViews[1];
        } else {
            for (int e = 0; e < 2; ++e) {
                views[e].pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
                views[e].pose.position = {e == 0 ? -0.032f : 0.032f, 1.6f, 0.0f};
                views[e].fov = {-0.87f, 0.87f, 0.87f, -0.87f};
            }
        }
        // "standin 1 45": the head tilted up that far about its own right (test shots into the sky).
        if (const float pitch = input::StandInPitch(); pitch != 0.0f) {
            const XrQuaternionf t{std::sin(pitch / 2), 0.0f, 0.0f, std::cos(pitch / 2)};
            for (XrView& v : views) {
                const XrQuaternionf q = v.pose.orientation;
                v.pose.orientation = {q.w * t.x + q.x * t.w + q.y * t.z - q.z * t.y,
                                      q.w * t.y - q.x * t.z + q.y * t.w + q.z * t.x,
                                      q.w * t.z + q.x * t.y - q.y * t.x + q.z * t.w,
                                      q.w * t.w - q.x * t.x - q.y * t.y - q.z * t.z};
            }
        }
        // "headwave 30 2": the head turning, to see which pose each picture gets.
        if (const float yaw = camera::HeadWaveYaw(); yaw != 0.0f) {
            const XrQuaternionf t{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
            for (XrView& v : views) {
                const XrQuaternionf q = v.pose.orientation;
                v.pose.orientation = {t.w * q.x + t.x * q.w + t.y * q.z - t.z * q.y, t.w * q.y - t.x * q.z + t.y * q.w + t.z * q.x,
                                      t.w * q.z + t.x * q.y - t.y * q.x + t.z * q.w, t.w * q.w - t.x * q.x - t.y * q.y - t.z * q.z};
            }
        }
    } else if (!Check(xrLocateViews(g_session, &locate, &viewState, 2, &viewCount, views), "xrLocateViews") ||
               !(viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT)) {
        return;
    } else {
        g_lastViews[0] = views[0], g_lastViews[1] = views[1];
        g_haveLastViews = true;
    }
    // The head replay (headtest.h): with nobody in the headset, a made-up head in place of this one. Everything from the filter on is as played.
    if (g_replay.wanted)
        ReplayHead(views, displayTime, headless);
    // weapon.txt "poserate": the head's own turn rate for the time the head is located for, as the runtime has it from the headset's motion sensors (the headset itself in
    // the room's space, its angular velocity asked for with its place); under a head replay the made-up head's own. A headless frame's stood-in eyes have none.
    g_headRate.given = false;
    if (camera::PoseTry(camera::kPoseRate) != 0.0f) {
        if (g_replay.thisFrame) {
            double rate[3];
            headtest::RateAt(g_replay.config, g_replay.base, g_replay.locatedAt, g_replay.locatedFrame, rate);
            g_headRate.rate = {static_cast<float>(rate[0]), static_cast<float>(rate[1]), static_cast<float>(rate[2])};
            g_headRate.given = true;
        } else if (!headless) {
            XrSpaceVelocity velocity{XR_TYPE_SPACE_VELOCITY};
            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
            location.next = &velocity;
            if (XR_SUCCEEDED(xrLocateSpace(g_viewSpace, g_space, displayTime, &location)) && (velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT)) {
                g_headRate.rate = velocity.angularVelocity;
                g_headRate.given = true;
            }
        }
    }
    // The head as located for this frame's display time, before any filter (under a head replay, the made-up located head): what the edge's line holds the picture's
    // pose against.
    const XrQuaternionf locatedTurn = views[kLeftEye].pose.orientation;
    FilterViews(views, displayTime);

    // Alternate eyes only work if the game builds exactly one camera between two Presents: with
    // none, this frame was built for the eye before; with two, one frame was never shown. Count.
    {
        static uint32_t builds[4] = {};
        static ULONGLONG lastLog = 0;
        ++builds[std::min(camera::TakeCameraBuilds(), 3u)];
        ULONGLONG now = GetTickCount64();
        if (now - lastLog >= 1000) {
            lastLog = now;
            Log("Frames: camera builds between two Presents: none %u, one %u, two %u, more %u", builds[0], builds[1],
                builds[2], builds[3]);
            std::fill(std::begin(builds), std::end(builds), 0u);
        }
    }

    // Doubling test (Scroll Lock): frozen, nothing new is shown, so turning the head shows
    // whether the compositor holds a still picture steady; mono, every frame goes to both eyes
    // from between them, so there is no alternating.
    if (g_stereoTest == kTestFreeze || camera::PoseTry(camera::kFreezeView) != 0.0f) {
        if (g_eyeValid[kLeftEye] && g_eyeValid[kRightEye])
            BuildProjection(out);
        return;
    }
    // This frame belongs to one eye (in the mono test, both); remember the pose it is shown for.
    // The camera is turned with the left eye's orientation for either eye, so both images are
    // handed over with that orientation and their own eye's position: if the headset's eyes look
    // in slightly different directions (angled displays), the right image would otherwise sit off.
    int lag = config::Get().poseLag;
    const XrPosef* guessed = lag > 0 && g_posesKept >= lag ? g_poseHistory[(g_poseNewest - (lag - 1) + kPoseHistory) % kPoseHistory] : nullptr;
    // The pose this picture was really drawn with, named by its own main view; not named, as old as the last one's.
    const XrPosef* drawnPoses = nullptr;
    bool drawnNamed = false;  // (for the head replay's line: named by its own main view, and which pose)
    uint32_t drawnPose = 0;
    {
        uint32_t drawnId = 0;
        if (camera::TakeDrawnPose(drawnId) && g_poseSeq - drawnId < kPoseRing) {
            const uint32_t age = g_poseSeq - drawnId + 1;
            g_poseAgeChanges += age != g_drawnAge;
            g_drawnAge = age;
            ++g_poseAges[std::min(age, 5u)];
            drawnPoses = g_poseById[drawnId % kPoseRing];
            drawnNamed = true;
            drawnPose = drawnId;
            if (guessed) {
                const float off = AngleBetween(guessed[kLeftEye].orientation, drawnPoses[kLeftEye].orientation);
                g_poseGuessOff += off;
                g_poseGuessWorst = std::max(g_poseGuessWorst, off);
            }
        } else {
            ++g_poseUnnamed;
            if (g_poseSeq > g_drawnAge && g_drawnAge <= kPoseRing) {
                drawnPose = g_poseSeq - g_drawnAge + 1;
                drawnPoses = g_poseById[drawnPose % kPoseRing];
            }
        }
        if (const ULONGLONG now = GetTickCount64(); now - g_poseLogged >= 1000) {
            g_poseLogged = now;
            const uint32_t named = g_poseAges[0] + g_poseAges[1] + g_poseAges[2] + g_poseAges[3] + g_poseAges[4] + g_poseAges[5];
            Log("Head poses: pictures drawn with the pose 1 frame old %u, 2 %u, 3 %u, 4 %u, older %u, not named %u; it changed %u times; pose_lag's guess off by %.2f degrees on average, %.2f at the worst%s",
                g_poseAges[1], g_poseAges[2], g_poseAges[3], g_poseAges[4], g_poseAges[5], g_poseUnnamed, g_poseAgeChanges, named ? g_poseGuessOff / named : 0.0, g_poseGuessWorst,
                camera::PoseByDraw() ? "" : " (the guess in use)");
            std::fill(std::begin(g_poseAges), std::end(g_poseAges), 0u);
            g_poseUnnamed = g_poseAgeChanges = 0;
            g_poseGuessOff = 0.0;
            g_poseGuessWorst = 0.0f;
        }
    }
    auto renderPose = [&](int e) {
        const XrPosef* poses = camera::PoseByDraw() && drawnPoses ? drawnPoses : guessed;
        XrPosef pose = poses ? poses[e] : views[e].pose;  // lag 0, or not enough frames yet
        pose.orientation = poses ? poses[kLeftEye].orientation : views[kLeftEye].pose.orientation;
        // weapon.txt "posetest 2": the layer told otherwise every other second, the picture not.
        if (const float degrees = camera::PoseTry(3); degrees != 0.0f && (GetTickCount64() / 1000) % 2 == 0) {
            const float half = degrees * kPi / 360.0f, s = std::sin(half), c = std::cos(half);
            const XrQuaternionf q = pose.orientation;
            pose.orientation = {c * q.x + s * q.z, c * q.y + s * q.w, c * q.z - s * q.x, c * q.w - s * q.y};
        }
        return pose;
    };
    {
        static ULONGLONG lastLog = 0;
        ULONGLONG now = GetTickCount64();
        if (now - lastLog >= 10000) {
            lastLog = now;
            const XrQuaternionf& l = views[kLeftEye].pose.orientation;
            const XrQuaternionf& r = views[kRightEye].pose.orientation;
            float dot = std::fabs(l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w);
            float angle = 2.0f * std::acos(std::min(dot, 1.0f)) * 180.0f / kPi;
            const XrFovf& fl = views[kLeftEye].fov;
            const XrFovf& fr = views[kRightEye].fov;
            Log("Eyes: %.2f degrees apart in direction; left fov %.1f/%.1f/%.1f/%.1f, right %.1f/%.1f/%.1f/%.1f "
                "(left/right/up/down, degrees)",
                angle, fl.angleLeft * 180.0f / kPi, fl.angleRight * 180.0f / kPi, fl.angleUp * 180.0f / kPi,
                fl.angleDown * 180.0f / kPi, fr.angleLeft * 180.0f / kPi, fr.angleRight * 180.0f / kPi,
                fr.angleUp * 180.0f / kPi, fr.angleDown * 180.0f / kPi);
        }
    }
    const XrVector3f& a = views[kLeftEye].pose.position;
    const XrVector3f& b = views[kRightEye].pose.position;
    float ipd = std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
    // How rough the located head is, once a second: how far each frame's place and turn are from carrying on as the last two frames went (what moving the head smoothly would have no
    // more than a little of). All of the place goes into the game's cameras, and the compositor can't correct it.
    {
        static XrVector3f s_place[2] = {};
        static XrQuaternionf s_turn[2] = {};
        static int s_have = 0;
        static double s_placeSum = 0.0, s_turnSum = 0.0, s_speedSum = 0.0;
        static float s_placeWorst = 0.0f, s_turnWorst = 0.0f;
        static uint32_t s_count = 0;
        static ULONGLONG s_logged = 0;
        const XrVector3f place{(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
        const XrQuaternionf turn = views[kLeftEye].pose.orientation;
        if (s_have >= 2) {
            const float dx = place.x - 2 * s_place[0].x + s_place[1].x, dy = place.y - 2 * s_place[0].y + s_place[1].y, dz = place.z - 2 * s_place[0].z + s_place[1].z;
            const float placeRough = std::sqrt(dx * dx + dy * dy + dz * dz) * 1000.0f;
            const float step = AngleBetween(turn, s_turn[0]), lastStep = AngleBetween(s_turn[0], s_turn[1]);
            const float turnRough = std::fabs(step - lastStep);
            s_placeSum += placeRough, s_turnSum += turnRough, s_speedSum += step;
            s_placeWorst = std::max(s_placeWorst, placeRough);
            s_turnWorst = std::max(s_turnWorst, turnRough);
            ++s_count;
        }
        s_place[1] = s_place[0], s_place[0] = place;
        s_turn[1] = s_turn[0], s_turn[0] = turn;
        s_have = std::min(s_have + 1, 2);
        if (const ULONGLONG now = GetTickCount64(); now - s_logged >= 1000 && s_count) {
            s_logged = now;
            // (A head replay's seconds are marked: the scripts that tell a worn headset by this line must not take the made-up head for a person's.)
            Log("Head rough%s: turning %.0f degrees a second; a frame's turn off the last's by %.3f degrees on average, %.2f at the worst; its place off a straight line by %.2f mm on average, %.2f at the worst",
                now - g_replay.lastAt < 1500 ? " (a replay's made-up head)" : "", s_speedSum / s_count * 72.0, s_turnSum / s_count, s_turnWorst, s_placeSum / s_count, s_placeWorst);
            s_placeSum = s_turnSum = s_speedSum = 0.0;
            s_placeWorst = s_turnWorst = 0.0f;
            s_count = 0;
        }
    }
    {
        XrPosef head = views[kLeftEye].pose;
        head.position = {(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
        // weapon.txt "standin 1": a right hand while the controller sleeps. Beside a pose candidate it aims where the head faces as the aim's limit takes it (g_facing), not
        // along the picture's own pose: the user's aim stood at that limit through the head shakes of 2026-11-12, so this is the look yaw a candidate hands the game.
        XrPosef standInHead = head;
        if (g_facingSplit)
            standInHead.orientation = g_facing;
        input::StandIn(standInHead);
        // The hands' places are measured from the head and drawn into the next picture, from the pose that picture's views were built with: measured from
        // the newest, the gun and arms were off by the head's turn in the frames between, and by more or less from frame to frame as the build fell either side of this.
        uint32_t handsPose = 0;
        if (uint32_t nextId = 0; camera::PoseByDraw() && camera::NextDrawnPose(nextId) && g_poseSeq - nextId < kPoseRing) {
            const XrPosef* p = g_poseById[nextId % kPoseRing];
            head.orientation = p[kLeftEye].orientation;
            head.position = {(p[kLeftEye].position.x + p[kRightEye].position.x) / 2, (p[kLeftEye].position.y + p[kRightEye].position.y) / 2,
                             (p[kLeftEye].position.z + p[kRightEye].position.z) / 2};
            handsPose = nextId;
        }
        camera::SetHandsPose(handsPose);
        hands::Update(head);
    }
    // weapon.txt "watch": where the head and hands are, for lining the game's arms up with them.
    if (skin::Watching()) {
        static ULONGLONG lastPoses = 0;
        ULONGLONG now = GetTickCount64();
        if (now - lastPoses >= 1000) {
            lastPoses = now;
            const XrQuaternionf& h = views[kLeftEye].pose.orientation;
            char hands[2][128] = {"untracked", "untracked"};
            for (int i = 0; i < 2; ++i) {
                XrPosef p;
                if (input::HandPose(i, p))
                    snprintf(hands[i], sizeof(hands[i]), "(%.3f %.3f %.3f) q (%.3f %.3f %.3f %.3f)", p.position.x,
                             p.position.y, p.position.z, p.orientation.x, p.orientation.y, p.orientation.z,
                             p.orientation.w);
            }
            Log("Poses: head (%.3f %.3f %.3f) q (%.3f %.3f %.3f %.3f); left hand %s; right hand %s",
                (a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2, h.x, h.y, h.z, h.w, hands[0], hands[1]);
        }
    }
    int eye = g_nextEye;
    bool perEye = g_stereoTest == kTestViews && views::Active();
    bool mono = g_stereoTest == kTestMono;
    // Until BF3 draws the two views (the first frames, or no hook), depth 3D.
    bool fromDepth = g_stereoTest == kTestDepth || (g_stereoTest == kTestViews && !perEye);
    bool hybrid = g_stereoTest == kTestHybrid;
    g_sideBySide = false;
    g_fillActive = false;
    // Three pictures in a row, so a picture that missed its turn in ordinary play doesn't flick. Counted with
    // the headset asleep too (headless): a load's first pictures set it, and only the headset's branch below
    // cleared it, so it stayed on and kept the aim, mouse aim and the free hands off (the self-test, 20:34).
    bool unturned = false;
    if (perEye) {
        static int s_unturnedRun = 0, s_scriptedRun = 0, s_cutsceneRun = 0;
        int scripted = 0;
        const bool none = g_viewTurnOn && camera::TakeTurned(&scripted) == 0;
        s_unturnedRun = none ? s_unturnedRun + 1 : 0;
        unturned = s_unturnedRun >= 3;
        if (unturned != g_unturnedLogged) {
            g_unturnedLogged = unturned;
            Log("Views: %s", unturned ? "the game draws without the head's turn (a cutscene's camera): the picture "
                                        "is kept still in the room, the head looks about it"
                                      : "the head's turn is in the game's camera again");
        }
        // Counted as one run with the unturned ones: a cutscene going from one to the other (a load's empty views,
        // then the drive) never leaves the look angles to be written or looked for in between.
        s_scriptedRun = scripted > 0 ? s_scriptedRun + 1 : 0;
        s_cutsceneRun = none || scripted > 0 ? s_cutsceneRun + 1 : 0;
        g_cutscene = s_cutsceneRun >= 3;
        if ((s_scriptedRun >= 3) != g_scriptedLogged) {
            g_scriptedLogged = s_scriptedRun >= 3;
            Log("Views: %s", g_scriptedLogged ? "the game draws from a scripted camera (a cutscene's or a ride's): "
                                                "it's turned to the head about its own yaw, the horizon level, and "
                                                "the rest is as in a cutscene (the look angles left alone, no free "
                                                "hands)"
                                              : "the game draws from its scripted camera no more");
        }
    }
    if (static float s_fillShot = 0.0f; camera::PoseTry(camera::kFillShot) != s_fillShot) {
        s_fillShot = camera::PoseTry(camera::kFillShot);
        if (headless || perEye)
            FilmFill(back, desc, views);
    }
    if (headless) {
        depth3d::SaveIfAsked(g_device, back, L"eye_both.bmp");
    } else if (perEye) {
        // Both eyes are in this picture, side by side: the left eye's swapchain takes it whole and
        // each eye shows its half; or, with the picture's edge carried outwards round it, a bigger one.
        const bool fill = camera::PoseTry(camera::kEdgeFill) != 0.0f && desc.SampleDesc.Count == 1 && PrepareFill(desc, views);
        const bool leftHeld = HoldLeftEye(back, desc);
        if (fill ? !SubmitFilled(back, desc) : !SubmitImage(back, desc, g_swapchains[kLeftEye]))
            return;
        g_fillActive = fill;
        for (int e = 0; e < 2; ++e) {
            if (e == kLeftEye && leftHeld && g_eyeValid[e])
                continue;  // its picture is the frame before's, and so is its pose
            g_eyePose[e] = renderPose(e);
            if (unturned)
                g_eyePose[e].orientation = {0.0f, std::sin(g_viewTurnYaw / 2), 0.0f, std::cos(g_viewTurnYaw / 2)};
            g_eyeValid[e] = true;
        }
        g_sideBySide = true;
        depth3d::SaveIfAsked(g_device, back, L"eye_both.bmp");  // depth3d.txt "save N"
    } else if (fromDepth || hybrid) {
        // Hybrid: this frame was rendered from `eye`, which gets it unchanged; the other eye is a
        // full eye distance away.
        if (!SubmitDepth3D(back, desc, ipd, hybrid ? (eye == kLeftEye ? -1.0f : 1.0f) : 2.0f))
            return;
        for (int e = 0; e < 2; ++e) {
            g_eyePose[e] = renderPose(e);
            g_eyeValid[e] = true;
        }
    } else {
        for (int e = 0; e < 2; ++e) {
            if (e != eye && !mono)
                continue;
            if (!SubmitImage(back, desc, g_swapchains[e]))
                return;
            g_eyePose[e] = renderPose(e);
            g_eyeValid[e] = true;
        }
        UpdateMirror(back, desc, mono ? kLeftEye : eye);
    }
    // The pose this picture goes out with as it's handed over (headless under a head replay: as it would be), for the two meters that run with somebody in the headset
    // too and for the head replay's line with the camera it was drawn from.
    if (const bool handed = !headless && perEye; handed || g_replay.thisFrame) {
        XrPosef shown[2] = {renderPose(kLeftEye), renderPose(kRightEye)};
        if (handed) {
            shown[kLeftEye] = g_eyePose[kLeftEye], shown[kRightEye] = g_eyePose[kRightEye];
        } else if (unturned) {
            for (XrPosef& pose : shown)
                pose.orientation = {0.0f, std::sin(g_viewTurnYaw / 2), 0.0f, std::cos(g_viewTurnYaw / 2)};
        }
        const bool byDraw = camera::PoseByDraw() && drawnPoses;
        // (The right eye's turn: both eyes go out with one turn, but with the left eye at half rate, weapon.txt "halfeye", the left one's is the picture before's on the
        // frames between.)
        const XrQuaternionf& goes = shown[kRightEye].orientation;
        const headtest::Turn picture{goes.x, goes.y, goes.z, goes.w};
        const headtest::Turn located{locatedTurn.x, locatedTurn.y, locatedTurn.z, locatedTurn.w};
        WorldTurn world;
        // (A picture the game drew without the head's turn goes out as seen from where it was drawn, level: neither meter has anything to say of it.)
        if (!unturned) {
            // The world's turn: the camera rows the picture's own main-view uploads carried, each against the camera this pose stands for with the body the view
            // build of those rows had (that build's rows, its own head taken off, this pose put on). Told the pose it was drawn with, nothing; told another
            // build's, the head's turn between the two poses, which is how far the headset then moves the whole world. Nothing of it goes by the picture's name,
            // and the body's own turn between two builds (the stick, a scripted camera) is not in it: taken with the body of the build the pose told belongs to,
            // a stick turning 120 degrees a second read as 2.5 or 5.8 degrees where the head's step was 4.2.
            camera::DrawnWorld drawn;
            const bool have = camera::DrawnWorldView(drawn);
            if (have) {
                auto apart = [&picture](const float* rows, const camera::DrawnWorld::Build& b) {
                    return b.have ? headtest::CameraApart(rows, headtest::CameraOfPose(b.rows, {b.head[0], b.head[1], b.head[2], b.head[3]}, picture)) : -1.0;
                };
                world.last = apart(drawn.rows, drawn.build);
                if (drawn.haveFirst)
                    world.first = apart(drawn.firstRows, drawn.firstBuild);
                world.most = drawn.haveMost ? apart(drawn.mostRows, drawn.mostBuild) : world.last;
                world.mixed = drawn.mixed;
            }
            if (have && world.most >= 0.0)
                g_worldSecond.Add(world.most, world.last, drawn.marked, world.first, drawn.mixed, drawn.handsOff);
            else
                g_worldSecond.AddNone();
            // The edge: this picture's pose against the head as located for this frame's display time, in the frames in which that head turns.
            static XrQuaternionf s_locatedWas{0.0f, 0.0f, 0.0f, 1.0f};
            static XrTime s_locatedAt = 0;
            double speed = 0.0;
            if (s_locatedAt != 0 && displayTime > s_locatedAt && displayTime - s_locatedAt < 100000000)
                speed = headtest::Between({s_locatedWas.x, s_locatedWas.y, s_locatedWas.z, s_locatedWas.w}, located) / (static_cast<double>(displayTime - s_locatedAt) * 1e-9);
            s_locatedWas = locatedTurn;
            s_locatedAt = displayTime;
            g_edgeSecond.Add(headtest::Slide(picture, located), headtest::Between(picture, located), speed);
        }
        if (const ULONGLONG now = GetTickCount64(); now - g_metersLogged >= 1000) {
            g_metersLogged = now;
            char line[800];
            if (g_worldSecond.Line(line, sizeof(line)) > 0)
                Log("%s", line);
            g_worldSecond.Clear();
            if (g_edgeSecond.Line(line, sizeof(line)) > 0)
                Log("%s", line);
            g_edgeSecond.Clear();
        }
        if (g_replay.thisFrame)
            NoteReplayFrame(views, shown, byDraw ? g_drawnAge : guessed ? static_cast<uint32_t>(lag) : 0u,
                            byDraw ? (drawnNamed ? headtest::kNamed : headtest::kGuessed) : guessed ? headtest::kLag : headtest::kNewest, drawnNamed, drawnPose, world);
    }
    g_nextEye = 1 - eye;

    // Alternating: the game's next frame is for the other eye, so its camera is shifted half the
    // eye distance that way. Mono and depth 3D render from between the eyes. Hybrid moves the
    // camera by the share of the eye distance depth 3D uses, so the made eye and the rendered
    // one stay that far apart whichever is rendered.
    float side = (g_nextEye == kLeftEye ? -1.0f : 1.0f) * (g_swapEyes ? -1.0f : 1.0f);
    camera::SetEyeOffset(perEye || mono || fromDepth ? 0.0f
                                                     : 0.5f * ipd * side * (hybrid ? depth3d::Strength() : 1.0f));
    // Per-eye views: each view half the eye distance out from the middle of the head.
    const float leftSide = g_swapEyes ? 1.0f : -1.0f;
    views::SetEyeOffsets(0.5f * ipd * leftSide, -0.5f * ipd * leftSide);

    // The look angles (what BF3 aims and fires along) follow the right controller when it's
    // tracked, otherwise the head. With the controller aiming, the render cameras are turned
    // so the picture still shows where the head looks.
    if (config::Get().headTracking) {
        const XrQuaternionf& head = views[kLeftEye].pose.orientation;
        // Where the head faces, for the aim's limit and the walk's direction: the picture's own pose as ever, or beside a pose candidate the smooth head without any lead
        // (g_facing), so the game's look yaw and the bullets' line never take a candidate's steps. (The head aiming by itself, with no hand, still goes by the picture's pose:
        // the game's camera then is the picture, and the layer is told that pose.)
        const XrQuaternionf& facing = g_facingSplit ? g_facing : head;
        XrPosef hand;
        // Free hands (hands.cpp): the game aims along the rifle as drawn in whichever hand holds it (the left, or by its handguard
        // alone), gliding into a hand, or easing to the head while nobody holds it. It's aimed "with the hand" all the while, so a drop
        // or a pickup never switches the source (no aim::Reset, the picture's body kept: no jump). False (dormant, or the right on the
        // pistol grip): the right controller, as before.
        XrQuaternionf freeAim{0.0f, 0.0f, 0.0f, 1.0f};
        const bool fromHands = config::Get().controllerAim && hands::Aim(freeAim);
        const bool handAim = config::Get().controllerAim && input::AimPose(hand);
        bool useHand = fromHands || handAim;
        if (useHand != g_aimingWithHand) {
            aim::Reset();  // switching source: don't jump by the difference between them
            g_aimingWithHand = useHand;
            Log("Aiming with %s", useHand ? (fromHands ? "the rifle in the hands (free hands)" : "the right controller") : "the head");
        }
        float yaw = 0.0f, pitch = 0.0f;
        // With both hands on the gun it points towards the left hand as far as the hands agree
        // (hands.cpp, steer.h).
        XrQuaternionf twoHanded;
        LookAngles(fromHands ? freeAim : useHand ? (hands::AimOrientation(twoHanded) ? twoHanded : hand.orientation) : head, yaw, pitch);
        if (useHand) {
            // BF3 only draws what's in front of where it aims. With the hand far off to one side,
            // what the head looks at isn't drawn at all, so the aim is kept within a limit of the
            // view, like a shoulder that can't swing a rifle behind the player.
            float headYaw = 0.0f, headPitch = 0.0f;
            LookAngles(facing, headYaw, headPitch);
            float limit = config::Get().maxAimOffsetDeg * kPi / 180.0f;
            float offset = yaw - headYaw;
            if (offset > kPi)
                offset -= 2 * kPi;
            if (offset < -kPi)
                offset += 2 * kPi;
            yaw = headYaw + std::clamp(offset, -limit, limit);
            pitch = headPitch + std::clamp(pitch - headPitch, -limit, limit);
            yaw += aim::TestTurn();  // weapon.txt "aimtest": a gun turn without moving it
        }
        // While the game draws without the head's turn (a cutscene, the jump onto the train in The
        // Great Destroyer) it doesn't use the look angles either: nothing is written then. The game
        // crashed four times at that jump (2026-10-31), the last 5 s after the picture went unturned
        // with the angles still written 144 times a second; afterwards they're taken up from where
        // the game left them, without a jump. Paused alone that wasn't enough: it crashed 2 s after the
        // picture was turned again, the angles written where they had been found before the jump.
        // They're forgotten and looked for again (aim::Forget). A scripted camera's picture, turned to the head
        // now (camera.cpp), is the game's camera all the same: g_cutscene, not "unturned", decides.
        // The picture's body (below), the latest picture's, read first: the aim starts its look yaw from it.
        static float s_body = 0.0f, s_heldYaw = 0.0f;
        static bool s_haveBody = false;
        static int s_viewMode = -1;  // the last frame's ViewMode
        // Started again (a recenter, a mode switch: ResetEyes) or without the view turn (the head aiming): a body
        // from before is stale, so the next look starts from the game's camera, as before.
        if (g_bodyStale || !useHand) {
            s_haveBody = false;
            s_viewMode = -1;
            g_bodyStale = false;
        }
        float shownCamera = 0.0f, shownTaken = 0.0f;
        const bool shown = camera::ShownTurn(shownCamera, shownTaken);
        if (s_viewMode == kViewFollow && shown) {
            s_body = std::remainder(shownCamera + s_heldYaw, 2 * kPi);
            s_haveBody = true;
        }
        const float stick = input::TakeTurn();
        if (g_cutscene) {
            aim::Forget();
            g_turnDirect = false;
        } else {
            g_turnDirect = aim::Update(yaw, pitch, stick, s_haveBody ? &s_body : nullptr);
        }
        // The bare arms of The Great Destroyer follow the controllers only once the game has left the
        // view to the player for 3 s: no cutscene picture, no scripted turn
        // (the user: free hands, but not in the interrogation and its QTEs).
        {
            static ULONGLONG s_calmSince = 0;
            const ULONGLONG now = GetTickCount64();
            // Without waiting for the aim: the free arms came 5-10 s after a load, the game's own arms shown
            // meanwhile (the user, 16:08), and on the moving train its search failed for minutes (15:55).
            // Cutscenes and scripted turns still switch them off, which keeps them out of the QTEs.
            // Half a second after a cutscene's picture or a load (the game's own hands showed 3 s longer:
            // "it's not right away", 16:25); 3 s after a scripted turn, which the QTE fight is made of.
            static bool s_afterScripted = false;
            const bool calm = !g_cutscene && !aim::Scripted();
            if (aim::Scripted())
                s_afterScripted = true;
            else if (g_cutscene)
                s_afterScripted = false;
            if (!calm)
                s_calmSince = 0;
            else if (!s_calmSince)
                s_calmSince = now;
            const bool allowed = calm && now - s_calmSince > (s_afterScripted ? 3000u : 500u);
            skin::SetBareHandsAllowed(allowed);
        }
        // The picture takes the hand's turn back out of the game's camera, which follows the hand
        // only while the look angles are written. Until they are (a new level, a checkpoint), the
        // hand's turn as it last was is taken out, so moving the gun doesn't move the view (it did,
        // user 2026-10-30) and the head still turns it.
        static float s_handYaw = 0.0f;
        if (g_turnDirect || !useHand)
            s_handYaw = yaw;
        const float viewYaw = s_handYaw;
        // While mouse aim holds its corrections for the search's nudge of a candidate (aim::Nudging), the game's
        // facing keeps the stick's turn but not the hand's: it stays where the hand was as the nudge began. The
        // walk is taken from there, as it was while mouse aim paused outright; the picture takes out the hand's
        // turn as it is (viewYaw, LookYaw).
        static float s_facingYaw = 0.0f;
        if (!aim::Nudging())
            s_facingYaw = viewYaw;
        // One body yaw for the picture, the game yaw the room's straight ahead shows (camera.cpp: shown = body -
        // head), kept from frame to frame. With the look yaw written (or steered by the mouse) it is the look yaw
        // plus the hand's, whatever the game's camera did, and aim.cpp starts the look yaw from it wherever it
        // starts again. Without (a scripted moment, a cutscene, mouse aim waiting) the picture follows the game's
        // camera from the yaw the last picture took out of it, so the hand-over doesn't move it: each one did by
        // the gap between the camera and the yaw aimed for (up to 82 degrees in The Great Destroyer, 18:31:16) or
        // by the hand's turn while frozen. At a scripted moment from the hand's, as before: the game's turn shows.
        float lookYaw = 0.0f, lookSign = -1.0f;
        const bool look = useHand && g_turnDirect && aim::LookYaw(lookYaw, lookSign);
        if (look) {
            s_body = std::remainder(lookYaw + viewYaw, 2 * kPi);
            s_viewMode = kViewLook;
        } else {
            if (s_viewMode != kViewFollow)
                s_heldYaw = aim::Scripted() || !shown || s_viewMode < 0 ? viewYaw : shownTaken;
            // A scripted camera's picture takes no yaw out of it (camera.cpp: the room's straight ahead shows its own
            // yaw), so the body and the walk go by that, and the player's camera, should it come back before the look
            // does, carries on from it. Held from the hand as the follow began, the body was off by the hand's yaw,
            // and the look after the cutscene would have started from there. Going into one, the picture moves once by
            // the hand's yaw it was held with (the car's forward put at the room's), at its first scripted picture, not
            // at the game's own cut: in the drive the cut into the car (00:35:03.277) came 16 pictures of the player's
            // kind of camera before it (the last at .499), so the move shows a quarter of a second after the cut. Back
            // to the player's camera before the look, nothing moves (the held yaw stays 0); in the interrogation it's
            // once at each hand-over from the look.
            if (g_scriptedLogged && shown)
                s_heldYaw = shownTaken;
            if (shown)
                s_body = std::remainder(shownCamera + s_heldYaw, 2 * kPi);
            s_viewMode = kViewFollow;
        }
        s_haveBody = useHand && (s_haveBody || look || shown);
        if (!useHand)
            s_viewMode = -1;  // the head aiming, no view turn: started again once the hand aims
        // Walking goes where the head faces, not where the gun points (user, 2026-10-30): the game
        // walks along its own facing, which is the hand's yaw the view turn takes out while the look is
        // written, and the yaw held while the picture follows the game's camera.
        if (useHand) {
            float headYaw = 0.0f, headPitch = 0.0f;
            LookAngles(facing, headYaw, headPitch);
            input::SetMoveTurn(std::remainder(headYaw - (look ? s_facingYaw : s_heldYaw), 2 * kPi));
        } else {
            input::SetMoveTurn(0.0f);
        }
        if (useHand) {
            camera::ViewTurn turn{{head.x, head.y, head.z, head.w}, s_heldYaw};
            turn.smoothHead[0] = facing.x, turn.smoothHead[1] = facing.y, turn.smoothHead[2] = facing.z, turn.smoothHead[3] = facing.w;
            turn.currentYaw = viewYaw;
            g_viewTurnOn = true;
            // A cutscene's picture is kept still where the hand faced (the Views switch above, next frame), as
            // before 2026-10-31 night. Kept where the last picture showed the game's camera instead, the
            // interrogation's QTE fight was "completely upside down" (the user, 20:50; the cause isn't known yet:
            // camera.cpp logs the refused cameras' up and forward to tell).
            g_viewTurnYaw = s_facingYaw;
            turn.haveLook = look;
            turn.lookYaw = lookYaw;
            turn.lookSign = lookSign;
            turn.nudging = aim::Nudging();
            // The head's place, for the game's cameras (headmove.h, where they're built), with this frame's turn.
            turn.headPlace[0] = (a.x + b.x) / 2;
            turn.headPlace[1] = (a.y + b.y) / 2;
            turn.headPlace[2] = (a.z + b.z) / 2;
            constexpr XrViewStateFlags kPlaceKnown = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_POSITION_TRACKED_BIT;
            // (A replayed head's place is known as it is made: it goes into the cameras as a tracked one does, headless too.)
            turn.placeTracked = g_replay.thisFrame || (!headless && (viewState.viewStateFlags & kPlaceKnown) == kPlaceKnown);
            turn.facingYaw = look ? s_facingYaw : s_heldYaw;
            turn.scriptedHint = g_cutscene || aim::Scripted();
            turn.poseId = ++g_poseSeq;
            g_poseById[g_poseSeq % kPoseRing][kLeftEye] = views[kLeftEye].pose;
            g_poseById[g_poseSeq % kPoseRing][kRightEye] = views[kRightEye].pose;
            camera::SetViewTurn(&turn);
            // The picture's turn: the look yaw written against the controller's (camera.cpp).
            if (turn.haveLook)
                input::NoteTurnStep(1, turn.lookSign * turn.lookYaw - yaw);
            // Pictures turned from the game camera's own yaw (the hand's frozen) instead of the aim's: each hand-
            // over between the two moves the picture by the gap between them (the interrogation room's
            // "teleporting", 2026-10-31). Counted outside cutscenes (a scripted camera's picture takes nothing out),
            // logged once a second when there were any.
            {
                static int s_own = 0, s_pictures = 0, s_switches = 0;
                static bool s_lastLook = true;
                static ULONGLONG s_countedAt = 0;
                if (!g_cutscene) {
                    ++s_pictures;
                    s_own += turn.haveLook ? 0 : 1;
                    s_switches += turn.haveLook != s_lastLook ? 1 : 0;
                    s_lastLook = turn.haveLook;
                }
                const ULONGLONG now = GetTickCount64();
                if (now - s_countedAt >= 1000) {
                    if (s_own)
                        Log("View: %d of the last second's %d pictures turned from the game camera's own yaw, not the "
                            "aim's (%d hand-overs%s)",
                            s_own, s_pictures, s_switches, aim::Scripted() ? "; a scripted moment" : "");
                    s_countedAt = now;
                    s_own = s_pictures = s_switches = 0;
                }
            }
        } else {
            camera::SetViewTurn(nullptr);
            g_viewTurnOn = false;
        }
    }
    // Free hands: a cutscene or a scripted moment (as the bare arms' calm above, this frame's aim::Update done): nothing changes hands
    // meanwhile. Each frame, with or without head tracking; hands::Update takes it next frame.
    hands::SetScripted(g_cutscene || aim::Scripted());
    input::SetScripted(g_cutscene, aim::Scripted(), aim::ScriptedArmed());  // (the quick-time events on the grips: qte.h)
    g_poseNewest = (g_poseNewest + 1) % kPoseHistory;
    g_poseHistory[g_poseNewest][kLeftEye] = views[kLeftEye].pose;
    g_poseHistory[g_poseNewest][kRightEye] = views[kRightEye].pose;
    g_posesKept = std::min(g_posesKept + 1, kPoseHistory);

    // Both eyes are needed for a projection layer; the very first frame shows nothing.
    if (headless || !g_eyeValid[kLeftEye] || !g_eyeValid[kRightEye])
        return;
    BuildProjection(out);
}

// Where each frame's time goes, logged once a second: the graphics card sat at 37% while the game
// ran at 55 fps, so the frame is waiting somewhere else.
struct FrameTiming {
    LARGE_INTEGER frequency{}, lastFrame{};
    double frame = 0, wait = 0, build = 0, end = 0;
    int frames = 0;
    ULONGLONG lastLog = 0;
    double Ms(const LARGE_INTEGER& a, const LARGE_INTEGER& b) const {
        return 1000.0 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(frequency.QuadPart);
    }
} g_timing;

// Handing a frame to SteamVR (xrEndFrame) took about 7 ms of every frame, even with nothing to
// show, while the game waited. With [stereo] async_handover=1 it happens on a thread of its own
// as the game makes its next frame, which waits for it only before starting the next frame's
// OpenXR calls. SteamVR then uses the game's graphics device from that thread too, so the device
// is switched to guarding itself against two threads.
struct Handover {
    std::thread thread;
    std::mutex mutex;
    std::condition_variable cv;
    bool started = false, pending = false, busy = false;
    FrameLayers layers;
    XrTime displayTime = 0;
    double ms = 0.0;
    int frames = 0;
} g_handover;

// The layers point into themselves: a copy must point into the copy.
void CopyLayers(const FrameLayers& from, FrameLayers& to) {
    to = from;
    to.projection.views = to.views;
    if (from.count && from.list[0] == reinterpret_cast<const XrCompositionLayerBaseHeader*>(&from.projection))
        to.list[0] = reinterpret_cast<XrCompositionLayerBaseHeader*>(&to.projection);
    else if (from.count)
        to.list[0] = reinterpret_cast<XrCompositionLayerBaseHeader*>(&to.quad);
}

void HandoverThread() {
    std::unique_lock lock(g_handover.mutex);
    for (;;) {
        g_handover.cv.wait(lock, [] { return g_handover.pending; });
        g_handover.pending = false;
        g_handover.busy = true;
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = g_handover.displayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = g_handover.layers.count;
        endInfo.layers = g_handover.layers.list;
        lock.unlock();
        LARGE_INTEGER a, b;
        QueryPerformanceCounter(&a);
        Check(xrEndFrame(g_session, &endInfo), "xrEndFrame");
        QueryPerformanceCounter(&b);
        lock.lock();
        g_handover.busy = false;
        g_handover.ms += g_timing.Ms(a, b);
        ++g_handover.frames;
        g_handover.cv.notify_all();
    }
}

void WaitHandover() {
    if (!g_handover.started)
        return;
    std::unique_lock lock(g_handover.mutex);
    g_handover.cv.wait(lock, [] { return !g_handover.pending && !g_handover.busy; });
}

void StartHandover(const FrameLayers& layers, XrTime displayTime) {
    if (!g_handover.started) {
        ID3D11Multithread* guard = nullptr;
        if (SUCCEEDED(g_device->QueryInterface(IID_PPV_ARGS(&guard)))) {
            guard->SetMultithreadProtected(TRUE);
            guard->Release();
        }
        g_handover.thread = std::thread(HandoverThread);
        g_handover.thread.detach();  // lives as long as the game
        g_handover.started = true;
        Log("Handover: xrEndFrame now runs on its own thread (async_handover=1)");
    }
    std::lock_guard lock(g_handover.mutex);
    CopyLayers(layers, g_handover.layers);
    g_handover.displayTime = displayTime;
    g_handover.pending = true;
    g_handover.cv.notify_all();
}

void RenderFrame(IDXGISwapChain* swap) {
    LARGE_INTEGER t0, t1, t2, t3;
    if (!g_timing.frequency.QuadPart)
        QueryPerformanceFrequency(&g_timing.frequency);
    QueryPerformanceCounter(&t0);
    if (g_timing.lastFrame.QuadPart)
        g_timing.frame += g_timing.Ms(g_timing.lastFrame, t0);
    g_timing.lastFrame = t0;

    ID3D11Texture2D* back = nullptr;
    if (FAILED(swap->GetBuffer(0, IID_PPV_ARGS(&back))))
        return;
    D3D11_TEXTURE2D_DESC desc;
    back->GetDesc(&desc);

    if (g_resizePending || desc.Width != g_scWidth || desc.Height != g_scHeight || desc.Format != g_scSourceFormat) {
        DestroySwapchains();
        CreateSwapchains(desc);
        g_resizePending = false;
    }

    // The last frame's handover (async_handover) must be done before this frame begins.
    const bool async = config::Get().asyncHandover;
    launchreport::Inside(launchreport::kHandingOver);  // where the drawing thread is, for the launch report's watch
    WaitHandover();
    // Blocks until the headset wants a new frame, so the game is paced to the display.
    XrFrameState frame{XR_TYPE_FRAME_STATE};
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    launchreport::Inside(launchreport::kWaitingForHeadset);
    const bool begun = Check(xrWaitFrame(g_session, &waitInfo, &frame), "xrWaitFrame") &&
                       Check(xrBeginFrame(g_session, &beginInfo), "xrBeginFrame");
    launchreport::Inside(launchreport::kInGame);
    if (!begun) {
        input::LetGo();
        back->Release();
        return;
    }
    QueryPerformanceCounter(&t1);
    launchreport::Picture(back, god::PlayerClientSoldier() != 0);  // the launch report's step 8: in a level, the picture not black

    // How evenly the frames come, once a second: the time between coming out of xrWaitFrame, and the step of the display times the runtime gives. An average of 13.9 ms says nothing of these.
    {
        static LARGE_INTEGER s_lastOut{};
        static XrTime s_lastDisplay = 0;
        static double s_gapLow = 1e9, s_gapHigh = 0.0, s_stepLow = 1e9, s_stepHigh = 0.0;
        static uint32_t s_uneven = 0, s_doubled = 0;
        static ULONGLONG s_logged = 0;
        if (s_lastOut.QuadPart) {
            const double gap = g_timing.Ms(s_lastOut, t1), step = static_cast<double>(frame.predictedDisplayTime - s_lastDisplay) / 1e6;
            s_gapLow = std::min(s_gapLow, gap), s_gapHigh = std::max(s_gapHigh, gap);
            s_stepLow = std::min(s_stepLow, step), s_stepHigh = std::max(s_stepHigh, step);
            s_uneven += std::fabs(gap - 13.89) > 2.0;
            s_doubled += step > 20.0 || step < 8.0;
        }
        s_lastOut = t1;
        s_lastDisplay = frame.predictedDisplayTime;
        if (const ULONGLONG now = GetTickCount64(); now - s_logged >= 1000) {
            s_logged = now;
            Log("Pace: out of xrWaitFrame every %.1f to %.1f ms (%u frames over 2 ms off), the display times %.1f to %.1f ms apart (%u off by a frame)", s_gapLow, s_gapHigh, s_uneven, s_stepLow, s_stepHigh, s_doubled);
            s_gapLow = s_stepLow = 1e9;
            s_gapHigh = s_stepHigh = 0.0;
            s_uneven = s_doubled = 0;
        }
    }

    // Frames asked for again (the headset back on): the head's place starts afresh, as with a recenter, this height standing. The
    // room stays as it was, and so do the aim and the hands.
    static bool s_asked = false;
    if (frame.shouldRender && !s_asked)
        headmove::NoteRecentered();
    s_asked = frame.shouldRender;

    // The runtime's own recenter (spacechange.h): the mod's own then too, once the runtime's poses are in its moved space, so the two never add up.
    if (spacechange::Due(g_spaceChange, frame.predictedDisplayTime)) {
        g_recenterPending = true;
        Log("Recenter: after the VR runtime's own recenter");
    }
    if (g_recenterPending)
        Recenter(frame.predictedDisplayTime);
    const XrTime locatedFor = frame.predictedDisplayTime + static_cast<XrTime>(camera::PoseTry(2) * 1e6f);  // weapon.txt "poseahead": camera.h
    g_frameShownAt = frame.predictedDisplayTime;  // (the head replay's clock)
    PollHeadTest();
    input::Sync(g_session, g_space, locatedFor);
    // The headset not asking for frames, with the stand-in hand on: the frame's work goes on
    // headless (BuildStereo), for testing with nobody in the headset. weapon.txt "forceheadless 1" does
    // the same while it asks (awake on a desk), so a film's head looks where "standin" says.
    const bool headless = (!frame.shouldRender || g_forceHeadless.load(std::memory_order_relaxed)) && g_scWidth && input::StandInOn() &&
                          config::Get().mode == config::Mode::Stereo && g_stereoTest == kTestViews;
    input::UpdateTurn(g_turnDirect && (frame.shouldRender || headless) && config::Get().mode == config::Mode::Stereo &&
                      config::Get().headTracking);
    depth3d::Probe();

    FrameLayers layers;
    if (frame.shouldRender && g_scWidth && !headless) {
        if (config::Get().mode == config::Mode::Stereo)
            BuildStereo(back, desc, locatedFor, layers);
        else
            BuildTheater(back, desc, layers);
    } else if (g_scWidth && config::Get().mode == config::Mode::Stereo && g_stereoTest == kTestDepth) {
        PreviewDepth3D(back, desc);
    } else if (headless) {
        BuildStereo(back, desc, locatedFor, layers, true);
        layers.count = 0;
    } else if (g_scWidth && config::Get().mode == config::Mode::Stereo && g_stereoTest == kTestViews) {
        depth3d::SaveIfAsked(g_device, back, L"eye_both.bmp");  // with the headset idle too
    }
    gputime::Stamp(gputime::kSubmitted);

    QueryPerformanceCounter(&t2);
    if (async) {
        StartHandover(layers, frame.predictedDisplayTime);
    } else {
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frame.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = layers.count;
        endInfo.layers = layers.list;
        launchreport::Inside(launchreport::kHandingOver);
        Check(xrEndFrame(g_session, &endInfo), "xrEndFrame");
        launchreport::Inside(launchreport::kInGame);
    }
    if (layers.count)
        launchreport::HeadsetFrame();

    // The window, and OBS's game capture with it: one eye's view instead of both side by side, now the headset has
    // the frame (streamview.cpp). From where the left eye's picture was drawn for, steadied; with the headset
    // idle, the middle of the picture. Its field of view is one eye's half whether or not the headset had the frame
    // (g_sideBySide is set only when it did: idle, the eye was taken for the whole window and the view came out
    // twice as wide as it should, "it looks stretched", the user, 2026-11-01).
    if (config::Get().mode == config::Mode::Stereo && g_stereoTest == kTestViews && views::Active() && g_scWidth) {
        const bool posed = frame.shouldRender && g_sideBySide && g_eyeValid[kLeftEye];
        streamview::Draw(g_device, back, posed ? &g_eyePose[kLeftEye].orientation : nullptr, GameFov(true));
    }

    back->Release();
    QueryPerformanceCounter(&t3);
    g_timing.wait += g_timing.Ms(t0, t1);
    g_timing.build += g_timing.Ms(t1, t2);
    g_timing.end += g_timing.Ms(t2, t3);
    ++g_timing.frames;
    ULONGLONG now = GetTickCount64();
    if (now - g_timing.lastLog >= 1000 && g_timing.frames) {
        double n = g_timing.frames;
        Log("Frame timing (ms per frame): whole frame %.1f, of it waiting for the headset %.1f, our copy %.1f, "
            "handing over %.1f; %d frames",
            g_timing.frame / n, g_timing.wait / n, g_timing.build / n, g_timing.end / n, g_timing.frames);
        perfinfo::LogStats();
        if (async) {
            std::lock_guard lock(g_handover.mutex);
            if (g_handover.frames)
                Log("Handover: xrEndFrame took %.1f ms on its own thread", g_handover.ms / g_handover.frames);
            g_handover.ms = 0.0;
            g_handover.frames = 0;
        }
        g_timing = FrameTiming{g_timing.frequency, g_timing.lastFrame};
        g_timing.lastLog = now;
    }
}

}

namespace vr {

void SetHalfEye(bool on) {
    if (g_halfEye.exchange(on) != on)
        Log("Headset: the left eye %s", on ? "at half rate, every other frame the one before (a test)" : "at full rate");
}

void SetForceHeadless(bool on) {
    if (g_forceHeadless.exchange(on) != on)
        Log("Headset: %s", on ? "frames worked headless while the stand-in is on, the headset awake or not (a test: nothing handed to it)"
                              : "frames handed to the headset again whenever it asks");
}

void OnPresent(IDXGISwapChain* swap) {
    gputime::Stamp(gputime::kHudEnd);
    overlay::FinishFrame(swap);  // the HUD's done: the enemies' orange markers come off it
    gputime::Stamp(gputime::kKeyDone);
    overlay::NewFrame();         // the next frame's eye pictures follow
    // Hotkeys and the camera tools work without a headset.
    PollHotkeys();
    {
        // Per-eye views: two views from BF3, in a window twice as wide (each eye a square half).
        bool perEye = config::Get().mode == config::Mode::Stereo && g_stereoTest == kTestViews;
        views::Update(perEye);
        window::SetWide(perEye);
        // The window fitted to the monitor in the shape of the picture it shows: the stream view's while it's drawn
        // (on, and the headset's frame loop running, which draws it); else both eyes side by side, 2:1 (the review:
        // without SteamVR, the 2:1 picture was squeezed into a 16:9 window).
        const bool streamDrawn = perEye && streamview::On() && g_running && !g_failed && g_scWidth;
        window::SetShape(streamDrawn ? streamview::Aspect() : 0.0f);
        // The game's own fullscreen leaves its window to it (window.cpp); looked at twice a second.
        static int s_fullscreenCheck = 0;
        if (++s_fullscreenCheck >= 36) {
            s_fullscreenCheck = 0;
            BOOL fullscreen = FALSE;
            if (SUCCEEDED(swap->GetFullscreenState(&fullscreen, nullptr)))
                window::SetGameFullscreen(fullscreen != FALSE);
        }
    }
    window::Maintain();
    fov::Apply();
    god::Apply();
    bodies::Apply();
    pad::KeepGlobalInput();  // (the game's pad read with its window in the background: the stick walks behind another window; a look a second)
    // The game's dropped guns (pickups.h), every frame, headset or not: a swap is told to ammo before ammo meets the new gun.
    pickups::OnFrame();
    ammo::Update();
    input::UpdateM320();
    input::WatchPose();  // (the soldier's pose from the game, headset or not: the stick's stance acts on it, and the stance for the floor below)
    footsteps::OnFrame();
    // The game's own ground for things let go of (ground.h): its eye and stance for the feet and the probe, then its frame (hooks once asked, the probe, the log).
    if (float eye[3]; skin::GameEyeWorld(eye))
        ground::SetEye(eye, input::StanceIndex());
    ground::OnFrame();
    skin::Update();
    skin::Probe();
    camera::Probe();
    aim::Probe();

    if (g_failed || !TryInit(swap)) {
        input::LetGo();
        return;
    }
    PollEvents();
    if (g_running)
        RenderFrame(swap);
    else
        input::LetGo();
}

void OnResize() {
    g_resizePending = true;
}

}
