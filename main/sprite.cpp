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
    // sprite_->setTextColor(fgColor_, bgColor_);

    textBuffer_.textPieceCount = 0;
    AddTextPiece("\0", fgColor, bgColor);
}

Sprite::~Sprite() {
}

void Sprite::Render() {
    sprite_->fillSprite(bgColor_);
    sprite_->setCursor(textOffset_[0], textOffset_[1]);
    for(int i=0; i<textBuffer_.textPieceCount; i++){
        sprite_->setTextColor(textBuffer_.pieces[i].fgColor, textBuffer_.pieces[i].bgColor);
        sprite_->print(textBuffer_.pieces[i].text);
    }
    sprite_->pushSprite(x1_, y1_);
}

void Sprite::SetText(const char* text, int textPieceIndex) {
    auto dst = textBuffer_.pieces[textPieceIndex].text;
    strncpy(dst, text, 32);
    dst[31] = 0;
}

void Sprite::AddTextPiece(const char* text, uint16_t fgColor, uint16_t bgColor) {
    TextPiece piece;
    piece.bgColor = bgColor;
    piece.fgColor = fgColor;
    textBuffer_.pieces[textBuffer_.textPieceCount] = piece;
    SetText(text, textBuffer_.textPieceCount ++);
}
