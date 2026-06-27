#include <DxLib.h>
#include <memory>
#include <tchar.h>
#include "IScene.h"
#include "TitleScreen.h"
#include "SongSelect.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // DxLib初期化設定
    ChangeWindowMode(TRUE);
    SetWaitVSyncFlag(TRUE);
    SetGraphMode(1280, 720, 32);
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    SetWindowSize(1280, 720);
    SetMainWindowText(_T("TaikoCpp"));

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    // インターフェース型のポインタで管理
    std::unique_ptr<IScene> currentScene = std::make_unique<TitleScreen>();

    // FPS計算用
    int frameCount = 0;
    int lastTime = GetNowCount();
    int fps = 0;

    // メインループ
    while (ProcessMessage() == 0) {
        ClearDrawScreen();

        // シーンの更新と遷移判定
        if (currentScene->Update()) {
            if (dynamic_cast<TitleScreen*>(currentScene.get())) {
                currentScene = std::make_unique<SongSelect>();
            }
            else {
                currentScene = std::make_unique<TitleScreen>();
            }
        }

        // シーンの描画
        currentScene->Draw();

        // FPS計算
        frameCount++;
        int currentTime = GetNowCount();
        if (currentTime - lastTime >= 1000) {
            fps = frameCount * 1000 / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
        DrawFormatString(10, 10, GetColor(255, 255, 0), _T("FPS: %d"), fps);

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}