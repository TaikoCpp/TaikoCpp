#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "SongInfo.h"

namespace fs = std::filesystem;

class TJAParser {
public:
    static const std::map<int, std::string> DIFFS;

    explicit TJAParser(const fs::path& path, int start_delay_ms = 0);

    // Parse a single difficulty chart into SongInfo
    SongInfo parse(int diff);

    static std::wstring ReadTitle(const fs::path& path);

private:
    fs::path file_path;
    int start_delay_ms;
    float current_ms_ = 0.0f;
    std::vector<std::string> data;
    TJAMetadata metadata;
    TJAEXData ex_data;

    struct ParserState {
        float time_signature = 4.0f / 4.0f;
        float bpm = 120.0f;
        float bpmchange_last_bpm = 120.0f;
        float scroll_x_modifier = 1.0f;
        float scroll_y_modifier = 0.0f;
        // scroll_type: チャート全体のスクロールモード
        //   NMSCROLL … note.bpm/120 × note.scroll_x で描画速度を計算 (GamePlay側で補正)
        //   BMSCROLL … BPM変化を bpmchange として Timeline に記録し、
        //               再生時に累積倍率を乗算して描画速度を決定
        //   HBSCROLL … BMSCROLL と同じ累積方式だが描画タイミングが異なる
        ScrollType scroll_type = ScrollType::NMSCROLL;
        // BMSCROLL/HBSCROLL 時の現在の bpmchange 累積倍率 (note.scroll_x に乗算して保存)
        float bpm_scroll_accum = 1.0f;

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

    NoteList master_notes;
    std::vector<NoteList> branch_m;
    std::vector<NoteList> branch_e;
    std::vector<NoteList> branch_n;
    ScrollType chart_scroll_type_ = ScrollType::NMSCROLL;  // パース結果のスクロールタイプ

    static std::string strip_comments(const std::string& line);
    static void trim(std::string& s);
    static std::vector<int> parse_int_list(const std::string& raw);
    static int parse_course_value(const std::string& course_str);

    void load_file_lines();
    void get_metadata();
    std::vector<std::vector<std::string>> data_to_notes(int diff);
    void notes_to_position(int diff);
    void get_moji(std::vector<Note*>& play_note_list, float ms_per_measure);
    static float get_ms_per_measure(float bpm_val, float time_sig);
    Note* add_bar(ParserState& state);
    Note* add_note(const std::string& item, ParserState& state);
    void dispatch_command(const std::string& part, ParserState& state);

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
    void set_branch_params(std::vector<Note*>* bar_list, const std::string& branch_params,
        Note* section_bar, ParserState& state);
};