#include "windows.h"
#include <DxLib.h>
#include <memory>
#include <locale>
#include <tchar.h>
#include "IScene.h"
#include "TitleScreen.h"
#include "SongSelect.h"
#include "DiffSelect.h"
#include "GamePlay.h"
#include "TJAParser.h"
#include "Config.h"          // ← 追加

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::locale::global(std::locale(""));

    // --- config.ini 読み込み（なければ自動生成） ---
    Config config;
    config.Load();

    ChangeWindowMode(TRUE);
    SetWaitVSyncFlag(TRUE);
    SetGraphMode(1280, 720, 32);
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    SetWindowSize(1280, 720);
    SetMainWindowText(_T("TaikoCpp"));

    int fontAdded = AddFontResourceEx(
        L"Theme\\default\\Fonts\\FOT-OedoKtr.otf", FR_PRIVATE, NULL);
    OutputDebugStringW(fontAdded > 0
        ? L"[Font] FOT-OedoKtr loaded OK\n"
        : L"[Font] FOT-OedoKtr FAILED - check path!\n");

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    std::unique_ptr<IScene> currentScene = std::make_unique<TitleScreen>();

    int frameCount = 0;
    int lastTime = GetNowCount();
    int fps = 0;
    bool showFps = true;
    bool prevF2 = false;
    bool autoPlay = config.autoPlay;  // ← config から初期値を取得
    bool prevF1 = false;

    while (ProcessMessage() == 0) {
        ClearDrawScreen();

        bool curF2 = CheckHitKey(KEY_INPUT_F2) != 0;
        if (curF2 && !prevF2) showFps = !showFps;
        prevF2 = curF2;

        bool curF1 = CheckHitKey(KEY_INPUT_F1) != 0;
        if (curF1 && !prevF1) {
            autoPlay = !autoPlay;
            config.autoPlay = autoPlay;  // ← 変更を config に反映
            config.Save();               // ← 即座に保存
        }
        prevF1 = curF1;

        if (currentScene->Update()) {
            TitleScreen* title = dynamic_cast<TitleScreen*>(currentScene.get());
            SongSelect* ss = dynamic_cast<SongSelect*>(currentScene.get());
            DiffSelect* ds = dynamic_cast<DiffSelect*>(currentScene.get());
            GamePlay* gp = dynamic_cast<GamePlay*>(currentScene.get());

            if (title) {
                currentScene = std::make_unique<SongSelect>();
            }
            else if (ss) {
                const SongEntry* selected = ss->GetSelectedSong();
                if (selected) {
                    currentScene = std::make_unique<DiffSelect>(*selected);
                }
                else {
                    currentScene = std::make_unique<TitleScreen>();
                }
            }
            else if (ds) {
                int diffId = ds->GetSelectedDiffId();
                if (diffId >= 0) {
                    const SongEntry& song = ds->GetSongEntry();
                    TJAParser parser(song.tjaPath);
                    SongInfo chart = parser.parse(diffId);
                    currentScene = std::make_unique<GamePlay>(
                        song, diffId, std::move(chart), autoPlay);
                }
                else {
                    currentScene = std::make_unique<SongSelect>();
                }
            }
            else if (gp) {
                currentScene.reset();
                currentScene = std::make_unique<SongSelect>();
            }
        }

        currentScene->Draw();

        frameCount++;
        int currentTime = GetNowCount();
        if (currentTime - lastTime >= 1000) {
            fps = frameCount * 1000 / (currentTime - lastTime);
            frameCount = 0;
            lastTime = currentTime;
        }
        if (showFps)
            DrawFormatString(10, 10, GetColor(255, 255, 0), _T("FPS: %d"), fps);
        DrawFormatString(10, 30, GetColor(255, 215, 0), _T("AUTO: %s [F1]"),
            autoPlay ? _T("ON") : _T("OFF"));

        ScreenFlip();
    }

    RemoveFontResourceEx(
        L"Theme\\default\\Fonts\\FOT-OedoKtr.otf", FR_PRIVATE, NULL);
    DxLib_End();
    return 0;
}