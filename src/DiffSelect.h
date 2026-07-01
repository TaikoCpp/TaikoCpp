#pragma once
#include <DxLib.h>
#include <vector>
#include <string>
#include "IScene.h"
#include "SongSelect.h"  // SongEntry を含む
#include "SongInfo.h"    // PlayOptions を共通で使うため包含

struct DiffEntry {
    int    diffId;       // 0=Easy 1=Normal 2=Hard 3=Oni 4=Edit
    std::wstring label;  // 表示ラベル
    int    level;        // 表示レベル
};

class DiffSelect : public IScene {
public:
    explicit DiffSelect(const SongEntry& song);
    ~DiffSelect();
    bool Update() override;
    void Draw() override;

    // 選択難易度ID (未選択時 -1)
    int GetSelectedDiffId() const { return decided ? diffs[selectedIndex].diffId : -1; }
    const SongEntry& GetSongEntry() const { return songEntry; }

    // 演奏オプション取得
    const PlayOptions& GetPlayOptions() const { return options; }

private:
    SongEntry    songEntry;
    std::vector<DiffEntry> diffs;
    int          selectedIndex = 0;
    bool         decided = false;

    // フォント
    int fontTitle = -1;
    int fontButton = -1;
    int fontLevel = -1;

    // 効果音
    int sndDong = -1;
    int sndKa = -1;

    // キー入力の前フレーム状態
    bool prevD = false, prevK = false;
    bool prevJ = false, prevF = false;
    bool prevEsc = false;

    static constexpr int OPT_ROW_COUNT = 3; // 音符 / 速度 / 隠し
    PlayOptions options;
    bool optionsVisible = false;
    int  optionsSel = 0; // 0=音符, 1=速度, 2=隠し
    bool prevO = false;
    bool prevMouseLeft = false;
    void CycleOption(int row, int delta); // ← 追加

    // ユーティリティ
    static unsigned int DiffColor(int diffId);
    static unsigned int DiffColorDark(int diffId);
    static const wchar_t* DiffName(int diffId);
};