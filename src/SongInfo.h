#pragma once
#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace fs = std::filesystem;

// Forward declarations
class Note;
class Drumroll;
class Balloon;
class TimelineObject;
class NoteList;
struct CourseData;

enum class NoteType : int {
    NONE = 0,
    DON = 1,
    KAT = 2,
    DON_L = 3,
    KAT_L = 4,
    ROLL_HEAD = 5,
    ROLL_HEAD_L = 6,
    BALLOON_HEAD = 7,
    TAIL = 8,
    KUSUDAMA = 9
};

enum class ScrollType : int {
    NMSCROLL = 0,
    BMSCROLL = 1,
    HBSCROLL = 2
};

enum class Difficulty : int {
    EASY = 0,
    NORMAL = 1,
    HARD = 2,
    ONI = 3,
    EDIT = 4,
    TOWER = 5,
    DAN = 6
};

struct TimingPoint {
    float ms = 0.0f;
    float bpm = 120.0f;
    float time_signature = 4.0f / 4.0f;
    float scroll_x = 1.0f;
    float scroll_y = 0.0f;
    bool gogo = false;
    std::string lyric;
    bool is_branch_start = false;
    std::string branch_params;
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
    fs::path wave;
    float demostart = 0.0f;
    float offset = 0.0f;
    float bpm = 120.0f;
    fs::path bgmovie;
    float movieoffset = 0.0f;
    std::string scene_preset;
    std::map<int, CourseData> course_data;
};

struct TJAEXData {
    bool new_audio = false;
    bool old_audio = false;
    bool limited_time = false;
    bool is_new = false;
};

class Note {
public:
    NoteType type = NoteType::NONE;
    float hit_ms = 0.0f;
    float load_ms = 0.0f;
    float unload_ms = 0.0f;
    float bpm = 120.0f;
    float scroll_x = 1.0f;
    float scroll_y = 0.0f;
    float sudden_appear_ms = 0.0f;
    float sudden_moving_ms = 0.0f;
    bool display = true;
    int index = 0;
    int moji = 0;
    std::string branch_params;
    bool is_branch_start = false;

    virtual ~Note() = default;
    virtual Note* clone() const {
        return new Note(*this);
    }

    bool operator<(const Note& other) const { return hit_ms < other.hit_ms; }
    bool operator<=(const Note& other) const { return hit_ms <= other.hit_ms; }
    bool operator>(const Note& other) const { return hit_ms > other.hit_ms; }
    bool operator>=(const Note& other) const { return hit_ms >= other.hit_ms; }
    bool operator==(const Note& other) const { return hit_ms == other.hit_ms; }
};

class Drumroll : public Note {
public:
    Drumroll(const Note& source) {
        type = source.type;
        hit_ms = source.hit_ms;
        load_ms = source.load_ms;
        unload_ms = source.unload_ms;
        bpm = source.bpm;
        scroll_x = source.scroll_x;
        scroll_y = source.scroll_y;
        sudden_appear_ms = source.sudden_appear_ms;
        sudden_moving_ms = source.sudden_moving_ms;
        display = source.display;
        index = source.index;
        moji = source.moji;
        branch_params = source.branch_params;
        is_branch_start = source.is_branch_start;
        color = 255;
    }

    Drumroll* clone() const override {
        return new Drumroll(*this);
    }

    int color = 0;
};

class Balloon : public Note {
public:
    Balloon(const Note& source, bool is_kusudama = false) {
        type = source.type;
        hit_ms = source.hit_ms;
        load_ms = source.load_ms;
        unload_ms = source.unload_ms;
        bpm = source.bpm;
        scroll_x = source.scroll_x;
        scroll_y = source.scroll_y;
        sudden_appear_ms = source.sudden_appear_ms;
        sudden_moving_ms = source.sudden_moving_ms;
        display = source.display;
        index = source.index;
        moji = source.moji;
        branch_params = source.branch_params;
        is_branch_start = source.is_branch_start;
        count = 1;
        popped = false;
        is_kusudama = is_kusudama;
    }

    Balloon* clone() const override {
        return new Balloon(*this);
    }

    int count = 0;
    bool popped = false;
    bool is_kusudama = false;
};

class TimelineObject {
public:
    float hit_ms = 0.0f;
    float load_ms = 0.0f;
    float judge_pos_x = 0.0f;
    float judge_pos_y = 0.0f;
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    float bpm = 120.0f;
    float bpmchange = 1.0f;
    float delay = 0.0f;
    bool gogo_time = false;
    std::string branch_params;
    bool is_branch_start = false;
    bool is_section_marker = false;
    std::string lyric;
};

class NoteList {
public:
    std::vector<Note*> play_notes;
    std::vector<Note*> draw_notes;
    std::vector<Note*> bars;
    std::vector<TimelineObject*> timeline;

    // Constructors
    NoteList() = default;
    NoteList(NoteList&& other) noexcept :
        play_notes(std::move(other.play_notes)),
        draw_notes(std::move(other.draw_notes)),
        bars(std::move(other.bars)),
        timeline(std::move(other.timeline)) {
        // Note: moving a vector of pointers does not nullify the pointers in the source.
        // To avoid double delete, we nullify the pointers in the source after moving.
        for (auto*& ptr : other.play_notes) {
            ptr = nullptr;
        }
        for (auto*& ptr : other.draw_notes) {
            ptr = nullptr;
        }
        for (auto*& ptr : other.bars) {
            ptr = nullptr;
        }
        for (auto*& ptr : other.timeline) {
            ptr = nullptr;
        }
        // Clear the source vectors (now they contain nullptr, but we clear to be safe)
        other.play_notes.clear();
        other.draw_notes.clear();
        other.bars.clear();
        other.timeline.clear();
    }
    NoteList& operator=(NoteList&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            for (auto* n : play_notes) delete n;
            for (auto* n : draw_notes) delete n;
            for (auto* n : bars) delete n;
            for (auto* t : timeline) delete t;
            // Steal resources from other
            play_notes = std::move(other.play_notes);
            draw_notes = std::move(other.draw_notes);
            bars = std::move(other.bars);
            timeline = std::move(other.timeline);
            // Nullify pointers in other
            for (auto*& ptr : other.play_notes) {
                ptr = nullptr;
            }
            for (auto*& ptr : other.draw_notes) {
                ptr = nullptr;
            }
            for (auto*& ptr : other.bars) {
                ptr = nullptr;
            }
            for (auto*& ptr : other.timeline) {
                ptr = nullptr;
            }
            // Clear the source vectors
            other.play_notes.clear();
            other.draw_notes.clear();
            other.bars.clear();
            other.timeline.clear();
        }
        return *this;
    }

    // Copy constructor
    NoteList(const NoteList& other) {
        // Deep copy of notes
        for (auto* note : other.play_notes) {
            play_notes.push_back(note->clone());
        }
        for (auto* note : other.draw_notes) {
            draw_notes.push_back(note->clone());
        }
        for (auto* note : other.bars) {
            bars.push_back(note->clone());
        }
        // Deep copy of timeline objects
        for (auto* obj : other.timeline) {
            timeline.push_back(new TimelineObject(*obj));
        }
    }

    // Copy assignment operator
    NoteList& operator=(const NoteList& other) {
        if (this != &other) {
            // Clean up current resources
            for (auto* n : play_notes) delete n;
            for (auto* n : draw_notes) delete n;
            for (auto* n : bars) delete n;
            for (auto* t : timeline) delete t;
            play_notes.clear();
            draw_notes.clear();
            bars.clear();
            timeline.clear();

            // Deep copy from other
            for (auto* note : other.play_notes) {
                play_notes.push_back(note->clone());
            }
            for (auto* note : other.draw_notes) {
                draw_notes.push_back(note->clone());
            }
            for (auto* note : other.bars) {
                bars.push_back(note->clone());
            }
            for (auto* obj : other.timeline) {
                timeline.push_back(new TimelineObject(*obj));
            }
        }
        return *this;
    }

    ~NoteList() {
        for (auto* n : play_notes) delete n;
        for (auto* n : draw_notes) delete n;
        for (auto* n : bars) delete n;
        for (auto* t : timeline) delete t;
    }
};

struct SongInfo {
    TJAMetadata metadata;
    TJAEXData ex_data;
    NoteList master_notes;
    std::vector<NoteList> branch_m;
    std::vector<NoteList> branch_e;
    std::vector<NoteList> branch_n;
    // チャート全体のスクロールモード (TJAParser がパース時に設定)
    // NMSCROLL: GamePlay で (note.bpm/120) × note.scroll_x × BASE_PX を使う
    // BMSCROLL/HBSCROLL: note.bpm=120 固定、note.scroll_x に bpmchange 累積倍率込み
    ScrollType scroll_type = ScrollType::NMSCROLL;
};