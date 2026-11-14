#pragma once

// The mod goes only into Battlefield 3's single-player campaign, never into a multiplayer or co-op game: injecting into the
// game on a server can get an account banned. Two gates use this one rule: the injector reads the started bf3.exe's command
// line before it loads anything into it (injector/main.cpp), and the DLL reads its own at load and stays out unless it is the
// campaign (dllmain.cpp). Header only, nothing of Windows: tools/campaigntest.cpp runs it offline.
//
// How Battlelog starts the game (the EA app's own logs of the starts, the secrets left out):
//   PLAY CAMPAIGN:    bf3.exe -webMode SP -Origin_NoAppFocus  -AuthCode <code> -requestState State_ResumeCampaign
//                         -requestStateParams "<data personaref=\"<n>\" levelmode=\"sp\" logintoken=\"<token>\"></data>"
//   joining a server: -webMode MP ... -requestState State_ClaimReservation
//                         -requestStateParams "<data putinsquad=\"true\" gameid=\"<n>\" role=\"soldier\" personaref=\"<n>\" levelmode=\"mp\" ...>"
// Co-op was never seen started; its -webMode is not SP (a guess, and the rule needs no more: anything not SP is refused).
//
// The campaign is: at least one -webMode, every one of them SP; every -requestState (if any) the campaign's: State_ResumeCampaign,
// the only one PLAY CAMPAIGN has been seen to send, or another State_...Campaign... with no word of an online game in it (a guess
// at what a new player's start might send; never seen); no level mode other than sp given anywhere after the program's name (the
// params' levelmode, each time it's there), and no server named (gameid, putinsquad). Anything else, an empty line or one that
// couldn't be read: not the campaign. Names and values are compared without regard to case. The command line is split as the
// Microsoft C runtime splits it, so an option's name inside a quoted value (the params, a folder's name) is never taken for
// the option. Never print the line itself: it carries the account's codes.

#include <cstddef>
#include <string>
#include <vector>

namespace campaign {

enum class Verdict {
    kCampaign,         // the single-player campaign, as Battlelog's PLAY CAMPAIGN starts it
    kNoLine,           // the command line couldn't be read
    kNoWebMode,        // no -webMode: not started by Battlelog (a bare bf3.exe, a tool, something else)
    kNotSinglePlayer,  // a -webMode other than SP: multiplayer or co-op
    kOnlineState,      // a -requestState other than the campaign's: joining a server or a co-op game
    kOnlineParams,     // -requestStateParams for a level mode other than sp, or naming a server
};

struct Judgement {
    Verdict verdict = Verdict::kNoLine;
    // The word that decided it, safe to show: "-webMode MP", "-requestState State_ClaimReservation", "levelmode mp", "gameid".
    // Never a code or a token.
    std::string detail;
};

namespace detail {

inline bool IsSpace(wchar_t c) { return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r'; }

inline wchar_t Lower(wchar_t c) { return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c - L'A' + L'a') : c; }

inline bool Same(const std::wstring& a, const wchar_t* b) {
    size_t i = 0;
    for (; i < a.size() && b[i]; ++i)
        if (Lower(a[i]) != Lower(b[i]))
            return false;
    return i == a.size() && !b[i];
}

inline std::wstring Lowered(const std::wstring& s) {
    std::wstring out(s);
    for (wchar_t& c : out)
        c = Lower(c);
    return out;
}

// A -requestState of the campaign's: State_ResumeCampaign (every PLAY CAMPAIGN seen), or another State_...Campaign... (none seen;
// a new player's start might carry one) with no word of an online game in it.
inline bool CampaignState(const std::wstring& state) {
    const std::wstring low = Lowered(state);
    if (low == L"state_resumecampaign")
        return true;
    if (low.rfind(L"state_", 0) != 0 || low.find(L"campaign") == std::wstring::npos)
        return false;
    for (const wchar_t* online : {L"coop", L"co-op", L"connect", L"reservation", L"join", L"server", L"matchmak", L"party", L"squad", L"lobby",
                                  L"friend", L"multi", L"game"})
        if (low.find(online) != std::wstring::npos)
            return false;
    return true;
}

// A value to show: letters, digits and _ - . only, at most 40 of them (a value is a short word; anything else is cut off).
inline std::string Shown(const std::wstring& s) {
    std::string out;
    for (wchar_t c : s) {
        if (out.size() >= 40)
            break;
        const bool plain = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'-' || c == L'.';
        out += plain ? static_cast<char>(c) : '?';
    }
    return out.empty() ? std::string("(none)") : out;
}

// The word after the key at `at` ("levelmode="), without quotes or backslashes: levelmode=\"sp\" gives sp.
inline void ValueAt(const std::wstring& lowered, size_t at, const wchar_t* key, std::wstring& value) {
    size_t i = at + std::char_traits<wchar_t>::length(key);
    while (i < lowered.size() && (lowered[i] == L'\\' || lowered[i] == L'"' || lowered[i] == L'\'' || IsSpace(lowered[i])))
        ++i;
    value.clear();
    while (i < lowered.size() && lowered[i] != L'\\' && lowered[i] != L'"' && lowered[i] != L'\'' && lowered[i] != L'>' && lowered[i] != L'/' &&
           !IsSpace(lowered[i]))
        value += lowered[i++];
}

}  // namespace detail

// A command line split into its words as the Microsoft C runtime splits it (CommandLineToArgvW agrees): the program's name
// first (up to a space, or between quotes, no escapes), then words parted by spaces and tabs outside quotes, where 2n
// backslashes before a quote are n and the quote opens or closes, 2n+1 are n and a quote, and "" inside quotes is a quote.
inline std::vector<std::wstring> Split(const wchar_t* line) {
    std::vector<std::wstring> words;
    if (!line)
        return words;
    const wchar_t* p = line;
    // The program's name.
    std::wstring name;
    if (*p == L'"') {
        ++p;
        while (*p && *p != L'"')
            name += *p++;
        if (*p == L'"')
            ++p;
    } else {
        while (*p && *p != L' ' && *p != L'\t')
            name += *p++;
    }
    words.push_back(name);
    // The rest.
    for (;;) {
        while (*p == L' ' || *p == L'\t')
            ++p;
        if (!*p)
            break;
        std::wstring word;
        bool quoted = false;
        while (*p) {
            if (!quoted && (*p == L' ' || *p == L'\t'))
                break;
            if (*p == L'\\') {
                size_t slashes = 0;
                while (*p == L'\\')
                    ++slashes, ++p;
                if (*p == L'"') {
                    word.append(slashes / 2, L'\\');
                    if (slashes % 2) {
                        word += L'"';
                        ++p;
                    }
                    // An even run: the quote is handled below, as a quote.
                } else {
                    word.append(slashes, L'\\');
                }
                continue;
            }
            if (*p == L'"') {
                if (quoted && p[1] == L'"') {  // "" inside quotes: one quote, still quoted
                    word += L'"';
                    p += 2;
                    continue;
                }
                quoted = !quoted;
                ++p;
                continue;
            }
            word += *p++;
        }
        words.push_back(word);
    }
    return words;
}

// The verdict on a command line (GetCommandLineW's, or one read out of another process). Null: kNoLine.
inline Judgement Judge(const wchar_t* line) {
    Judgement j;
    if (!line) {
        j.verdict = Verdict::kNoLine;
        j.detail = "no command line";
        return j;
    }
    const std::vector<std::wstring> words = Split(line);
    std::vector<std::wstring> modes, states;
    std::wstring rest;  // every word after the program's name, lowered: the params' markers are looked for in all of it
    for (size_t i = 1; i < words.size(); ++i) {
        const std::wstring& w = words[i];
        rest += detail::Lowered(w) + L' ';
        std::vector<std::wstring>* into = detail::Same(w, L"-webMode") ? &modes : detail::Same(w, L"-requestState") ? &states : nullptr;
        if (!into)
            continue;
        // Its value is the next word; at the end of the line, none (which nothing allows).
        into->push_back(i + 1 < words.size() ? words[i + 1] : std::wstring());
        if (i + 1 < words.size())
            rest += detail::Lowered(words[i + 1]) + L' ';
        ++i;
    }
    if (modes.empty()) {
        j.verdict = Verdict::kNoWebMode;
        j.detail = "no -webMode";
        return j;
    }
    for (const std::wstring& m : modes)
        if (!detail::Same(m, L"SP")) {
            j.verdict = Verdict::kNotSinglePlayer;
            j.detail = "-webMode " + detail::Shown(m);
            return j;
        }
    for (const std::wstring& s : states)
        if (!detail::CampaignState(s)) {
            j.verdict = Verdict::kOnlineState;
            j.detail = "-requestState " + detail::Shown(s);
            return j;
        }
    // The params' level mode, each time it's given, wherever it is (quoted as Battlelog quotes it, or not and so split into words).
    for (size_t at = rest.find(L"levelmode="); at != std::wstring::npos; at = rest.find(L"levelmode=", at + 1)) {
        std::wstring mode;
        detail::ValueAt(rest, at, L"levelmode=", mode);
        if (mode != L"sp") {
            j.verdict = Verdict::kOnlineParams;
            j.detail = "levelmode " + detail::Shown(mode);
            return j;
        }
    }
    for (const wchar_t* server : {L"gameid=", L"putinsquad="})
        if (rest.find(server) != std::wstring::npos) {
            j.verdict = Verdict::kOnlineParams;
            j.detail = detail::Shown(std::wstring(server, std::char_traits<wchar_t>::length(server) - 1));
            return j;
        }
    j.verdict = Verdict::kCampaign;
    j.detail = states.empty() ? "-webMode SP" : "-webMode SP, -requestState " + detail::Shown(states.back());
    return j;
}

// The verdict in plain words, for the injector's window and the mod's log.
inline const char* Words(Verdict v) {
    switch (v) {
    case Verdict::kCampaign:
        return "the single-player campaign (Battlelog's PLAY CAMPAIGN)";
    case Verdict::kNoLine:
        return "how it was started couldn't be read";
    case Verdict::kNoWebMode:
        return "it wasn't started by Battlelog's PLAY CAMPAIGN";
    case Verdict::kNotSinglePlayer:
        return "it was started for multiplayer or co-op";
    case Verdict::kOnlineState:
        return "it was started to join a server or a co-op game";
    case Verdict::kOnlineParams:
        return "it was started for a multiplayer or co-op level";
    }
    return "unknown";
}

}  // namespace campaign
