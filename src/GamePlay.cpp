#define NOMINMAX
#include "GamePlay.h"
#include <algorithm>
#include <cmath>

namespace fs = std::filesystem;

GamePlay::GamePlay(const SongEntry& song, int diffId, SongInfo&& chartData, bool autoPlay)
    : songEntry(song), diffId(diffId), chart(std::move(chartData)), autoPlay(autoPlay) {
    fontUI = CreateFontToHandle(L"\u30e1\u30a4\u30ea\u30aa", 22, 2, DX_FONTTYPE_ANTIALIASING_4X4);
    fontScore = CreateFontToHandle(L"\u30e1\u30a4\u30ea\u30aa", 36, 3, DX_FONTTYPE_ANTIALIASING_4X4);

    for (int i = 0; i < SND_BUF; i++) {
        sndDon[i] = LoadSoundMem(L"Theme\\default\\sounds\\dong.wav");
        sndKa[i] = LoadSoundMem(L"Theme\\default\\sounds\\ka.wav");
    }

    // Load all images from the 05_Game skin folder
    backgroundImage = LoadGraph(L"Theme\\default\\img\\05_Game\\1P_Background.png");
    baseImage = LoadGraph(L"Theme\\default\\img\\05_Game\\Base.png");
    courseSymbolImage = LoadGraph(L"Theme\\default\\img\\05_Game\\coursesymbol_oni.png");
    mainBackgroundImage = LoadGraph(L"Theme\\default\\img\\05_Game\\Background_Main.png");
    subBackgroundImage = LoadGraph(L"Theme\\default\\img\\05_Game\\Background_Sub.png");
    frameImage = LoadGraph(L"Theme\\default\\img\\05_Game\\1P_Frame.png");

    // Notes スプライトシート
    notesImage = LoadGraph(L"Theme\\default\\img\\05_Game\\Notes.png");

    for (Note* note : chart.master_notes.play_notes) {
        if (!IsHittable(*note)) continue;
        notes.push_back({ note, false });
    }
    std::sort(notes.begin(), notes.end(),
        [](const ActiveNote& a, const ActiveNote& b) {
            return a.note->hit_ms < b.note->hit_ms;
        });

    if (!notes.empty()) {
        songEndMs = notes.back().note->hit_ms + 3000.0f;
    }
    else {
        songEndMs = 5000.0f;
    }

    if (!chart.metadata.wave.empty()) {
        std::wstring wavePath = PathToWide(chart.metadata.wave);
        sndWave = LoadSoundMem(wavePath.c_str());
    }

    playStartMs = GetNowCount();
    if (sndWave != -1) {
        PlaySoundMem(sndWave, DX_PLAYTYPE_BACK);
    }
}

GamePlay::~GamePlay() {
    if (sndWave != -1) {
        StopSoundMem(sndWave);
        DeleteSoundMem(sndWave);
    }
    for (int i = 0; i < SND_BUF; i++) {
        if (sndDon[i] != -1) DeleteSoundMem(sndDon[i]);
        if (sndKa[i] != -1) DeleteSoundMem(sndKa[i]);
    }
    if (fontUI != -1) DeleteFontToHandle(fontUI);
    if (fontScore != -1) DeleteFontToHandle(fontScore);
    if (backgroundImage != -1) DeleteGraph(backgroundImage);
    if (baseImage != -1) DeleteGraph(baseImage);
    if (courseSymbolImage != -1) DeleteGraph(courseSymbolImage);
    if (mainBackgroundImage != -1) DeleteGraph(mainBackgroundImage);
    if (subBackgroundImage != -1) DeleteGraph(subBackgroundImage);
    if (frameImage != -1) DeleteGraph(frameImage);
    if (notesImage != -1) DeleteGraph(notesImage);
}

std::wstring GamePlay::PathToWide(const fs::path& p) const {
    return p.wstring();
}

float GamePlay::GetChartMs() const {
    return static_cast<float>(GetNowCount() - playStartMs) +
        chart.metadata.offset * 1000.0f;
}

// スクロール実効速度を計算する。
// NMSCROLL: note.bpm を実BPMとして保持しているため (note.bpm/120) で BPM 補正する。
// BMSCROLL/HBSCROLL: note.bpm=120 固定 (TJAParser が bpmchange 累積倍率を scroll_x に焼き込み済み)
//   → (note.bpm/120) = 1.0 となり BPM 補正は自動的に無効化される。
float GamePlay::CalcScrollPxPerMs(const Note& note) const {
    float bpmRatio = (note.bpm > 0.0f) ? (note.bpm / 120.0f) : 1.0f;
    return BASE_SCROLL_PX_PER_MS * bpmRatio * note.scroll_x * SCROLL_SPEED;
}

float GamePlay::NoteScreenX(const Note& note) const {
    float chartMs = GetChartMs();
    return static_cast<float>(JUDGE_X) +
        (note.hit_ms - chartMs) * CalcScrollPxPerMs(note);
}

bool GamePlay::IsHittable(const Note& note) const {
    switch (note.type) {
    case NoteType::DON:
    case NoteType::KAT:
    case NoteType::DON_L:
    case NoteType::KAT_L:
    case NoteType::ROLL_HEAD:
    case NoteType::ROLL_HEAD_L:
    case NoteType::BALLOON_HEAD:
    case NoteType::KUSUDAMA:
        return true;
    default:
        return false;
    }
}

void GamePlay::RegisterJudge(JudgeResult result, NoteType type) {
    bool isBig = type == NoteType::DON_L || type == NoteType::KAT_L ||
        type == NoteType::ROLL_HEAD_L;

    switch (result) {
    case JudgeResult::Ryo:
        ryoCount++;
        combo++;
        maxCombo = (std::max)(maxCombo, combo);  // NOMINMAX�΍�
        score += isBig ? 330 : 330;
        break;
    case JudgeResult::Ka:
        kaCount++;
        combo = 0;
        score += isBig ? 160 : 160;
        break;
    case JudgeResult::Fuka:
        fukaCount++;
        combo = 0;
        break;
    default:
        break;
    }
}

void GamePlay::JudgeNote(ActiveNote& active, bool don, bool ka) {
    if (active.judged) return;

    float chartMs = GetChartMs();
    float absDelta = std::fabs(chartMs - active.note->hit_ms);

    NoteType type = active.note->type;
    bool expectDon = type == NoteType::DON || type == NoteType::DON_L ||
        type == NoteType::ROLL_HEAD || type == NoteType::ROLL_HEAD_L ||
        type == NoteType::BALLOON_HEAD || type == NoteType::KUSUDAMA;
    bool expectKa = type == NoteType::KAT || type == NoteType::KAT_L;

    if (absDelta <= JUDGE_RYO_MS) {
        if ((expectDon && don) || (expectKa && ka)) {
            active.judged = true;
            RegisterJudge(JudgeResult::Ryo, type);
            if (don) {
                int slot = FindFreeSoundSlot(sndDon, sndDonIdx);
                if (sndDon[slot] != -1) PlaySoundMem(sndDon[slot], DX_PLAYTYPE_BACK);
            }
            if (ka) {
                int slot = FindFreeSoundSlot(sndKa, sndKaIdx);
                if (sndKa[slot] != -1) PlaySoundMem(sndKa[slot], DX_PLAYTYPE_BACK);
            }
        }
    }
    else if (absDelta <= JUDGE_KA_MS) {
        if ((expectDon && don) || (expectKa && ka)) {
            active.judged = true;
            RegisterJudge(JudgeResult::Ka, type);
            if (don) {
                int slot = FindFreeSoundSlot(sndDon, sndDonIdx);
                if (sndDon[slot] != -1) PlaySoundMem(sndDon[slot], DX_PLAYTYPE_BACK);
            }
            if (ka) {
                int slot = FindFreeSoundSlot(sndKa, sndKaIdx);
                if (sndKa[slot] != -1) PlaySoundMem(sndKa[slot], DX_PLAYTYPE_BACK);
            }
        }
    }
}

void GamePlay::ProcessInput(bool donPressed, bool kaPressed) {
    float chartMs = GetChartMs();

    for (auto& active : notes) {
        if (active.judged) continue;
        float delta = std::fabs(chartMs - active.note->hit_ms);
        if (delta > JUDGE_KA_MS) continue;
        if (donPressed || kaPressed) {
            JudgeNote(active, donPressed, kaPressed);
        }
    }
}

bool GamePlay::Update() {
    bool curDon = (CheckHitKey(KEY_INPUT_J) || CheckHitKey(KEY_INPUT_F)) != 0;
    bool curKa = (CheckHitKey(KEY_INPUT_D) || CheckHitKey(KEY_INPUT_K)) != 0;
    bool curEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;
    bool curF2 = CheckHitKey(KEY_INPUT_F2) != 0;

    // F2 で FPS 表示トグル
    if (curF2 && !prevF2) showFps = !showFps;
    prevF2 = curF2;

    if (curEsc && !prevEsc) {
        prevDon = curDon; prevKa = curKa; prevEsc = curEsc;
        return true;
    }

    if ((curDon && !prevDon) || (curKa && !prevKa)) {
        ProcessInput(curDon && !prevDon, curKa && !prevKa);
    }

    // オートプレイ: hit_ms を過ぎたノートを自動で叩く
    if (autoPlay) {
        float chartMs = GetChartMs();
        for (auto& active : notes) {
            if (active.judged) continue;
            if (chartMs < active.note->hit_ms) continue;  // まだ時間が来ていない
            NoteType type = active.note->type;
            bool isDon = type == NoteType::DON || type == NoteType::DON_L ||
                type == NoteType::ROLL_HEAD || type == NoteType::ROLL_HEAD_L ||
                type == NoteType::BALLOON_HEAD || type == NoteType::KUSUDAMA;
            bool isKa = type == NoteType::KAT || type == NoteType::KAT_L;
            JudgeNote(active, isDon, isKa);
        }
    }

    float chartMs = GetChartMs();
    while (nextNoteIndex < notes.size()) {
        ActiveNote& active = notes[nextNoteIndex];
        if (active.judged) {
            nextNoteIndex++;
            continue;
        }
        if (chartMs - active.note->hit_ms > JUDGE_KA_MS) {
            active.judged = true;
            RegisterJudge(JudgeResult::Fuka, active.note->type);
            nextNoteIndex++;
        }
        else {
            break;
        }
    }

    if (chartMs >= songEndMs) {
        prevDon = curDon; prevKa = curKa; prevEsc = curEsc;
        return true;
    }

    prevDon = curDon; prevKa = curKa; prevEsc = curEsc;
    return false;
}

void GamePlay::DrawNoteSprite(int col, int row, int cx, int cy, int dst_h, bool trans) const {
    if (notesImage == -1) return;
    int src_x = NS_SRC_X[col];
    int src_w = NS_SRC_W[col];
    // dst_w は元画像の縦横比を保って計算
    int dst_w = static_cast<int>(src_w * (static_cast<float>(dst_h) / NS_SRC_H));
    int src_y = row * NS_ROW_H;
    int dx1 = cx - dst_w / 2;
    int dy1 = cy - dst_h / 2;
    int dx2 = dx1 + dst_w;
    int dy2 = dy1 + dst_h;
    DrawRectExtendGraph(dx1, dy1, dx2, dy2,
        src_x, src_y, src_w, NS_SRC_H, notesImage, trans ? TRUE : FALSE);
}

void GamePlay::DrawNote(const Note& note, float x) const {
    const int cy = LANE_CY;
    const int cx = static_cast<int>(x);

    switch (note.type) {
    case NoteType::DON:
        DrawNoteSprite(1, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::KAT:
        DrawNoteSprite(2, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::DON_L:
        DrawNoteSprite(3, 0, cx, cy, NS_DST_H_L);
        break;
    case NoteType::KAT_L:
        DrawNoteSprite(4, 0, cx, cy, NS_DST_H_L);
        break;
    case NoteType::ROLL_HEAD:
    case NoteType::ROLL_HEAD_L: {
        bool isLarge = (note.type == NoteType::ROLL_HEAD_L);
        int bodyCol = isLarge ? 8 : 6;
        int headCol = isLarge ? 3 : 1;
        int dstH = isLarge ? NS_DST_H_L : NS_DST_H_S;
        int bodySrcW = NS_SRC_W[bodyCol];
        int bodyDstW = static_cast<int>(bodySrcW * (static_cast<float>(dstH) / NS_SRC_H));
        if (bodyDstW <= 0) bodyDstW = 1;
        float chartMs = GetChartMs();
        int rollEndX;
        if (note.unload_ms > note.hit_ms) {
            float endScreenX = static_cast<float>(JUDGE_X) +
                (note.unload_ms - chartMs) * CalcScrollPxPerMs(note);
            rollEndX = static_cast<int>(endScreenX);
        }
        else {
            rollEndX = cx + 120;
        }
        for (int bx = cx; bx < rollEndX; bx += bodyDstW) {
            int bx1 = bx;
            int bx2 = bx + bodyDstW;
            DrawRectExtendGraph(bx1, cy - dstH / 2, bx2, cy + dstH / 2,
                NS_SRC_X[bodyCol], 0, bodySrcW, NS_SRC_H, notesImage, TRUE);
        }
        DrawNoteSprite(headCol, 0, cx, cy, dstH);
        break;
    }
    case NoteType::BALLOON_HEAD:
        DrawNoteSprite(5, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::KUSUDAMA:
        DrawNoteSprite(9, 0, cx, cy, NS_DST_H_L);
        break;
    default:
        break;
    }
}


int GamePlay::FindFreeSoundSlot(const int* sndBuf, int& idx) const {
    for (int i = 0; i < SND_BUF; i++) {
        int slot = (idx + i) % SND_BUF;
        if (sndBuf[slot] != -1 && !CheckSoundMem(sndBuf[slot])) {
            idx = (slot + 1) % SND_BUF;
            return slot;
        }
    }
    int slot = idx;
    idx = (idx + 1) % SND_BUF;
    return slot;
}

void GamePlay::DrawBarLines() const {
    float chartMs = GetChartMs();
    const int SW = 1280;
    const unsigned int barColor = GetColor(200, 200, 200);

    for (const Note* bar : chart.master_notes.bars) {
        if (!bar) continue;
        if (!bar->display) continue;  // #BARLINEOFF で非表示にされた小節線をスキップ
        float x = static_cast<float>(JUDGE_X) +
            (bar->hit_ms - chartMs) * CalcScrollPxPerMs(*bar);
        if (x < 0 || x > SW) continue;
        int ix = static_cast<int>(x);
        // レーン全高に渡る縦線 (幅2px)
        DrawBox(ix - 1, LANE_TOP, ix + 1, LANE_BOTTOM, barColor, TRUE);
    }
}

void GamePlay::Draw() {
    const int SW = 1280, SH = 720;
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    // --- 背景色 ---
    DrawBox(0, 0, SW, SH, GetColor(0x60, 0x60, 0x60), TRUE);

    // --- 1P_Background: 左側太鼓エリア (0,182)-(333,358) ---
    if (backgroundImage != -1) {
        DrawExtendGraph(0, 182, 333, 358, backgroundImage, FALSE);
    }

    // --- Background_Main: レーン背景を黒塗りで (329,183)-(1280,357) ---
    DrawBox(329, 183, 1280, 357, GetColor(0, 0, 0), TRUE);

    // --- Background_Sub: レーン下のサブ背景帯 (334,323)-(1280,350) ---
    if (subBackgroundImage != -1) {
        DrawExtendGraph(334, 323, 1280, 350, subBackgroundImage, TRUE);
    }

    // --- coursesymbol_oni: 左上の難易度シンボル ---
    // 画像2252x720, スケール150%, 画面左端に対応するsrc_x=894
    if (courseSymbolImage != -1) {
        DrawRectExtendGraph(0, 182, 1280, 357,
            894, 115, 1280, 175,
            courseSymbolImage, TRUE);
    }

    // --- Base: 太鼓本体 (140,160)-(346,388) ---
    if (baseImage != -1) {
        DrawExtendGraph(140, 160, 346, 388, baseImage, TRUE);
    }

    // --- 1P_Frame: レーン枠 (328,134)-(1279,358) ---
    // 1P_Frame.png は 951x224 → 全体を引き伸ばして描画（いじらない）
    if (frameImage != -1) {
        DrawExtendGraph(328, 134, 1279, 358, frameImage, TRUE);
    }

    // --- 判定ライン ---
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // --- 小節線 ---
    DrawBarLines();

    // --- 判定枠 (Notes.png col=0): ノーツより後ろに描画 ---
    DrawNoteSprite(0, 0, JUDGE_X, LANE_CY, NS_DST_H_JUDGE);

    // --- ノーツ描画 ---
    for (const auto& active : notes) {
        if (active.judged) continue;
        float x = NoteScreenX(*active.note);
        if (x < -60 || x > SW + 60) continue;
        DrawNote(*active.note, x);
    }

    // --- スコア・コンボ・判定表示 ---
    // スコア (右上)
    DrawStringToHandle(SW - 300, 10, (std::wstring(L"SCORE: ") + std::to_wstring(score)).c_str(),
        GetColor(255, 255, 255), fontScore);
    // コンボ (判定円の上)
    if (combo > 0) {
        std::wstring comboStr = std::to_wstring(combo);
        DrawStringToHandle(JUDGE_X - 20, LANE_CY - 70, comboStr.c_str(),
            GetColor(255, 215, 0), fontScore);
    }
    // 判定結果カウント (右下)
    DrawFormatStringToHandle(SW - 250, SH - 90, GetColor(255, 80, 80), fontUI,
        L"良: %d", ryoCount);
    DrawFormatStringToHandle(SW - 250, SH - 65, GetColor(100, 180, 255), fontUI,
        L"可: %d", kaCount);
    DrawFormatStringToHandle(SW - 250, SH - 40, GetColor(160, 160, 160), fontUI,
        L"不可: %d", fukaCount);

    // --- オートプレイ表示 ---
    if (autoPlay) {
        DrawStringToHandle(10, 40, L"AUTO", GetColor(255, 215, 0), fontUI);
    }
}