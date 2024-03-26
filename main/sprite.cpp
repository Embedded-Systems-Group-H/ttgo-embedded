#include "sprite.hpp"

#include <string.h>

Sprite::Sprite(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, uint16_t fgColor, uint16_t bgColor, uint8_t fontsize) :
    x1_(x1), y1_(y1), x2_(x2), y2_(y2),
    sprite_(sprite),
    fgColor_(fgColor),
    bgColor_(bgColor),
    fontsize_(fontsize)
{
    sprite_->createSprite(x2_ - x1_, y2_ - y1_);
    sprite_->setTextFont(fontsize_);
    sprite_->setTextColor(fgColor_, bgColor_);
    memset(text_, 0, sizeof text_);
}

Sprite::~Sprite() {
}

void Sprite::Render(const char* text) {
    sprite_->fillSprite(bgColor_);
    sprite_->setCursor(textOffset_[0], textOffset_[1]);
    sprite_->print(text);
    sprite_->pushSprite(x1_, y1_);
}

void Sprite::Render() {
    sprite_->fillSprite(bgColor_);
    if(strlen(text_)){
        sprite_->setCursor(textOffset_[0], textOffset_[1]);
        sprite_->print(text_);
    }
    sprite_->pushSprite(x1_, y1_);
}

void Sprite::SetText(const char* text) {
    strncpy(text_, text, (sizeof text_));
    text_[sizeof text_ - 1] = 0;
}
