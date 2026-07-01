#pragma once
#include <DxLib.h>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;
static constexpr float PLAY_SPEED_TABLE[] = {
    0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f
};
static constexpr int PLAY_SPEED_TABLE_SIZE = 10;
static constexpr int PLAY_SPEED_DEFAULT_INDEX = 2; // 1.0x


struct PlayOptions {
    int noteMode = 0;

    int speedIndex = PLAY_SPEED_DEFAULT_INDEX;
    bool hidden = false;
};

enum class NoteType {
    NONE = -1,
    DON = 0,
    KAT = 1,
    DON_L = 2,
    KAT_L = 3,
    ROLL_HEAD = 4,
    ROLL_HEAD_L = 5,
    BALLOON_HEAD = 6,
    KUSUDAMA = 7,
    TAIL = 8
};

enum class ScrollType {
    NMSCROLL,
    BMSCROLL,
    HBSCROLL
};

struct TimelineObject {
    float hit_ms = 0.0f;
    float load_ms = 0.0f;
    float bpm = 0.0f;
    float bpmchange = 0.0f;
    float delay = 0.0f;
    float judge_pos_x = 0.0f;
    float judge_pos_y = 0.0f;
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    bool gogo_time = false;
    std::string lyric;
};

struct Note {
    virtual ~Note() {}
    float hit_ms = 0.0f;
    NoteType type = NoteType::NONE;
    bool display = true;
    float scroll_y = 0.0f;
    float bpm = 120.0f;
    float scroll_x = 1.0f;
    float sudden_appear_ms = 0.0f;
    float sudden_moving_ms = 0.0f;
    int index = 0;
    float unload_ms = 0.0f;
    int moji = 0;
    bool is_branch_start = false;
    std::string branch_params;
    bool is_section_bar = false;
};

struct Drumroll : public Note {
    Drumroll() = default;
    Drumroll(const Note& base) : Note(base) {}
    int dummy = 0;
};

struct Balloon : public Note {
    Balloon() = default;
    Balloon(const Note& base, bool explosive = false) : Note(base), explosive(explosive) {}
    int count = 0;
    bool explosive = false;
};

inline bool IsValidNotePtr(const Note* p) {
    if (!p) return false;
    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    return (addr != static_cast<uintptr_t>(-1)) && (addr >= 0x10000);
}

struct NoteList {
    std::vector<Note*> play_notes;
    std::vector<Note*> draw_notes;
    std::vector<Note*> bars;
    std::vector<TimelineObject*> timeline;

    NoteList() = default;
    NoteList(const NoteList&) = delete;
    NoteList& operator=(const NoteList&) = delete;

    NoteList(NoteList&& other) noexcept
        : play_notes(std::move(other.play_notes))
        , draw_notes(std::move(other.draw_notes))
        , bars(std::move(other.bars))
        , timeline(std::move(other.timeline))
    {
    }

    NoteList& operator=(NoteList&& other) noexcept {
        if (this != &other) {
            ClearAll();
            play_notes = std::move(other.play_notes);
            draw_notes = std::move(other.draw_notes);
            bars = std::move(other.bars);
            timeline = std::move(other.timeline);
        }
        return *this;
    }

    void ClearAll() {
        std::unordered_set<Note*> unique;
        unique.reserve(play_notes.size() + draw_notes.size() + bars.size());
        for (Note* p : play_notes) if (p) unique.insert(p);
        for (Note* p : draw_notes) if (p) unique.insert(p);
        for (Note* p : bars)       if (p) unique.insert(p);
        for (Note* p : unique) {
            if (!IsValidNotePtr(p)) continue;
            delete p;
        }
        for (TimelineObject* t : timeline) {
            if (!t) continue;
            uintptr_t addr = reinterpret_cast<uintptr_t>(t);
            if (addr == static_cast<uintptr_t>(-1) || addr < 0x10000) continue;
            delete t;
        }
        play_notes.clear();
        draw_notes.clear();
        bars.clear();
        timeline.clear();
    }

    ~NoteList() { ClearAll(); }
};

struct CourseData {
    int level = 0;
    std::vector<int> balloon;
    std::vector<int> scoreinit;
    int scorediff = 0;
    bool is_branching = false;
};

struct TJAMetadata {
    std::map<std::string, std::string> title;
    std::map<std::string, std::string> subtitle;
    std::string genre;
    float bpm = 0.0f;
    fs::path wave;
    float offset = 0.0f;
    float demostart = 0.0f;
    fs::path bgmovie;
    float movieoffset = 0.0f;
    std::string scene_preset;
    std::map<int, CourseData> course_data;
};

struct TJAEXData {
    bool new_audio = false;
    bool old_audio = false;
    bool limited_time = false;
};

struct SongInfo {
    TJAMetadata metadata;
    TJAEXData ex_data;
    NoteList master_notes;
    std::vector<NoteList> branch_m;
    std::vector<NoteList> branch_e;
    std::vector<NoteList> branch_n;
    ScrollType scroll_type = ScrollType::NMSCROLL;
};