#include <DMDESP.h>
#include <fonts/Mono5x7.h>

#define PANEL_WIDTH 1
#define PANEL_HEIGHT 1
DMDESP display(PANEL_WIDTH, PANEL_HEIGHT);

void setup()
{
    display.start();
    display.setBrightness(255);
    display.setFont(Mono5x7);
    display.drawString(0, 0, "Hello World");
}

void loop()
{
    display.loop();
}