#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "lilygo.hpp"

class Sprite{
public:
    Sprite(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, uint16_t fgColor, uint16_t bgColor, uint8_t fontsize);
    ~Sprite();

    virtual void Render(const char* text);
    virtual void Render();
    void SetText(const char* text);

protected:
    unsigned x1_, y1_, x2_, y2_;
    TFT_eSprite* sprite_;
    uint16_t bgColor_;
    uint16_t fgColor_;
    uint8_t fontsize_;
    inline static const int textOffset_[2] = {3, 3};
    char text_[32];
};

#endif // __SPRITE_H__