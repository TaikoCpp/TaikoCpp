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
    GamePlay(const SongEntry& song, int diffId, SongInfo&& chart, bool autoPlay = false, const PlayOptions& options = PlayOptions());
    ~GamePlay();
    bool Update() override;
    void Draw() override;
private:
    SongEntry songEntry;
    int diffId;
    SongInfo chart;
    PlayOptions playOptions; 

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

    // ロール（連打）管理
    struct RollState {
        Note* note = nullptr;
        bool  active = false;
        int   hitCount = 0;
        int   lastAutoMs = 0;
    };
    RollState roll;

    static constexpr int SND_BUF = 32;
    int sndDon[SND_BUF] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    int sndKa[SND_BUF] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
    int sndDonIdx = 0;
    int sndKaIdx = 0;
    int sndWave = -1;
    int fontUI = -1;
    int fontScore = -1;
    int fontSongTitle = -1;          // 右上の曲名表示用フォント (ＤＦＰ勘亭流)
    int titleAnimStartMs = 0;        // 右上曲名フェードアニメーションの基準時刻
    int   titleScrollLastMs = 0;
    int   titleScrollPauseUntilMs = 0;
    float titleScrollOffset = 0.0f;
    int   titleScrollDir = 1;
    int rtTitle = -1;   // 曲タイトル事前描画バッファ
    int rtNumLabel = -1;   // 「○曲目」事前描画バッファ

    // スキン画像
    int backgroundImage = -1;
    int baseImage = -1;
    int courseSymbolImage = -1;
    int mainBackgroundImage = -1;
    int subBackgroundImage = -1;
    int frameImage = -1;

    // Notes.png スプライトシート
    int notesImage = -1;

    // デバッグ表示フラグ (F2 でトグル)
    bool showFps = true;
    bool prevF2 = false;

    // オートプレイ
    bool autoPlay = false;

    bool prevDon = false;
    bool prevKa = false;
    bool prevEsc = false;
    bool prevF1 = false;

    // リザルト表示
    bool showResult = false;
    int  resultStartMs = 0;

    static constexpr int JUDGE_X = 410;   // col0判定枠の中心x (theme.aup2より)

    // スクロール速度
    // BPM120・#SCROLL 1.0 のとき: 870px / (4 × 500ms) = 0.435 px/ms
    // NMSCROLL … GamePlay で (note.bpm/120) × note.scroll_x を乗算
    // BMSCROLL/HBSCROLL … note.bpm=120 固定 (BPM補正不要)、note.scroll_x に累積倍率込み
    static constexpr float SCROLL_SPEED = 1.0f;             // ← ここを変えるだけで速さ調整
    static constexpr float BASE_SCROLL_PX_PER_MS = 0.435f;  // BPM120, scroll_x=1.0, speed=1.0
    static constexpr float JUDGE_RYO_MS = 35.0f;
    static constexpr float JUDGE_KA_MS = 80.0f;

    // ↓ 追加：ノーツが右端(x=1280)から判定枠(JUDGE_X=410)まで移動するのに要する時間
// = (1280 - 410) / (0.435 * 1.0) = 2000ms
    static constexpr float PREROLL_MS =
        (1280.0f - static_cast<float>(JUDGE_X)) / (BASE_SCROLL_PX_PER_MS * SCROLL_SPEED);

    // 既存の bool audioStarted などはない → 追加
    bool audioStarted = false;   // プリロール後に音声開始したか

    // Notes.png スプライト定義 (src_x, src_w, row0_y=0)
    // [0]判定枠  [1]小ドン  [2]小カッ  [3]大ドン  [4]大カッ
    // [5]バルーン [6]ロールbody小  [7]大バルーン顔  [8]ロールbody大  [9]kusudama
    static constexpr int NS_SRC_X[10] = { 11, 159, 289, 401, 531, 679,  780, 1051, 1170, 1459 };
    static constexpr int NS_SRC_W[10] = { 106,  74,  74, 110, 110,  70,  165,  106,  185,  179 };
    static constexpr int NS_SRC_H = 130;   // 1行の高さ
    static constexpr int NS_ROW_H = 130;   // 各行のy幅

    // 描画先サイズ: theme.aup2実測値 (src 130px を 0.9862倍 ≒ 128px)
    // 小・大・判定枠すべて同じ高さ128px (AviUtlでも行変えのみでサイズ同一)
    static constexpr int NS_DST_H_S = 128;
    static constexpr int NS_DST_H_L = 128;
    static constexpr int NS_DST_H_JUDGE = 128;

    // レーン定数
    static constexpr int LANE_TOP = 183;
    static constexpr int LANE_BOTTOM = 357;
    static constexpr int LANE_CY = 258;  // Notes row0 center (theme.aup2より)

    float GetChartMs() const;
    float CalcScrollPxPerMs(const Note& note) const;
    float NoteScreenX(const Note& note) const;
    bool IsHittable(const Note& note) const;
    void ProcessInput(bool donPressed, bool kaPressed);
    void JudgeNote(ActiveNote& active, bool don, bool ka);
    void RegisterJudge(JudgeResult result, NoteType type);
    void DrawNote(const Note& note, float x) const;
    void DrawNoteSprite(int col, int row, int cx, int cy, int dst_h, bool trans = true) const;
    void DrawBarLines() const;
    void DrawSongTitleCycle();
    void DrawSongTitleCycleText(const std::wstring& text, int x, int y, float alpha) const;
    std::wstring PathToWide(const std::filesystem::path& p) const;
    int FindFreeSoundSlot(const int* sndBuf, int& idx) const;
};