#include "button.hpp"

#include <string.h>

Button::Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, bool(*function)()):
    x1_(x1), y1_(y1), x2_(x2), y2_(y2),
    sprite_(sprite),
    fgColor_(fgColor),
    bgColor_(bgColor),
    function_(function)
{
    strncpy(text_, text, (sizeof text) - 1);
}


void Button::Render() {
    sprite_->setTextFont(2);
    sprite_->fillSprite(bgColor_);
    sprite_->setTextColor(fgColor_, bgColor_);
    sprite_->setCursor(offset_[0], offset_[1]);
    sprite_->print(text_);
    sprite_->pushSprite(x1_, y1_);    
}

bool Button::HandleEvent(uint16_t x, uint16_t y) {
    return InsideButton(x, y) ? false : function_ ? function_() : true;
}

bool Button::InsideButton(uint16_t x, uint16_t y) const {
    return x1_ <= x && x2_ > x && y1_ <= y && y2_ > y;
}
