#include "button.hpp"
#include <string.h>

Button::Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, void(*function)()):
    x1_(x1), y1_(y1), x2_(x2), y2_(y2),
    sprite_(sprite),
    fgColor_(fgColor),
    bgColor_(bgColor),
    function_(function)
{
    strcpy_s(text_, sizeof text_, text);
}


void Button::Render() {
    sprite_->setTextFont(2);
    sprite_->fillSprite(bgColor_);
    sprite_->setTextColor(textColor_, bgColor_);
    sprite_->setCursor(offset_[0], offset_[1]);
    sprite_->print(text_);
    sprite_->pushSprite(x1_, y1_);    
}

bool Button::HandleEvent(uint16_t mouseX, uint16_t mouseY) {
    return InsideButton(x, y) ? false : function_ ? function_() : true;
}

bool Button::InsideButton(uint16_t x, uint16_t y) const {
    return b->x1 <= x && b->x2 > x && b->y1 <= y && b->y2 > y;
}
