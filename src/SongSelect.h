#pragma once
#include <DxLib.h>
#include <string>
#include "SongInfo.h"

class SongSelect {
public:
    SongSelect(SongInfo songInfo = SongInfo());
    bool Update();
    void Draw();

private:
    SongInfo songInfo;
    int selectedIndex = 0;
};