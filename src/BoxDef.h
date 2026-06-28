#pragma once
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct BoxDefInfo {
    std::wstring title;
    std::wstring genre;
    int backColorR = -1;
    int backColorG = -1;
    int backColorB = -1;
    int frontColorR = -1;
    int frontColorG = -1;
    int frontColorB = -1;
    bool hasBackColor = false;
    bool hasFrontColor = false;
};

class BoxDefParser {
public:
    static bool Exists(const fs::path& dir);
    static BoxDefInfo Parse(const fs::path& dir);
    static std::wstring GetFolderTitle(const fs::path& dir);
};
