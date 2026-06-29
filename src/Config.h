#pragma once
#include <string>

class Config {
public:
    bool autoPlay = false;

    void Load(const std::string& path = "config.ini");
    void Save(const std::string& path = "config.ini") const;
};