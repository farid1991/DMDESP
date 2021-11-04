#ifndef DMDESP_H
#define DMDESP_H

#include <SPI.h>

#include "Bitmap.h"

// Pins on the DMDESP connector board.
#define DMD_PIN_A 16             //D0 // A PHASE_LSB
#define DMD_PIN_B 12             //D6 // B PHASE_MSB
#define DMD_PIN_LATCH 0          //D3 // SCLK
#define DMD_PIN_OUTPUT_ENABLE 15 //D8 // nOE
#define DMD_PIN_SPI_MOSI 13      //D7 // R SPI Master Out, Slave In
#define DMD_PIN_SPI_SCK 14       //D5 // CLK SPI Serial Clock

// Dimension information for the display.
#define DMDESP_NUM_COLUMNS 32 // Number of columns in a panel.
#define DMDESP_NUM_ROWS 16    // Number of rows in a panel.

// Refresh times.
#define DMDESP_REFRESH_US 100

class DMDESP : public Bitmap
{
public:
    explicit DMDESP(int widthPanels = 1, int heightPanels = 1);
    ~DMDESP();

    bool IsUseDoubleBuffer() const { return useDoubleBuffer; }
    void setDoubleBuffer(bool state);
    void swapBuffers();
    void swapBuffersAndCopy();

    void start();
    void refresh();
    void loop();

    void setBrightness(uint8_t brightness);

private:
    // Disable copy constructor and operator=().
    DMDESP(const DMDESP &other) : Bitmap(other) {}
    DMDESP &operator=(const DMDESP &) { return *this; }

    uint8_t brightness;
    bool useDoubleBuffer;
    uint8_t phase;
    uint8_t *fb0;
    uint8_t *fb1;
    uint8_t *displayfb;
    uint64_t lastRefresh;
};

#endif
