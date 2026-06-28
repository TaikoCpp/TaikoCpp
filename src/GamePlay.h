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

    static constexpr int SND_BUF = 4;
    int sndDon[SND_BUF] = { -1,-1,-1,-1 };
    int sndKa[SND_BUF] = { -1,-1,-1,-1 };
    int sndDonIdx = 0;
    int sndKaIdx = 0;
    int sndWave = -1;
    int fontUI = -1;
    int fontScore = -1;

    bool prevDon = false;
    bool prevKa = false;
    bool prevEsc = false;

    static constexpr int JUDGE_X = 220;
    static constexpr float SCROLL_PX_PER_MS = 0.55f;
    static constexpr float JUDGE_RYO_MS = 35.0f;
    static constexpr float JUDGE_KA_MS = 80.0f;

    float GetChartMs() const;
    float NoteScreenX(const Note& note) const;
    bool IsHittable(const Note& note) const;
    void ProcessInput(bool donPressed, bool kaPressed);
    void JudgeNote(ActiveNote& active, bool don, bool ka);
    void RegisterJudge(JudgeResult result, NoteType type);
    void DrawNote(const Note& note, float x) const;
    std::wstring PathToWide(const std::filesystem::path& p) const;
};