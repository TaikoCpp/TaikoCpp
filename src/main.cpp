#include <DxLib.h>
#include "TitleScreen.h"
#include "SongSelect.h"

enum AppState {
    STATE_TITLE,
    STATE_SONGSELECT
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE);
    SetWaitVSyncFlag(TRUE);
    SetGraphMode(1280, 720, 32);
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    SetWindowSize(1280, 720);

    if (DxLib_Init() == -1) return -1;

    SetMainWindowText(L"TaikoCpp"); // ウインドウタイトル設定
    SetDrawScreen(DX_SCREEN_BACK);

    TitleScreen title;
    SongSelect songSelect;
    AppState state = STATE_TITLE;

    // メインループ
    while (ProcessMessage() == 0) {
        ClearDrawScreen(); // 描画開始前に一度だけクリア

        switch (state) {
        case STATE_TITLE:
            if (title.Update()) state = STATE_SONGSELECT;
            title.Draw();
            break;
        case STATE_SONGSELECT:
            if (songSelect.Update()) state = STATE_TITLE;
            songSelect.Draw();
            break;
        }

        ScreenFlip(); // 最後に一度だけ反映
    }

    DxLib_End();
    return 0;
}