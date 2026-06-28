#pragma once
#include <DxLib.h>
#include <vector>
#include <string>
#include "IScene.h"
#include "SongSelect.h"  // SongEntry の定義を含む

struct DiffEntry {
    int    diffId;       // 0=Easy 1=Normal 2=Hard 3=Oni 4=Edit
    std::wstring label;  // 表示名
    int    level;        // 星レベル
};

class DiffSelect : public IScene {
public:
    explicit DiffSelect(const SongEntry& song);
    ~DiffSelect();
    bool Update() override;
    void Draw() override;

    // 決定された難易度ID（-1 = まだ未決定）
    int GetSelectedDiffId() const { return decided ? diffs[selectedIndex].diffId : -1; }

private:
    SongEntry    songEntry;
    std::vector<DiffEntry> diffs;
    int          selectedIndex = 0;
    bool         decided = false;

    // フォントハンドル
    int fontTitle = -1;
    int fontButton = -1;
    int fontLevel = -1;

    // サウンド
    int sndDong = -1;
    int sndKa = -1;

    // キー入力（エッジ検出）
    bool prevD = false, prevK = false;
    bool prevJ = false, prevF = false;
    bool prevEsc = false;

    // 難易度ごとの色
    static unsigned int DiffColor(int diffId);
    static unsigned int DiffColorDark(int diffId);
    static const wchar_t* DiffName(int diffId);
};