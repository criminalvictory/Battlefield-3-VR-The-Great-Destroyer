// The campaign-only rule (src/campaign.h, header only, as injector/main.cpp and src/dllmain.cpp run it), the injector's reading of
// another process's command line (injector/cmdline.h) and the OpenXR runtime's choice (src/xrpick.h), without the game.
//   1  the rule: Battlelog's campaign start as the EA app's logs show it (made-up codes) taken, with its spellings; the multiplayer start
//      as they show it (-webMode MP, State_ClaimReservation, a server's gameid) refused, and every other way of not being the campaign
//      (co-op, another state, another level mode, a server named, no -webMode, none at all, an option only inside a quoted value or the
//      program's own folder); the word that decided it, and never a code or a token in it
//   2  the split: as the system's own CommandLineToArgvW splits the same lines (quotes, backslashes, tabs, the program's name)
//   3  a running process's command line read from outside as the injector reads it (its 32-bit PEB, by asking, and the two together):
//      every character as it was started with (the campaign's and the multiplayer's lines, letters outside ASCII, 30000 characters, the
//      program's name alone), its verdict as the line's own; a process that has ended: nothing read, no verdict but "can't tell"
//   4  the injector itself (only with "--injector <path>": its own build): --judge says campaign (exit 0), multiplayer (3) or can't
//      tell (1); waiting for a process by name, a multiplayer start is refused and waited past, nothing loaded; a campaign start is
//      judged so and loaded into (a stand-in "DLL" the system refuses, so nothing is, and the injector ends before it would ever let go
//      of keys); never a code in what it writes (a copy of it in a folder of its own under the temp folder, the targets copies of this
//      program)
//   5  the OpenXR runtime: SteamVR's 32-bit one preferred when it's there and the one the loader would take is another (Meta's, none),
//      left alone when it's SteamVR's already, when SteamVR's isn't there, or bf3vr.ini says not to; its file's name in any spelling
//   6  the time: a judgement of a 30000-character line well under a millisecond
//   7  the mod's sounds after an update in place (gamesounds.exe beside this program, or "--gamesounds <path>": a copy of it in a folder
//      of its own under the temp folder, the game never read): --list names every clip once, at once; with every clip there --missing
//      makes nothing and ends at once, whatever the game's folder; with some gone (a newer version's new guns) it asks for only those
//      and leaves every clip that's there as it was, byte for byte and to the time written; an empty file counts as missing
// Usage: campaigntest [--injector <path to injector.exe>] [--gamesounds <path to gamesounds.exe>]
// The exit code is the number of failed checks.

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../injector/cmdline.h"
#include "../src/campaign.h"
#include "../src/xrpick.h"

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

void Info(const char* id, const char* what, const char* format, ...) {
    char numbers[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, "INFO", what, numbers);
}

std::string Narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w)
        s += c < 128 ? static_cast<char>(c) : '?';
    return s;
}

const char* Name(campaign::Verdict v) {
    switch (v) {
    case campaign::Verdict::kCampaign: return "campaign";
    case campaign::Verdict::kNoLine: return "no line";
    case campaign::Verdict::kNoWebMode: return "no -webMode";
    case campaign::Verdict::kNotSinglePlayer: return "not SP";
    case campaign::Verdict::kOnlineState: return "online state";
    case campaign::Verdict::kOnlineParams: return "online params";
    }
    return "?";
}

// The made-up codes the lines carry where the real ones carry the account's: never to be seen in a verdict's words.
const wchar_t kCode[] = L"CODExq7SECRET", kToken[] = L"TOKENzz9SECRET";

// Battlelog's starts, as the EA app's own logs show them, with made-up codes. The campaign's (the PEB of the game itself):
std::wstring CampaignLine(const std::wstring& program = L"bf3.exe") {
    return program + L" -webMode SP -Origin_NoAppFocus  -AuthCode " + kCode +
           L" -requestState State_ResumeCampaign -requestStateParams \"<data personaref=\\\"1234567890\\\" levelmode=\\\"sp\\\" logintoken=\\\"" +
           kToken + L"\\\"></data>\" -Online.BlazeLogLevel 2 -Online.DirtysockLogLevel 2";
}
// Joining a server (the line the EA app hands the web helper, the game's own the same but its --activate-webhelper):
std::wstring ServerLine(const std::wstring& program = L"bf3.exe") {
    return program + L" -webMode MP -Origin_NoAppFocus  -AuthCode " + kCode +
           L" -requestState State_ClaimReservation -requestStateParams \"<data putinsquad=\\\"true\\\" gameid=\\\"998877\\\" role=\\\"soldier\\\" "
           L"personaref=\\\"1234567890\\\" levelmode=\\\"mp\\\" logintoken=\\\"" +
           kToken + L"\\\"></data>\" -Online.BlazeLogLevel 2 -Online.DirtysockLogLevel 2";
}

bool HasCode(const std::string& s) { return s.find("SECRET") != std::string::npos || s.find("CODExq7") != std::string::npos || s.find("TOKENzz9") != std::string::npos; }

// ---- 1: the rule ----
void RowRule() {
    struct Case {
        const char* what;
        std::wstring line;
        campaign::Verdict want;
        const char* detail;  // the word wanted, or null for any
    };
    const std::wstring tail = std::wstring(L" -requestStateParams \"<data personaref=\\\"1\\\" levelmode=\\\"sp\\\" logintoken=\\\"") + kToken + L"\\\"></data>\"";
    const std::vector<Case> cases = {
        {"PLAY CAMPAIGN, as logged", CampaignLine(), campaign::Verdict::kCampaign, "-webMode SP, -requestState State_ResumeCampaign"},
        {"the same, the program's folder quoted with spaces", CampaignLine(L"\"E:\\Steam Library\\steamapps\\common\\Battlefield 3\\bf3.exe\""), campaign::Verdict::kCampaign, nullptr},
        {"the same, the web helper's own word in it", L"bf3.exe -webMode SP -Origin_NoAppFocus --activate-webhelper -AuthCode X -requestState State_ResumeCampaign" + tail, campaign::Verdict::kCampaign, nullptr},
        {"names and values in other cases", L"bf3.exe -WEBMODE sp -REQUESTSTATE state_resumecampaign" + tail, campaign::Verdict::kCampaign, nullptr},
        {"tabs and runs of spaces", L"bf3.exe\t-webMode\t\tSP   -requestState \t State_ResumeCampaign", campaign::Verdict::kCampaign, nullptr},
        {"SP quoted", L"bf3.exe -webMode \"SP\" -requestState \"State_ResumeCampaign\"", campaign::Verdict::kCampaign, nullptr},
        {"-webMode SP alone (the EA app's hand-over start)", L"bf3.exe -webMode SP -Origin_NoAppFocus", campaign::Verdict::kCampaign, "-webMode SP"},
        {"-webMode SP twice", L"bf3.exe -webMode SP -webMode SP -requestState State_ResumeCampaign", campaign::Verdict::kCampaign, nullptr},
        {"another campaign state (a guess at a new player's start)", L"bf3.exe -webMode SP -requestState State_NewCampaign", campaign::Verdict::kCampaign, "-webMode SP, -requestState State_NewCampaign"},
        {"SP, a co-op campaign state", L"bf3.exe -webMode SP -requestState State_CoopCampaign", campaign::Verdict::kOnlineState, "-requestState State_CoopCampaign"},
        {"SP, a campaign state joining a game", L"bf3.exe -webMode SP -requestState State_JoinCampaignGame", campaign::Verdict::kOnlineState, "-requestState State_JoinCampaignGame"},
        {"SP, a campaign word without State_", L"bf3.exe -webMode SP -requestState ResumeCampaign", campaign::Verdict::kOnlineState, "-requestState ResumeCampaign"},
        {"joining a server, as logged", ServerLine(), campaign::Verdict::kNotSinglePlayer, "-webMode MP"},
        {"joining a server by its id", L"bf3.exe -webMode MP -requestState State_ConnectToGameId -requestStateParams \"<data gameid=\\\"5\\\" levelmode=\\\"mp\\\">\"", campaign::Verdict::kNotSinglePlayer, "-webMode MP"},
        {"co-op (a guess at its word)", L"bf3.exe -webMode COOP -requestState State_ClaimReservation", campaign::Verdict::kNotSinglePlayer, "-webMode COOP"},
        {"co-op in lower case", L"bf3.exe -webmode coop", campaign::Verdict::kNotSinglePlayer, "-webMode coop"},
        {"SP then MP", L"bf3.exe -webMode SP -webMode MP -requestState State_ResumeCampaign", campaign::Verdict::kNotSinglePlayer, "-webMode MP"},
        {"MP then SP", L"bf3.exe -webMode MP -webMode SP -requestState State_ResumeCampaign", campaign::Verdict::kNotSinglePlayer, "-webMode MP"},
        {"-webMode with no value at the end", L"bf3.exe -requestState State_ResumeCampaign -webMode", campaign::Verdict::kNotSinglePlayer, "-webMode (none)"},
        {"-webMode SPX", L"bf3.exe -webMode SPX", campaign::Verdict::kNotSinglePlayer, "-webMode SPX"},
        {"SP but a server's state", L"bf3.exe -webMode SP -requestState State_ClaimReservation", campaign::Verdict::kOnlineState, "-requestState State_ClaimReservation"},
        {"SP but joining by id", L"bf3.exe -webMode SP -requestState State_ConnectToGameId", campaign::Verdict::kOnlineState, "-requestState State_ConnectToGameId"},
        {"SP, a second state not the campaign's", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -requestState State_JoinServer", campaign::Verdict::kOnlineState, "-requestState State_JoinServer"},
        {"SP, a state with no value", L"bf3.exe -webMode SP -requestState", campaign::Verdict::kOnlineState, "-requestState (none)"},
        {"SP, the params' level mode mp", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -requestStateParams \"<data levelmode=\\\"mp\\\">\"", campaign::Verdict::kOnlineParams, "levelmode mp"},
        {"SP, the params' level mode coop", L"bf3.exe -webMode SP -requestStateParams \"<data levelmode=\\\"coop\\\">\"", campaign::Verdict::kOnlineParams, "levelmode coop"},
        {"SP, the params' quotes unescaped (split into words)", L"bf3.exe -webMode SP -requestStateParams <data levelmode=\"mp\">", campaign::Verdict::kOnlineParams, "levelmode mp"},
        {"SP, the level mode given twice, sp then mp", L"bf3.exe -webMode SP -requestStateParams \"<data levelmode=\\\"sp\\\" levelmode=\\\"mp\\\">\"", campaign::Verdict::kOnlineParams, "levelmode mp"},
        {"SP, a level mode on its own outside the params", L"bf3.exe -webMode SP -requestState State_ResumeCampaign levelmode=mp", campaign::Verdict::kOnlineParams, "levelmode mp"},
        {"SP, a server named in the params", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -requestStateParams \"<data gameid=\\\"5\\\" levelmode=\\\"sp\\\">\"", campaign::Verdict::kOnlineParams, "gameid"},
        {"SP, a squad in the params", L"bf3.exe -webMode SP -requestStateParams \"<data putinsquad=\\\"true\\\">\"", campaign::Verdict::kOnlineParams, "putinsquad"},
        {"no -webMode: the game alone", L"bf3.exe", campaign::Verdict::kNoWebMode, "no -webMode"},
        {"no -webMode: other options", L"bf3.exe -dx11 -requestState State_ResumeCampaign", campaign::Verdict::kNoWebMode, nullptr},
        {"an empty line", L"", campaign::Verdict::kNoWebMode, nullptr},
        {"-webMode SP only inside the params of a server's start", L"bf3.exe -webMode MP -requestStateParams \"-webMode SP\"", campaign::Verdict::kNotSinglePlayer, "-webMode MP"},
        {"-webMode SP only inside a quoted value", L"bf3.exe -something \"-webMode SP\"", campaign::Verdict::kNoWebMode, nullptr},
        {"-webMode SP only in the program's folder", L"\"C:\\x -webMode SP\\bf3.exe\" -dx11", campaign::Verdict::kNoWebMode, nullptr},
        {"another option that begins the same", L"bf3.exe -webModeX SP", campaign::Verdict::kNoWebMode, nullptr},
        {"-webMode as a value of another option", L"bf3.exe -name -webMode", campaign::Verdict::kNotSinglePlayer, "-webMode (none)"},
        // The names anywhere else after the program's (a game that read its line otherwise than the split would see them): refused.
        {"the campaign's options, -webMode MP inside a quoted value", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -foo \"x -webMode MP\"",
         campaign::Verdict::kNotSinglePlayer, "-webMode mp"},
        {"the campaign's options, -webMode=MP beside them", L"bf3.exe -webMode SP -webMode=MP -requestState State_ResumeCampaign", campaign::Verdict::kNotSinglePlayer,
         "-webMode mp"},
        {"the campaign's options, -webMode:MP beside them", L"bf3.exe -webMode SP -webMode:MP", campaign::Verdict::kNotSinglePlayer, "-webMode mp"},
        {"the campaign's options, +webMode MP beside them", L"bf3.exe -webMode SP +webMode MP", campaign::Verdict::kNotSinglePlayer, "-webMode mp"},
        {"the campaign's options, -webModeX MP beside them", L"bf3.exe -webMode SP -webModeX MP", campaign::Verdict::kNotSinglePlayer, "-webMode x"},
        {"a server's state inside the campaign's params", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -requestStateParams \"<data -requestState State_ClaimReservation>\"",
         campaign::Verdict::kOnlineState, "-requestState state_claimreservation"},
        {"-requestState=State_ClaimReservation beside the campaign's", L"bf3.exe -webMode SP -requestState State_ResumeCampaign -requestState=State_ClaimReservation",
         campaign::Verdict::kOnlineState, "-requestState state_claimreservation"},
        // ... and never more than that: the same names saying the campaign, or only in the program's folder, still the campaign.
        {"the campaign's options, -webMode=SP beside them", L"bf3.exe -webMode SP -webMode=SP -requestState State_ResumeCampaign", campaign::Verdict::kCampaign,
         "-webMode SP, -requestState State_ResumeCampaign"},
        {"webMode MP and requestState only in the program's folder", L"\"C:\\webMode MP -requestState x\\bf3.exe\" -webMode SP -requestState State_ResumeCampaign" + tail,
         campaign::Verdict::kCampaign, "-webMode SP, -requestState State_ResumeCampaign"},
    };
    int right = 0, words = 0, coded = 0;
    std::string wrong;
    for (const Case& c : cases) {
        const campaign::Judgement j = campaign::Judge(c.line.c_str());
        const bool verdictOk = j.verdict == c.want;
        const bool detailOk = !c.detail || j.detail == c.detail;
        right += verdictOk && detailOk;
        words += detailOk;
        coded += HasCode(j.detail);
        if (!verdictOk || !detailOk)
            wrong += std::string(wrong.empty() ? "" : "; ") + c.what + ": " + Name(j.verdict) + " (" + j.detail + "), wanted " + Name(c.want) +
                     (c.detail ? std::string(" (") + c.detail + ")" : "");
    }
    Report("1a", "each start's verdict and the word that decided it", right == static_cast<int>(cases.size()), "%d of %zu right%s%s", right,
           cases.size(), wrong.empty() ? "" : ": ", wrong.c_str());
    Report("1b", "never a code or a token in a verdict's word", coded == 0, "%d of %zu words carry one", coded, cases.size());
    const campaign::Judgement none = campaign::Judge(nullptr);
    Report("1c", "no line at all: can't tell", none.verdict == campaign::Verdict::kNoLine, "%s (%s)", Name(none.verdict), none.detail.c_str());
    int said = 0;
    for (campaign::Verdict v : {campaign::Verdict::kCampaign, campaign::Verdict::kNoLine, campaign::Verdict::kNoWebMode, campaign::Verdict::kNotSinglePlayer,
                                campaign::Verdict::kOnlineState, campaign::Verdict::kOnlineParams})
        said += std::strlen(campaign::Words(v)) > 10 && std::strcmp(campaign::Words(v), "unknown") != 0;
    Report("1d", "every verdict has its plain words", said == 6, "%d of 6", said);
    // A value to show is a short word: anything else in it is cut down to letters, digits and _ - .
    const campaign::Judgement odd = campaign::Judge(L"bf3.exe -webMode \"M P<script>&'\\\"verylongvaluexxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"");
    Report("1e", "an odd value is shown cut down", odd.verdict == campaign::Verdict::kNotSinglePlayer && odd.detail.size() <= 9 + 40 &&
                                                       odd.detail.find('<') == std::string::npos && odd.detail.find(' ', 9) == std::string::npos,
           "%s (%s)", Name(odd.verdict), odd.detail.c_str());
}

// ---- 2: the split against the system's ----
void RowSplit() {
    const std::vector<std::wstring> lines = {
        CampaignLine(),
        ServerLine(),
        L"bf3.exe a b c",
        L"\"C:\\Program Files\\x\\bf3.exe\" -a \"b c\" d",
        L"bf3.exe \"a\\\"b\" c",
        L"bf3.exe a\\\\\"b c\" d",
        L"bf3.exe a\\\\\\\"b c",
        L"bf3.exe a\\b\\\\c \\\\",
        L"bf3.exe \"\" x \"\"",
        L"bf3.exe \t a\t\tb  ",
        L"bf3.exe \"unterminated quote here",
        L"C:\\a\\b.exe -x=\"1 2\" -y",
        L"bf3.exe -webMode SP -requestStateParams \"<data a=\\\"1\\\" b=\\\"2\\\"></data>\"",
    };
    int same = 0;
    std::string wrong;
    for (const std::wstring& line : lines) {
        int count = 0;
        LPWSTR* argv = CommandLineToArgvW(line.c_str(), &count);
        const std::vector<std::wstring> mine = campaign::Split(line.c_str());
        bool ok = argv && static_cast<int>(mine.size()) == count;
        for (int i = 0; ok && i < count; ++i)
            ok = mine[i] == argv[i];
        if (argv)
            LocalFree(argv);
        same += ok;
        if (!ok)
            wrong += (wrong.empty() ? "" : " / ") + Narrow(line.substr(0, 60));
    }
    Report("2", "the line split as the system splits it", same == static_cast<int>(lines.size()), "%d of %zu lines alike%s%s", same, lines.size(),
           wrong.empty() ? "" : ": not ", wrong.c_str());
}

// ---- 3: a running process's line read from outside ----
std::wstring SelfPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return path;
}

struct Child {
    PROCESS_INFORMATION pi{};
    bool ok = false;
};

Child Start(const std::wstring& exe, const std::wstring& line, DWORD flags = 0) {
    Child c;
    STARTUPINFOW si{sizeof(si)};
    std::vector<wchar_t> buffer(line.begin(), line.end());
    buffer.push_back(0);
    c.ok = CreateProcessW(exe.c_str(), buffer.data(), nullptr, nullptr, FALSE, flags | CREATE_NO_WINDOW, nullptr, nullptr, &si, &c.pi) != 0;
    return c;
}

void Stop(Child& c) {
    if (!c.ok)
        return;
    TerminateProcess(c.pi.hProcess, 0);
    WaitForSingleObject(c.pi.hProcess, 5000);
    CloseHandle(c.pi.hThread);
    CloseHandle(c.pi.hProcess);
    c.ok = false;
}

void RowRead() {
    const std::wstring self = SelfPath();
    const std::wstring quoted = L"\"" + self + L"\"";
    std::wstring longLine = quoted + L" --child -x ";
    while (longLine.size() < 30000)
        longLine += L"abcdefghij";
    longLine += L" -webMode SP -requestState State_ResumeCampaign";
    struct Case {
        const char* what;
        std::wstring line;
    };
    const std::vector<Case> cases = {
        {"the campaign's line", CampaignLine(quoted) + L" --child"},
        {"the multiplayer line", ServerLine(quoted) + L" --child"},
        {"letters outside ASCII", quoted + L" --child -webMode SP -requestStateParams \"<data name=\\\"S\u00f8ren \u65e5\u672c \u0416\\\" levelmode=\\\"sp\\\">\""},
        {"30000 characters", longLine},
        {"the program's name and --child alone", quoted + L" --child"},
    };
    for (int how = 0; how < 3; ++how) {
        const char* const kHow[3] = {"its PEB", "asking the system", "the injector's way (PEB, then asking)"};
        int right = 0, verdicts = 0, started = 0;
        std::string wrong;
        std::string used;
        for (const Case& c : cases) {
            Child child = Start(self, c.line);
            started += child.ok;
            std::wstring read;
            DWORD error = 0;
            bool got = false;
            const char* by = "";
            // Read once it's running (the injector reads after 3 s): tried for up to 2 s.
            for (int t = 0; child.ok && !got && t < 40; ++t) {
                Sleep(50);
                got = how == 0 ? cmdline::ReadByPeb(child.pi.hProcess, read, error)
                    : how == 1 ? cmdline::ReadByQuery(child.pi.hProcess, read, error)
                               : cmdline::ReadCommandLine(child.pi.hProcess, read, error, &by);
            }
            Stop(child);
            const bool same = got && read == c.line;
            right += same;
            if (how == 2 && got)
                used += std::string(used.empty() ? "" : ", ") + by;
            const campaign::Judgement mine = campaign::Judge(got ? read.c_str() : nullptr), want = campaign::Judge(c.line.c_str());
            verdicts += got && mine.verdict == want.verdict && mine.detail == want.detail;
            if (!same)
                wrong += std::string(wrong.empty() ? "" : "; ") + c.what + (got ? ": different text (" + std::to_string(read.size()) + " characters of " +
                                                                                       std::to_string(c.line.size()) + ")"
                                                                                 : ": not read, error " + std::to_string(error));
        }
        char id[8];
        std::snprintf(id, sizeof(id), "3%c", 'a' + how);
        char what[160];
        std::snprintf(what, sizeof(what), "a running process's command line read by %s, every character", kHow[how]);
        Report(id, what, started == static_cast<int>(cases.size()) && right == static_cast<int>(cases.size()) && verdicts == right,
               "%d started, %d read alike, %d verdicts as the line's own%s%s%s%s", started, right, verdicts, used.empty() ? "" : "; read from its ",
               used.c_str(), wrong.empty() ? "" : "; ", wrong.c_str());
    }
    // A process that has ended: nothing read, and so no verdict but "can't tell".
    Child gone = Start(self, quoted + L" --child -webMode SP");
    if (gone.ok) {
        TerminateProcess(gone.pi.hProcess, 0);
        WaitForSingleObject(gone.pi.hProcess, 5000);
    }
    std::wstring read;
    DWORD error = 0;
    const bool got = gone.ok && cmdline::ReadCommandLine(gone.pi.hProcess, read, error);
    const campaign::Judgement j = campaign::Judge(got ? read.c_str() : nullptr);
    Report("3d", "a process that has ended: nothing read, can't tell", gone.ok && !got && j.verdict == campaign::Verdict::kNoLine,
           "read %s (error %lu), verdict %s", got ? "something" : "nothing", error, Name(j.verdict));
    if (gone.ok) {
        CloseHandle(gone.pi.hThread);
        CloseHandle(gone.pi.hProcess);
    }
    // A process not yet running (created suspended): which ways read it. Not judged: the injector reads after 3 s of running.
    Child held = Start(self, CampaignLine(quoted) + L" --child", CREATE_SUSPENDED);
    std::wstring a, b;
    DWORD ea = 0, eb = 0;
    const bool byPeb = held.ok && cmdline::ReadByPeb(held.pi.hProcess, a, ea);
    const bool byQuery = held.ok && cmdline::ReadByQuery(held.pi.hProcess, b, eb);
    Stop(held);
    Info("3e", "a process created suspended", "its PEB: %s (error %lu); asking: %s (error %lu)", byPeb ? (a == CampaignLine(quoted) + L" --child" ? "read alike" : "read, different") : "not read",
         ea, byQuery ? (b == CampaignLine(quoted) + L" --child" ? "read alike" : "read, different") : "not read", eb);
}

// ---- 4: the injector itself ----
struct Ran {
    DWORD code = 0xFFFFFFFF;
    std::string out;
};

// Runs a command line, its output caught, at most `seconds`; then it's ended.
Ran Run(const std::wstring& exe, const std::wstring& line, const std::wstring& folder, int seconds) {
    Ran r;
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE readEnd = nullptr, writeEnd = nullptr;
    if (!CreatePipe(&readEnd, &writeEnd, &sa, 0))
        return r;
    SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = writeEnd;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buffer(line.begin(), line.end());
    buffer.push_back(0);
    const bool ok = CreateProcessW(exe.c_str(), buffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                                   folder.empty() ? nullptr : folder.c_str(), &si, &pi) != 0;
    CloseHandle(writeEnd);
    if (!ok) {
        CloseHandle(readEnd);
        return r;
    }
    const ULONGLONG until = GetTickCount64() + seconds * 1000ull;
    for (;;) {
        DWORD avail = 0;
        while (PeekNamedPipe(readEnd, nullptr, 0, nullptr, &avail, nullptr) && avail) {
            char chunk[4096];
            DWORD got = 0;
            if (!ReadFile(readEnd, chunk, std::min<DWORD>(avail, sizeof(chunk)), &got, nullptr) || !got)
                break;
            r.out.append(chunk, got);
        }
        if (WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0 || GetTickCount64() > until)
            break;
    }
    if (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess, 0xDEAD);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    DWORD avail = 0;
    while (PeekNamedPipe(readEnd, nullptr, 0, nullptr, &avail, nullptr) && avail) {
        char chunk[4096];
        DWORD got = 0;
        if (!ReadFile(readEnd, chunk, std::min<DWORD>(avail, sizeof(chunk)), &got, nullptr) || !got)
            break;
        r.out.append(chunk, got);
    }
    GetExitCodeProcess(pi.hProcess, &r.code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readEnd);
    return r;
}

std::string OneLine(std::string s) {
    for (char& c : s)
        if (c == '\r' || c == '\n')
            c = ' ';
    return s.size() > 300 ? s.substr(0, 300) + "..." : s;
}

void RowInjector(const std::wstring& injector) {
    if (injector.empty()) {
        Info("4", "the injector itself", "SKIP: no --injector <path> given (the package's build runs it)");
        return;
    }
    if (GetFileAttributesW(injector.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Report("4", "the injector itself", false, "%s isn't there", Narrow(injector).c_str());
        return;
    }
    const std::wstring self = SelfPath(), quoted = L"\"" + self + L"\"";
    // --judge on a campaign start, a multiplayer start and no process at all.
    Child camp = Start(self, CampaignLine(quoted) + L" --child"), server = Start(self, ServerLine(quoted) + L" --child");
    Sleep(300);
    const Ran a = Run(injector, L"\"" + injector + L"\" --judge " + std::to_wstring(camp.pi.dwProcessId), L"", 20);
    const Ran b = Run(injector, L"\"" + injector + L"\" --judge " + std::to_wstring(server.pi.dwProcessId), L"", 20);
    const Ran c = Run(injector, L"\"" + injector + L"\" --judge 4", L"", 20);  // (pid 4 is the system's: not to be opened)
    Stop(camp);
    Stop(server);
    Report("4a", "--judge: the campaign's start", a.code == 0 && a.out.find("the single-player campaign") != std::string::npos && !HasCode(a.out),
           "exit %lu: %s", a.code, OneLine(a.out).c_str());
    Report("4b", "--judge: the multiplayer start", b.code == 3 && b.out.find("multiplayer or co-op") != std::string::npos && !HasCode(b.out),
           "exit %lu: %s", b.code, OneLine(b.out).c_str());
    Report("4c", "--judge: a process it can't read", c.code == 1, "exit %lu: %s", c.code, OneLine(c.out).c_str());

    // Waiting by name: a copy of the injector in a folder of its own with a stand-in DLL (never loaded: only the multiplayer start comes),
    // the multiplayer start as a process of the name it waits for (a copy of this program), refused and waited past.
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    const std::wstring folder = std::wstring(temp) + L"campaigntest_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring copy = folder + L"\\injector.exe", dll = folder + L"\\bf3vr.dll", target = folder + L"\\campaigntest_target.exe";
    const bool copied = CopyFileW(injector.c_str(), copy.c_str(), FALSE) && CopyFileW(self.c_str(), target.c_str(), FALSE);
    {
        std::ofstream stand(dll, std::ios::binary);
        stand << "a stand-in: the injector only looks for it; nothing loads it here";
    }
    Child mp = copied ? Start(target, ServerLine(L"\"" + target + L"\"") + L" --child") : Child{};
    // It waits for the target, gives it 3 s, judges it, refuses and waits on: ended after 8 s.
    const Ran w = Run(copy, L"\"" + copy + L"\" campaigntest_target.exe", folder, 8);
    std::string log;
    {
        std::ifstream f(folder + L"\\injector.log", std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        log = ss.str();
    }
    // Was the stand-in ever loaded into the target? A module of that name in it would say so.
    bool loaded = false;
    if (mp.ok) {
        HMODULE modules[512];
        DWORD needed = 0;
        using Enum = BOOL(WINAPI*)(HANDLE, HMODULE*, DWORD, LPDWORD);
        if (const auto enumModules = reinterpret_cast<Enum>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32EnumProcessModules"))) {
            if (enumModules(mp.pi.hProcess, modules, sizeof(modules), &needed)) {
                using Name = DWORD(WINAPI*)(HANDLE, HMODULE, LPWSTR, DWORD);
                const auto nameOf = reinterpret_cast<Name>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32GetModuleBaseNameW"));
                for (DWORD i = 0; nameOf && i < needed / sizeof(HMODULE) && i < 512; ++i) {
                    wchar_t name[MAX_PATH] = {};
                    nameOf(mp.pi.hProcess, modules[i], name, MAX_PATH);
                    loaded = loaded || _wcsicmp(name, L"bf3vr.dll") == 0;
                }
            }
        }
    }
    const bool started = mp.ok;
    Stop(mp);
    const bool refused = log.find("REFUSED: it was started for multiplayer or co-op (-webMode MP)") != std::string::npos;
    const bool waitsOn = log.find("Waiting for a campaign start") != std::string::npos;
    const bool stillRunning = w.code == 0xDEAD;  // ended by this test after 8 s: it was still waiting
    Report("4d", "waiting by name: a multiplayer start refused, nothing loaded, waited past",
           copied && started && refused && waitsOn && stillRunning && !loaded && !HasCode(log) && !HasCode(w.out),
           "copied %d, started %d, refused %d, waits on %d, still waiting at 8 s %d, the DLL in it %d, a code in its words %d; its log: %s", copied, started,
           refused, waitsOn, stillRunning, loaded, HasCode(log) || HasCode(w.out), OneLine(log).c_str());
    // A campaign start by name: judged the campaign and loaded into. The stand-in isn't a real DLL, so the system refuses to load it inside the
    // target and the injector ends there, saying so (before it would wait for the game's end and let go of keys: no input is ever sent here).
    Child sp = copied ? Start(target, CampaignLine(L"\"" + target + L"\"") + L" --child") : Child{};
    const bool spStarted = sp.ok;
    const Ran v = Run(copy, L"\"" + copy + L"\" campaigntest_target.exe", folder, 20);
    std::string spLog;
    {
        std::ifstream f(folder + L"\\injector.log", std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        spLog = ss.str();
    }
    Stop(sp);
    const bool judged = spLog.find("Campaign start (-webMode SP, -requestState State_ResumeCampaign; read from its PEB): loading the mod") != std::string::npos;
    const bool tried = spLog.find("LoadLibrary failed inside the game") != std::string::npos;
    Report("4e", "waiting by name: the campaign's start judged so and loaded into (a stand-in the system refuses: nothing loaded)",
           spStarted && judged && tried && v.code == 1 && !HasCode(spLog) && !HasCode(v.out), "started %d, judged the campaign %d, the load tried %d, exit %lu; its log: %s",
           spStarted, judged, tried, v.code, OneLine(spLog).c_str());
    DeleteFileW(copy.c_str());
    DeleteFileW(dll.c_str());
    DeleteFileW(target.c_str());
    DeleteFileW((folder + L"\\injector.log").c_str());
    RemoveDirectoryW(folder.c_str());
}

// ---- 5: the OpenXR runtime ----
void RowRuntime() {
    const std::wstring steam = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR\\steamxr_win32.json";
    const std::wstring meta = L"C:\\Program Files\\Meta Horizon\\Support\\oculus-runtime\\oculus_openxr_32.json";
    const std::wstring other = L"D:\\VR\\SomeRuntime\\runtime_x86.json";
    const std::wstring vd = L"C:\\Program Files\\Virtual Desktop Streamer\\OpenXR\\virtualdesktop-openxr-32.json";
    struct Case {
        const char* what;
        std::wstring override, active, steamVr;
        bool prefer, set;
        bool vdxr = false;     // VDXR asked for (build\runtime.txt "vdxr")
        std::wstring vdFound;  // VDXR's 32-bit manifest found on the PC
        bool toVd = false;     // the choice is VDXR's (else SteamVR's, when set)
        bool missing = false;  // VDXR asked for but not found
    };
    const std::vector<Case> cases = {
        {"this PC: Steam's XR_RUNTIME_JSON, corrected to SteamVR's 32-bit", steam, steam, steam, true, false},
        {"SteamVR the active runtime, nothing in the environment", L"", steam, steam, true, false},
        {"a Quest PC: Meta's active, SteamVR installed", L"", meta, steam, true, true},
        {"no 32-bit runtime registered, SteamVR installed", L"", L"", steam, true, true},
        {"XR_RUNTIME_JSON naming another runtime, SteamVR installed", other, steam, steam, true, true},
        {"XR_RUNTIME_JSON naming SteamVR's under another folder", L"E:\\SteamLibrary\\steamapps\\common\\SteamVR\\steamxr_win32.json", meta, steam, true, false},
        {"Meta's active, no SteamVR", L"", meta, L"", true, false},
        {"nothing at all", L"", L"", L"", true, false},
        {"Meta's active, SteamVR installed, prefer_steamvr=0", L"", meta, steam, false, false},
        // VDXR only when asked for: then always, whatever is active; asked for but missing, the choice as without it.
        {"VDXR asked for and there, Meta's active", L"", meta, steam, true, true, true, vd, true, false},
        {"VDXR asked for and there, SteamVR's active", L"", steam, steam, true, true, true, vd, true, false},
        {"VDXR asked for and there, Steam's XR_RUNTIME_JSON naming SteamVR's", steam, steam, steam, true, true, true, vd, true, false},
        {"VDXR asked for and there, VDXR's active", L"", vd, steam, true, true, true, vd, true, false},
        {"VDXR asked for and there, prefer_steamvr=0", L"", meta, steam, false, true, true, vd, true, false},
        {"VDXR asked for and there, no SteamVR", L"", L"", L"", true, true, true, vd, true, false},
        {"VDXR asked for but not there, Meta's active, SteamVR installed", L"", meta, steam, true, true, true, L"", false, true},
        {"VDXR asked for but not there, no SteamVR", L"", meta, L"", true, false, true, L"", false, true},
        {"VDXR not asked for, VDXR's active, SteamVR installed: SteamVR's as before", L"", vd, steam, true, true},
        {"VDXR not asked for, VDXR's active, prefer_steamvr=0: left", L"", vd, steam, false, false},
    };
    int right = 0;
    std::string wrong;
    for (const Case& c : cases) {
        const xrpick::Choice ch = xrpick::Pick(c.override, c.active, c.steamVr, c.prefer, c.vdxr, c.vdFound);
        const bool ok = ch.set == c.set && (!ch.set || ch.manifest == (c.toVd ? c.vdFound : c.steamVr)) && ch.vdxr == c.toVd &&
                        ch.vdxrMissing == c.missing && std::strlen(ch.why) > 10;
        right += ok;
        if (!ok)
            wrong += std::string(wrong.empty() ? "" : "; ") + c.what + ": " + (ch.set ? "set" : "kept") + " (" + ch.why + ")";
    }
    Report("5a", "SteamVR's 32-bit runtime preferred, VDXR when asked for, or the choice left", right == static_cast<int>(cases.size()), "%d of %zu right%s%s", right,
           cases.size(), wrong.empty() ? "" : ": ", wrong.c_str());
    struct Name {
        std::wstring path;
        bool steamVr;
    };
    const std::vector<Name> names = {
        {L"C:\\x\\STEAMXR_WIN32.JSON", true},  {L"C:/x/steamxr_win32.json", true},   {L"steamxr_win32.json", true},
        {L"\"C:\\x\\steamxr_win32.json\"", true}, {L"C:\\x\\steamxr_win64.json", false}, {L"C:\\x\\steamxr_win32.json.bak", false},
        {L"C:\\steamxr_win32.json\\other.json", false}, {L"", false},
    };
    int named = 0;
    for (const Name& n : names)
        named += xrpick::IsSteamVr32(n.path) == n.steamVr;
    Report("5b", "SteamVR's 32-bit manifest known by its file's name in any spelling", named == static_cast<int>(names.size()), "%d of %zu", named,
           names.size());
    const std::vector<Name> vdNames = {
        {vd, true}, {L"C:\\X\\VIRTUALDESKTOP-OPENXR-32.JSON", true}, {L"\"C:\\x\\virtualdesktop-openxr-32.json\"", true},
        {L"C:\\x\\virtualdesktop-openxr.json", false}, {L"C:\\x\\steamxr_win32.json", false}, {L"", false},
    };
    int vdNamed = 0;
    for (const Name& n : vdNames)
        vdNamed += xrpick::IsVdxr32(n.path) == n.steamVr;
    Report("5c", "VDXR's 32-bit manifest known by its file's name, and only it", vdNamed == static_cast<int>(vdNames.size()), "%d of %zu",
           vdNamed, vdNames.size());
}

// ---- 6: the time ----
void RowTime() {
    std::wstring line = L"bf3.exe -x ";
    while (line.size() < 30000)
        line += L"abcdefghij";
    line += L" -webMode SP -requestState State_ResumeCampaign";
    const auto t0 = std::chrono::steady_clock::now();
    int campaigns = 0;
    for (int i = 0; i < 100; ++i)
        campaigns += campaign::Judge(line.c_str()).verdict == campaign::Verdict::kCampaign;
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 100.0;
    const campaign::Judgement j = campaign::Judge(CampaignLine().c_str());
    const auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i)
        campaigns += campaign::Judge(CampaignLine().c_str()).verdict == campaign::Verdict::kCampaign;
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t1).count() / 1000.0;
    Report("6", "the time", campaigns == 1100 && j.verdict == campaign::Verdict::kCampaign && ms < 1.0, "%.3f ms for 30000 characters, %.2f us for the campaign's line",
           ms, us);
}

// ---- 7: the mod's sounds after an update in place ----
void RowSounds(std::wstring cutter) {
    if (cutter.empty()) {
        const std::wstring self = SelfPath();
        cutter = self.substr(0, self.find_last_of(L'\\') + 1) + L"gamesounds.exe";
        if (GetFileAttributesW(cutter.c_str()) == INVALID_FILE_ATTRIBUTES) {
            Info("7", "the mod's sounds after an update in place", "SKIP: no gamesounds.exe beside this program and no --gamesounds <path> given");
            return;
        }
    }
    if (GetFileAttributesW(cutter.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Report("7", "the mod's sounds after an update in place", false, "%s isn't there", Narrow(cutter).c_str());
        return;
    }
    // A copy in a folder of its own: its sounds folder is the one beside it. "The game" is that folder too: it holds no game, so a run
    // that goes on to read the game says so and ends with 3, nothing written.
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    const std::wstring folder = std::wstring(temp) + L"campaigntest_sounds_" + std::to_wstring(GetCurrentProcessId());
    const std::wstring sounds = folder + L"\\sounds", copy = folder + L"\\gamesounds.exe";
    CreateDirectoryW(folder.c_str(), nullptr);
    CreateDirectoryW(sounds.c_str(), nullptr);
    const bool copied = CopyFileW(cutter.c_str(), copy.c_str(), FALSE) != 0;
    const std::wstring quoted = L"\"" + copy + L"\"", noGame = L" --game \"" + folder + L"\"";

    const auto t0 = std::chrono::steady_clock::now();
    const Ran list = Run(copy, quoted + L" --list", folder, 20);
    const double listMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::vector<std::string> names;
    bool plain = true;
    {
        std::stringstream ss(list.out);
        for (std::string line; std::getline(ss, line);) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                line.pop_back();
            if (line.empty())
                continue;
            plain = plain && std::all_of(line.begin(), line.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-'; });
            names.push_back(line);
        }
    }
    std::vector<std::string> sorted = names;
    std::sort(sorted.begin(), sorted.end());
    const bool once = std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
    auto has = [&](const char* name) { return std::find(names.begin(), names.end(), name) != names.end(); };
    Report("7a", "--list: every clip the mod plays, each once, at once, the game not looked for",
           copied && list.code == 0 && names.size() >= 100 && plain && once && has("mag_out") && has("bolt") && has("pistol_slide") && listMs < 5000,
           "copied %d, exit %lu, %zu names, plain %d, each once %d, in %.0f ms", copied, list.code, names.size(), plain, once, listMs);
    if (!copied || list.code != 0 || names.size() < 4) {
        DeleteFileW(copy.c_str());
        RemoveDirectoryW(sounds.c_str());
        RemoveDirectoryW(folder.c_str());
        return;
    }

    // Every clip there (stand-ins: a wave file's header and some bytes; nothing reads them as sound here).
    auto path = [&](const std::string& name) { return sounds + L"\\" + std::wstring(name.begin(), name.end()) + L".wav"; };
    auto write = [&](const std::string& name, size_t bytes) {
        std::ofstream f(path(name), std::ios::binary);
        const std::string body = "RIFF....WAVEfmt a stand-in for " + name + " ";
        for (size_t i = 0; i < bytes; ++i)
            f.put(body[i % body.size()]);
    };
    struct Stamp {
        FILETIME written{};
        DWORD size = 0;
        bool there = false;
        bool operator==(const Stamp& o) const { return there == o.there && size == o.size && CompareFileTime(&written, &o.written) == 0; }
    };
    auto stamp = [&](const std::string& name) {
        Stamp s;
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (GetFileAttributesExW(path(name).c_str(), GetFileExInfoStandard, &info)) {
            s.there = true;
            s.size = info.nFileSizeLow;
            s.written = info.ftLastWriteTime;
        }
        return s;
    };
    for (const std::string& n : names)
        write(n, 200);
    std::vector<Stamp> before;
    for (const std::string& n : names)
        before.push_back(stamp(n));
    auto unchanged = [&](size_t skipFirst, size_t skipSecond) {
        int same = 0;
        for (size_t i = 0; i < names.size(); ++i)
            same += i == skipFirst || i == skipSecond || stamp(names[i]) == before[i];
        return same;
    };
    Sleep(50);
    const auto t1 = std::chrono::steady_clock::now();
    const Ran none = Run(copy, quoted + L" --missing" + noGame, folder, 20);
    const double noneMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
    const int sameNone = unchanged(names.size(), names.size());
    Report("7b", "--missing with every clip there: nothing made, ended at once, the game not read",
           none.code == 0 && none.out.find("nothing made") != std::string::npos && sameNone == static_cast<int>(names.size()) && noneMs < 5000,
           "exit %lu in %.0f ms, %d of %zu clips as they were: %s", none.code, noneMs, sameNone, names.size(), OneLine(none.out).c_str());

    // An update in place: two clips of the newer version aren't there (one gone, one an empty file). Only those are asked for; with no
    // game to read the run ends there (3), and no clip that's there was touched.
    const size_t gone = names.size() / 3, empty = names.size() - 2;
    DeleteFileW(path(names[gone]).c_str());
    write(names[empty], 0);
    const Ran some = Run(copy, quoted + L" --missing" + noGame, folder, 20);
    const std::string wanted = "2 of the " + std::to_string(names.size()) + " clips the mod plays aren't there yet: " + names[gone] + " " + names[empty];
    const int sameSome = unchanged(gone, empty);
    Report("7c", "--missing with two gone: only those asked for, every clip that's there left as it was",
           some.code == 3 && some.out.find(wanted) != std::string::npos && sameSome == static_cast<int>(names.size()) && !stamp(names[gone]).there,
           "exit %lu (3: no game here), %d of %zu clips as they were: %s", some.code, sameSome - 2, names.size() - 2, OneLine(some.out).c_str());

    for (const std::string& n : names)
        DeleteFileW(path(n).c_str());
    DeleteFileW(copy.c_str());
    RemoveDirectoryW(sounds.c_str());
    RemoveDirectoryW(folder.c_str());
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    // A child of row 3 or 4: it only waits to be read, and is ended (by itself after a minute at the latest).
    for (int i = 1; i < argc; ++i)
        if (wcscmp(argv[i], L"--child") == 0) {
            Sleep(60000);
            return 0;
        }
    std::wstring injector, cutter;
    for (int i = 1; i + 1 < argc; ++i)
        if (wcscmp(argv[i], L"--injector") == 0)
            injector = argv[i + 1];
        else if (wcscmp(argv[i], L"--gamesounds") == 0)
            cutter = argv[i + 1];
    RowRule();
    RowSplit();
    RowRead();
    RowInjector(injector);
    RowRuntime();
    RowTime();
    RowSounds(cutter);
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}
