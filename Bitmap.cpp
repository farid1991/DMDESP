#include <stdlib.h>
#include <string.h>
#include <WString.h>

#include "Bitmap.h"

Bitmap::Bitmap(int width, int height)
    : scr_width(width), scr_height(height), scr_stride((width + 7) / 8), frame_buffer(0), _font(0), textColor(White)
{
    // Allocate memory for the framebuffer and clear it (1 = pixel off).
    unsigned int size = scr_stride * scr_height;
    frame_buffer = (uint8_t *)malloc(size);
    if (frame_buffer)
        memset(frame_buffer, 0xFF, size);
}

Bitmap::~Bitmap()
{
    if (frame_buffer)
        free(frame_buffer);
}

void Bitmap::clearScreen()
{
    unsigned int size = scr_stride * scr_height;
    memset(frame_buffer, 0xFF, size);
}

void Bitmap::fillScreen()
{
    unsigned int size = scr_stride * scr_height;
    memset(frame_buffer, 0x00, size);
}

bool Bitmap::getPixel(int x, int y) const
{
    if (((unsigned int)x) >= ((unsigned int)scr_width) ||
        ((unsigned int)y) >= ((unsigned int)scr_height))
        return false;
    uint8_t *ptr = frame_buffer + y * scr_stride + (x >> 3);
    if (*ptr & ((uint8_t)0x80) >> (x & 0x07))
        return false;
    else
        return true;
}

void Bitmap::setPixel(int x, int y, uint8_t color)
{
    if (((unsigned int)x) >= ((unsigned int)scr_width) ||
        ((unsigned int)y) >= ((unsigned int)scr_height))
        return; // Pixel is off-screen.
    uint8_t *ptr = frame_buffer + y * scr_stride + (x >> 3);
    if (color)
        *ptr &= ~(((uint8_t)0x80) >> (x & 0x07));
    else
        *ptr |= (((uint8_t)0x80) >> (x & 0x07));
}

void Bitmap::drawLine(int x1, int y1, int x2, int y2, uint8_t color)
{
    // Midpoint line scan-conversion algorithm from "Computer Graphics:
    // Principles and Practice", Second Edition, Foley, van Dam, et al.
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xstep, ystep;
    int d, incrE, incrNE;
    if (dx < 0)
    {
        xstep = -1;
        dx = -dx;
    }
    else
    {
        xstep = 1;
    }
    if (dy < 0)
    {
        ystep = -1;
        dy = -dy;
    }
    else
    {
        ystep = 1;
    }
    if (dx >= dy)
    {
        d = 2 * dy - dx;
        incrE = 2 * dy;
        incrNE = 2 * (dy - dx);
        setPixel(x1, y1, color);
        while (x1 != x2)
        {
            if (d <= 0)
            {
                d += incrE;
            }
            else
            {
                d += incrNE;
                y1 += ystep;
            }
            x1 += xstep;
            setPixel(x1, y1, color);
        }
    }
    else
    {
        d = 2 * dx - dy;
        incrE = 2 * dx;
        incrNE = 2 * (dx - dy);
        setPixel(x1, y1, color);
        while (y1 != y2)
        {
            if (d <= 0)
            {
                d += incrE;
            }
            else
            {
                d += incrNE;
                x1 += xstep;
            }
            y1 += ystep;
            setPixel(x1, y1, color);
        }
    }
}

void Bitmap::drawRect(int x1, int y1, int x2, int y2, uint8_t borderColor, uint8_t fillColor)
{
    int temp;
    if (x1 > x2)
    {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2)
    {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    if (fillColor == borderColor)
    {
        fill(x1, y1, x2 - x1 + 1, y2 - y1 + 1, fillColor);
    }
    else
    {
        drawLine(x1, y1, x2, y1, borderColor);
        if (y1 < y2)
            drawLine(x2, y1 + 1, x2, y2, borderColor);
        if (x1 < x2)
            drawLine(x2 - 1, y2, x1, y2, borderColor);
        if (y1 < (y2 - 1))
            drawLine(x1, y2 - 1, x1, y1 + 1, borderColor);
        if (fillColor != NoFill)
            fill(x1 + 1, y1 + 1, x2 - x1 - 1, y2 - y1 - 1, fillColor);
    }
}

void Bitmap::drawFilledRect(int x1, int y1, int x2, int y2, uint8_t color)
{
    drawRect(x1, y1, x2, y2, color, color);
}

void Bitmap::drawCircle(int centerX, int centerY, int radius, uint8_t borderColor, uint8_t fillColor)
{
    // Midpoint circle scan-conversion algorithm using second-order
    // differences from "Computer Graphics: Principles and Practice",
    // Second Edition, Foley, van Dam, et al.
    if (radius < 0)
        radius = -radius;
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    int deltaE = 3;
    int deltaSE = 5 - 2 * radius;
    drawCirclePoints(centerX, centerY, radius, x, y, borderColor, fillColor);
    while (y > x)
    {
        if (d < 0)
        {
            d += deltaE;
            deltaE += 2;
            deltaSE += 2;
        }
        else
        {
            d += deltaSE;
            deltaE += 2;
            deltaSE += 4;
            --y;
        }
        ++x;
        drawCirclePoints(centerX, centerY, radius, x, y, borderColor, fillColor);
    }
}

void Bitmap::drawFilledCircle(int centerX, int centerY, int radius, uint8_t color)
{
    drawCircle(centerX, centerY, radius, color, color);
}

void Bitmap::drawBitmap(int x, int y, const Bitmap &bitmap, uint8_t color)
{
    int bitmap_w = bitmap.getWidth();
    int bitmap_s = bitmap.getStride();
    int bitmap_h = bitmap.getHeight();
    uint8_t invColor = !color;
    for (uint8_t by = 0; by < bitmap_h; ++by)
    {
        const uint8_t *line = bitmap.getFrameBuffer() + by * bitmap_s;
        uint8_t mask = 0x80;
        uint8_t value = *line++;
        for (uint8_t bx = 0; bx < bitmap_w; ++bx)
        {
            if (value & mask)
                setPixel(x + bx, y + by, invColor);
            else
                setPixel(x + bx, y + by, color);
            mask >>= 1;
            if (!mask)
            {
                mask = 0x80;
                value = *line++;
            }
        }
    }
}

void Bitmap::drawBitmap(int x, int y, PGM_VOID_P bitmap, uint8_t color)
{
    uint8_t bitmap_w = pgm_read_byte(bitmap);
    uint8_t bitmap_s = (bitmap_w + 7) >> 3;
    uint8_t bitmap_h = pgm_read_byte(bitmap + 1);
    uint8_t invColor = !color;
    for (uint8_t by = 0; by < bitmap_h; ++by)
    {
        const uint8_t *line = ((const uint8_t *)bitmap) + 2 + by * bitmap_s;
        uint8_t mask = 0x80;
        uint8_t value = pgm_read_byte(line);
        for (uint8_t bx = 0; bx < bitmap_w; ++bx)
        {
            if (value & mask)
                setPixel(x + bx, y + by, color);
            else
                setPixel(x + bx, y + by, invColor);
            mask >>= 1;
            if (!mask)
            {
                mask = 0x80;
                ++line;
                value = pgm_read_byte(line);
            }
        }
    }
}

void Bitmap::drawInvertedBitmap(int x, int y, const Bitmap &bitmap)
{
    drawBitmap(x, y, bitmap, Black);
}

void Bitmap::drawInvertedBitmap(int x, int y, PGM_VOID_P bitmap)
{
    drawBitmap(x, y, bitmap, Black);
}

void Bitmap::setFont(const uint8_t *font)
{
    _font = (uint8_t *)font;
}

#define Font_IsFixed(font) (pgm_read_byte((font)) == 0 && \
                            pgm_read_byte((font) + 1) == 0)
#define Font_getWidth(font) (pgm_read_byte((font) + 2))
#define Font_getHeight(font) (pgm_read_byte((font) + 3))
#define Font_getFirstChar(font) (pgm_read_byte((font) + 4))
#define Font_getCharCount(font) (pgm_read_byte((font) + 5))

void Bitmap::drawString(int x, int y, const char *str, int len)
{
    if (!_font)
        return;
    uint8_t font_height = Font_getHeight(_font);
    if (len < 0)
        len = strlen(str);
    while (len-- > 0)
    {
        x += drawChar(x, y, *str++);
        //if (len > 0) {
        fill(x, y, 1, font_height, !textColor);
        ++x;
        //}
        if (x >= scr_width)
            break;
    }
}

void Bitmap::drawString(int x, int y, const String &str, int start, int len)
{
    if (!_font)
        return;
    uint8_t font_height = Font_getHeight(_font);
    if (len < 0)
        len = str.length() - start;
    while (len-- > 0)
    {
        x += drawChar(x, y, str[start++]);
        if (len > 0)
        {
            fill(x, y, 1, font_height, !textColor);
            ++x;
        }
        if (x >= scr_width)
            break;
    }
}

void Bitmap::drawString_P(int x, int y, PGM_P str, int len)
{
    // if (!str) return;
    if (len < 0)
        len = strlen_P((PGM_P)str);
    char buff[64];
    memcpy_P(buff, str, len);
    drawString(x, y, buff, len);
}

void Bitmap::drawString_P(int x, int y, const __FlashStringHelper *str, int len)
{
    // if (!str) return;
    if (len < 0)
        len = strlen_P((PGM_P)str);
    char buff[64];
    memcpy_P(buff, str, len);
    drawString(x, y, buff, len);
}

int Bitmap::drawChar(int x, int y, char ch)
{
    uint8_t font_height = Font_getHeight(_font);
    if (ch == ' ')
    {
        // Font may not have space, or it is zero-width.  Calculate
        // the real size and fill the space.
        int spaceWidth = getCharWidth('n');
        fill(x, y, spaceWidth, font_height, !textColor);
        return spaceWidth;
    }
    uint8_t first_char = Font_getFirstChar(_font);
    uint8_t char_count = Font_getCharCount(_font);
    uint8_t char_index = (uint8_t)ch;
    if (char_index < first_char || char_index >= (first_char + char_count))
        return 0;
    char_index -= first_char;
    uint8_t heightBytes = (font_height + 7) >> 3;

    uint8_t char_width;
    const uint8_t *image;
    if (Font_IsFixed(_font))
    {
        // Fixed-width font.
        char_width = Font_getWidth(_font);
        image = ((const uint8_t *)_font) + 6 + char_index * heightBytes * char_width;
    }
    else
    {
        // Variable-width font.
        char_width = pgm_read_byte(_font + 6 + char_index);
        image = ((const uint8_t *)_font) + 6 + char_count;
        for (uint8_t temp = 0; temp < char_index; ++temp)
        {
            // Scan through all previous characters to find the starting
            // location for this one.
            image += pgm_read_byte(_font + 6 + temp) * heightBytes;
        }
    }
    if ((x + char_width) <= 0 || (y + font_height) <= 0)
        return char_width; // Character is off the top or left of the screen.
    uint8_t invColor = !textColor;
    for (uint8_t cx = 0; cx < char_width; ++cx)
    {
        for (uint8_t cy = 0; cy < heightBytes; ++cy)
        {
            uint8_t value = pgm_read_byte(image + cy * char_width + cx);
            int posn;
            if (heightBytes > 1 && cy == (heightBytes - 1))
                posn = font_height - 8;
            else
                posn = cy * 8;
            for (uint8_t bit = 0; bit < 8; ++bit)
            {
                if ((posn + bit) >= (cy * 8) && (posn + bit) <= font_height)
                {
                    if (value & 0x01)
                        setPixel(x + cx, y + posn + bit, textColor);
                    else
                        setPixel(x + cx, y + posn + bit, invColor);
                }
                value >>= 1;
            }
        }
    }
    return char_width;
}

int Bitmap::getCharWidth(char letter) const
{
    uint8_t index = (uint8_t)letter;
    if (!_font)
        return 0;
    uint8_t first_char = Font_getFirstChar(_font);
    uint8_t char_count = Font_getCharCount(_font);
    if (index == ' ')
        index = 'n'; // In case the font does not contain space.
    if (index < first_char || index >= (first_char + char_count))
        return 0;
    if (Font_IsFixed(_font))
        return Font_getWidth(_font);
    else
        return pgm_read_byte(_font + 6 + (index - first_char));
}

int Bitmap::getTextWidth(const char *str, int len) const
{
    int text_width = 0;
    if (len < 0)
        len = strlen(str);
    while (len-- > 0)
    {
        text_width += getCharWidth(*str++);
        if (len > 0)
            ++text_width;
    }
    return text_width;
}

int Bitmap::getTextWidth(const String &str, int start, int len) const
{
    int text_width = 0;
    if (len < 0)
        len = str.length() - start;
    while (len-- > 0)
    {
        text_width += getCharWidth(str[start++]);
        if (len > 0)
            ++text_width;
    }
    return text_width;
}

int Bitmap::getTextHeight() const
{
    if (_font)
        return Font_getHeight(_font);
    else
        return 0;
}

void Bitmap::copy(int x, int y, int width, int height, Bitmap *dest, int destX, int destY)
{
    if (dest == this)
    {
        // Copying to within the same bitmap, so copy in a direction
        // that will prevent problems with overlap.
        blit(x, y, x + width - 1, y + height - 1, destX, destY);
    }
    else
    {
        // Copying to a different bitmap.
        while (height > 0)
        {
            for (int tempx = 0; tempx < width; ++tempx)
                dest->setPixel(destX + tempx, destY, getPixel(x + tempx, y));
            ++y;
            ++destY;
            --height;
        }
    }
}

void Bitmap::fill(int x, int y, int width, int height, uint8_t color)
{
    while (height > 0)
    {
        for (int temp = 0; temp < width; ++temp)
            setPixel(x + temp, y, color);
        ++y;
        --height;
    }
}

void Bitmap::fill(int x, int y, int width, int height, PGM_VOID_P pattern, uint8_t color)
{
    uint8_t bitmap_w = pgm_read_byte(pattern);
    uint8_t bitmap_s = (bitmap_w + 7) >> 3;
    uint8_t bitmap_h = pgm_read_byte(pattern + 1);
    if (!bitmap_w || !bitmap_h)
        return;
    uint8_t invColor = !color;
    for (int tempy = 0; tempy < height; ++tempy)
    {
        const uint8_t *startLine = ((const uint8_t *)pattern) + 2 + (tempy % bitmap_h) * bitmap_s;
        const uint8_t *line = startLine;
        uint8_t mask = 0x80;
        uint8_t value = pgm_read_byte(line++);
        int bit = 0;
        for (int tempx = 0; tempx < width; ++tempx)
        {
            if (value & mask)
                setPixel(x + tempx, y + tempy, color);
            else
                setPixel(x + tempx, y + tempy, invColor);
            if (++bit >= bitmap_w)
            {
                mask = 0x80;
                line = startLine;
                value = pgm_read_byte(line++);
                bit = 0;
            }
            else
            {
                mask >>= 1;
                if (!mask)
                {
                    mask = 0x80;
                    value = pgm_read_byte(line++);
                }
            }
        }
    }
}

void Bitmap::scroll(int dx, int dy, uint8_t fillColor)
{
    scroll(0, 0, scr_width, scr_height, dx, dy, fillColor);
}

void Bitmap::scroll(int x, int y, int width, int height, int dx, int dy, uint8_t fillColor)
{
    // Bail out if no scrolling at all.
    if (!dx && !dy)
        return;

    // Clamp the scroll region to the extents of the bitmap.
    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if ((x + width) > scr_width)
        width = scr_width - x;
    if ((y + height) > scr_height)
        height = scr_height - y;
    if (width <= 0 || height <= 0)
        return;

    // Scroll the region in the specified direction.
    if (dy < 0)
    {
        if (dx < 0)
            blit(x - dx, y - dy, x + width - 1 + dx, y + height - 1 + dy, x, y);
        else
            blit(x, y - dy, x + width - 1 - dx, y + height - 1 + dy, x + dx, y);
    }
    else
    {
        if (dx < 0)
            blit(x - dx, y, x + width - 1 + dx, y + height - 1 - dy, x, y + dy);
        else
            blit(x, y, x + width - 1 - dx, y + height - 1 - dy, x + dx, y + dy);
    }

    // Fill the pixels that were uncovered by the scroll.
    if (dy < 0)
    {
        fill(x, y + height + dy, width, -dy, fillColor);
        if (dx < 0)
            fill(x + width + dx, y, -dx, height + dy, fillColor);
        else if (dx > 0)
            fill(x, y, dx, height + dy, fillColor);
    }
    else if (dy > 0)
    {
        fill(x, y, width, -dy, fillColor);
        if (dx < 0)
            fill(x + width + dx, y + dy, -dx, height - dy, fillColor);
        else if (dx > 0)
            fill(x, y + dy, dx, height - dy, fillColor);
    }
    else if (dx < 0)
    {
        fill(x + width + dx, y, -dx, height, fillColor);
    }
    else if (dx > 0)
    {
        fill(x, y, dx, height, fillColor);
    }
}

void Bitmap::invert(int x, int y, int width, int height)
{
    while (height > 0)
    {
        for (int tempx = x + width - 1; tempx >= x; --tempx)
            setPixel(tempx, y, !getPixel(tempx, y));
        --height;
        ++y;
    }
}

void Bitmap::blit(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (y3 < y1 || (y1 == y3 && x3 <= x1))
    {
        for (int tempy = y1; tempy <= y2; ++tempy)
        {
            int y = y1 - tempy + y3;
            int x = x3 - x1;
            for (int tempx = x1; tempx <= x2; ++tempx)
                setPixel(x + tempx, y, getPixel(tempx, tempy));
        }
    }
    else
    {
        for (int tempy = y2; tempy >= y1; --tempy)
        {
            int y = y1 - tempy + y3;
            int x = x3 - x1;
            for (int tempx = x2; tempx >= x1; --tempx)
                setPixel(x + tempx, y, getPixel(tempx, tempy));
        }
    }
}

void Bitmap::drawCirclePoints(int centerX, int centerY, int radius, int x, int y, uint8_t borderColor, uint8_t fillColor)
{
    if (x != y)
    {
        setPixel(centerX + x, centerY + y, borderColor);
        setPixel(centerX + y, centerY + x, borderColor);
        setPixel(centerX + y, centerY - x, borderColor);
        setPixel(centerX + x, centerY - y, borderColor);
        setPixel(centerX - x, centerY - y, borderColor);
        setPixel(centerX - y, centerY - x, borderColor);
        setPixel(centerX - y, centerY + x, borderColor);
        setPixel(centerX - x, centerY + y, borderColor);
        if (fillColor != NoFill)
        {
            if (radius > 1)
            {
                drawLine(centerX - x + 1, centerY + y, centerX + x - 1, centerY + y, fillColor);
                drawLine(centerX - y + 1, centerY + x, centerX + y - 1, centerY + x, fillColor);
                drawLine(centerX - x + 1, centerY - y, centerX + x - 1, centerY - y, fillColor);
                drawLine(centerX - y + 1, centerY - x, centerX + y - 1, centerY - x, fillColor);
            }
            else if (radius == 1)
            {
                setPixel(centerX, centerY, fillColor);
            }
        }
    }
    else
    {
        setPixel(centerX + x, centerY + y, borderColor);
        setPixel(centerX + y, centerY - x, borderColor);
        setPixel(centerX - x, centerY - y, borderColor);
        setPixel(centerX - y, centerY + x, borderColor);
        if (fillColor != NoFill)
        {
            if (radius > 1)
            {
                drawLine(centerX - x + 1, centerY + y, centerX + x - 1, centerY + y, fillColor);
                drawLine(centerX - x + 1, centerY - y, centerX + x - 1, centerY - y, fillColor);
            }
            else if (radius == 1)
            {
                setPixel(centerX, centerY, fillColor);
            }
        }
    }
}
