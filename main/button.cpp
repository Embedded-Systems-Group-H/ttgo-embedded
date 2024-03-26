#include "button.hpp"

#include <string.h>

Button::Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, bool(*function)()):
    Sprite(x1, y1, x2, y2, sprite, fgColor, bgColor, 2),
    function_(function)
{
    SetText(text);
}

bool Button::HandleEvent(uint16_t x, uint16_t y) {
    return InsideButton(x, y) ? false : function_ ? function_() : true;
}

bool Button::InsideButton(uint16_t x, uint16_t y) const {
    return x1_ <= x && x2_ > x && y1_ <= y && y2_ > y;
}
