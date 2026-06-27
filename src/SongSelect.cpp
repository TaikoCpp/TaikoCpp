#include "SongSelect.h"
#include <string>
#include <codecvt>
#include <locale>
#include <utility>

SongSelect::SongSelect(SongInfo songInfo) : songInfo(std::move(songInfo)) {}

bool SongSelect::Update() {
    // Return true when ESC is pressed to go back to the title screen
    if (CheckHitKey(KEY_INPUT_ESCAPE) != 0) {
        return true;
    }
    // For now, Enter does nothing (could be used to start a song)
    return false;
}

void SongSelect::Draw() {

    const int screenWidth = 1280;
    const int screenHeight = 720;

    // Title - use song title if available, otherwise default
    const int titleFontSize = 48;
    SetFontSize(titleFontSize);
    std::wstring titleText;
    if (!songInfo.metadata.title.empty()) {
        // Get English title if available, otherwise first available
        auto it = songInfo.metadata.title.find("en");
        if (it != songInfo.metadata.title.end()) {
            titleText = std::wstring(it->second.begin(), it->second.end());
        } else {
            // Use first available title
            titleText = std::wstring(songInfo.metadata.title.begin()->second.begin(),
                                   songInfo.metadata.title.begin()->second.end());
        }
    } else {
        titleText = L"Select Song";
    }
    int titleWidth = GetDrawStringWidth(titleText.c_str(), static_cast<int>(titleText.length()));
    DrawString((screenWidth - titleWidth) / 2, 100, titleText.c_str(), GetColor(255, 255, 255));

    // Display some metadata
    const int infoFontSize = 24;
    SetFontSize(infoFontSize);
    std::wstring infoText = L"BPM: " + std::to_wstring(songInfo.metadata.bpm);
    int infoWidth = GetDrawStringWidth(infoText.c_str(), static_cast<int>(infoText.length()));
    DrawString((screenWidth - infoWidth) / 2, 200, infoText.c_str(), GetColor(255, 255, 255));

    // Artist/other info if available
    // For simplicity, we'll show genre if available
    if (!songInfo.metadata.genre.empty()) {
        std::wstring genreText = L"Genre: " + std::wstring(songInfo.metadata.genre.begin(), songInfo.metadata.genre.end());
        int genreWidth = GetDrawStringWidth(genreText.c_str(), static_cast<int>(genreText.length()));
        DrawString((screenWidth - genreWidth) / 2, 230, genreText.c_str(), GetColor(255, 255, 255));
    }

    // Sample song list (we could use actual notes from songInfo, but for simplicity we'll show placeholders)
    const int itemFontSize = 28;
    SetFontSize(itemFontSize);
    const wchar_t* items[] = {
        L"Song 1: Song Title A",
        L"Song 2: Song Title B (Hard)",
        L"Song Title C (Oni)"
    };
    int startY = 300;
    int spacing = 40;
    for (int i = 0; i < 3; ++i) {
        int itemWidth = GetDrawStringWidth(items[i], static_cast<int>(wcslen(items[i])));
        DrawString((screenWidth - itemWidth) / 2, startY + i * spacing, items[i], GetColor(255, 255, 255));
    }

    // Instructions
    const int instFontSize = 24;
    SetFontSize(instFontSize);
    const wchar_t* instr = L"Press ENTER to select, ESC to go back";
    int instrWidth = GetDrawStringWidth(instr, static_cast<int>(wcslen(instr)));
    DrawString((screenWidth - instrWidth) / 2, screenHeight - 50, instr, GetColor(255, 255, 255));

    // Reset font size to default (optional)
    SetFontSize(20);
}