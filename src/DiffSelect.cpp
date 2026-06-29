#include "DiffSelect.h"
#include "TJAParser.h"
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

unsigned int DiffSelect::DiffColor(int diffId) {
    switch (diffId) {
    case 0: return GetColor(120, 200, 80);   // Easy   : ??
    case 1: return GetColor(80, 160, 220);  // Normal : ??
    case 2: return GetColor(220, 160, 40);   // Hard   : ??
    case 3: return GetColor(220, 60, 60);   // Oni    : ??
    case 4: return GetColor(160, 80, 200);  // Edit   : ??
    default: return GetColor(150, 150, 150);
    }
}

unsigned int DiffSelect::DiffColorDark(int diffId) {
    switch (diffId) {
    case 0: return GetColor(60, 120, 30);
    case 1: return GetColor(30, 80, 140);
    case 2: return GetColor(140, 90, 10);
    case 3: return GetColor(140, 20, 20);
    case 4: return GetColor(90, 30, 130);
    default: return GetColor(80, 80, 80);
    }
}

const wchar_t* DiffSelect::DiffName(int diffId) {
    switch (diffId) {
    case 0: return L"?????";
    case 1: return L"????";
    case 2: return L"?????????";
    case 3: return L"????";
    case 4: return L"Edit";
    default: return L"???";
    }
}

// ?????? ?R???X?g???N?^ ??????????????????????????????????????????????????????????????????????????????????????????????
DiffSelect::DiffSelect(const SongEntry& song) : songEntry(song) {
    fontTitle = CreateFontToHandle(L"FOT-OedoKtr", 36, 3, DX_FONTTYPE_ANTIALIASING_4X4);
    fontButton = CreateFontToHandle(L"FOT-OedoKtr", 28, 3, DX_FONTTYPE_ANTIALIASING_4X4);
    fontLevel = CreateFontToHandle(L"FOT-OedoKtr", 20, 2, DX_FONTTYPE_ANTIALIASING_4X4);

    sndDong = LoadSoundMem(L"Theme\\default\\sounds\\dong.wav");
    sndKa = LoadSoundMem(L"Theme\\default\\sounds\\ka.wav");

    // prev?????????i???????????m?h?~?j
    prevD = CheckHitKey(KEY_INPUT_D) != 0;
    prevK = CheckHitKey(KEY_INPUT_K) != 0;
    prevJ = CheckHitKey(KEY_INPUT_J) != 0;
    prevF = CheckHitKey(KEY_INPUT_F) != 0;
    prevEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;

    // TJA???y??p?[?X??????^???x?????
    // TJAParser::ReadTitle ????l?? COURSE: ?s?????E??
    {
        std::ifstream f(song.tjaPath, std::ios::binary);
        std::string line;
        int currentDiff = -1;
        std::set<int> found;
        std::map<int, int> levelByDiff;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.rfind("COURSE:", 0) == 0) {
                std::string val = line.substr(7);
                // trim
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of(" \t\r\n") + 1);
                std::transform(val.begin(), val.end(), val.begin(), ::tolower);
                if (val == "easy" || val == "0") currentDiff = 0;
                else if (val == "normal" || val == "1") currentDiff = 1;
                else if (val == "hard" || val == "2") currentDiff = 2;
                else if (val == "oni" || val == "3") currentDiff = 3;
                else if (val == "edit" || val == "4") currentDiff = 4;
                else if (val == "tower" || val == "5") currentDiff = 5;
                else if (val == "dan" || val == "6") currentDiff = 6;
                if (currentDiff >= 0) found.insert(currentDiff);
            }
            else if (currentDiff >= 0 && line.rfind("LEVEL:", 0) == 0) {
                levelByDiff[currentDiff] = 0;
                std::string val = line.substr(6);
                val.erase(0, val.find_first_not_of(" \t"));
                if (!val.empty()) {
                    try { levelByDiff[currentDiff] = static_cast<int>(std::stof(val)); }
                    catch (...) {}
                }
            }
        }
        for (int id : found) {
            DiffEntry de;
            de.diffId = id;
            de.label = DiffName(id);
            auto lit = levelByDiff.find(id);
            de.level = (lit != levelByDiff.end()) ? lit->second : 0;
            diffs.push_back(de);
        }
        // diffId ????\?[?g
        std::sort(diffs.begin(), diffs.end(),
            [](const DiffEntry& a, const DiffEntry& b) { return a.diffId < b.diffId; });
    }
}

DiffSelect::~DiffSelect() {
    if (fontTitle != -1) DeleteFontToHandle(fontTitle);
    if (fontButton != -1) DeleteFontToHandle(fontButton);
    if (fontLevel != -1) DeleteFontToHandle(fontLevel);
    if (sndDong != -1) DeleteSoundMem(sndDong);
    if (sndKa != -1) DeleteSoundMem(sndKa);
}

bool DiffSelect::Update() {
    bool curD = CheckHitKey(KEY_INPUT_D) != 0;
    bool curK = CheckHitKey(KEY_INPUT_K) != 0;
    bool curJ = CheckHitKey(KEY_INPUT_J) != 0;
    bool curF = CheckHitKey(KEY_INPUT_F) != 0;
    bool curEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;

    // D ?? ?E??i??????x?j
    if (curD && !prevD) {
        selectedIndex = (selectedIndex + 1) % (int)diffs.size();
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
    }
    // K ?? ????i?O????x?j
    if (curK && !prevK) {
        selectedIndex = (selectedIndex - 1 + (int)diffs.size()) % (int)diffs.size();
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
    }

    // J/F ?? ????
    if ((curJ && !prevJ) || (curF && !prevF)) {
        if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
        decided = true;
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        return true;
    }

    // ESC ?? ??I??????
    if (curEsc && !prevEsc) {
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        return true;
    }

    prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
    return false;
}

void DiffSelect::Draw() {
    const int SW = 1280, SH = 720;

    // ?w?i
    DrawBox(0, 0, SW, SH / 2, GetColor(245, 175, 175), TRUE);
    DrawBox(0, SH / 2, SW, SH, GetColor(225, 150, 150), TRUE);

    // ??^?C?g??
    int tw = GetDrawStringWidthToHandle(songEntry.title.c_str(),
        (int)songEntry.title.size(), fontTitle);
    DrawStringToHandle(SW / 2 - tw / 2 + 2, 52, songEntry.title.c_str(),
        GetColor(100, 50, 50), fontTitle);
    DrawStringToHandle(SW / 2 - tw / 2, 50, songEntry.title.c_str(),
        GetColor(255, 255, 255), fontTitle);

    // ?d????
    DrawBox(0, 108, SW, 110, GetColor(255, 200, 200), TRUE);

    if (diffs.empty()) {
        const wchar_t* msg = L"????????^???x??????????";
        int mw = GetDrawStringWidthToHandle(msg, (int)wcslen(msg), fontButton);
        DrawStringToHandle(SW / 2 - mw / 2, SH / 2 - 20, msg,
            GetColor(255, 255, 255), fontButton);
        return;
    }

    const int BTN_W = 200;
    const int BTN_H = 200;
    const int BTN_SEL_H = 230;  // ?I??????????
    const int GAP = 20;
    const int RADIUS = 14;

    int totalW = (int)diffs.size() * BTN_W + ((int)diffs.size() - 1) * GAP;
    int startX = (SW - totalW) / 2;
    int centerY = SH / 2 + 30;

    for (int i = 0; i < (int)diffs.size(); i++) {
        bool sel = (i == selectedIndex);
        int  bh = sel ? BTN_SEL_H : BTN_H;
        int  x1 = startX + i * (BTN_W + GAP);
        int  x2 = x1 + BTN_W;
        int  y1 = centerY - bh / 2;
        int  y2 = centerY + bh / 2;

        unsigned int col = DiffColor(diffs[i].diffId);
        unsigned int colDark = DiffColorDark(diffs[i].diffId);

        // ?e
        if (sel) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
            DrawBox(x1 + 6, y1 + 8, x2 + 6, y2 + 8, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        // ?{?^???{??i??: ?????F?A????: ????F??O???f?[?V???????j
        int mid = y1 + bh * 2 / 3;
        DrawBox(x1, y1, x2, mid, col, TRUE);
        DrawBox(x1, mid, x2, y2, colDark, TRUE);

        // ?p???????l????w?i?F??h?????
        // ?iDxLib ??p???`?????????A?V???v????l?p?????j

        // ?I??: ???g
        if (sel) {
            DrawBox(x1, y1, x2, y1 + 4, GetColor(255, 255, 255), TRUE);
            DrawBox(x1, y2 - 4, x2, y2, GetColor(255, 255, 255), TRUE);
            DrawBox(x1, y1, x1 + 4, y2, GetColor(255, 255, 255), TRUE);
            DrawBox(x2 - 4, y1, x2, y2, GetColor(255, 255, 255), TRUE);
        }

        // ???x??
        const wchar_t* name = diffs[i].label.c_str();
        int nw = GetDrawStringWidthToHandle(name, (int)wcslen(name), fontButton);
        int nx = x1 + (BTN_W - nw) / 2;
        int ny = y1 + bh / 2 - 18;
        // ?e?e?L?X?g
        DrawStringToHandle(nx + 1, ny + 1, name, GetColor(0, 0, 0), fontButton);
        DrawStringToHandle(nx, ny, name, GetColor(255, 255, 255), fontButton);

        if (diffs[i].level > 0) {
            std::wstring stars;
            for (int s = 0; s < diffs[i].level && s < 10; s++) stars += L"\u2605";
            int sw = GetDrawStringWidthToHandle(stars.c_str(),
                static_cast<int>(stars.size()), fontLevel);
            DrawStringToHandle(x1 + (BTN_W - sw) / 2, y1 + bh / 2 + 16,
                stars.c_str(), GetColor(255, 240, 120), fontLevel);
        }

        // ?? ?I??C???W?P?[?^?i?????j
        if (sel) {
            int ax = x1 + BTN_W / 2;
            int ay = y2 + 18;
            DrawTriangle(ax - 14, ay, ax + 14, ay, ax, ay + 16,
                GetColor(255, 255, 255), TRUE);
        }
    }

    // ???? ????q???g ????????????????????????????????????????????????????????????????????????????????????????????????
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
    DrawBox(0, SH - 44, SW, SH, GetColor(20, 20, 20), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const wchar_t* hint = L"D / K : ???E????@?@J / F : ????@?@ESC : ??I??????";
    int hw = GetDrawStringWidthToHandle(hint, (int)wcslen(hint), fontLevel);
    DrawStringToHandle(SW / 2 - hw / 2, SH - 34, hint, GetColor(180, 180, 180), fontLevel);
}