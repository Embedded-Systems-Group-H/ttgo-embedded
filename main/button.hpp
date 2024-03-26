#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "lilygo.hpp"
#include "sprite.hpp"

class Button : public Sprite{
public:
    Button(unsigned x1, unsigned y1, unsigned x2, unsigned y2, TFT_eSprite* sprite, const char* text, uint16_t fgColor, uint16_t bgColor, bool(*function)() = nullptr);

    virtual void Render() override;
    bool HandleEvent(uint16_t x, uint16_t y);
    bool InsideButton(uint16_t x, uint16_t y) const;

protected:
    bool(*function_)();
    char text_[32];
};

#endif // __BUTTON_H__