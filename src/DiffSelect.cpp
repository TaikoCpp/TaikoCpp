#include "DiffSelect.h"
#include "TJAParser.h"
#include <fstream>
#include <map>
#include <set>
#include <algorithm>

unsigned int DiffSelect::DiffColor(int diffId) {
    switch (diffId) {
    case 0: return GetColor(120, 200, 80);
    case 1: return GetColor(80, 160, 220);
    case 2: return GetColor(220, 160, 40);
    case 3: return GetColor(220, 60, 60);
    case 4: return GetColor(160, 80, 200);
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
    case 0: return L"Easy";
    case 1: return L"Normal";
    case 2: return L"Hard";
    case 3: return L"Oni";
    case 4: return L"Edit";
    default: return L"Unknown";
    }
}

DiffSelect::DiffSelect(const SongEntry& song) : songEntry(song), options() {
    fontTitle = CreateFontToHandle(L"FOT-大江戸勘亭流 Std E", 36, 3, DX_FONTTYPE_ANTIALIASING_4X4, DX_CHARSET_DEFAULT);
    fontButton = CreateFontToHandle(L"FOT-大江戸勘亭流 Std E", 28, 3, DX_FONTTYPE_ANTIALIASING_4X4, DX_CHARSET_DEFAULT);
    fontLevel = CreateFontToHandle(L"FOT-大江戸勘亭流 Std E", 20, 2, DX_FONTTYPE_ANTIALIASING_4X4, DX_CHARSET_DEFAULT);

    sndDong = LoadSoundMem(L"Theme\\default\\sounds\\dong.wav");
    sndKa = LoadSoundMem(L"Theme\\default\\sounds\\ka.wav");

    prevD = CheckHitKey(KEY_INPUT_D) != 0;
    prevK = CheckHitKey(KEY_INPUT_K) != 0;
    prevJ = CheckHitKey(KEY_INPUT_J) != 0;
    prevF = CheckHitKey(KEY_INPUT_F) != 0;
    prevEsc = CheckHitKey(KEY_INPUT_ESCAPE) != 0;
    prevO = CheckHitKey(KEY_INPUT_O) != 0;

    // TJA: COURSE / LEVEL の解析
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
    bool curO = CheckHitKey(KEY_INPUT_O) != 0;

    int  mx = 0, my = 0;
    GetMousePoint(&mx, &my);
    int  mouseInput = GetMouseInput();
    bool mouseLeft = (mouseInput & MOUSE_INPUT_LEFT) != 0;

    const int BTN_X1 = 12, BTN_Y1 = 12, BTN_X2 = 84, BTN_Y2 = 44;

    // OP ボタン・マウス処理
    if (mouseLeft && !prevMouseLeft) {
        // OP ボタン領域
        if (mx >= BTN_X1 && mx <= BTN_X2 && my >= BTN_Y1 && my <= BTN_Y2) {
            optionsVisible = !optionsVisible;
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }
        // オプション領域クリック
        if (optionsVisible) {
            const int optX = 12, optY = 54, optW = 320, lineH = 28;
            for (int i = 0; i < OPT_ROW_COUNT; ++i) {
                int ly1 = optY + i * (lineH + 6);
                int ly2 = ly1 + lineH;
                if (mx >= optX && mx <= optX + optW && my >= ly1 && my <= ly2) {
                    optionsSel = i;
                    CycleOption(i, +1);
                    if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
                }
            }
        }
    }

    // Oキーでオプション表示切替
    if (curO && !prevO) {
        optionsVisible = !optionsVisible;
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
    }

    // オプション表示中の入力処理
    if (optionsVisible) {
        if (curD && !prevD) {
            optionsSel = (optionsSel + 1) % OPT_ROW_COUNT;
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }
        if (curK && !prevK) {
            optionsSel = (optionsSel - 1 + OPT_ROW_COUNT) % OPT_ROW_COUNT;
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }
        if (curJ && !prevJ) {
            CycleOption(optionsSel, +1);
            if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
        }
        if (curF && !prevF) {
            CycleOption(optionsSel, -1);
            if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
        }
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        prevO = curO; prevMouseLeft = mouseLeft;
        return false;
    }

    // 難易度移動処理
    if (!diffs.empty()) {
        if (curD && !prevD) {
            selectedIndex = (selectedIndex + 1) % (int)diffs.size();
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }
        if (curK && !prevK) {
            selectedIndex = (selectedIndex - 1 + (int)diffs.size()) % (int)diffs.size();
            if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        }

        if ((curJ && !prevJ) || (curF && !prevF)) {
            if (sndDong != -1) PlaySoundMem(sndDong, DX_PLAYTYPE_BACK);
            if (selectedIndex >= 0 && selectedIndex < (int)diffs.size()) {
                decided = true;
                prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
                prevO = curO; prevMouseLeft = mouseLeft;
                return true;
            }
        }
    }

    // ESC で戻る
    if (curEsc && !prevEsc) {
        if (sndKa != -1) PlaySoundMem(sndKa, DX_PLAYTYPE_BACK);
        prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
        prevO = curO; prevMouseLeft = mouseLeft;
        return true;
    }

    prevD = curD; prevK = curK; prevJ = curJ; prevF = curF; prevEsc = curEsc;
    prevO = curO; prevMouseLeft = mouseLeft;
    return false;
}

// オプションを切り替える (delta: +1=増加, -1=減少)
void DiffSelect::CycleOption(int row, int delta) {
    switch (row) {
    case 0: // 音符モード (通常 / きまぐれ / でたらめ / あべこべ)
        options.noteMode = (options.noteMode + 4 + delta) % 4;
        break;
    case 1: // 速度 (PLAY_SPEED_TABLE のインデックス)
        options.speedIndex = (options.speedIndex + PLAY_SPEED_TABLE_SIZE + delta) % PLAY_SPEED_TABLE_SIZE;
        break;
    case 2: // 表示: 隠し譜面表示 ON/OFF
        options.hidden = !options.hidden;
        break;
    }
}

void DiffSelect::Draw() {
    const int SW = 1280, SH = 720;

    DrawBox(0, 0, SW, SH / 2, GetColor(245, 175, 175), TRUE);
    DrawBox(0, SH / 2, SW, SH, GetColor(225, 150, 150), TRUE);

    int tw = GetDrawStringWidthToHandle(songEntry.title.c_str(),
        (int)songEntry.title.size(), fontTitle);
    DrawStringToHandle(SW / 2 - tw / 2 + 2, 52, songEntry.title.c_str(),
        GetColor(100, 50, 50), fontTitle);
    DrawStringToHandle(SW / 2 - tw / 2, 50, songEntry.title.c_str(),
        GetColor(255, 255, 255), fontTitle);

    DrawBox(0, 108, SW, 110, GetColor(255, 200, 200), TRUE);

    const int BTN_W = 200;
    const int BTN_H = 200;
    const int BTN_SEL_H = 230;
    const int GAP = 20;

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

        if (sel) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 90);
            DrawBox(x1 + 6, y1 + 8, x2 + 6, y2 + 8, GetColor(0, 0, 0), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }

        int mid = y1 + bh * 2 / 3;
        DrawBox(x1, y1, x2, mid, col, TRUE);
        DrawBox(x1, mid, x2, y2, colDark, TRUE);

        if (sel) {
            DrawBox(x1, y1, x2, y1 + 4, GetColor(255, 255, 255), TRUE);
            DrawBox(x1, y2 - 4, x2, y2, GetColor(255, 255, 255), TRUE);
            DrawBox(x1, y1, x1 + 4, y2, GetColor(255, 255, 255), TRUE);
            DrawBox(x2 - 4, y1, x2, y2, GetColor(255, 255, 255), TRUE);
        }

        const wchar_t* name = diffs[i].label.c_str();
        int nw = GetDrawStringWidthToHandle(name, (int)wcslen(name), fontButton);
        DrawStringToHandle(x1 + (BTN_W - nw) / 2 + 1, y1 + bh / 2 - 17,
            name, GetColor(0, 0, 0), fontButton);
        DrawStringToHandle(x1 + (BTN_W - nw) / 2, y1 + bh / 2 - 18,
            name, GetColor(255, 255, 255), fontButton);

        if (diffs[i].level > 0) {
            std::wstring stars;
            for (int s = 0; s < diffs[i].level && s < 10; s++) stars += L"\u2605";
            int sw2 = GetDrawStringWidthToHandle(stars.c_str(),
                static_cast<int>(stars.size()), fontLevel);
            DrawStringToHandle(x1 + (BTN_W - sw2) / 2, y1 + bh / 2 + 16,
                stars.c_str(), GetColor(255, 240, 120), fontLevel);
        }

        if (sel) {
            int ax = x1 + BTN_W / 2;
            int ay = y2 + 18;
            DrawTriangle(ax - 14, ay, ax + 14, ay, ax, ay + 16,
                GetColor(255, 255, 255), TRUE);
        }
    }

    if (optionsVisible) {
        const int optX = 12, optY = 54, optW = 360;
        const int lineH = 28;
        const int panH = OPT_ROW_COUNT * (lineH + 6) + 6;

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 220);
        DrawBox(optX, optY, optX + optW, optY + panH, GetColor(20, 20, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 音符モードの表示ラベル
        static const wchar_t* noteLabels[] = { L"通常", L"きまぐれ", L"でたらめ", L"あべこべ" };

        wchar_t speedBuf[32];
        float sv = PLAY_SPEED_TABLE[options.speedIndex];
        if (sv == (int)sv)
            swprintf_s(speedBuf, L"%.0fx", sv);
        else
            swprintf_s(speedBuf, L"%.2gx", sv);

        const wchar_t* rows[OPT_ROW_COUNT][2] = {
            { L"譜面: ",  noteLabels[options.noteMode] },
            { L"速度: ",  speedBuf                     },
            { L"隠し: ",  options.hidden ? L"ON" : L"OFF" },
        };

        int nx = optX + 8;
        for (int i = 0; i < OPT_ROW_COUNT; ++i) {
            int ny = optY + 6 + i * (lineH + 6);
            bool sel = (optionsSel == i);
            unsigned int col = sel ? GetColor(255, 220, 80) : GetColor(220, 220, 220);

            std::wstring line = std::wstring(rows[i][0]) + rows[i][1];

            if (sel) line += L"     J/F 変更";

            DrawStringToHandle(nx + 1, ny + 1, line.c_str(), GetColor(0, 0, 0), fontLevel);
            DrawStringToHandle(nx, ny, line.c_str(), col, fontLevel);
        }
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
    DrawBox(0, SH - 44, SW, SH, GetColor(20, 20, 20), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const wchar_t* hint = L"D / K : 難易度移動    J / F : 決定    ESC : 戻る    O : オプション";
    int hw = GetDrawStringWidthToHandle(hint, (int)wcslen(hint), fontLevel);
    DrawStringToHandle(SW / 2 - hw / 2, SH - 34, hint, GetColor(180, 180, 180), fontLevel);
}