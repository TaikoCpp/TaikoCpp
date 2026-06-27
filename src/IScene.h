#pragma once

class IScene {
public:
    virtual ~IScene() = default;
    virtual bool Update() = 0;
    virtual void Draw() = 0;
};