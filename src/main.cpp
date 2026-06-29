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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::locale::global(std::locale(""));

    ChangeWindowMode(TRUE);
    SetWaitVSyncFlag(TRUE);
    SetGraphMode(1280, 720, 32);
    SetWindowSizeChangeEnableFlag(TRUE, FALSE);
    SetWindowSize(1280, 720);
    SetMainWindowText(_T("TaikoCpp"));

    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    std::unique_ptr<IScene> currentScene = std::make_unique<TitleScreen>();

    int frameCount = 0;
    int lastTime = GetNowCount();
    int fps = 0;
    bool showFps = true;
    bool prevF2 = false;
    bool autoPlay = false;
    bool prevF1 = false;

    while (ProcessMessage() == 0) {
        ClearDrawScreen();

        // F2 で FPS 表示トグル
        bool curF2 = CheckHitKey(KEY_INPUT_F2) != 0;
        if (curF2 && !prevF2) showFps = !showFps;
        prevF2 = curF2;

        // F1 でオートプレイトグル
        bool curF1 = CheckHitKey(KEY_INPUT_F1) != 0;
        if (curF1 && !prevF1) autoPlay = !autoPlay;
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
        DrawFormatString(10, 30, GetColor(255, 215, 0), _T("AUTO: %s [F1]"), autoPlay ? _T("ON") : _T("OFF"));

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}