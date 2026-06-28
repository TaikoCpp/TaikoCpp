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
            if (don && sndDon[sndDonIdx] != -1) {
                PlaySoundMem(sndDon[sndDonIdx], DX_PLAYTYPE_BACK);
                sndDonIdx = (sndDonIdx + 1) % SND_BUF;
            }
            if (ka && sndKa[sndKaIdx] != -1) {
                PlaySoundMem(sndKa[sndKaIdx], DX_PLAYTYPE_BACK);
                sndKaIdx = (sndKaIdx + 1) % SND_BUF;
            }
        }
    }
    else if (absDelta <= JUDGE_KA_MS) {
        if ((expectDon && don) || (expectKa && ka)) {
            active.judged = true;
            RegisterJudge(JudgeResult::Ka, type);
            if (don && sndDon[sndDonIdx] != -1) {
                PlaySoundMem(sndDon[sndDonIdx], DX_PLAYTYPE_BACK);
                sndDonIdx = (sndDonIdx + 1) % SND_BUF;
            }
            if (ka && sndKa[sndKaIdx] != -1) {
                PlaySoundMem(sndKa[sndKaIdx], DX_PLAYTYPE_BACK);
                sndKaIdx = (sndKaIdx + 1) % SND_BUF;
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

void GamePlay::DrawNote(const Note& note, float x) const {
    const int laneY = 400;
    const int radius = (note.type == NoteType::DON_L || note.type == NoteType::KAT_L ||
        note.type == NoteType::ROLL_HEAD_L) ? 28 : 20;

    bool isDon = note.type == NoteType::DON || note.type == NoteType::DON_L ||
        note.type == NoteType::ROLL_HEAD || note.type == NoteType::ROLL_HEAD_L ||
        note.type == NoteType::BALLOON_HEAD || note.type == NoteType::KUSUDAMA;
    bool isRoll = note.type == NoteType::ROLL_HEAD || note.type == NoteType::ROLL_HEAD_L;
    bool isBalloon = note.type == NoteType::BALLOON_HEAD || note.type == NoteType::KUSUDAMA;

    unsigned int color = isDon ? GetColor(220, 50, 50) : GetColor(80, 130, 220);

    if (isRoll) {
        DrawBox(static_cast<int>(x) - 8, laneY - 12, static_cast<int>(x) + 80, laneY + 12,
            color, TRUE);
    }
    else if (isBalloon) {
        DrawCircle(static_cast<int>(x), laneY, radius + 6, GetColor(255, 200, 80), TRUE);
        DrawCircle(static_cast<int>(x), laneY, radius, color, TRUE);
    }
    else {
        DrawCircle(static_cast<int>(x), laneY, radius, color, TRUE);
        DrawCircle(static_cast<int>(x), laneY, radius, GetColor(255, 255, 255), FALSE);
    }
}

void GamePlay::Draw() {
    const int SW = 1280, SH = 720;

    DrawBox(0, 0, SW, SH, GetColor(30, 25, 40), TRUE);
    DrawBox(0, 340, SW, 460, GetColor(50, 45, 60), TRUE);

    DrawLine(JUDGE_X, 320, JUDGE_X, 480, GetColor(255, 255, 255), FALSE);
    DrawCircle(JUDGE_X, 400, 36, GetColor(255, 255, 255), FALSE);

    for (const auto& active : notes) {
        if (active.judged) continue;
        float x = NoteScreenX(*active.note);
        if (x < -60 || x > SW + 60) continue;
        DrawNote(*active.note, x);
    }

    DrawFormatString(20, 20, GetColor(255, 255, 255), L"Score: %d", score);
    DrawFormatString(20, 55, GetColor(255, 220, 100), L"Combo: %d", combo);
    DrawFormatString(20, 90, GetColor(180, 180, 180),
        L"Ryo:%d  Ka:%d  Fuka:%d", ryoCount, kaCount, fukaCount);

    int tw = GetDrawStringWidthToHandle(songEntry.title.c_str(), -1, fontUI);
    DrawStringToHandle(SW / 2 - tw / 2, 12, songEntry.title.c_str(),
        GetColor(255, 255, 255), fontUI);

    const wchar_t* diffNames[] = { L"\u304b\u3093\u305f\u3093", L"\u3075\u3064\u3046",
        L"\u3080\u305a\u304b\u3057\u3044", L"\u304a\u306b", L"Edit" };
    if (diffId >= 0 && diffId <= 4) {
        DrawFormatString(SW - 160, 12, GetColor(255, 200, 200), L"%s", diffNames[diffId]);
    }

    DrawFormatString(20, SH - 40, GetColor(150, 150, 150),
        L"J/F=Don  D/K=Ka  ESC=Quit");
}