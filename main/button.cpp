#include "button.hpp"

#include <string.h>

Button::Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, uint8_t fontsize, bool(*function)(), int xOffsetExtra):
    Sprite(x1, y1, x2, y2, sprite, fgColor, bgColor, fontsize),
    function_(function),
    xOffsetExtra_(xOffsetExtra)
{
    SetText(text);
}

bool Button::HandleEvent(uint16_t x, uint16_t y) {
    return InsideButton(x, y) ? (function_ ? function_() : true) : false;
}

bool Button::InsideButton(uint16_t x, uint16_t y) const {
    return x1_ <= x && x2_ > x && y1_ <= y && y2_ > y;
}

void Button::Render() {
    sprite_->fillSprite(bgColor_);
    sprite_->setCursor(buttonOffset_[0] + xOffsetExtra_, buttonOffset_[1]);
    for(int i=0; i<textBuffer_.textPieceCount; i++){
        sprite_->setTextColor(textBuffer_.pieces[i].fgColor, textBuffer_.pieces[i].bgColor);
        sprite_->print(textBuffer_.pieces[i].text);
    }
    sprite_->pushSprite(x1_, y1_);
}
