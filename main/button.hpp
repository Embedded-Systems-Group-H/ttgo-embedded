#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "lilygo.hpp"

class Button{
public:
    Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, bool(*function)() = nullptr);

    void Render();
    bool HandleEvent(uint16_t mouseX, uint16_t mouseY);
    bool InsideButton(uint16_t x, uint16_t y) const;

private:
    unsigned x1_, y1_, x2_, y2_;
    TFT_eSprite* sprite_;
    uint16_t bgColor_;
    uint16_t fgColor_;
    bool(*function_)();
    char text_[32];

    inline static const int offset_[2] = {3, 3};
};

#endif // __BUTTON_H__