#pragma once
#include <string>

class Config {
public:
    bool autoPlay = false;
    bool vsync = true;   // true=VSync ON, false=OFF(120fps+)

    void Load(const std::string& path = "config.ini");
    void Save(const std::string& path = "config.ini") const;
};