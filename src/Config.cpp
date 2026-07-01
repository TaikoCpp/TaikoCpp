#include "Config.h"
#include <fstream>
#include <string>

void Config::Load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        Save(path);
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '[' || line[0] == ';') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "AutoPlay") autoPlay = (val == "1");
        if (key == "VSync")    vsync = (val != "0");
    }
}

void Config::Save(const std::string& path) const {
    std::ofstream ofs(path);
    ofs << "[Game]\n";
    ofs << "AutoPlay=" << (autoPlay ? "1" : "0") << "\n";
    ofs << "VSync=" << (vsync ? "1" : "0") << "\n";
}