#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif
#include "TJAParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace fs = std::filesystem;

const std::map<int, std::string> TJAParser::DIFFS = {
    {0, "easy"}, {1, "normal"}, {2, "hard"}, {3, "oni"},
    {4, "edit"}, {5, "tower"}, {6, "dan"}
};

TJAParser::TJAParser(const fs::path& path, int start_delay_ms)
    : file_path(path), start_delay_ms(start_delay_ms) {
}

std::string TJAParser::strip_comments(const std::string& line) {
    size_t pos = line.find("//");
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

void TJAParser::trim(std::string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    if (!s.empty()) {
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
    }
}

std::vector<int> TJAParser::parse_int_list(const std::string& raw) {
    std::vector<int> result;
    std::string normalized = raw;
    for (char& c : normalized) {
        if (c == '.') c = ',';
    }
    std::stringstream ss(normalized);
    std::string token;
    while (std::getline(ss, token, ',')) {
        trim(token);
        if (token.empty()) continue;
        try {
            result.push_back(static_cast<int>(std::stof(token)));
        }
        catch (...) {}
    }
    return result;
}

int TJAParser::parse_course_value(const std::string& course_str) {
    if (course_str == "6" || course_str == "dan") return 6;
    if (course_str == "5" || course_str == "tower") return 5;
    if (course_str == "4" || course_str == "edit" || course_str == "ura") return 4;
    if (course_str == "3" || course_str == "oni") return 3;
    if (course_str == "2" || course_str == "hard") return 2;
    if (course_str == "1" || course_str == "normal") return 1;
    if (course_str == "0" || course_str == "easy") return 0;
    try {
        return std::stoi(course_str);
    }
    catch (...) {
        return -1;
    }
}

void TJAParser::load_file_lines() {
    data.clear();
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string cleaned = strip_comments(line);
        trim(cleaned);
        if (!cleaned.empty()) {
            data.push_back(cleaned);
        }
    }
}

void TJAParser::get_metadata() {
    int current_diff = -1;

    for (const std::string& item : data) {
        if (item.rfind("#BRANCH", 0) == 0 && current_diff != -1) {
            metadata.course_data[current_diff].is_branching = true;
        }
        else if (!item.empty() && (item[0] == '#' || std::isdigit(static_cast<unsigned char>(item[0])))) {
            continue;
        }
        else if (item.rfind("SUBTITLE", 0) == 0) {
            std::string region_code = "en";
            if (item.length() > 8 && item[8] != ':') {
                if (item.length() >= 10) {
                    region_code = item.substr(8, 2);
                    std::transform(region_code.begin(), region_code.end(), region_code.begin(), ::tolower);
                }
            }
            std::string value = item.substr(item.find(':') + 1);
            while (value.find("--") != std::string::npos) {
                value.replace(value.find("--"), 2, "");
            }
            metadata.subtitle[region_code] = value;
            if (metadata.subtitle.count("ja") &&
                metadata.subtitle["ja"].find("\xe9\x99\x90\xe5\xae\x9a") != std::string::npos) {
                ex_data.limited_time = true;
            }
        }
        else if (item.rfind("TITLE", 0) == 0) {
            std::string region_code = "en";
            if (item.length() > 5 && item[5] != ':') {
                if (item.length() >= 7) {
                    region_code = item.substr(5, 2);
                    std::transform(region_code.begin(), region_code.end(), region_code.begin(), ::tolower);
                }
            }
            metadata.title[region_code] = item.substr(item.find(':') + 1);
        }
        else if (item.rfind("GENRE", 0) == 0) {
            metadata.genre = item.substr(item.find(':') + 1);
            trim(metadata.genre);
        }
        else if (item.rfind("BPM", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            try {
                metadata.bpm = data_str.empty() ? 0.0f : std::stof(data_str);
            }
            catch (...) {
                metadata.bpm = 0.0f;
            }
        }
        else if (item.rfind("WAVE", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            fs::path wave_path = file_path.parent_path() / data_str;
            metadata.wave = fs::exists(wave_path) ? wave_path : fs::path();
        }
        else if (item.rfind("OFFSET", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            try {
                metadata.offset = data_str.empty() ? 0.0f : std::stof(data_str);
            }
            catch (...) {
                metadata.offset = 0.0f;
            }
        }
        else if (item.rfind("DEMOSTART", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            try {
                metadata.demostart = data_str.empty() ? 0.0f : std::stof(data_str);
            }
            catch (...) {
                metadata.demostart = 0.0f;
            }
        }
        else if (item.rfind("BGMOVIE", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            fs::path bgmovie_path = file_path.parent_path() / data_str;
            metadata.bgmovie = fs::exists(bgmovie_path) ? bgmovie_path : fs::path();
        }
        else if (item.rfind("MOVIEOFFSET", 0) == 0) {
            std::string data_str = item.substr(item.find(':') + 1);
            trim(data_str);
            try {
                metadata.movieoffset = data_str.empty() ? 0.0f : std::stof(data_str);
            }
            catch (...) {
                metadata.movieoffset = 0.0f;
            }
        }
        else if (item.rfind("SCENEPRESET", 0) == 0) {
            metadata.scene_preset = item.substr(item.find(':') + 1);
            trim(metadata.scene_preset);
        }
        else if (item.rfind("COURSE", 0) == 0) {
            std::string course_str = item.substr(item.find(':') + 1);
            std::transform(course_str.begin(), course_str.end(), course_str.begin(), ::tolower);
            trim(course_str);
            current_diff = parse_course_value(course_str);
            if (current_diff != -1) {
                metadata.course_data[current_diff] = CourseData();
            }
        }
        else if (current_diff != -1) {
            if (item.rfind("LEVEL", 0) == 0) {
                std::string val = item.substr(item.find(':') + 1);
                trim(val);
                try {
                    metadata.course_data[current_diff].level =
                        val.empty() ? 0 : static_cast<int>(std::stof(val));
                }
                catch (...) {
                    metadata.course_data[current_diff].level = 0;
                }
            }
            else if (item.rfind("BALLOONNOR", 0) == 0 || item.rfind("BALLOONEXP", 0) == 0) {
                auto vals = parse_int_list(item.substr(item.find(':') + 1));
                auto& balloon = metadata.course_data[current_diff].balloon;
                balloon.insert(balloon.end(), vals.begin(), vals.end());
            }
            else if (item.rfind("BALLOONMAS", 0) == 0) {
                metadata.course_data[current_diff].balloon =
                    parse_int_list(item.substr(item.find(':') + 1));
            }
            else if (item.rfind("BALLOON", 0) == 0) {
                if (item.find(':') != std::string::npos) {
                    metadata.course_data[current_diff].balloon =
                        parse_int_list(item.substr(item.find(':') + 1));
                }
                else {
                    metadata.course_data[current_diff].balloon.clear();
                }
            }
            else if (item.rfind("SCOREINIT", 0) == 0) {
                metadata.course_data[current_diff].scoreinit =
                    parse_int_list(item.substr(item.find(':') + 1));
            }
            else if (item.rfind("SCOREDIFF", 0) == 0) {
                std::string val = item.substr(item.find(':') + 1);
                trim(val);
                try {
                    metadata.course_data[current_diff].scorediff =
                        val.empty() ? 0 : static_cast<int>(std::stof(val));
                }
                catch (...) {
                    metadata.course_data[current_diff].scorediff = 0;
                }
            }
        }
    }

    for (auto& kv : metadata.title) {
        std::string& title = kv.second;
        if (title.find("-New Audio-") != std::string::npos ||
            title.find("\xe2\x96\xbc\xe6\x96\xb0\xe6\x9b\xb2\xe2\x96\xbc") != std::string::npos) {
            while (title.find("-New Audio-") != std::string::npos) {
                title.erase(title.find("-New Audio-"), 11);
            }
            ex_data.new_audio = true;
        }
        else if (title.find("-Old Audio-") != std::string::npos) {
            while (title.find("-Old Audio-") != std::string::npos) {
                title.erase(title.find("-Old Audio-"), 11);
            }
            ex_data.old_audio = true;
        }
        else if (title.find("\xe9\x99\x90\xe5\xae\x9a") != std::string::npos) {
            ex_data.limited_time = true;
        }
    }
}

std::vector<std::vector<std::string>> TJAParser::data_to_notes(int diff) {
    auto it = DIFFS.find(diff);
    std::string diff_name = (it != DIFFS.end()) ? it->second : "";
    std::transform(diff_name.begin(), diff_name.end(), diff_name.begin(), ::tolower);

    int note_start = -1;
    int note_end = -1;
    bool target_found = false;
    ScrollType scroll_type = ScrollType::NMSCROLL;

    for (size_t i = 0; i < data.size(); ++i) {
        const std::string& line = data[i];
        if (line.rfind("COURSE:", 0) == 0) {
            std::string course_value = line.substr(7);
            std::transform(course_value.begin(), course_value.end(), course_value.begin(), ::tolower);
            trim(course_value);
            bool is_digit = !course_value.empty() &&
                std::all_of(course_value.begin(), course_value.end(), ::isdigit);
            target_found = (is_digit && std::stoi(course_value) == diff) || course_value == diff_name;
        }
        else if (target_found) {
            if (note_start == -1 && (line == "#START" || line == "#START P1")) {
                note_start = static_cast<int>(i) + 1;
            }
            else if (line == "#END" && note_start != -1) {
                note_end = static_cast<int>(i);
                break;
            }
            else if (line.find("#NMSCROLL") != std::string::npos) {
                scroll_type = ScrollType::NMSCROLL;
            }
            else if (line.find("#BMSCROLL") != std::string::npos) {
                scroll_type = ScrollType::BMSCROLL;
            }
            else if (line.find("#HBSCROLL") != std::string::npos) {
                scroll_type = ScrollType::HBSCROLL;
            }
        }
    }

    if (note_start == -1 || note_end == -1) {
        return {};
    }

    std::vector<std::vector<std::string>> notes;
    std::vector<std::string> bar;
    std::vector<std::string> section_data(data.begin() + note_start, data.begin() + note_end);

    if (scroll_type == ScrollType::NMSCROLL) bar.push_back("#NMSCROLL");
    else if (scroll_type == ScrollType::BMSCROLL) bar.push_back("#BMSCROLL");
    else if (scroll_type == ScrollType::HBSCROLL) bar.push_back("#HBSCROLL");

    for (const std::string& line : section_data) {
        if (line.rfind("#", 0) == 0) {
            bar.push_back(line);
        }
        else if (line == ",") {
            if (bar.empty() || std::all_of(bar.begin(), bar.end(),
                [](const std::string& s) { return s.rfind("#", 0) == 0; })) {
                bar.push_back("");
            }
            notes.push_back(bar);
            bar.clear();
        }
        else {
            if (!line.empty() && line.back() == ',') {
                bar.push_back(line.substr(0, line.size() - 1));
                notes.push_back(bar);
                bar.clear();
            }
            else {
                bar.push_back(line);
            }
        }
    }

    if (!bar.empty()) {
        notes.push_back(bar);
    }

    return notes;
}

float TJAParser::get_ms_per_measure(float bpm_val, float time_sig) {
    if (bpm_val == 0.0f) return 0.0f;
    return 60000.0f * (time_sig * 4.0f) / bpm_val;
}

void TJAParser::get_moji(std::vector<Note*>& play_note_list, float ms_per_measure) {
    static const int se_notes[] = { 0, 0, 3, 5, 6, 7, 8, 9, 10, 11, 12 };
    if (play_note_list.size() <= 1) return;

    Note* current = play_note_list.back();
    int cur_type = static_cast<int>(current->type);
    if (cur_type == 1) current->moji = 0;
    else if (cur_type == 2) current->moji = 3;
    else if (cur_type >= 0 && cur_type <= 9) current->moji = se_notes[cur_type];

    Note* prev = play_note_list[play_note_list.size() - 2];
    int prev_type = static_cast<int>(prev->type);
    float threshold = ms_per_measure / 8.0f - 1.0f;

    if (prev_type == 1) {
        prev->moji = (current->hit_ms - prev->hit_ms <= threshold) ? 1 : 0;
    }
    else if (prev_type == 2) {
        prev->moji = (current->hit_ms - prev->hit_ms <= threshold) ? 4 : 3;
    }
    else if (prev_type >= 0 && prev_type <= 9) {
        prev->moji = se_notes[prev_type];
    }

    if (play_note_list.size() > 3) {
        Note* n4 = play_note_list[play_note_list.size() - 4];
        Note* n3 = play_note_list[play_note_list.size() - 3];
        Note* n2 = play_note_list[play_note_list.size() - 2];
        if (static_cast<int>(n4->type) == 1 && static_cast<int>(n3->type) == 1 &&
            static_cast<int>(n2->type) == 1) {
            bool rapid = (n3->hit_ms - n4->hit_ms < ms_per_measure / 8.0f) &&
                (n2->hit_ms - n3->hit_ms < ms_per_measure / 8.0f);
            if (rapid) {
                if (play_note_list.size() > 5) {
                    Note* n5 = play_note_list[play_note_list.size() - 5];
                    bool spacing_before = n4->hit_ms - n5->hit_ms >= ms_per_measure / 8.0f;
                    bool spacing_after = current->hit_ms - n2->hit_ms >= ms_per_measure / 8.0f;
                    if (spacing_before && spacing_after) n3->moji = 2;
                }
                else {
                    n3->moji = 2;
                }
            }
        }
    }
}

Note* TJAParser::add_bar(ParserState& state) {
    Note* bar_line = new Note();
    bar_line->hit_ms = current_ms_;
    bar_line->type = NoteType::NONE;
    bar_line->display = state.barline_display;
    bar_line->bpm = state.bpm;
    bar_line->scroll_x = state.scroll_x_modifier;
    bar_line->scroll_y = state.scroll_y_modifier;

    if (state.barline_added) {
        bar_line->display = false;
    }
    if (state.is_branching) {
        bar_line->is_branch_start = true;
        state.is_branching = false;
    }
    if (state.is_section_start) {
        state.section_bar = bar_line;
        state.is_section_start = false;
    }
    return bar_line;
}

Note* TJAParser::add_note(const std::string& item, ParserState& state) {
    Note* note = new Note();
    note->hit_ms = current_ms_;
    state.delay_last_note_ms = current_ms_;
    note->display = true;
    note->type = static_cast<NoteType>(std::stoi(item));
    note->index = state.index;
    note->bpm = state.bpm;
    note->scroll_x = state.scroll_x_modifier;
    note->scroll_y = state.scroll_y_modifier;

    if (state.sudden_appear > 0 || state.sudden_moving > 0) {
        note->sudden_appear_ms = state.sudden_appear;
        note->sudden_moving_ms = state.sudden_moving;
    }

    if (item == "5" || item == "6") {
        Drumroll* drumroll = new Drumroll(*note);
        delete note;
        note = drumroll;
    }
    else if (item == "7" || item == "9") {
        state.balloon_index++;
        Balloon* balloon = new Balloon(*note, item == "9");
        if (!state.balloons.empty()) {
            balloon->count = state.balloons.front();
            state.balloons.erase(state.balloons.begin());
        }
        delete note;
        note = balloon;
    }
    else if (item == "8") {
        note->type = NoteType::TAIL;
        if (state.prev_note) {
            state.prev_note->unload_ms = current_ms_;
        }
    }

    return note;
}

void TJAParser::handle_measure(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    size_t pos = val.find('/');
    if (pos != std::string::npos) {
        state.time_signature = std::stof(val.substr(0, pos)) / std::stof(val.substr(pos + 1));
    }
}

void TJAParser::handle_scroll(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    if (state.scroll_type != ScrollType::BMSCROLL) {
        std::string normalized = val;
        normalized.erase(std::remove(normalized.begin(), normalized.end(), ','), normalized.end());
        normalized.erase(std::remove(normalized.begin(), normalized.end(), ' '), normalized.end());

        size_t i_pos = normalized.find('i');
        if (i_pos != std::string::npos) {
            std::string without_i = normalized.substr(0, i_pos);
            try {
                size_t sep = without_i.rfind('+');
                if (sep == std::string::npos || sep == 0) {
                    sep = without_i.rfind('-', without_i.size() - 1);
                }
                if (sep != std::string::npos && sep > 0) {
                    state.scroll_x_modifier = std::stof(without_i.substr(0, sep));
                    state.scroll_y_modifier = std::stof(without_i.substr(sep));
                }
                else {
                    state.scroll_x_modifier = 0.0f;
                    state.scroll_y_modifier = std::stof(without_i.empty() ? "1" : without_i);
                }
            }
            catch (...) {
                state.scroll_x_modifier = 1.0f;
                state.scroll_y_modifier = 0.0f;
            }
        }
        else {
            try {
                state.scroll_x_modifier = std::stof(normalized);
                state.scroll_y_modifier = 0.0f;
            }
            catch (...) {
                state.scroll_x_modifier = 1.0f;
                state.scroll_y_modifier = 0.0f;
            }
        }
    }
    else {
        try {
            state.scroll_x_modifier = std::stof(val);
            state.scroll_y_modifier = 0.0f;
        }
        catch (...) {
            state.scroll_x_modifier = 1.0f;
            state.scroll_y_modifier = 0.0f;
        }
    }
}

void TJAParser::handle_bpmchange(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    float parsed_bpm = std::stof(val);
    if (state.scroll_type == ScrollType::BMSCROLL || state.scroll_type == ScrollType::HBSCROLL) {
        float bpmchange = parsed_bpm / state.bpmchange_last_bpm;
        state.bpmchange_last_bpm = parsed_bpm;
        TimelineObject* obj = new TimelineObject();
        obj->hit_ms = current_ms_;
        obj->bpmchange = bpmchange;
        state.curr_timeline->push_back(obj);
    }
    else {
        TimelineObject* obj = new TimelineObject();
        obj->hit_ms = current_ms_;
        obj->bpm = parsed_bpm;
        state.bpm = parsed_bpm;
        state.curr_timeline->push_back(obj);
    }
}

void TJAParser::handle_section(const std::string&, ParserState& state) {
    state.is_section_start = true;
}

void TJAParser::set_branch_params(std::vector<Note*>* bar_list, const std::string& branch_params,
    Note* section_bar, ParserState& state) {
    if (bar_list && !bar_list->empty()) {
        size_t segment_index = bar_list->size() >= 2 ? bar_list->size() - 2 : bar_list->size() - 1;
        if (section_bar && section_bar->hit_ms < current_ms_) {
            auto it = std::find(bar_list->begin(), bar_list->end(), section_bar);
            if (it != bar_list->end()) {
                segment_index = static_cast<size_t>(std::distance(bar_list->begin(), it));
            }
        }
        if (segment_index < bar_list->size()) {
            (*bar_list)[segment_index]->branch_params = branch_params;
        }
    }
    else if (bar_list) {
        Note* bar_line = new Note();
        bar_line->hit_ms = current_ms_;
        bar_line->type = NoteType::NONE;
        bar_line->display = false;
        bar_line->branch_params = branch_params;
        bar_list->push_back(bar_line);
    }
}

void TJAParser::handle_branchstart(const std::string& part, ParserState& state) {
    std::string params = part;
    trim(params);
    state.start_branch_ms = current_ms_;
    state.start_branch_bpm = state.bpm;
    state.start_branch_time_sig = state.time_signature;
    state.start_branch_x_scroll = state.scroll_x_modifier;
    state.start_branch_y_scroll = state.scroll_y_modifier;
    state.start_branch_barline = state.barline_display;
    state.branch_balloon_index = state.balloon_index;

    set_branch_params(state.curr_bar_list, params, state.section_bar, state);
    if (!branch_m.empty()) {
        set_branch_params(&branch_m.back().bars, params, state.section_bar, state);
    }
    if (!branch_e.empty()) {
        set_branch_params(&branch_e.back().bars, params, state.section_bar, state);
    }
    if (!branch_n.empty()) {
        set_branch_params(&branch_n.back().bars, params, state.section_bar, state);
    }
    state.section_bar = nullptr;
}

void TJAParser::handle_branchend(ParserState& state) {
    state.curr_note_list = &master_notes.play_notes;
    state.curr_draw_list = &master_notes.draw_notes;
    state.curr_bar_list = &master_notes.bars;
    state.curr_timeline = &master_notes.timeline;
}

void TJAParser::handle_lyric(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    TimelineObject* obj = new TimelineObject();
    obj->hit_ms = current_ms_;
    obj->lyric = val;
    state.curr_timeline->push_back(obj);
}

void TJAParser::handle_jposscroll(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    std::istringstream iss(val);
    float duration_ms;
    std::string distance_str;
    int direction;
    if (!(iss >> duration_ms >> distance_str >> direction)) return;

    duration_ms *= 1000.0f;
    float delta_x = 0.0f, delta_y = 0.0f;
    size_t i_pos = distance_str.find('i');
    if (i_pos != std::string::npos) {
        std::string without_i = distance_str.substr(0, i_pos);
        try {
            size_t sep = without_i.rfind('+');
            if (sep == std::string::npos || sep == 0) sep = without_i.rfind('-', without_i.size() - 1);
            if (sep != std::string::npos && sep > 0) {
                delta_x = std::stof(without_i.substr(0, sep));
                delta_y = std::stof(without_i.substr(sep));
            }
            else {
                delta_y = std::stof(without_i.empty() ? "1" : without_i);
            }
        }
        catch (...) {}
    }
    else {
        try { delta_x = std::stof(distance_str); }
        catch (...) {}
    }
    if (direction == 0) {
        delta_x = -delta_x;
        delta_y = -delta_y;
    }

    TimelineObject* jpos = new TimelineObject();
    jpos->load_ms = current_ms_;
    jpos->hit_ms = current_ms_ + duration_ms;
    jpos->judge_pos_x = state.judge_pos_x;
    jpos->judge_pos_y = state.judge_pos_y;
    jpos->delta_x = delta_x;
    jpos->delta_y = delta_y;
    state.curr_timeline->push_back(jpos);

    state.judge_pos_x += delta_x;
    state.judge_pos_y += delta_y;
}

void TJAParser::handle_nmscroll(ParserState& state) { state.scroll_type = ScrollType::NMSCROLL; }
void TJAParser::handle_bmscroll(ParserState& state) { state.scroll_type = ScrollType::BMSCROLL; }
void TJAParser::handle_hbscroll(ParserState& state) { state.scroll_type = ScrollType::HBSCROLL; }
void TJAParser::handle_barlineon(ParserState& state) { state.barline_display = true; }
void TJAParser::handle_barlineoff(ParserState& state) { state.barline_display = false; }

void TJAParser::handle_gogostart(ParserState& state) {
    TimelineObject* obj = new TimelineObject();
    obj->hit_ms = current_ms_;
    obj->gogo_time = true;
    state.curr_timeline->push_back(obj);
}

void TJAParser::handle_gogoend(ParserState& state) {
    TimelineObject* obj = new TimelineObject();
    obj->hit_ms = current_ms_;
    obj->gogo_time = false;
    state.curr_timeline->push_back(obj);
}

void TJAParser::handle_delay(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    float delay_ms = std::stof(val) * 1000.0f;
    if (state.scroll_type == ScrollType::BMSCROLL || state.scroll_type == ScrollType::HBSCROLL) {
        if (delay_ms > 0) state.delay_current += delay_ms;
    }
    else {
        current_ms_ += delay_ms;
    }
}

void TJAParser::handle_sudden(const std::string& part, ParserState& state) {
    std::string val = part;
    trim(val);
    std::istringstream iss(val);
    float appear_duration, moving_duration;
    if (iss >> appear_duration >> moving_duration) {
        state.sudden_appear = appear_duration * 1000.0f;
        state.sudden_moving = moving_duration * 1000.0f;
        if (state.sudden_appear == 0.0f) state.sudden_appear = std::numeric_limits<float>::infinity();
        if (state.sudden_moving == 0.0f) state.sudden_moving = std::numeric_limits<float>::infinity();
    }
}

void TJAParser::handle_m(ParserState& state) {
    branch_m.emplace_back();
    state.curr_note_list = &branch_m.back().play_notes;
    state.curr_draw_list = &branch_m.back().draw_notes;
    state.curr_bar_list = &branch_m.back().bars;
    state.curr_timeline = &branch_m.back().timeline;
    current_ms_ = state.start_branch_ms;
    state.bpm = state.start_branch_bpm;
    state.time_signature = state.start_branch_time_sig;
    state.scroll_x_modifier = state.start_branch_x_scroll;
    state.scroll_y_modifier = state.start_branch_y_scroll;
    state.barline_display = state.start_branch_barline;
    state.balloon_index = state.branch_balloon_index;
    state.is_branching = true;
}

void TJAParser::handle_e(ParserState& state) {
    branch_e.emplace_back();
    state.curr_note_list = &branch_e.back().play_notes;
    state.curr_draw_list = &branch_e.back().draw_notes;
    state.curr_bar_list = &branch_e.back().bars;
    state.curr_timeline = &branch_e.back().timeline;
    current_ms_ = state.start_branch_ms;
    state.bpm = state.start_branch_bpm;
    state.time_signature = state.start_branch_time_sig;
    state.scroll_x_modifier = state.start_branch_x_scroll;
    state.scroll_y_modifier = state.start_branch_y_scroll;
    state.barline_display = state.start_branch_barline;
    state.balloon_index = state.branch_balloon_index;
    state.is_branching = true;
}

void TJAParser::handle_n(ParserState& state) {
    branch_n.emplace_back();
    state.curr_note_list = &branch_n.back().play_notes;
    state.curr_draw_list = &branch_n.back().draw_notes;
    state.curr_bar_list = &branch_n.back().bars;
    state.curr_timeline = &branch_n.back().timeline;
    current_ms_ = state.start_branch_ms;
    state.bpm = state.start_branch_bpm;
    state.time_signature = state.start_branch_time_sig;
    state.scroll_x_modifier = state.start_branch_x_scroll;
    state.scroll_y_modifier = state.start_branch_y_scroll;
    state.barline_display = state.start_branch_barline;
    state.balloon_index = state.branch_balloon_index;
    state.is_branching = true;
}

void TJAParser::dispatch_command(const std::string& part, ParserState& state) {
    if (part == "#NMSCROLL") { handle_nmscroll(state); return; }
    if (part == "#BMSCROLL") { handle_bmscroll(state); return; }
    if (part == "#HBSCROLL") { handle_hbscroll(state); return; }
    if (part.rfind("#BRANCHEND", 0) == 0) { handle_branchend(state); return; }
    if (part == "#M") { handle_m(state); return; }
    if (part == "#E") { handle_e(state); return; }
    if (part == "#N") { handle_n(state); return; }
    if (part == "#BARLINEON") { handle_barlineon(state); return; }
    if (part == "#BARLINEOFF") { handle_barlineoff(state); return; }
    if (part == "#GOGOSTART") { handle_gogostart(state); return; }
    if (part == "#GOGOEND") { handle_gogoend(state); return; }

    struct Cmd { const char* prefix; void (TJAParser::* fn)(const std::string&, ParserState&); };
    static const Cmd withArg[] = {
        { "#BRANCHSTART", &TJAParser::handle_branchstart },
        { "#JPOSCROLL", &TJAParser::handle_jposscroll },
        { "#BPMCHANGE", &TJAParser::handle_bpmchange },
        { "#MEASURE", &TJAParser::handle_measure },
        { "#SECTION", &TJAParser::handle_section },
        { "#SCROLL", &TJAParser::handle_scroll },
        { "#DELAY", &TJAParser::handle_delay },
        { "#SUDDEN", &TJAParser::handle_sudden },
        { "#LYRIC", &TJAParser::handle_lyric },
    };
    for (const auto& cmd : withArg) {
        if (part.rfind(cmd.prefix, 0) == 0) {
            (this->*cmd.fn)(part.substr(std::strlen(cmd.prefix)), state);
            return;
        }
    }
}

void TJAParser::notes_to_position(int diff) {
    auto bars = data_to_notes(diff);
    if (bars.empty()) return;

    ParserState state;
    state.bpm = metadata.bpm > 0.0f ? metadata.bpm : 120.0f;
    state.bpmchange_last_bpm = state.bpm;

    auto course_it = metadata.course_data.find(diff);
    if (course_it != metadata.course_data.end()) {
        state.balloons = course_it->second.balloon;
    }

    state.curr_note_list = &master_notes.play_notes;
    state.curr_draw_list = &master_notes.draw_notes;
    state.curr_bar_list = &master_notes.bars;
    state.curr_timeline = &master_notes.timeline;

    TimelineObject* init_bpm = new TimelineObject();
    init_bpm->hit_ms = static_cast<float>(start_delay_ms);
    init_bpm->bpm = state.bpm;
    master_notes.timeline.push_back(init_bpm);

    current_ms_ = static_cast<float>(start_delay_ms);
    state.delay_last_note_ms = current_ms_;

    for (const auto& bar : bars) {
        int bar_length = 0;
        for (const auto& part : bar) {
            if (part.rfind('#', 0) != 0) {
                bar_length += static_cast<int>(part.size());
            }
        }
        state.barline_added = false;

        for (const auto& part : bar) {
            if (part.rfind('#', 0) == 0) {
                dispatch_command(part, state);
                continue;
            }
            if (!part.empty() && !std::isdigit(static_cast<unsigned char>(part[0]))) {
                continue;
            }

            float ms_per_measure = get_ms_per_measure(state.bpm, state.time_signature);
            Note* bar_line = add_bar(state);
            state.curr_bar_list->push_back(bar_line);
            state.barline_added = true;

            float increment = 0.0f;
            if (part.empty()) {
                current_ms_ += ms_per_measure;
            }
            else {
                increment = ms_per_measure / static_cast<float>(bar_length);
            }

            for (char ch : part) {
                std::string item(1, ch);
                if (ch == '0' || !std::isdigit(static_cast<unsigned char>(ch))) {
                    state.delay_last_note_ms = current_ms_;
                    current_ms_ += increment;
                    continue;
                }
                if (ch == '9' && state.prev_note &&
                    static_cast<int>(state.prev_note->type) == 9) {
                    state.delay_last_note_ms = current_ms_;
                    current_ms_ += increment;
                    continue;
                }
                if (state.delay_current != 0.0f) {
                    TimelineObject* delay_obj = new TimelineObject();
                    delay_obj->hit_ms = state.delay_last_note_ms;
                    delay_obj->delay = state.delay_current;
                    state.curr_timeline->push_back(delay_obj);
                    state.delay_current = 0.0f;
                }

                Note* note = add_note(item, state);
                current_ms_ += increment;
                state.curr_note_list->push_back(note);
                state.curr_draw_list->push_back(note);
                get_moji(*state.curr_note_list, ms_per_measure);
                state.index++;
                state.prev_note = note;
            }
        }
    }
}

std::wstring TJAParser::ReadTitle(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return path.stem().wstring();

    // UTF-8 BOM検出（EF BB BF）
    bool isUtf8 = false;
    {
        unsigned char bom[3] = {};
        f.read(reinterpret_cast<char*>(bom), 3);
        if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
            isUtf8 = true;  // BOM付きUTF-8
        }
        else {
            f.seekg(0);  // BOMなし → 先頭に戻す
        }
    }

    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // BOMなしUTF-8の簡易検出（0x80以上のバイトがあればUTF-8と仮定）
        if (!isUtf8) {
            for (unsigned char c : line) {
                if (c >= 0x80) { isUtf8 = true; break; }
            }
        }

        if (line.rfind("TITLE:", 0) == 0) {
            std::string raw = line.substr(6);
            while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t')) raw.pop_back();
#if defined(_WIN32)
            UINT codePage = isUtf8 ? CP_UTF8 : 932;
            int len = MultiByteToWideChar(codePage, 0, raw.c_str(), -1, nullptr, 0);
            if (len > 1) {
                std::wstring ws(len - 1, L'\0');
                MultiByteToWideChar(codePage, 0, raw.c_str(), -1, ws.data(), len);
                return ws;
            }
#endif
            return std::wstring(raw.begin(), raw.end());
        }
        if (line.rfind("#START", 0) == 0) break;
    }
    return path.stem().wstring();
}

SongInfo TJAParser::parse(int diff) {
    master_notes = NoteList();
    branch_m.clear();
    branch_e.clear();
    branch_n.clear();
    metadata = TJAMetadata();
    ex_data = TJAEXData();

    load_file_lines();
    get_metadata();
    notes_to_position(diff);

    SongInfo songInfo;
    songInfo.metadata = metadata;
    songInfo.ex_data = ex_data;
    songInfo.master_notes = std::move(master_notes);
    songInfo.branch_m = std::move(branch_m);
    songInfo.branch_e = std::move(branch_e);
    songInfo.branch_n = std::move(branch_n);
    return songInfo;
}