#include "SongSelect.h"
#include "TJAParser.h"
#include <algorithm>

namespace fs = std::filesystem;

// ─── 角丸矩形（DxLib には無いので自前） ──────────────────────────
void SongSelect::DrawRoundRect(int x1, int y1, int x2, int y2, int r,
    unsigned int color, bool fill) {
    if (fill) {
        DrawBox(x1 + r, y1, x2 - r, y2, color, TRUE);
        DrawBox(x1, y1 + r, x2, y2 - r, color, TRUE);
        DrawCircle(x1 + r, y1 + r, r, color, TRUE);
        DrawCircle(x2 - r, y1 + r, r, color, TRUE);
        DrawCircle(x1 + r, y2 - r, r, color, TRUE);
        DrawCircle(x2 - r, y2 - r, r, color, TRUE);
    }
    else {
        DrawLine(x1 + r, y1, x2 - r, y1, color);
        DrawLine(x1 + r, y2, x2 - r, y2, color);
        DrawLine(x1, y1 + r, x1, y2 - r, color);
        DrawLine(x2, y1 + r, x2, y2 - r, color);
        DrawCircle(x1 + r, y1 + r, r, color, FALSE);
        DrawCircle(x2 - r, y1 + r, r, color, FALSE);
        DrawCircle(x1 + r, y2 - r, r, color, FALSE);
        DrawCircle(x2 - r, y2 - r, r, color, FALSE);
    }
}

// ─── コンストラクタ ───────────────────────────────────────────────
SongSelect::SongSelect() {
    // アンチエイリアス付きフォント作成
    // 第3引数: 太さ(1-9), 第4引数: フォントタイプ
    // DX_FONTTYPE_ANTIALIASING_4X4 = 高品質AA
    fontLarge = CreateFontToHandle(L"メイリオ", 38, 3,
        DX_FONTTYPE_ANTIALIASING_4X4);
    fontNormal = CreateFontToHandle(L"メイリオ", 28, 2,
        DX_FONTTYPE_ANTIALIASING_4X4);
    fontUI = CreateFontToHandle(L"メイリオ", 22, 1,
        DX_FONTTYPE_ANTIALIASING_4X4);

    // サウンドロード
    sndDong = LoadSoundMem(L"Theme\\default\\sounds\\dong.wav");
    sndKa = LoadSoundMem(L"Theme\\default\\sounds\\ka.wav");

    ScanSongs(fs::path("songs"));
    if (songs.empty()) {
        SongEntry dummy;
        dummy.title = L"（ ./songs/ に TJA ファイルが見つかりません）";
        songs.push_back(dummy);
    }
}

SongSelect::~SongSelect() {
    if (fontLarge != -1) DeleteFontToHandle(fontLarge);
    if (fontNormal != -1) DeleteFontToHandle(fontNormal);
    if (fontUI != -1) DeleteFontToHandle(fontUI);
    if (sndKa != -1) DeleteSoundMem(sndKa);
    if (sndDong != -1) DeleteSoundMem(sndDong);
}

void SongSelect::ScanSongs(const fs::path& songsDir) {
    if (!fs::exists(songsDir)) return;
    for (auto& entry : fs::recursive_directory_iterator(songsDir)) {
        if (entry.path().extension() == ".tja") {
            SongEntry se;
            se.tjaPath = entry.path();
            se.title = TJAParser::ReadTitle(entry.path());
            songs.push_back(se);
        }
    }
    std::sort(songs.begin(), songs.end(),
        [](const SongEntry& a, const SongEntry& b) { return a.title < b.title; });
}

// ─── Update ──────────────────────────────────────────────────────
bool SongSelect::Update() {
    bool curD = CheckHitKey(KEY_INPUT_D) != 0;
    bool curK = CheckHitKey(KEY_INPUT_K) != 0;
    bool curJ = CheckHitKey(KEY_INPUT_J) != 0;
    bool curF = CheckHitKey(KEY_INPUT_F) != 0;
    bool curEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;

    if (curD && !prevD) {
        selectedIndex = (selectedIndex + 1) % (int)songs.size();
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
    }
    if (curK && !prevK) {
        selectedIndex = (selectedIndex - 1 + (int)songs.size()) % (int)songs.size();
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
    }

    // スクロール目標
    const float ITEM_H = 84.0f;
    const float SCREEN_H = 720.0f;
    targetScrollY = selectedIndex * ITEM_H - SCREEN_H / 2.0f + ITEM_H / 2.0f;
    if (targetScrollY < 0.0f) targetScrollY = 0.0f;
    scrollY += (targetScrollY - scrollY) * 0.15f;

    if ((curJ && !prevJ) || (curF && !prevF)) {
        if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
        songDecided = true;
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        return true;
    }

    if (curEsc && !prevEsc) {
        if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        return true; // タイトルへ
    }

    prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
    return false;
}

// ─── Draw ────────────────────────────────────────────────────────
void SongSelect::Draw() {
    const int SW = 1280, SH = 720;

    // ── 背景: ピンクグラデーション風（上下2段）
    DrawBox(0, 0, SW, SH / 2, GetColor(245, 175, 175), TRUE);
    DrawBox(0, SH / 2, SW, SH, GetColor(225, 150, 150), TRUE);

    // ── ヘッダ
    const wchar_t* header = L"曲を選んでください";
    int hw = GetDrawStringWidthToHandle(header, (int)wcslen(header), fontUI);
    // 影
    DrawStringToHandle((SW - hw) / 2 + 2, 22, header,
        GetColor(100, 50, 50), fontUI);
    DrawStringToHandle((SW - hw) / 2, 20, header,
        GetColor(255, 255, 255), fontUI);

    // ── 仕切り線
    DrawBox(0, 60, SW, 62, GetColor(255, 200, 200), TRUE);

    // ── 曲リスト
    const float ITEM_H = 84.0f;
    const int   PAD_X = 180;
    const int   BOX_W = SW - PAD_X * 2;
    const int   RADIUS = 12;

    int n = (int)songs.size();
    for (int i = 0; i < n; i++) {
        bool   selected = (i == selectedIndex);
        float  itemH = selected ? 96.0f : ITEM_H;
        float  centerY = i * ITEM_H + ITEM_H / 2.0f - scrollY + 90.0f;
        int    top = (int)(centerY - itemH / 2.0f);
        int    bottom = (int)(centerY + itemH / 2.0f);

        if (bottom < 62 || top > SH - 50) continue;

        // 影（選択中だけ目立たせる）
        if (selected) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
            DrawRoundRect(PAD_X + 5, top + 6, PAD_X + BOX_W + 5, bottom + 6,
                RADIUS, GetColor(0, 0, 0), true);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ボックス本体
        unsigned int boxColor = selected
            ? GetColor(240, 240, 240)   // 選択中: ほぼ白
            : GetColor(80, 80, 90);     // 通常: 濃いグレー

        // 通常項目は半透明っぽく
        if (!selected) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
        }
        DrawRoundRect(PAD_X, top, PAD_X + BOX_W, bottom, RADIUS, boxColor, true);
        if (!selected) {
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // 選択中: 左端にアクセントバー
        if (selected) {
            DrawBox(PAD_X, top + RADIUS, PAD_X + 6, bottom - RADIUS,
                GetColor(220, 80, 80), TRUE);
            // 枠線
            DrawRoundRect(PAD_X, top, PAD_X + BOX_W, bottom, RADIUS,
                GetColor(200, 200, 200), false);
        }

        // テキスト
        int fh = selected ? fontLarge : fontNormal;
        int fs = selected ? 38 : 28;
        const std::wstring& title = songs[i].title;
        int tw = GetDrawStringWidthToHandle(title.c_str(), (int)title.size(), fh);
        int tx = PAD_X + (BOX_W - tw) / 2;
        int ty = top + (int)(itemH / 2.0f) - fs / 2;

        if (selected) {
            // 影テキスト
            DrawStringToHandle(tx + 1, ty + 1, title.c_str(),
                GetColor(160, 160, 160), fh);
            DrawStringToHandle(tx, ty, title.c_str(),
                GetColor(40, 40, 40), fh);
        }
        else {
            DrawStringToHandle(tx, ty, title.c_str(),
                GetColor(220, 220, 220), fh);
        }
    }

    // ── 下部ヒントバー
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
    DrawBox(0, SH - 44, SW, SH, GetColor(20, 20, 20), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const wchar_t* hint = L"D / K : 上下移動　　J / F : 決定　　ESC : タイトルへ";
    int hintW = GetDrawStringWidthToHandle(hint, (int)wcslen(hint), fontUI);
    DrawStringToHandle((SW - hintW) / 2, SH - 34, hint,
        GetColor(180, 180, 180), fontUI);
}