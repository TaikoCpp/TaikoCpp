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
    std::locale::global(std::locale(""));  // 日本語ファイル名対応

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

    while (ProcessMessage() == 0) {
        ClearDrawScreen();

        if (currentScene->Update()) {
            TitleScreen* title = dynamic_cast<TitleScreen*>(currentScene.get());
            if (title) {
                currentScene = std::make_unique<SongSelect>();
            }
            else {
                SongSelect* ss = dynamic_cast<SongSelect*>(currentScene.get());
                if (ss) {
                    const SongEntry* selected = ss->GetSelectedSong();
                    if (selected) {
                        currentScene = std::make_unique<DiffSelect>(*selected);
                    }
                    else {
                        currentScene = std::make_unique<TitleScreen>();
                    }
                }
                else {
                    DiffSelect* ds = dynamic_cast<DiffSelect*>(currentScene.get());
                    if (ds) {
                        int diffId = ds->GetSelectedDiffId();
                        if (diffId >= 0) {
                            const SongEntry& song = ds->GetSongEntry();
                            TJAParser parser(song.tjaPath);
                            SongInfo chart = parser.parse(diffId);
                            currentScene = std::make_unique<GamePlay>(
                                song, diffId, std::move(chart));
                        }
                        else {
                            currentScene = std::make_unique<SongSelect>();
                        }
                    }
                    else {
                        GamePlay* gp = dynamic_cast<GamePlay*>(currentScene.get());
                        if (gp) {
                            currentScene = std::make_unique<SongSelect>();
                        }
                    }
                }
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
        DrawFormatString(10, 10, GetColor(255, 255, 0), _T("FPS: %d"), fps);

        ScreenFlip();
    }

    DxLib_End();
    return 0;
}