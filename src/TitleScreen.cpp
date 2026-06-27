#include "TitleScreen.h"
#include <cstring>

bool TitleScreen::Update() {
    return CheckHitKey(KEY_INPUT_RETURN) != 0;
}

void TitleScreen::Draw() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetFontSize(48);
    const wchar_t* titleText = L"TaikoCpp";
    int titleWidth = GetDrawStringWidth(titleText, (int)wcslen(titleText));
    DrawString((screenWidth - titleWidth) / 2, (screenHeight / 2) - 48, titleText, GetColor(255, 255, 255));

    SetFontSize(24);
    const wchar_t* instr = L"Press ENTER to start";
    int instrWidth = GetDrawStringWidth(instr, (int)wcslen(instr));
    DrawString((screenWidth - instrWidth) / 2, (screenHeight / 2) + 48, instr, GetColor(255, 255, 255));
}