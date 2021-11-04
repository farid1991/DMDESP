#ifndef Bitmap_h
#define Bitmap_h

#include <Arduino.h>
#include <inttypes.h>
#include <sys/pgmspace.h>

// Six byte header at beginning of FontCreator font structure, stored in PROGMEM
struct FontHeader
{
    uint16_t size;
    uint8_t fixedWidth;
    uint8_t height;
    uint8_t firstChar;
    uint8_t charCount;
};

enum Color
{
    Black = 0,
    White = 1,
    NoFill = 2,
};

class DMDESP;
class String;

class Bitmap
{
public:
    Bitmap(int width, int height);
    ~Bitmap();

    bool isValid() const { return frame_buffer != 0; }

    // typedef PGM_VOID_P ProgMem;

    int getWidth() const { return scr_width; }
    int getHeight() const { return scr_height; }
    int getStride() const { return scr_stride; }
    int bitsPerPixel() const { return 1; }

    uint8_t *getFrameBuffer() { return frame_buffer; }
    const uint8_t *getFrameBuffer() const { return frame_buffer; }

    void clearScreen();
    void fillScreen();

    bool getPixel(int x, int y) const;
    void setPixel(int x, int y, uint8_t color);

    void drawLine(int x1, int y1, int x2, int y2, uint8_t color = White);
    void drawRect(int x1, int y1, int x2, int y2, uint8_t borderColor = White, uint8_t fillColor = NoFill);
    void drawFilledRect(int x1, int y1, int x2, int y2, uint8_t color = White);
    void drawCircle(int centerX, int centerY, int radius, uint8_t borderColor = White, uint8_t fillColor = NoFill);
    void drawFilledCircle(int centerX, int centerY, int radius, uint8_t color = White);

    void drawBitmap(int x, int y, const Bitmap &bitmap, uint8_t color = White);
    void drawBitmap(int x, int y, PGM_VOID_P bitmap, uint8_t color = White);
    void drawInvertedBitmap(int x, int y, const Bitmap &bitmap);
    void drawInvertedBitmap(int x, int y, PGM_VOID_P bitmap);

    uint8_t *getFont() const { return _font; }
    void setFont(const uint8_t *font);

    uint8_t getTextColor() const { return textColor; }
    void setTextColor(uint8_t color) { textColor = color; }

    int drawChar(int x, int y, char ch);
    void drawString(int x, int y, const char *str, int len = -1);
    void drawString(int x, int y, const String &str, int start = 0, int len = -1);
    void drawString_P(int x, int y, PGM_P str, int len = -1);
    void drawString_P(int x, int y, const __FlashStringHelper *str, int len = -1);

    int getCharWidth(char ch) const;
    int getTextWidth(const char *str, int len = -1) const;
    int getTextWidth(const String &str, int start = 0, int len = -1) const;
    int getTextHeight() const;

    void copy(int x, int y, int width, int height, Bitmap *dest, int destX, int destY);
    void fill(int x, int y, int width, int height, uint8_t color);
    void fill(int x, int y, int width, int height, PGM_VOID_P pattern, uint8_t color = White);

    void scroll(int dx, int dy, uint8_t fillColor = Black);
    void scroll(int x, int y, int width, int height, int dx, int dy, uint8_t fillColor = Black);

    void invert(int x, int y, int width, int height);

private:
    // Disable copy constructor and operator=().
    Bitmap(const Bitmap &) {}
    Bitmap &operator=(const Bitmap &) { return *this; }

    int scr_width;
    int scr_height;
    int scr_stride;
    uint8_t *frame_buffer;
    uint8_t *_font;
    uint8_t textColor;

    friend class DMDESP;

    void blit(int x1, int y1, int x2, int y2, int x3, int y3);
    void drawCirclePoints(int centerX, int centerY, int radius, int x, int y, uint8_t borderColor, uint8_t fillColor);
};

#endif
