#include "DMDESP.h"

#include <stdlib.h>
#include <string.h>

#include <Arduino.h>
#include "Bitmap.h"

DMDESP::DMDESP(int widthPanels, int heightPanels)
    : Bitmap(widthPanels * DMDESP_NUM_COLUMNS, heightPanels * DMDESP_NUM_ROWS), useDoubleBuffer(false), phase(0), fb0(0), fb1(0), displayfb(0), lastRefresh(millis())
{
    // Both rendering and display are to fb0 initially.
    fb0 = displayfb = frame_buffer;

    // Initialize SPI to MSB-first, mode 0, clock divider = 2.
    pinMode(DMD_PIN_SPI_SCK, OUTPUT);
    pinMode(DMD_PIN_SPI_MOSI, OUTPUT);
    GPOC = (1 << DMD_PIN_SPI_SCK);  // Set to LOW
    GPOC = (1 << DMD_PIN_SPI_MOSI); // Set to LOW

    // Initialize the DMD-specific pins.
    pinMode(DMD_PIN_A, OUTPUT);
    pinMode(DMD_PIN_B, OUTPUT);
    pinMode(DMD_PIN_LATCH, OUTPUT);
    pinMode(DMD_PIN_OUTPUT_ENABLE, OUTPUT);

    GPOC = (1 << DMD_PIN_A);             // Set to LOW
    GPOC = (1 << DMD_PIN_B);             // Set to LOW
    GPOC = (1 << DMD_PIN_LATCH);         // Set to LOW
    GPOC = (1 << DMD_PIN_OUTPUT_ENABLE); // Set to LOW
    GPOS = (1 << DMD_PIN_SPI_MOSI);      // Set to HIGH
}

DMDESP::~DMDESP()
{
    if (fb0)
        free(fb0);
    if (fb1)
        free(fb1);
    frame_buffer = 0; // Don't free the buffer again in the base class.
}

void DMDESP::setDoubleBuffer(bool state)
{
    if (state != useDoubleBuffer)
    {
        useDoubleBuffer = state;
        if (state)
        {
            // Allocate a new back buffer.
            unsigned int size = scr_stride * scr_height;
            fb1 = (uint8_t *)malloc(size);

            // Clear the new back buffer and then switch to it, leaving
            // the current contents of fb0 on the screen.
            if (fb1)
            {
                memset(fb1, 0xFF, size);
                ets_intr_lock(); // IRQ Disable
                frame_buffer = fb1;
                displayfb = fb0;
                ets_intr_unlock(); // IRQ Enable
            }
            else
            {
                // Failed to allocate the memory, so revert to single-buffered.
                useDoubleBuffer = false;
            }
        }
        else if (fb1)
        {
            // Disabling double-buffering, so forcibly switch to fb0.
            ets_intr_lock(); // IRQ Disable
            frame_buffer = fb0;
            displayfb = fb0;
            ets_intr_unlock(); // IRQ Enable

            // Free the unnecessary buffer.
            free(fb1);
            fb1 = 0;
        }
    }
}

void DMDESP::swapBuffers()
{
    if (useDoubleBuffer)
    {
        // Turn off interrupts while swapping buffers so that we don't
        // accidentally try to refresh() in the middle of this code.
        ets_intr_lock(); // IRQ Disable
        if (frame_buffer == fb0)
        {
            frame_buffer = fb1;
            displayfb = fb0;
        }
        else
        {
            frame_buffer = fb0;
            displayfb = fb1;
        }
        ets_intr_unlock(); // IRQ Enable
    }
}

void DMDESP::swapBuffersAndCopy()
{
    swapBuffers();
    if (useDoubleBuffer)
        memcpy((void *)frame_buffer, (void *)displayfb, scr_stride * scr_height);
}

extern "C"
{
#define USE_US_TIMER
#include "osapi.h"
#include "user_interface.h"
#define os_timer_arm_us(a, b, c) ets_timer_arm_new(a, b, c, 0)
}

ETSTimer dispTimer;
bool tickOccured;

void timerCallback(void *pArg)
{
    tickOccured = true;
}

void dispinit(void)
{
    system_timer_reinit();
    os_timer_setfn(&dispTimer, timerCallback, NULL);
    os_timer_arm_us(&dispTimer, DMDESP_REFRESH_US, true);
}

void DMDESP::loop()
{
    if (tickOccured)
    {
        tickOccured = false;
        refresh();
        system_timer_reinit();
    }
}

// Flip the bits in a byte.  Table generated by genflip.c
static const uint8_t flipBits[256] PROGMEM = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
    0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
    0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
    0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
    0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
    0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
    0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
    0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
    0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
    0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
    0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
    0x3F, 0xBF, 0x7F, 0xFF};

void DMDESP::refresh()
{
    // Transfer the data for the next group of interleaved rows.
    int stride4 = scr_stride * 4;
    volatile uint8_t *data0;
    volatile uint8_t *data1;
    volatile uint8_t *data2;
    volatile uint8_t *data3;
    bool flipRow = ((scr_height & 0x10) == 0);
    for (byte y = 0; y < scr_height; y += DMDESP_NUM_ROWS)
    {
        if (!flipRow)
        {
            // The panels in this row are the right way up.
            data0 = displayfb + scr_stride * (y + phase);
            data1 = data0 + stride4;
            data2 = data1 + stride4;
            data3 = data2 + stride4;
            for (int x = scr_stride; x > 0; --x)
            {
                SPI.write(*data3++);
                SPI.write(*data2++);
                SPI.write(*data1++);
                SPI.write(*data0++);
            }
            flipRow = true;
        }
        else
        {
            data0 = displayfb + scr_stride * (y + DMDESP_NUM_ROWS - phase) - 1;
            data1 = data0 - stride4;
            data2 = data1 - stride4;
            data3 = data2 - stride4;
            for (int x = scr_stride; x > 0; --x)
            {
                SPI.transfer(pgm_read_byte(&(flipBits[*data3--])));
                SPI.transfer(pgm_read_byte(&(flipBits[*data2--])));
                SPI.transfer(pgm_read_byte(&(flipBits[*data1--])));
                SPI.transfer(pgm_read_byte(&(flipBits[*data0--])));
            }
            flipRow = false;
        }
    }

    pinMode(DMD_PIN_OUTPUT_ENABLE, INPUT);

    GPOS = (1 << DMD_PIN_LATCH); // Set to HIGH
    GPOC = (1 << DMD_PIN_LATCH); // Set to LOW

    digitalWrite(DMD_PIN_A, bitRead(phase, LOW));
    digitalWrite(DMD_PIN_B, bitRead(phase, HIGH));

    pinMode(DMD_PIN_OUTPUT_ENABLE, OUTPUT);
    analogWrite(DMD_PIN_OUTPUT_ENABLE, brightness);
    phase = (phase + 1) & 0x03;
}

void DMDESP::start()
{
    analogWriteFreq(16384);
    pinMode(SCK, SPECIAL);
    pinMode(MOSI, SPECIAL);
    SPI1C = 0;
    SPI1U = SPIUMOSI | SPIUDUPLEX | SPIUSSE;
    SPI1U1 = (7 << SPILMOSI) | (7 << SPILMISO);
    SPI1C1 = 0;
    SPI1C &= ~(SPICWBO | SPICRBO);
    SPI1U &= ~(SPIUSME);
    SPI1P &= ~(1 << 29);
    SPI.setFrequency(10000000);
    uint8_t jsh = 0x11;
    while (jsh--)
    {
        if (jsh == DMD_PIN_A || jsh == DMD_PIN_OUTPUT_ENABLE || jsh == DMD_PIN_B || jsh == DMD_PIN_LATCH)
        {
            GPOC = (1 << jsh); // Set to LOW
            pinMode(jsh, OUTPUT);
        }
        else
        {
            continue;
        }
    }
    tickOccured = false;
    dispinit();
}

void DMDESP::setBrightness(uint8_t brightness)
{
    if (brightness > 255)
        brightness = 255;
    this->brightness = brightness;
}