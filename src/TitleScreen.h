#pragma once
#include "DxLib.h"
#include "IScene.h"

class TitleScreen : public IScene {
public:
    TitleScreen();
    ~TitleScreen();
    bool Update() override;
    void Draw() override;
private:
    int fontTitle = -1;
    int fontSub = -1;
};