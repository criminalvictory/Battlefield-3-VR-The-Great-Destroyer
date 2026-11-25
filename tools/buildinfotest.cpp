// The DLL's own build read back, without the game: tools/buildinfo.cmake writes build\generated\buildinfo.h at every build (the commit the source is at,
// its date, whether it had changes not committed), src/bf3vr.rc puts it in bf3vr.dll's version resource, and tools\launch_vr.ps1 and tools\report.ps1 read
// it from the DLL itself ([Diagnostics.FileVersionInfo]'s ProductVersion), never git's latest commit, which a source changed after the build is ahead of.
// Each check prints PASS, FAIL or SKIP with what it read; the exit code is the number failed.
//   b1  this build's name (buildinfo.h): "commit <40 hex> of <yyyy-mm-dd>[, with changes not committed]", or "commit not known" (no git); its short commit
//       the long one's first seven; nothing in it but the commit, its date and the flag (no path, no name)
//   b2  the DLL's version resource: its ProductVersion the launcher's and the report's pattern takes, its description and file name the mod's (SKIP for a
//       DLL from before the builds named their commit: no version resource at all)
//   b3  ... the very build these tests are (SKIP for a DLL older than them: tools\make.bat builds the tests alone while the game has the DLL locked)
//   b4  the launcher's and the report's own readers (launch_vr.ps1's DllBuild, report.ps1's RepDllBuild: taken out of the scripts by PowerShell's parser,
//       nothing else of them run) in Windows PowerShell, as the .bat files run them: each names the DLL's commit (SKIP without powershell.exe)
// Usage: buildinfotest [<bf3vr.dll>] (build\bf3vr.dll by default, from the project's folder: tools\ beside it). Reads only.

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include "buildinfo.h"

namespace {

int g_checks = 0, g_failed = 0, g_skipped = 0;
void Check(bool ok, const std::string& what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
}
void Skip(const std::string& what) {
    ++g_skipped;
    std::printf("SKIP %s\n", what.c_str());
}

// The launcher's and the report's pattern for the DLL's ProductVersion (tools\launch_vr.ps1 DllBuild, tools\report.ps1 RepDllBuild).
const std::regex kPattern("^commit ([0-9a-f]{7,40})(?: of ([0-9]{4}-[0-9]{2}-[0-9]{2}))?(, with changes not committed)?$");

std::string Narrow(const wchar_t* w) {
    std::string s;
    for (; w && *w; ++w)
        s += *w < 128 ? static_cast<char>(*w) : '?';
    return s;
}

// A string of the DLL's version resource, by its first translation ("" none).
std::string VersionString(const std::vector<uint8_t>& info, const wchar_t* name) {
    struct Translation {
        WORD language, codePage;
    };
    Translation* t = nullptr;
    UINT bytes = 0;
    if (!VerQueryValueW(info.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&t), &bytes) || bytes < sizeof(Translation))
        return "";
    wchar_t path[128];
    swprintf_s(path, L"\\StringFileInfo\\%04x%04x\\%s", t->language, t->codePage, name);
    wchar_t* value = nullptr;
    UINT chars = 0;
    if (!VerQueryValueW(info.data(), path, reinterpret_cast<void**>(&value), &chars) || !value)
        return "";
    return Narrow(value);
}

// Windows PowerShell's answers: the launcher's DllBuild and the report's RepDllBuild (with its RepFileThere), each taken out of its script by the parser
// and run on the DLL, their words as the launcher says them. False: powershell.exe didn't run.
bool ReadBack(const std::string& dll, std::string& launcher, std::string& report) {
    const std::string script =
        "$ErrorActionPreference = 'Stop'\n"
        "function Take([string]$file, [string[]]$names) {\n"
        "  $t = $null; $e = $null\n"
        "  $ast = [System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path $file).Path, [ref]$t, [ref]$e)\n"
        "  @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in $names }, $true) |\n"
        "    ForEach-Object { $_.Extent.Text }) -join \"`n\" }\n"
        ". ([scriptblock]::Create((Take 'tools\\launch_vr.ps1' @('DllBuild')) + \"`n\" + (Take 'tools\\report.ps1' @('RepDllBuild', 'RepFileThere'))))\n"
        "$path = [IO.Path]::GetFullPath('" + dll + "')\n"
        "$b = DllBuild $path\n"
        "'launcher=' + $(if ($b) { 'commit ' + $b.Short + $(if ($b.Date) { ' of ' + $b.Date }) + $(if ($b.Dirty) { ', with changes not committed' }) } else { 'none' })\n"
        "'report=' + (RepDllBuild $path)\n";
    // -EncodedCommand: the script as UTF-16 in base 64 (no quoting through the command line).
    std::string utf16;
    for (char c : script)
        utf16 += c, utf16 += '\0';
    static const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    for (size_t i = 0; i < utf16.size(); i += 3) {
        const unsigned n = (static_cast<uint8_t>(utf16[i]) << 16) | (i + 1 < utf16.size() ? static_cast<uint8_t>(utf16[i + 1]) << 8 : 0) |
                           (i + 2 < utf16.size() ? static_cast<uint8_t>(utf16[i + 2]) : 0);
        b64 += kB64[(n >> 18) & 63], b64 += kB64[(n >> 12) & 63];
        b64 += i + 1 < utf16.size() ? kB64[(n >> 6) & 63] : '=';
        b64 += i + 2 < utf16.size() ? kB64[n & 63] : '=';
    }
    std::wstring command = L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand " + std::wstring(b64.begin(), b64.end());
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        return false;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = writePipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> line(command.begin(), command.end());
    line.push_back(0);
    const BOOL started = CreateProcessW(nullptr, line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);
    if (!started) {
        CloseHandle(readPipe);
        return false;
    }
    std::string out;
    char buffer[4096];
    DWORD got = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &got, nullptr) && got)
        out.append(buffer, got);
    WaitForSingleObject(pi.hProcess, 60000);
    CloseHandle(pi.hProcess), CloseHandle(pi.hThread), CloseHandle(readPipe);
    auto value = [&](const char* key) {
        const size_t at = out.find(key);
        if (at == std::string::npos)
            return std::string("(not said: ") + out.substr(0, 300) + ")";
        const size_t end = out.find_first_of("\r\n", at);
        return out.substr(at + std::strlen(key), end == std::string::npos ? std::string::npos : end - at - std::strlen(key));
    };
    launcher = value("launcher="), report = value("report=");
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dll = argc > 1 ? argv[1] : "build\\bf3vr.dll";
    const std::string built = BF3VR_BUILD_TEXT, commit = BF3VR_COMMIT, shortCommit = BF3VR_COMMIT_SHORT, date = BF3VR_COMMIT_DATE;
    const bool dirty = BF3VR_COMMIT_DIRTY != 0;

    // b1: this build's own name.
    {
        std::smatch m;
        const bool known = !commit.empty();
        const bool matched = std::regex_match(built, m, kPattern);
        const bool ok = known ? matched && m[1].str() == commit && commit.size() == 40 && shortCommit == commit.substr(0, 7) && m[2].str() == date &&
                                    (m[3].matched == dirty)
                              : built.rfind("commit not known", 0) == 0 && shortCommit.empty() && date.empty() && !dirty;
        const bool clean = built.find('\\') == std::string::npos && built.find('/') == std::string::npos && built.find(':') == std::string::npos;
        Check(ok && clean, "b1 this build's name (build\\generated\\buildinfo.h): \"" + built + "\"" + (known ? "" : " (no git where it was built)") +
                               (dirty ? "; the source had changes not committed" : "") + "; no path or name in it");
    }

    // b2, b3: the DLL's.
    std::wstring wide(dll.begin(), dll.end());
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(wide.c_str(), &handle);
    std::vector<uint8_t> info(size ? size : 1);
    if (!size || !GetFileVersionInfoW(wide.c_str(), 0, size, info.data())) {
        if (GetFileAttributesW(wide.c_str()) == INVALID_FILE_ATTRIBUTES)
            Skip("b2 no " + dll + " (run from the project's folder, after a build): the DLL's own name");
        else
            Skip("b2 " + dll + " has no version resource: a DLL built before the builds named their commit (built again, it has)");
    } else {
        const std::string product = VersionString(info, L"ProductVersion"), description = VersionString(info, L"FileDescription"),
                          file = VersionString(info, L"OriginalFilename"), version = VersionString(info, L"FileVersion");
        std::smatch m;
        const bool matched = std::regex_match(product, m, kPattern) || product.rfind("commit not known", 0) == 0;
        // (Its FileVersion string too: .NET's FileVersionInfo, the launcher's and the report's reader, takes a string block as the file's only if it
        // holds one; without it every field the scripts read came back empty.)
        Check(matched && description == "BF3 VR mod" && file == "bf3vr.dll" && version == product,
              "b2 " + dll + "'s version resource: ProductVersion \"" + product + "\" (the launcher's and the report's pattern takes it: " + (matched ? "yes" : "NO") +
                  "), FileVersion the same (.NET reads the block only with one: " + (version == product ? "yes" : "NO") + "), FileDescription \"" + description +
                  "\", OriginalFilename \"" + file + "\"");
        if (product == built)
            Check(true, "b3 ... this very build's (" + (shortCommit.empty() ? std::string("its commit not known") : "commit " + shortCommit) + ")");
        else
            Skip("b3 ... not this build's (\"" + product + "\", these tests' \"" + built + "\"): a DLL older than the tests (built while the game had it locked?)");
        // b4: the launcher's and the report's own readers, out of tools\launch_vr.ps1 and tools\report.ps1 (nothing else of them run), in Windows
        // PowerShell as "Start BF3 VR.bat" and "Make a report.bat" run them: each names the DLL's commit, short.
        if (matched && m[1].matched) {
            std::string want = "commit " + m[1].str().substr(0, 7);
            if (m[2].matched)
                want += " of " + m[2].str();
            if (m[3].matched)
                want += m[3].str();
            std::string launcher, report;
            const bool ran = ReadBack(dll, launcher, report);
            if (!ran)
                Skip("b4 Windows PowerShell didn't run (powershell.exe): the launcher's and the report's readers");
            else
                Check(launcher == want && report == want, "b4 the launcher's reader (launch_vr.ps1 DllBuild) says \"" + launcher + "\", the report's (report.ps1 RepDllBuild) \"" +
                                                              report + "\": \"" + want + "\" wanted");
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}
