#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <memory>
#include "SongInfo.h"

namespace fs = std::filesystem;

class TJAParser {
public:
    // Difficulty mapping
    static const std::map<int, std::string> DIFFS;

    // Constructor
    explicit TJAParser(const fs::path& path, int start_delay_ms = 0);

    // Parse the file and populate the SongInfo
    SongInfo parse();

    // TJAファイルからTITLEだけを高速に読み取る（CP932対応）
    static std::wstring ReadTitle(const fs::path& path);

private:
    fs::path file_path;
    int start_delay_ms;
    std::vector<std::string> data; // lines after stripping comments and empty
    TJAMetadata metadata;
    TJAEXData ex_data;

    // Parsing state
    struct ParserState {
        float time_signature = 4.0f / 4.0f;
        float bpm = 120.0f;
        float bpmchange_last_bpm = 120.0f;
        float scroll_x_modifier = 1.0f;
        float scroll_y_modifier = 0.0f;
        ScrollType scroll_type = ScrollType::NMSCROLL;
        bool barline_display = true;
        std::vector<Note*>* curr_note_list = nullptr;
        std::vector<Note*>* curr_draw_list = nullptr;
        std::vector<Note*>* curr_bar_list = nullptr;
        std::vector<TimelineObject*>* curr_timeline = nullptr;
        int index = 0;
        std::vector<int> balloons;
        int balloon_index = 0;
        Note* prev_note = nullptr;
        bool barline_added = false;
        float sudden_appear = 0.0f;
        float sudden_moving = 0.0f;
        float judge_pos_x = 0.0f;
        float judge_pos_y = 0.0f;
        float delay_current = 0.0f;
        float delay_last_note_ms = 0.0f;
        bool is_branching = false;
        bool is_section_start = false;
        float start_branch_ms = 0.0f;
        float start_branch_bpm = 120.0f;
        float start_branch_time_sig = 4.0f / 4.0f;
        float start_branch_x_scroll = 1.0f;
        float start_branch_y_scroll = 0.0f;
        bool start_branch_barline = false;
        int branch_balloon_index = 0;
        Note* section_bar = nullptr;
    };

    // Helper functions
    static std::string strip_comments(const std::string& line);
    static std::string test_encoding(const fs::path& file_path);
    void get_metadata();
    std::vector<std::vector<std::string>> data_to_notes(int diff);
    void get_moji(std::vector<Note*>& play_note_list, float ms_per_measure);
    float get_ms_per_measure(float bpm_val, float time_sig);
    void add_bar(ParserState& state);
    Note* add_note(const std::string& item, ParserState& state);

    // Command handlers
    void handle_measure(const std::string& part, ParserState& state);
    void handle_scroll(const std::string& part, ParserState& state);
    void handle_bpmchange(const std::string& part, ParserState& state);
    void handle_section(const std::string& part, ParserState& state);
    void handle_branchstart(const std::string& part, ParserState& state);
    void handle_branchend(ParserState& state);
    void handle_lyric(const std::string& part, ParserState& state);
    void handle_jposscroll(const std::string& part, ParserState& state);
    void handle_nmscroll(ParserState& state);
    void handle_bmscroll(ParserState& state);
    void handle_hbscroll(ParserState& state);
    void handle_barlineon(ParserState& state);
    void handle_barlineoff(ParserState& state);
    void handle_gogostart(ParserState& state);
    void handle_gogoend(ParserState& state);
    void handle_delay(const std::string& part, ParserState& state);
    void handle_sudden(const std::string& part, ParserState& state);
    void handle_m(ParserState& state);
    void handle_e(ParserState& state);
    void handle_n(ParserState& state);

    // Branch handling
    void set_branch_params(std::vector<Note*>* bar_list, const std::string& branch_params, Note* section_bar, ParserState& state);

    // Note lists
    NoteList master_notes;
    std::vector<NoteList> branch_m;
    std::vector<NoteList> branch_e;
    std::vector<NoteList> branch_n;
};