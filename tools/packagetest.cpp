// The release's played settings (tools\package.ps1's bf3vr.ini) without packing anything: its DevKeysLine and ShippedIni taken out of it by
// PowerShell's parser (nothing else of it run: no worktree, no build, no zip) and run in Windows PowerShell on made-up lines. Each check prints PASS,
// FAIL or SKIP with what it read; the exit code is the number failed.
//   p1  package.ps1 parses, with its DevKeysLine and ShippedIni
//   p2  DevKeysLine finds src\config.cpp's own comment on dev_keys (the default bf3vr.ini's line, read here on its own), which names no Page Up / Page Down
//   p3  ShippedIni on a played bf3vr.ini as the packing PC's held it on 2026-11-25 (its dev_keys line naming Page Up / Page Down: a player who follows it
//       turns on F12, Steam's screenshot key, and the rest of the developer's keys; the integration review of 2026-11-25, finding 13, and its review of the
//       keys' lane): that line shipped as src\config.cpp's, and nothing else changed but as before (the two lines on top, a line naming a folder of
//       this PC left out, god mode on)
//   p4  ... and on release\settings\bf3vr.ini (the GitHub workflow's): every line of it as it is, the two lines on top; on one without [god]: god mode added
// Usage: packagetest (from the project's folder: tools\package.ps1, src\ and release\ beside it). Reads only; writes only its own script under %TEMP%,
// removed after.

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

// The checks, in Windows PowerShell: the project's folder as its argument. Each check a "PASS id ..." or "FAIL id ..." line, "DONE" at its end.
const char kScript[] = R"PS(param([string]$root)
$ErrorActionPreference = 'Stop'
function Say([string]$id, [bool]$ok, [string]$what) { Write-Output ('{0} {1} {2}' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $id, $what) }
$pageWords = '(?i)Page ?Up|Page ?Down|PgUp|PgDn'
# The lines ShippedIni gives, its two on top and the last newline's empty piece off; $null when it doesn't end in a newline or has fewer.
function Body([string]$text) {
    $all = $text -split "`n"
    if ($all.Count -lt 3 -or $all[-1] -ne '') { return $null }
    if ($all.Count -eq 3) { return , @() }  # (2..1 would count down)
    , @($all[2..($all.Count - 2)])
}
function Same([object[]]$a, [object[]]$b) {
    if ($null -eq $a -or $null -eq $b -or $a.Count -ne $b.Count) { return $false }
    for ($i = 0; $i -lt $a.Count; $i++) { if (-not ([string]$a[$i] -ceq [string]$b[$i])) { return $false } }
    $true
}
$id = 'p1'
try {
    $tokens = $null; $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'tools\package.ps1'), [ref]$tokens, [ref]$errors)
    $fns = @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in 'DevKeysLine', 'ShippedIni' }, $true))
    Say 'p1' ($errors.Count -eq 0 -and $fns.Count -eq 2) ('tools\package.ps1 parses: {0} errors{1}; its DevKeysLine and ShippedIni: {2} of 2' -f $errors.Count,
        $(if ($errors.Count) { ', the first: ' + $errors[0].Message + ' at line ' + $errors[0].Extent.StartLineNumber } else { '' }), $fns.Count)
    if ($fns.Count -ne 2) { 'DONE'; exit 0 }
    foreach ($fn in $fns) { . ([scriptblock]::Create($fn.Extent.Text)) }

    # p2: src\config.cpp's own line (the string "; dev_keys=1: ...\n" of its default bf3vr.ini), read here by position, and DevKeysLine's.
    $id = 'p2'
    $code = [IO.File]::ReadAllText((Join-Path $root 'src\config.cpp'))
    $own = ''
    $at = $code.IndexOf('"; dev_keys=1:')
    if ($at -ge 0) { $end = $code.IndexOf('\n"', $at); if ($end -gt $at) { $own = $code.Substring($at + 1, $end - $at - 1) } }
    $found = DevKeysLine $code
    Say 'p2' ($own -ne '' -and $found -ceq $own -and $own -notmatch $pageWords) ('src\config.cpp''s line: "{0}"{1}; DevKeysLine: {2}' -f $own,
        $(if ($own -match $pageWords) { ' (NAMES Page Up / Page Down)' } else { '' }), $(if ($found -ceq $own) { 'the same' } elseif ($found) { '"' + $found + '"' } else { 'nothing' }))

    # p3: the packing PC's played bf3vr.ini as it was on 2026-11-25 (its line 6), with a folder of this PC in a line and god mode off.
    $id = 'p3'
    $old = '; dev_keys=1: the developer''s keys too (F1, F5-F7, F10-F12, Delete, Page Up/Down, Pause, Ctrl+F2/F3/F4, Ctrl+Scroll Lock).'
    $played = @('; bf3vr settings. Changes take effect next time the mod is loaded.', '', '[general]',
        '; theater = flat floating screen, stereo = per-eye projection (alternate-eye rendering)',
        '; F8 switches between them in game, F9 recenters (the mod''s keys count with the game''s window in front).', $old, 'mode=stereo', '',
        '[stereo]', 'render_fov=106', 'dump_to=C:\BF3 test\dump.bin', 'pixels_per_tangent=906.2271', '', '[god]', 'enabled=0', 'value=100000')
    $want = @('; bf3vr settings. Changes take effect next time the mod is loaded.', '', '[general]',
        '; theater = flat floating screen, stereo = per-eye projection (alternate-eye rendering)',
        '; F8 switches between them in game, F9 recenters (the mod''s keys count with the game''s window in front).', $own, 'mode=stereo', '',
        '[stereo]', 'render_fov=106', 'pixels_per_tangent=906.2271', '', '[god]', 'enabled=1', 'value=100000')
    $text = ShippedIni $played $own
    $body = Body $text
    $shippedDev = @($body | Where-Object { $_ -match '^\s*;\s*dev_keys\s*=\s*1\s*:' })
    $top = @($text -split "`n" | Select-Object -First 2)
    Say 'p3' ((Same $body $want) -and $shippedDev.Count -eq 1 -and $shippedDev[0] -ceq $own -and $top.Count -eq 2 -and $top[0].StartsWith('; ') -and $top[1].StartsWith('; ')) (
        'the PC''s played bf3vr.ini (its dev_keys line naming Page Up / Page Down): shipped "{0}"{1}; the rest {2}' -f $(if ($shippedDev.Count) { $shippedDev[0] } else { '(no dev_keys line)' }),
        $(if ($shippedDev.Count -and $shippedDev[0] -match $pageWords) { ' (NAMES Page Up / Page Down)' } else { '' }),
        $(if (Same $body $want) { 'as before (the folder''s line out, god mode on)' } elseif ($null -eq $body) { 'not ending in a newline' } else { 'NOT as expected: ' + (($body | Select-Object -First 20) -join ' | ') }))

    # p4: release\settings\bf3vr.ini (the GitHub workflow's: its line already the mod's own) through it: every line as it was; one without [god]: added.
    $id = 'p4'
    $release = [IO.File]::ReadAllLines((Join-Path $root 'release\settings\bf3vr.ini'))
    $relBody = Body (ShippedIni $release $own)
    $noGod = Body (ShippedIni @('[general]', 'mode=theater') $own)
    $godWant = @('[general]', 'mode=theater', '', '[god]', 'enabled=1', 'value=100000')
    Say 'p4' ($release.Count -gt 0 -and (Same $relBody $release) -and (Same $noGod $godWant)) ('release\settings\bf3vr.ini ({0} lines): {1}; a played one without [god]: {2}' -f $release.Count,
        $(if (Same $relBody $release) { 'every line as it is' } else { 'CHANGED' }), $(if (Same $noGod $godWant) { 'god mode added' } else { 'NOT as expected: ' + ($noGod -join ' | ') }))
} catch {
    Say $id $false ('the check itself failed: ' + $_.Exception.Message)
}
'DONE'
)PS";

int g_failed = 0;

// Runs the script in Windows PowerShell and returns what it printed (false: powershell.exe didn't start).
bool RunScript(const std::wstring& script, const std::wstring& root, std::string& out) {
    std::wstring command = L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" + script + L"\" \"" + root + L"\"";
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
    char buffer[4096];
    DWORD got = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &got, nullptr) && got)
        out.append(buffer, got);
    WaitForSingleObject(pi.hProcess, 120000);
    CloseHandle(pi.hProcess), CloseHandle(pi.hThread), CloseHandle(readPipe);
    return true;
}

}  // namespace

int main() {
    wchar_t cwd[MAX_PATH] = {};
    GetCurrentDirectoryW(MAX_PATH, cwd);
    const std::wstring root = cwd;
    for (const wchar_t* need : {L"\\tools\\package.ps1", L"\\src\\config.cpp", L"\\release\\settings\\bf3vr.ini"}) {
        if (GetFileAttributesW((root + need).c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::printf("FAIL p1 no tools\\package.ps1, src\\config.cpp or release\\settings\\bf3vr.ini here: run from the project's folder\n1 checks failed\n");
            return 1;
        }
    }
    // Its own folder under %TEMP% for the script.
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const std::wstring folder = std::wstring(temp) + L"bf3vr-packagetest-" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring script = folder + L"\\packagetest.ps1";
    bool wrote = false;
    if (FILE* f = nullptr; _wfopen_s(&f, script.c_str(), L"wb") == 0 && f) {
        wrote = std::fwrite(kScript, 1, sizeof(kScript) - 1, f) == sizeof(kScript) - 1;
        std::fclose(f);
    }
    std::string out;
    const bool ran = wrote && RunScript(script, root, out);
    DeleteFileW(script.c_str());
    RemoveDirectoryW(folder.c_str());
    if (!wrote) {
        std::printf("FAIL p1 the checks' script couldn't be written under %%TEMP%%\n1 checks failed\n");
        return 1;
    }
    if (!ran) {
        std::printf("SKIP p1-p4 Windows PowerShell didn't run (powershell.exe)\n0 checks failed\n");
        return 0;
    }
    int checks = 0;
    bool done = false;
    size_t at = 0;
    while (at < out.size()) {
        size_t end = out.find('\n', at);
        if (end == std::string::npos)
            end = out.size();
        std::string text = out.substr(at, end - at);
        at = end + 1;
        while (!text.empty() && (text.back() == '\r' || text.back() == ' '))
            text.pop_back();
        if (text.empty())
            continue;
        if (text == "DONE") {
            done = true;
            continue;
        }
        const bool pass = text.rfind("PASS ", 0) == 0, fail = text.rfind("FAIL ", 0) == 0;
        checks += pass || fail;
        g_failed += fail;
        std::printf("%s\n", text.c_str());
    }
    if (!done) {
        ++g_failed;
        std::printf("FAIL the checks' script didn't finish (no DONE)\n");
    }
    std::printf("%d of %d checks failed\n", g_failed, checks);
    return g_failed;
}
