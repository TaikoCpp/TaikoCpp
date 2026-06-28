#pragma once
#include <DxLib.h>
#include <vector>
#include <string>
#include <filesystem>
#include "IScene.h"
#include "SongSelect.h"
#include "SongInfo.h"

enum class JudgeResult {
    None,
    Ryo,
    Ka,
    Fuka
};

struct ActiveNote {
    Note* note = nullptr;
    bool judged = false;
};

// Notes.png スプライトシート定数
// 行: row0=通常(y=0), row1=中(y=130), row2=大(y=260)  高さ130px/行
// 列 (x, w):
struct NoteSprite {
    int src_x, src_w;           // Notes.png 上のクロップ (高さは常に130)
    int dst_w_small;            // 小サイズ描画幅
    int dst_w_large;            // 大サイズ描画幅
};

class GamePlay : public IScene {
public:
    GamePlay(const SongEntry& song, int diffId, SongInfo&& chart);
    ~GamePlay();
    bool Update() override;
    void Draw() override;

private:
    SongEntry songEntry;
    int diffId;
    SongInfo chart;

    std::vector<ActiveNote> notes;
    size_t nextNoteIndex = 0;

    int playStartMs = 0;
    float songEndMs = 0.0f;

    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int ryoCount = 0;
    int kaCount = 0;
    int fukaCount = 0;

    static constexpr int SND_BUF = 32;
    int sndDon[SND_BUF] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    int sndKa[SND_BUF] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    int sndDonIdx = 0;
    int sndKaIdx = 0;
    int sndWave = -1;
    int fontUI = -1;
    int fontScore = -1;

    // スキン画像
    int backgroundImage = -1;
    int baseImage = -1;
    int courseSymbolImage = -1;
    int mainBackgroundImage = -1;
    int frameImage = -1;

    // Notes.png スプライトシート
    int notesImage = -1;

    // デバッグ表示フラグ (F2 でトグル)
    bool showFps = true;
    bool prevF2 = false;

    bool prevDon = false;
    bool prevKa = false;
    bool prevEsc = false;

    static constexpr int JUDGE_X = 390;
    static constexpr float SCROLL_PX_PER_MS = 0.55f;
    static constexpr float JUDGE_RYO_MS = 35.0f;
    static constexpr float JUDGE_KA_MS = 80.0f;

    // Notes.png スプライト定義 (src_x, src_w, row0_y=0)
    // [0]判定枠  [1]小ドン  [2]小カッ  [3]大ドン  [4]大カッ
    // [5]バルーン [6]ロールbody小  [7]大バルーン顔  [8]ロールbody大  [9]kusudama
    static constexpr int NS_SRC_X[10] = { 11, 159, 289, 401, 531, 679,  780, 1051, 1170, 1459 };
    static constexpr int NS_SRC_W[10] = { 106,  74,  74, 110, 110,  70,  165,  106,  185,  179 };
    static constexpr int NS_SRC_H = 130;   // 1行の高さ
    static constexpr int NS_ROW_H = 130;   // 各行のy幅

    // 描画先サイズ: TJAPlayer3 参照実装に合わせ src と同じ高さで描画
    // レーン高さ 174px に対して: 小=130px(等倍), 大=174px(レーン全高)
    static constexpr int NS_DST_H_S = 130;
    static constexpr int NS_DST_H_L = 174;

    // レーン定数
    static constexpr int LANE_TOP = 183;
    static constexpr int LANE_BOTTOM = 357;
    static constexpr int LANE_CY = 270;  // (183+357)/2

    float GetChartMs() const;
    float NoteScreenX(const Note& note) const;
    bool IsHittable(const Note& note) const;
    void ProcessInput(bool donPressed, bool kaPressed);
    void JudgeNote(ActiveNote& active, bool don, bool ka);
    void RegisterJudge(JudgeResult result, NoteType type);
    void DrawNote(const Note& note, float x) const;
    void DrawNoteSprite(int col, int row, int cx, int cy, int dst_h, bool trans = true) const;
    void DrawBarLines() const;
    std::wstring PathToWide(const std::filesystem::path& p) const;
    int FindFreeSoundSlot(const int* sndBuf, int& idx) const;
};