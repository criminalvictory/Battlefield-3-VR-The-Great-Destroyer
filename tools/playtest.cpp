// The mod's reload sounds (src/sounds.cpp) played outside the game, to hear or record them on their
// own: playtest [name=bolt] [pan=0.3]. The clips are build\sounds\<name>.wav, next to this program.
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/sounds.h"

namespace logx {
void Write(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}
}

// playtest --count: how many clips build\sounds holds. playtest --make <game folder>: the clips not there
// (all, or a newer version's new ones) made by build\gamesounds.exe as the mod does at its start, its lines
// printed, waited for until their count holds still 15 s (tools/soundsmaketest.cpp checks the same offline).
int main(int argc, char** argv) {
    if (argc > 1 && !std::strcmp(argv[1], "--count")) {
        printf("%d sounds\n", sounds::Count());
        return 0;
    }
    if (argc > 2 && !std::strcmp(argv[1], "--make")) {
        wchar_t game[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, argv[2], -1, game, MAX_PATH);
        sounds::MakeMissing([](const char* line) { printf("LAUNCH: %s\n", line); },
                            [](const char* line) { printf("LAUNCH PROBLEM: %s\n", line); }, game);
        int last = -1, still = 0;
        for (int s = 0; s < 300 && still < 15; ++s, Sleep(1000)) {
            const int n = sounds::Count();
            still = n == last && n > 0 ? still + 1 : 0;
            if (n != last)
                printf("%d s: %d sounds\n", s, n);
            last = n;
        }
        return 0;
    }
    sounds::Play(argc > 1 ? argv[1] : "bolt", argc > 2 ? static_cast<float>(std::atof(argv[2])) : 0.3f);
    Sleep(1000);
    return 0;
}
