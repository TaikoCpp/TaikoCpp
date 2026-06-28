#define NOMINMAX
#include "GamePlay.h"
#include <algorithm>
#include <cmath>

namespace fs = std::filesystem;

GamePlay::GamePlay(const SongEntry& song, int diffId, SongInfo&& chartData)
    : songEntry(song), diffId(diffId), chart(std::move(chartData)) {
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

float GamePlay::NoteScreenX(const Note& note) const {
    float chartMs = GetChartMs();
    return static_cast<float>(JUDGE_X) +
        (note.hit_ms - chartMs) * SCROLL_PX_PER_MS * note.scroll_x;
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
                if (sndDon[slot] != -1) { StopSoundMem(sndDon[slot]); PlaySoundMem(sndDon[slot], DX_PLAYTYPE_BACK); }
            }
            if (ka) {
                int slot = FindFreeSoundSlot(sndKa, sndKaIdx);
                if (sndKa[slot] != -1) { StopSoundMem(sndKa[slot]); PlaySoundMem(sndKa[slot], DX_PLAYTYPE_BACK); }
            }
        }
    }
    else if (absDelta <= JUDGE_KA_MS) {
        if ((expectDon && don) || (expectKa && ka)) {
            active.judged = true;
            RegisterJudge(JudgeResult::Ka, type);
            if (don) {
                int slot = FindFreeSoundSlot(sndDon, sndDonIdx);
                if (sndDon[slot] != -1) { StopSoundMem(sndDon[slot]); PlaySoundMem(sndDon[slot], DX_PLAYTYPE_BACK); }
            }
            if (ka) {
                int slot = FindFreeSoundSlot(sndKa, sndKaIdx);
                if (sndKa[slot] != -1) { StopSoundMem(sndKa[slot]); PlaySoundMem(sndKa[slot], DX_PLAYTYPE_BACK); }
            }
        }
    }
}

void GamePlay::ProcessInput(bool donPressed, bool kaPressed) {
    float chartMs = GetChartMs();

    bool noteHit = false;
    for (auto& active : notes) {
        if (active.judged) continue;
        float delta = std::fabs(chartMs - active.note->hit_ms);
        if (delta > JUDGE_KA_MS) continue;
        if (donPressed || kaPressed) {
            JudgeNote(active, donPressed, kaPressed);
            noteHit = true;
            break;
        }
    }

    // ノーツにヒットしなかった場合でも打鍵音を再生する
    if (!noteHit) {
        if (donPressed) {
            int slot = FindFreeSoundSlot(sndDon, sndDonIdx);
            if (sndDon[slot] != -1) { StopSoundMem(sndDon[slot]); PlaySoundMem(sndDon[slot], DX_PLAYTYPE_BACK); }
        }
        if (kaPressed) {
            int slot = FindFreeSoundSlot(sndKa, sndKaIdx);
            if (sndKa[slot] != -1) { StopSoundMem(sndKa[slot]); PlaySoundMem(sndKa[slot], DX_PLAYTYPE_BACK); }
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
        // col=1(小ドン), row=0, 小サイズ
        DrawNoteSprite(1, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::KAT:
        // col=2(小カッ)
        DrawNoteSprite(2, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::DON_L:
        // col=3(大ドン)
        DrawNoteSprite(3, 0, cx, cy, NS_DST_H_L);
        break;
    case NoteType::KAT_L:
        // col=4(大カッ)
        DrawNoteSprite(4, 0, cx, cy, NS_DST_H_L);
        break;
    case NoteType::ROLL_HEAD:
    case NoteType::ROLL_HEAD_L: {
        // ロール: body を head の右側に敷き詰め、最後に head を最前面に描く
        bool isLarge = (note.type == NoteType::ROLL_HEAD_L);
        int bodyCol = isLarge ? 8 : 6;   // ロールbody大/小
        int headCol = isLarge ? 3 : 1;   // 大ドン/小ドン顔
        int dstH = isLarge ? NS_DST_H_L : NS_DST_H_S;
        // body の描画幅を計算 (縦横比を保つ)
        int bodySrcW = NS_SRC_W[bodyCol];
        int bodyDstW = static_cast<int>(bodySrcW * (static_cast<float>(dstH) / NS_SRC_H));
        if (bodyDstW <= 0) bodyDstW = 1;
        // ロール終端 x: unload_ms (TAIL の時刻) をスクリーン座標に変換
        float chartMs = GetChartMs();
        int rollEndX;
        if (note.unload_ms > note.hit_ms) {
            float endScreenX = static_cast<float>(JUDGE_X) +
                (note.unload_ms - chartMs) * SCROLL_PX_PER_MS * note.scroll_x;
            rollEndX = static_cast<int>(endScreenX);
        }
        else {
            rollEndX = cx + 120;
        }
        // body を head(cx) から rollEndX へ左→右に並べる
        for (int bx = cx; bx < rollEndX; bx += bodyDstW) {
            int bx1 = bx;
            int bx2 = bx + bodyDstW;
            DrawRectExtendGraph(bx1, cy - dstH / 2, bx2, cy + dstH / 2,
                NS_SRC_X[bodyCol], 0, bodySrcW, NS_SRC_H, notesImage, TRUE);
        }
        // head を最前面に
        DrawNoteSprite(headCol, 0, cx, cy, dstH);
        break;
    }
    case NoteType::BALLOON_HEAD:
        // col=5(バルーン小黄色顔)
        DrawNoteSprite(5, 0, cx, cy, NS_DST_H_S);
        break;
    case NoteType::KUSUDAMA:
        // col=9(kusudama/風船)
        DrawNoteSprite(9, 0, cx, cy, NS_DST_H_L);
        break;
    default:
        break;
    }
}

// 再生が終わっているスロットを優先して返す。全スロット再生中なら最も古いものを返す。
int GamePlay::FindFreeSoundSlot(const int* sndBuf, int& idx) const {
    for (int i = 0; i < SND_BUF; i++) {
        int slot = (idx + i) % SND_BUF;
        if (sndBuf[slot] != -1 && !CheckSoundMem(sndBuf[slot])) {
            idx = (slot + 1) % SND_BUF;
            return slot;
        }
    }
    // 全スロット再生中 → ラウンドロビンで現在位置を使う（最終手段）
    int slot = idx;
    idx = (idx + 1) % SND_BUF;
    return slot;
}

void GamePlay::DrawBarLines() const {
    // chart.master_notes.bars には小節線ノートが入っている
    // 小節線は半透明の白い縦線としてレーン上に描画する
    float chartMs = GetChartMs();
    const int SW = 1280;
    const unsigned int barColor = GetColor(200, 200, 200);

    for (const Note* bar : chart.master_notes.bars) {
        if (!bar) continue;
        float x = static_cast<float>(JUDGE_X) +
            (bar->hit_ms - chartMs) * SCROLL_PX_PER_MS * bar->scroll_x;
        if (x < 0 || x > SW) continue;
        int ix = static_cast<int>(x);
        // レーン全高に渡る縦線 (幅2px)
        DrawBox(ix - 1, LANE_TOP, ix + 1, LANE_BOTTOM, barColor, TRUE);
    }
}

void GamePlay::Draw() {
    // -----------------------------------------------------------------------
    // 座標変換の説明:
    //   .aup2 の座標系: 1920x1080、中心原点 (960, 540)
    //   ゲーム画面:    1280x720、左上原点
    //   スケール係数:  sx = 1280/1920 = 2/3、sy = 720/1080 = 2/3
    //
    //   aup2 の中心座標 → 1920x1080 上のスクリーン座標:
    //     screen_cx = 960 + aup_x
    //     screen_cy = 540 + aup_y
    //
    //   画像のスケール後サイズ:
    //     scaled_w = img_w * scale
    //     scaled_h = img_h * scale
    //
    //   1920x1080 上の左上:
    //     tl_x = screen_cx - scaled_w / 2
    //     tl_y = screen_cy - scaled_h / 2
    //
    //   1280x720 への変換:
    //     draw_x1 = tl_x * (2/3)
    //     draw_y1 = tl_y * (2/3)
    //     draw_x2 = (tl_x + scaled_w) * (2/3)
    //     draw_y2 = (tl_y + scaled_h) * (2/3)
    // -----------------------------------------------------------------------

    const int SW = 1280, SH = 720;

    // 画質向上: バイリニアフィルタを使用
    SetDrawMode(DX_DRAWMODE_BILINEAR);

    // ------------------------------------------------------------------
    // Layer 0: グレー背景 (図形・背景 #606060)
    // ------------------------------------------------------------------
    DrawBox(0, 0, SW, SH, GetColor(0x60, 0x60, 0x60), TRUE);

    // ------------------------------------------------------------------
    // Layer 1: 1P_Background.png  (透過なし)
    //   aup2: X=-710, Y=-135, scale=150%  /  image: 333x176
    //   1920x1080 center: (250, 405), scaled: 499.5x264
    //   TL: (0.25, 273) → 1280x720: (0, 182)
    //   元画像が scale=150% で 333*1.5=499.5 ≒ 500px 幅になるが、
    //   1280x720 への縮小後は 333px 幅に戻り等倍 → DrawGraph で劣化なし
    // ------------------------------------------------------------------
    if (backgroundImage != -1) {
        DrawGraph(0, 182, backgroundImage, FALSE);  // 透過なし(FALSE)、等倍描画
    }

    // ------------------------------------------------------------------
    // Layer 2: Base.png  (太鼓の台座)
    //   aup2: X=-595.41, Y=-129.23, scale=200%  /  image: 154x171
    //   1280x720: (140, 160) → (346, 388)  (206x228 に拡大)
    // ------------------------------------------------------------------
    if (baseImage != -1) {
        DrawExtendGraph(140, 160, 346, 388, baseImage, TRUE);
    }

    // ------------------------------------------------------------------
    // Layer 3: coursesymbol_oni.png
    //   aup2: X=-612, Y=-173, scale=150%  /  image: 2252x720
    //   1920x1080 center: (348, 367), scaled: 3378x1080
    //   TL(1920x1080): (-1341, -173) → 1280x720: (-894, -115)
    //   BR(1920x1080): (2037, 907)   → 1280x720: (1358, 605)
    //   画面に見える範囲: (0,0)→(1280,605)
    //   元画像上の対応クロップ:
    //     描画全体サイズ = 2252x720 (縮小率がちょうど 2/3*1.5=1.0 倍で等倍)
    //     src_x = 894, src_y = 115, width = 1280, height = 605
    //   → DrawRectGraph で等倍コピー (最高画質)
    // ------------------------------------------------------------------
    if (courseSymbolImage != -1) {
        DrawRectGraph(0, 0,
            894, 115, 1280, 605,
            courseSymbolImage, TRUE);
    }

    // ------------------------------------------------------------------
    // Layer 4: Background_Main.png  (単色化=黒、画像不要)
    //   aup2: X=480, Y=-135, scale=200%  /  image: 947x130
    //   1280x720: (329, 183) → (1280, 357)
    //   単色化(黒, 強さ=100%) → 黒矩形で代替
    // ------------------------------------------------------------------
    DrawBox(329, 183, 1280, 357, GetColor(0, 0, 0), TRUE);

    // ------------------------------------------------------------------
    // Layer 5: 1P_Frame.png  (デコレーションフレーム)
    //   aup2: X=250, Y=-170, scale=150%  /  image: 951x224
    //   1280x720 TL: (331, 135), scaled: 1426x336
    //   右端が画面外 → src を (1280-331)/1.5 = 632px 分だけ切り出し
    //   DrawRectExtendGraph でクロップ+拡大
    // ------------------------------------------------------------------
    if (frameImage != -1) {
        DrawRectExtendGraph(331, 135, 1280, 359,
            0, 0, 633, 224,
            frameImage, TRUE);
    }

    // ------------------------------------------------------------------
    // 判定ライン & 判定円
    // ------------------------------------------------------------------
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(JUDGE_X, 183, JUDGE_X, 357, GetColor(255, 255, 255), FALSE);
    DrawCircle(JUDGE_X, 270, 36, GetColor(255, 255, 255), FALSE);

    // ------------------------------------------------------------------
    // 小節線描画 (ノーツより下のレイヤー)
    // ------------------------------------------------------------------
    DrawBarLines();

    // ------------------------------------------------------------------
    // ノーツ描画
    // ------------------------------------------------------------------
    for (const auto& active : notes) {
        if (active.judged) continue;
        float x = NoteScreenX(*active.note);
        if (x < -60 || x > SW + 60) continue;
        DrawNote(*active.note, x);
    }

}