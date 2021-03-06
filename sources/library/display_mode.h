#pragma once
#include "string_stack.h"

enum display_mode_t
{
    DISPLAYMODE_FULL,
    DISPLAYMODE_SPEEDOMETER,
    DISPLAYMODE_ENTITYREPORT,
    DISPLAYMODE_MEASUREMENT,
    DISPLAYMODE_ANGLETRACKING
};

class IDisplayMode 
{
public:
    virtual ~IDisplayMode() {};
    virtual void Render2D(int scrWidth, int scrHeight) = 0;
    virtual void Render3D() = 0;
    virtual bool KeyInput(int isKeyDown, int keyNum, const char *bindName) = 0;
};
