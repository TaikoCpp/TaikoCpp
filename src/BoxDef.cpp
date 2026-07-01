#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif
#include "BoxDef.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cwctype>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

std::wstring DecodeToWide(const std::string& text, int codepage) {
    if (text.empty()) return L"";
#if defined(_WIN32)
    int len = MultiByteToWideChar(codepage, 0, text.c_str(), -1, nullptr, 0);
    if (len <= 1) return std::wstring(text.begin(), text.end());
    std::wstring ws(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(codepage, 0, text.c_str(), -1, ws.data(), len);
    return ws;
#else
    (void)codepage;
    return std::wstring(text.begin(), text.end());
#endif
}

std::string ReadFileBytes(const fs::path& path) {
    // platform-safe open for paths that may contain non-ASCII characters
    std::ifstream f;
#if defined(_WIN32)
    f.open(path.wstring(), std::ios::binary);
#else
    f.open(path.string(), std::ios::binary);
#endif
    if (!f.is_open()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::wstring BytesToWide(const std::string& bytes) {
    if (bytes.empty()) return L"";

    // UTF-8 BOM
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        return DecodeToWide(bytes.substr(3), CP_UTF8);
    }

    // UTF-16 LE BOM
    if (bytes.size() >= 2 &&
        static_cast<unsigned char>(bytes[0]) == 0xFF &&
        static_cast<unsigned char>(bytes[1]) == 0xFE) {
#if defined(_WIN32)
        const wchar_t* wdata = reinterpret_cast<const wchar_t*>(bytes.data() + 2);
        size_t wlen = (bytes.size() - 2) / sizeof(wchar_t);
        return std::wstring(wdata, wlen);
#else
        return std::wstring(bytes.begin(), bytes.end());
#endif
    }

    // UTF-16 BE BOM
    if (bytes.size() >= 2 &&
        static_cast<unsigned char>(bytes[0]) == 0xFE &&
        static_cast<unsigned char>(bytes[1]) == 0xFF) {
        std::wstring ws;
        for (size_t i = 2; i + 1 < bytes.size(); i += 2) {
            wchar_t ch = static_cast<wchar_t>(
                (static_cast<unsigned char>(bytes[i]) << 8) |
                static_cast<unsigned char>(bytes[i + 1]));
            ws.push_back(ch);
        }
        return ws;
    }

#if defined(_WIN32)
    return DecodeToWide(bytes, 932);
#else
    return DecodeToWide(bytes, CP_UTF8);
#endif
}

std::wstring WideTrim(std::wstring s) {
    auto notSpace = [](wchar_t c) { return !std::iswspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

bool ParseHexColor(const std::wstring& value, int& r, int& g, int& b) {
    std::wstring hex = value;
    if (!hex.empty() && hex[0] == L'#') hex = hex.substr(1);
    if (hex.size() != 6) return false;
    try {
        r = std::stoi(hex.substr(0, 2), nullptr, 16);
        g = std::stoi(hex.substr(2, 2), nullptr, 16);
        b = std::stoi(hex.substr(4, 2), nullptr, 16);
        return true;
    }
    catch (...) {
        return false;
    }
}

std::wstring ExtractValue(const std::wstring& line) {
    size_t pos = line.find(L':');
    if (pos == std::wstring::npos) return L"";
    return WideTrim(line.substr(pos + 1));
}

void ApplyGenreDefaults(const std::wstring& genre, BoxDefInfo& info) {
    if (info.hasBackColor) return;

    std::wstring g = genre;
    for (auto& c : g) c = static_cast<wchar_t>(std::towlower(c));

    if (g.find(L"anime") != std::wstring::npos || g.find(L"\u30a2\u30cb\u30e1") != std::wstring::npos) {
        info.backColorR = 255; info.backColorG = 140; info.backColorB = 180;
    }
    else if (g.find(L"vocaloid") != std::wstring::npos || g.find(L"\u30dc\u30fc\u30ab\u30ed\u30a4\u30c9") != std::wstring::npos) {
        info.backColorR = 80; info.backColorG = 180; info.backColorB = 220;
    }
    else if (g.find(L"j-pop") != std::wstring::npos || g.find(L"jpop") != std::wstring::npos) {
        info.backColorR = 255; info.backColorG = 152; info.backColorB = 0;
    }
    else if (g.find(L"game") != std::wstring::npos || g.find(L"\u30b2\u30fc\u30e0") != std::wstring::npos) {
        info.backColorR = 120; info.backColorG = 100; info.backColorB = 200;
    }
    else if (g.find(L"classic") != std::wstring::npos || g.find(L"\u30af\u30e9\u30b7\u30c3\u30af") != std::wstring::npos) {
        info.backColorR = 180; info.backColorG = 150; info.backColorB = 100;
    }
    else if (g.find(L"variety") != std::wstring::npos || g.find(L"\u30d0\u30e9\u30a8\u30c6\u30a3") != std::wstring::npos) {
        info.backColorR = 220; info.backColorG = 80; info.backColorB = 80;
    }
    else {
        info.backColorR = 100; info.backColorG = 130; info.backColorB = 180;
    }
    info.hasBackColor = true;
}

} // namespace

bool BoxDefParser::Exists(const fs::path& dir) {
    try {
        // まず標準パスでチェック
        fs::path p = dir / "box.def";
        if (fs::exists(p) && fs::is_regular_file(p)) return true;

        // ディレクトリ自体が存在するか確認
        if (!fs::exists(dir) || !fs::is_directory(dir)) return false;

        // フォルダ内のファイル名を列挙して case-insensitive に "box.def" を探す
#if defined(_WIN32)
        for (const auto& e : fs::directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            std::wstring name = e.path().filename().wstring();
            std::wstring lower;
            lower.reserve(name.size());
            for (wchar_t c : name) lower.push_back(std::towlower(c));
            if (lower == L"box.def") return true;
        }
#else
        for (const auto& e : fs::directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            std::string name = e.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name == "box.def") return true;
        }
#endif
    }
    catch (...) {
        // 例外発生時は false を返す（ファイルアクセス権などで落ちるのを防ぐ）
    }
    return false;
}

BoxDefInfo BoxDefParser::Parse(const fs::path& dir) {
    BoxDefInfo info;
    info.title = dir.filename().wstring();

    fs::path boxPath = dir / "box.def";
    if (!fs::exists(boxPath)) return info;

    std::wstring content = BytesToWide(ReadFileBytes(boxPath));
    std::wstring titleJa;
    std::wstring titleDefault;

    size_t start = 0;
    while (start < content.size()) {
        size_t end = content.find(L'\n', start);
        if (end == std::wstring::npos) end = content.size();
        std::wstring line = content.substr(start, end - start);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        start = end + 1;

        line = WideTrim(line);
        if (line.empty() || line[0] != L'#') continue;

        if (line.rfind(L"#TITLE:", 0) == 0) {
            titleDefault = ExtractValue(line);
        }
        else if (line.rfind(L"#TITLEJA:", 0) == 0) {
            titleJa = ExtractValue(line);
        }
        else if (line.rfind(L"#GENRE:", 0) == 0) {
            info.genre = ExtractValue(line);
        }
        else if (line.rfind(L"#BACKCOLOR:", 0) == 0) {
            if (ParseHexColor(ExtractValue(line), info.backColorR, info.backColorG, info.backColorB)) {
                info.hasBackColor = true;
            }
        }
        else if (line.rfind(L"#FORECOLOR:", 0) == 0 ||
                 line.rfind(L"#FRONTCOLOR:", 0) == 0) {
            if (ParseHexColor(ExtractValue(line), info.frontColorR, info.frontColorG, info.frontColorB)) {
                info.hasFrontColor = true;
            }
        }
        else if (line.rfind(L"#BOXCOLOR:", 0) == 0) {
            if (!info.hasBackColor &&
                ParseHexColor(ExtractValue(line), info.backColorR, info.backColorG, info.backColorB)) {
                info.hasBackColor = true;
            }
        }
        else if (line.rfind(L"#BGCOLOR:", 0) == 0) {
            if (!info.hasBackColor &&
                ParseHexColor(ExtractValue(line), info.backColorR, info.backColorG, info.backColorB)) {
                info.hasBackColor = true;
            }
        }
    }

    if (!titleJa.empty()) info.title = titleJa;
    else if (!titleDefault.empty()) info.title = titleDefault;
    else if (!info.genre.empty()) info.title = info.genre;
    else info.title = dir.filename().wstring();

    ApplyGenreDefaults(info.genre, info);
    return info;
}

std::wstring BoxDefParser::GetFolderTitle(const fs::path& dir) {
    return Parse(dir).title;
}
