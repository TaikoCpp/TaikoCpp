#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include <filesystem>
#include "IScene.h"
#include "SongInfo.h"

namespace fs = std::filesystem;

struct SongEntry {
    std::wstring title;
    fs::path tjaPath;
};

class SongSelect : public IScene {
public:
    SongSelect();
    ~SongSelect();
    bool Update() override;
    void Draw() override;

private:
    void ScanSongs(const fs::path& songsDir);
    void DrawRoundRect(int x1, int y1, int x2, int y2, int r, unsigned int color, bool fill);

    std::vector<SongEntry> songs;
    int selectedIndex = 0;

    float scrollY = 0.0f;
    float targetScrollY = 0.0f;

    // フォントハンドル
    int fontLarge = -1;  // 選択中曲名
    int fontNormal = -1;  // 通常曲名
    int fontUI = -1;  // ヘッダ・ヒント

    // キー入力（エッジ検出）
    bool prevD = false, prevK = false;
    bool prevJ = false, prevF = false;
    bool prevEsc = false;
};