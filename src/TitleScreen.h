#pragma once
#include "DxLib.h"
#include "IScene.h"

class TitleScreen : public IScene {
public:
    bool Update() override;
    void Draw() override;
};