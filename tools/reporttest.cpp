// The player's report (tools\report.ps1, "Make a report.bat") without the game: its pieces taken out of it by PowerShell's parser (nothing else of it
// run) and run in Windows PowerShell as the .bat runs it, on a made-up log written under %TEMP% (never the project's build\, where the report reads the
// live log). Each check prints PASS, FAIL or SKIP with what it read; the exit code is the number failed.
//   r1  report.ps1 parses
//   r2  every hosting line of the mod's (each Log("Host: ...") in src\*.cpp, its format filled in, after the log's time) is one of the report's key
//       lines: hosting a gun let go of is on by default, and a report could say that nothing was kept but never why (the integration review of
//       2026-11-25, finding 12: only the once-a-second line and one other matched, by other words in them)
//   r3  ... and the word alone elsewhere isn't: "localhost:" or "Host:" inside another line, a line without the log's time
//   r4  the key lines' section as the report makes it, from a long made-up log (the hosting's lines twice, in its middle, hundreds of other lines
//       round them): every hosting line in it, each kind once with its count ("(x2)": never shown before 2026-11-25, PowerShell's comma binding before
//       + kept every count at 1), none of the others
//   r5  ... and from a log of many kinds: every kind up to 200, past that the first 50 and the last 150 with a line saying how many are left out between
//       (the last 150 alone crowded out the start's kinds, the picture's, the controllers', the view's, one more for each hosting kind: the review of
//       the keys' lane, 2026-11-25)
// Usage: reporttest (from the project's folder: tools\report.ps1 and src\ beside it). Reads only; writes only its own folder under %TEMP%, removed after.

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

// The checks, in Windows PowerShell: the project's folder and the made-up log's folder as its arguments. Each check a "PASS id ..." or "FAIL id ..."
// line, "DONE" at its end.
const char kScript[] = R"PS(param([string]$root, [string]$tmp)
$ErrorActionPreference = 'Stop'
function Say([string]$id, [bool]$ok, [string]$what) { Write-Output ('{0} {1} {2}' -f $(if ($ok) { 'PASS' } else { 'FAIL' }), $id, $what) }
function Short([string]$line) { $t = $line -replace '^\[[^\]]*\] ', ''; if ($t.Length -gt 90) { $t.Substring(0, 90) + '...' } else { $t } }
$id = 'r1'
try {
    $tokens = $null; $errors = $null
    $ast = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'tools\report.ps1'), [ref]$tokens, [ref]$errors)
    Say 'r1' ($errors.Count -eq 0) ('tools\report.ps1 parses: {0} errors{1}' -f $errors.Count, $(if ($errors.Count) { ', the first: ' + $errors[0].Message + ' at line ' + $errors[0].Extent.StartLineNumber } else { '' }))
    # The key lines' pattern as the script holds it ('' unescaped by the parser), the section's own block, and the functions it calls.
    $id = 'r2'
    $assign = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -eq '$keys' -and $n.Right.Extent.Text -like '*Held gun*' }, $true) | Select-Object -First 1
    $call = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.CommandAst] -and $n.GetCommandName() -eq 'RepTry' -and $n.CommandElements[-1].Extent.Text -like '*key lines*' }, $true) | Select-Object -First 1
    $fns = @($ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in 'RepPrivate', 'RepCut', 'RepAdd', 'RepSection', 'RepTry' }, $true))
    if (-not $assign -or -not $call -or $fns.Count -ne 5) {
        Say 'r2' $false 'tools\report.ps1 has no key-line pattern ($keys = ''...Held gun...''), no section of its key lines, or not all of RepPrivate, RepCut, RepAdd, RepSection and RepTry'
        'DONE'
        exit 0
    }
    $keys = $assign.Right.Expression.Value

    # r2: the mod's hosting lines as it writes them: each Log("Host: ...") format in src\*.cpp (its first piece), filled in, after the log's time.
    $formats = New-Object System.Collections.Generic.List[string]
    foreach ($f in @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Filter '*.cpp')) {
        foreach ($m in [regex]::Matches([IO.File]::ReadAllText($f.FullName), 'Log\("(Host: (?:[^"\\]|\\.)*)"')) { $formats.Add($m.Groups[1].Value) }
    }
    $pct = [string][char]1
    $lines = @($formats | ForEach-Object {
        $t = $_.Replace('%%', $pct)
        $t = [regex]::Replace($t, '%[-+ #0]*\d*(?:\.\d+)?[fg]', '1.25')
        $t = [regex]::Replace($t, '%[-+ #0]*\d*(?:ll|l|h|z|I64)?[diux]', '5')
        $t = [regex]::Replace($t, '%[-+ #0]*\d*s', 'M4A1')
        '[12:10:00.000] ' + $t.Replace($pct, '%').Replace('\"', '"').Replace('\\', '\')
    })
    $missed = @($lines | Where-Object { -not [regex]::IsMatch($_, $keys) })
    Say 'r2' ($lines.Count -gt 0 -and $missed.Count -eq 0) ('the mod''s hosting lines (Log("Host: ...") in src\*.cpp, {0} of them, filled in after the log''s time): {1} not key lines{2}' -f
        $lines.Count, $missed.Count, $(if ($missed.Count) { ': ' + (($missed | ForEach-Object { Short $_ }) -join ' | ') } elseif (-not $lines.Count) { ' (none found: run from the project''s folder)' } else { '' }))

    # r3: the word elsewhere.
    $id = 'r3'
    $not = @('[12:00:00.000] Net: connecting to localhost: 27060', '[12:00:00.000] Net: the Host: field of a reply', 'Host: a line without the log''s time')
    $taken = @($not | Where-Object { [regex]::IsMatch($_, $keys) })
    Say 'r3' ($taken.Count -eq 0) ('lines with the word elsewhere ("localhost:", "Host:" inside a line, no log time): {0} of {1} taken{2}' -f $taken.Count, $not.Count,
        $(if ($taken.Count) { ': ' + ($taken -join ' | ') } else { '' }))

    # r4: the section itself (RepTry and its block, as the report runs them) on a made-up log in this test's own folder.
    $id = 'r4'
    foreach ($fn in $fns) { . ([scriptblock]::Create($fn.Extent.Text)) }
    $repBuild = $tmp
    $script:repNames = @()
    $script:repLines = New-Object System.Collections.Generic.List[string]
    $log = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt 100; $i++) { $log.Add(('[12:00:{0:00}.000] start filler {1}' -f ($i % 60), $i)) }
    foreach ($l in $lines) { $log.Add($l) }
    for ($i = 0; $i -lt 200; $i++) { $log.Add(('[12:15:{0:00}.000] middle filler {1}' -f ($i % 60), $i)) }
    foreach ($l in $lines) { $log.Add($l) }
    for ($i = 0; $i -lt 400; $i++) { $log.Add(('[12:20:{0:00}.000] later filler {1}' -f ($i % 60), $i)) }
    [IO.File]::WriteAllLines((Join-Path $tmp 'bf3vr.log'), $log, (New-Object Text.UTF8Encoding $false))
    RepTry ($call.CommandElements[1].ScriptBlock.GetScriptBlock()) 'the mod''s log''s key lines'
    $out = @($script:repLines)
    $kinds = @($lines | Sort-Object -Unique)
    $absent = @($kinds | Where-Object { $l = $_; -not @($out | Where-Object { $_.StartsWith('  ' + $l) }).Count })
    $counted = @($kinds | Where-Object { $l = $_; @($out | Where-Object { $_ -ceq ('  ' + $l + '   (x2)') }).Count -eq 1 })
    $filler = @($out | Where-Object { $_ -match 'filler' }).Count
    $failed = @($out | Where-Object { $_ -match 'couldn''t be read' })
    Say 'r4' ($kinds.Count -gt 0 -and $absent.Count -eq 0 -and $counted.Count -eq $kinds.Count -and $filler -eq 0 -and -not $failed.Count) (
        'the report''s key lines from a made-up log of {0} lines (the hosting''s {1} kinds twice, in its middle): {2} not in it{3}; {4} of {1} once with "(x2)"; other lines in it: {5}{6}' -f
        $log.Count, $kinds.Count, $absent.Count, $(if ($absent.Count) { ': ' + (($absent | ForEach-Object { Short $_ }) -join ' | ') } else { '' }), $counted.Count, $filler,
        $(if ($failed.Count) { '; ' + $failed[0] } else { '' }))

    # r5: a long log's many kinds (each a name of letters: digits are one kind): 260 of them, the first 50 and the last 150 in the section with a line for
    # the 60 between; 179, every one (the last 150 alone dropped the first 29 and 110: the start's kinds, the review of the keys' lane, 2026-11-25).
    $id = 'r5'
    function Letters([int]$n) { [string][char](97 + [int][math]::Floor($n / 26)) + [string][char](97 + ($n % 26)) }
    $res = New-Object System.Collections.Generic.List[string]
    $ok = $true
    foreach ($count in 260, 179) {
        $script:repLines = New-Object System.Collections.Generic.List[string]
        $log = New-Object System.Collections.Generic.List[string]
        for ($i = 0; $i -lt $count; $i++) { $log.Add(('[12:30:{0:00}.000] Held gun: kind {1} in the hands' -f ($i % 60), (Letters $i))) }
        [IO.File]::WriteAllLines((Join-Path $tmp 'bf3vr.log'), $log, (New-Object Text.UTF8Encoding $false))
        RepTry ($call.CommandElements[1].ScriptBlock.GetScriptBlock()) 'the mod''s log''s key lines'
        $out = @($script:repLines)
        $seen = @(for ($i = 0; $i -lt $count; $i++) { $tail = 'Held gun: kind ' + (Letters $i) + ' in the hands'; if (@($out | Where-Object { $_.EndsWith($tail) }).Count -eq 1) { $i } })
        $want = if ($count -gt 200) { @(0..49) + @(($count - 150)..($count - 1)) } else { @(0..($count - 1)) }
        $gaps = @($out | Where-Object { $_ -match '^\s+\.\.\. (\d+) kinds left out' } | ForEach-Object { [int]([regex]::Match($_, '(\d+) kinds').Groups[1].Value) })
        $gapWant = if ($count -gt 200) { @($count - 200) } else { @() }
        $fine = ($seen -join ',') -eq ($want -join ',') -and ($gaps -join ',') -eq ($gapWant -join ',')
        $ok = $ok -and $fine
        $res.Add(('{0} kinds: {1} shown{2}, left out between: {3}' -f $count, $seen.Count, $(if ($seen.Count) { ' (from ' + (Letters $seen[0]) + ')' } else { '' }),
            $(if ($gaps.Count) { $gaps -join ', ' } else { 'no line' })))
    }
    Say 'r5' $ok ('a long log''s kinds in the section (each kind once): ' + ($res -join '; ') + ' (the first 50 and the last 150 past 200, every one up to it)')
} catch {
    Say $id $false ('the check itself failed: ' + $_.Exception.Message)
}
'DONE'
)PS";

int g_failed = 0;

std::wstring Wide(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

// Runs the script in Windows PowerShell and returns what it printed (false: powershell.exe didn't start).
bool RunScript(const std::wstring& script, const std::wstring& root, const std::wstring& folder, std::string& out) {
    std::wstring command = L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" + script + L"\" \"" + root + L"\" \"" + folder + L"\"";
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
    if (GetFileAttributesW((root + L"\\tools\\report.ps1").c_str()) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW((root + L"\\src").c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::printf("FAIL r1 no tools\\report.ps1 or src\\ here: run from the project's folder\n1 checks failed\n");
        return 1;
    }
    // Its own folder under %TEMP% for the script and the made-up log.
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const std::wstring folder = std::wstring(temp) + L"bf3vr-reporttest-" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring script = folder + L"\\reporttest.ps1", log = folder + L"\\bf3vr.log";
    bool wrote = false;
    if (FILE* f = nullptr; _wfopen_s(&f, script.c_str(), L"wb") == 0 && f) {
        wrote = std::fwrite(kScript, 1, sizeof(kScript) - 1, f) == sizeof(kScript) - 1;
        std::fclose(f);
    }
    std::string out;
    const bool ran = wrote && RunScript(script, root, folder, out);
    DeleteFileW(log.c_str());
    DeleteFileW(script.c_str());
    RemoveDirectoryW(folder.c_str());
    if (!wrote) {
        std::printf("FAIL r1 the checks' script couldn't be written under %%TEMP%%\n1 checks failed\n");
        return 1;
    }
    if (!ran) {
        std::printf("SKIP r1-r5 Windows PowerShell didn't run (powershell.exe)\n0 checks failed\n");
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
