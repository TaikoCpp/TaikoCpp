#include "TitleScreen.h"
#include <cstring>

TitleScreen::TitleScreen() {
    fontTitle = CreateFontToHandle(L"FOT-OedoKtr", 48, 4, DX_FONTTYPE_ANTIALIASING_4X4);
    fontSub = CreateFontToHandle(L"FOT-OedoKtr", 24, 2, DX_FONTTYPE_ANTIALIASING_4X4);
}

TitleScreen::~TitleScreen() {
    if (fontTitle != -1) DeleteFontToHandle(fontTitle);
    if (fontSub != -1) DeleteFontToHandle(fontSub);
}

bool TitleScreen::Update() {
    return CheckHitKey(KEY_INPUT_RETURN) != 0;
}

void TitleScreen::Draw() {
    const int SW = 1280, SH = 720;

    const wchar_t* titleText = L"TaikoCpp";
    int tw = GetDrawStringWidthToHandle(titleText, (int)wcslen(titleText), fontTitle);
    DrawStringToHandle((SW - tw) / 2, SH / 2 - 48, titleText,
        GetColor(255, 255, 255), fontTitle);

    const wchar_t* instr = L"Press ENTER to start";
    int iw = GetDrawStringWidthToHandle(instr, (int)wcslen(instr), fontSub);
    DrawStringToHandle((SW - iw) / 2, SH / 2 + 48, instr,
        GetColor(200, 200, 200), fontSub);
}