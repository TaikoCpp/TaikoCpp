#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include <filesystem>
#include "IScene.h"
#include "SongInfo.h"
#include "BoxDef.h"

namespace fs = std::filesystem;

struct SongEntry {
    std::wstring title;
    fs::path tjaPath;
};

enum class SelectItemType {
    Folder,
    Song
};

struct SelectItem {
    SelectItemType type = SelectItemType::Song;
    std::wstring title;
    fs::path path;
    BoxDefInfo boxInfo;
};

class SongSelect : public IScene {
public:
    SongSelect();
    ~SongSelect();
    bool Update() override;
    void Draw() override;

    // åàíËÇ≥ÇÍÇΩã»ÇéÊìæÅiUpdate Ç™ true Çï‘ÇµÇΩå„Ç…éQè∆Åj
    const SongEntry* GetSelectedSong() const {
        return songDecided ? &selectedSong : nullptr;
    }

private:
    void LoadDirectory(const fs::path& dir);
    std::vector<fs::path> FindTjaFilesInDirectory(const fs::path& dir) const;
    void DrawRoundRect(int x1, int y1, int x2, int y2, int r, unsigned int color, bool fill);

    fs::path songsRoot;
    fs::path currentDir;
    std::vector<SelectItem> items;
    SongEntry selectedSong;
    int  selectedIndex = 0;
    bool songDecided = false;

    float scrollY = 0.0f;
    float targetScrollY = 0.0f;

    int fontLarge = -1;
    int fontNormal = -1;
    int fontUI = -1;

    int sndDong = -1;
    int sndKa = -1;

    bool prevD = false, prevK = false;
    bool prevJ = false, prevF = false;
    bool prevEsc = false;
};
