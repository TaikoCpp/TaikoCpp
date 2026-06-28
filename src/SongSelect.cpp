#include "SongSelect.h"
#include "TJAParser.h"
#include <algorithm>

namespace fs = std::filesystem;

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

SongSelect::SongSelect() {
    fontLarge = CreateFontToHandle(L"\u30e1\u30a4\u30ea\u30aa", 38, 3,
        DX_FONTTYPE_ANTIALIASING_4X4);
    fontNormal = CreateFontToHandle(L"\u30e1\u30a4\u30ea\u30aa", 28, 2,
        DX_FONTTYPE_ANTIALIASING_4X4);
    fontUI = CreateFontToHandle(L"\u30e1\u30a4\u30ea\u30aa", 22, 1,
        DX_FONTTYPE_ANTIALIASING_4X4);

    sndDong = LoadSoundMem(L"Theme\\default\\sounds\\dong.wav");
    sndKa = LoadSoundMem(L"Theme\\default\\sounds\\ka.wav");

    songsRoot = fs::path("songs");
    currentDir = songsRoot;
    LoadDirectory(currentDir);

    if (items.empty()) {
        SelectItem dummy;
        dummy.type = SelectItemType::Song;
        dummy.title = L"(./songs/    TJA  t @ C          ܂   )";
        items.push_back(dummy);
    }
}

SongSelect::~SongSelect() {
    if (fontLarge != -1) DeleteFontToHandle(fontLarge);
    if (fontNormal != -1) DeleteFontToHandle(fontNormal);
    if (fontUI != -1) DeleteFontToHandle(fontUI);
    if (sndKa != -1) DeleteSoundMem(sndKa);
    if (sndDong != -1) DeleteSoundMem(sndDong);
}

std::vector<fs::path> SongSelect::FindTjaFilesInDirectory(const fs::path& dir) const {
    std::vector<fs::path> result;
    if (!fs::exists(dir)) return result;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == L".tja") {
            result.push_back(entry.path());
        }
        else if (entry.is_directory()) {
            auto sub = FindTjaFilesInDirectory(entry.path());
            result.insert(result.end(), sub.begin(), sub.end());
        }
    }
    return result;
}

void SongSelect::LoadDirectory(const fs::path& dir) {
    items.clear();
    selectedIndex = 0;
    scrollY = 0.0f;
    targetScrollY = 0.0f;
    currentDir = dir;

    if (!fs::exists(dir)) return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        if (!BoxDefParser::Exists(entry.path())) continue;

        SelectItem item;
        item.type = SelectItemType::Folder;
        item.path = entry.path();
        item.boxInfo = BoxDefParser::Parse(entry.path());
        item.title = item.boxInfo.title;
        items.push_back(item);
    }

    auto tjaFiles = FindTjaFilesInDirectory(dir);
    std::sort(tjaFiles.begin(), tjaFiles.end());

    for (const auto& tjaPath : tjaFiles) {
        SelectItem item;
        item.type = SelectItemType::Song;
        item.path = tjaPath;
        item.title = TJAParser::ReadTitle(tjaPath);
        items.push_back(item);
    }

    std::stable_sort(items.begin(), items.end(),
        [](const SelectItem& a, const SelectItem& b) {
            if (a.type != b.type) return a.type < b.type;
            return a.title < b.title;
        });
}

bool SongSelect::Update() {
    bool curD = CheckHitKey(KEY_INPUT_D) != 0;
    bool curK = CheckHitKey(KEY_INPUT_K) != 0;
    bool curJ = CheckHitKey(KEY_INPUT_J) != 0;
    bool curF = CheckHitKey(KEY_INPUT_F) != 0;
    bool curEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;

    // items     Ȃ   ͏      X L b v
    if (!items.empty()) {
        if (curD && !prevD) {
            selectedIndex = (selectedIndex + 1) % (int)items.size();
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }
        if (curK && !prevK) {
            selectedIndex = (selectedIndex - 1 + (int)items.size()) % (int)items.size();
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }

        const float ITEM_H = 84.0f;
        const float SCREEN_H = 720.0f;
        targetScrollY = selectedIndex * ITEM_H - SCREEN_H / 2.0f + ITEM_H / 2.0f;
        if (targetScrollY < 0.0f) targetScrollY = 0.0f;
        scrollY += (targetScrollY - scrollY) * 0.15f;

        if ((curJ && !prevJ) || (curF && !prevF)) {
            if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);

            // selectedIndex    ͈͓  ł  邱 Ƃ O ̂  ߕۏ 
            if (selectedIndex >= 0 && selectedIndex < (int)items.size()) {
                const SelectItem& item = items[selectedIndex];
                if (item.type == SelectItemType::Folder) {
                    LoadDirectory(item.path);
                }
                else if (item.path.extension() == L".tja") {
                    selectedSong.title = item.title;
                    selectedSong.tjaPath = item.path;
                    songDecided = true;
                    prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
                    return true;
                }
            }

            prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
            return false;
        }
    }

    if (curEsc && !prevEsc) {
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);

        if (currentDir != songsRoot && !currentDir.empty()) {
            LoadDirectory(currentDir.parent_path());
            prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
            return false;
        }

        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        return true;
    }

    prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
    return false;
}

void SongSelect::Draw() {
    const int SW = 1280, SH = 720;

    DrawBox(0, 0, SW, SH / 2, GetColor(245, 175, 175), TRUE);
    DrawBox(0, SH / 2, SW, SH, GetColor(225, 150, 150), TRUE);

    std::wstring header = L"\u66f2\u3092\u9078\u3093\u3067\u304f\u3060\u3055\u3044";
    if (currentDir != songsRoot && BoxDefParser::Exists(currentDir)) {
        header = BoxDefParser::GetFolderTitle(currentDir);
    }
    else if (currentDir != songsRoot) {
        header = currentDir.filename().wstring();
    }

    int hw = GetDrawStringWidthToHandle(header.c_str(), (int)header.size(), fontUI);
    DrawStringToHandle((SW - hw) / 2 + 2, 22, header.c_str(),
        GetColor(100, 50, 50), fontUI);
    DrawStringToHandle((SW - hw) / 2, 20, header.c_str(),
        GetColor(255, 255, 255), fontUI);

    DrawBox(0, 60, SW, 62, GetColor(255, 200, 200), TRUE);

    const float ITEM_H = 84.0f;
    const int   PAD_X = 180;
    const int   BOX_W = SW - PAD_X * 2;
    const int   RADIUS = 12;

    int n = (int)items.size();
    for (int i = 0; i < n; i++) {
        bool   selected = (i == selectedIndex);
        bool   isFolder = (items[i].type == SelectItemType::Folder);
        float  itemH = selected ? 96.0f : ITEM_H;
        float  centerY = i * ITEM_H + ITEM_H / 2.0f - scrollY + 90.0f;
        int    top = (int)(centerY - itemH / 2.0f);
        int    bottom = (int)(centerY + itemH / 2.0f);

        if (bottom < 62 || top > SH - 50) continue;

        if (selected) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
            DrawRoundRect(PAD_X + 5, top + 6, PAD_X + BOX_W + 5, bottom + 6,
                RADIUS, GetColor(0, 0, 0), true);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        unsigned int boxColor;
        if (selected) {
            boxColor = GetColor(240, 240, 240);
        }
        else if (isFolder && items[i].boxInfo.hasBackColor) {
            boxColor = GetColor(items[i].boxInfo.backColorR,
                items[i].boxInfo.backColorG, items[i].boxInfo.backColorB);
        }
        else if (isFolder) {
            boxColor = GetColor(60, 90, 140);
        }
        else {
            boxColor = GetColor(80, 80, 90);
        }

        if (!selected) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, isFolder ? 230 : 200);
        }
        DrawRoundRect(PAD_X, top, PAD_X + BOX_W, bottom, RADIUS, boxColor, true);
        if (!selected) {
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        if (selected) {
            DrawBox(PAD_X, top + RADIUS, PAD_X + 6, bottom - RADIUS,
                GetColor(220, 80, 80), TRUE);
            DrawRoundRect(PAD_X, top, PAD_X + BOX_W, bottom, RADIUS,
                GetColor(200, 200, 200), false);
        }

        int fh = selected ? fontLarge : fontNormal;
        int fs = selected ? 38 : 28;

        std::wstring displayTitle = items[i].title;
        if (isFolder) displayTitle = L"\u25b6 " + displayTitle;

        int tw = GetDrawStringWidthToHandle(displayTitle.c_str(), (int)displayTitle.size(), fh);
        int tx = PAD_X + (BOX_W - tw) / 2;
        int ty = top + (int)(itemH / 2.0f) - fs / 2;

        unsigned int textColor;
        if (selected) {
            textColor = GetColor(40, 40, 40);
        }
        else if (isFolder && items[i].boxInfo.hasFrontColor) {
            textColor = GetColor(items[i].boxInfo.frontColorR,
                items[i].boxInfo.frontColorG, items[i].boxInfo.frontColorB);
        }
        else {
            textColor = GetColor(220, 220, 220);
        }

        if (selected) {
            DrawStringToHandle(tx + 1, ty + 1, displayTitle.c_str(),
                GetColor(160, 160, 160), fh);
        }
        DrawStringToHandle(tx, ty, displayTitle.c_str(), textColor, fh);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
    DrawBox(0, SH - 44, SW, SH, GetColor(20, 20, 20), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const wchar_t* hint = (currentDir != songsRoot)
        ? L"D / K : \u4e0a\u4e0b\u79fb\u52d5\u3000\u3000J / F : \u6c7a\u5b9a\u3000\u3000ESC : \u623b\u308b"
        : L"D / K : \u4e0a\u4e0b\u79fb\u52d5\u3000\u3000J / F : \u6c7a\u5b9a\u3000\u3000ESC : \u30bf\u30a4\u30c8\u30eb\u3078";
    int hintW = GetDrawStringWidthToHandle(hint, (int)wcslen(hint), fontUI);
    DrawStringToHandle((SW - hintW) / 2, SH - 34, hint,
        GetColor(180, 180, 180), fontUI);
}