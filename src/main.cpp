#include <DxLib.h>
#include <memory>
#include <tchar.h>
#include "IScene.h"
#include "TitleScreen.h"
#include "SongSelect.h"
#include "DiffSelect.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE);
    SetWaitVSyncFlag(TRUE);
    SetGraphMode(1280, 720, 32);
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    SetWindowSize(1280, 720);
    SetMainWindowText(_T("TaikoCpp"));

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    std::unique_ptr<IScene> currentScene = std::make_unique<TitleScreen>();

    // FPS計算用
    int frameCount = 0;
    int lastTime = GetNowCount();
    int fps = 0;

    while (ProcessMessage() == 0) {
        ClearDrawScreen();

        if (currentScene->Update()) {
            // ── TitleScreen → SongSelect
            if (auto* title = dynamic_cast<TitleScreen*>(currentScene.get())) {
                currentScene = std::make_unique<SongSelect>();
            }
            // ── SongSelect → DiffSelect（曲決定） or TitleScreen（ESC）
            else if (auto* ss = dynamic_cast<SongSelect*>(currentScene.get())) {
                const SongEntry* selected = ss->GetSelectedSong();
                if (selected) {
                    currentScene = std::make_unique<DiffSelect>(*selected);
                }
                else {
                    // ESC → タイトルへ
                    currentScene = std::make_unique<TitleScreen>();
                }
            }
            // ── DiffSelect → SongSelect（ESC or 決定後、将来はゲームプレイへ）
            else if (auto* ds = dynamic_cast<DiffSelect*>(currentScene.get())) {
                int diffId = ds->GetSelectedDiffId();
                if (diffId >= 0) {
                    // TODO: GamePlay シーンへ遷移
                    // currentScene = std::make_unique<GamePlay>(song, diffId);
                    // 暫定: SongSelect に戻る
                    currentScene = std::make_unique<SongSelect>();
                }
                else {
                    // ESC → SongSelect へ
                    currentScene = std::make_unique<SongSelect>();
                }
            }
        }

        currentScene->Draw();

        // FPS表示
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