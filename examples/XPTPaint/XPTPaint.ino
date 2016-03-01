#include <Arduino.h>
#include <SPI.h>

#include <Adafruit_ILI9341.h>
#include <XPT2046.h>

// Modify the following two lines to match your hardware
#define XPT_CS 7
#define XPT_IRQ 6
XPT2046 touch = XPT2046(XPT_CS, XPT_IRQ);

#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
	delay(1000);
	tft.begin();
	touch.begin(tft.width(), tft.height());
	tft.fillScreen(ILI9341_BLACK);

	// Set 270° Rotation
	tft.setRotation(3);
	touch.setRotation(touch.ROT_270);

	// Replace these for your screen module (Use calibration example to get these values)
	touch.setCalibration(TS_Point(192, 1728), TS_Point(194, 275), TS_Point(1744, 1707), TS_Point(1744, 288));
}

static uint16_t prev_x = 0xffff, prev_y = 0xffff;

void loop() {
	if (touch.isTouching()) {
		TS_Point touchPosition;
		touch.getPosition(touchPosition);

		if (prev_x == 0xffff) {
			tft.drawPixel(touchPosition.x, touchPosition.y, ILI9341_WHITE);
		} else {
			tft.drawLine(prev_x, prev_y, touchPosition.x, touchPosition.y, ILI9341_WHITE);
		}

		prev_x = touchPosition.x;
		prev_y = touchPosition.y;
	} else {
		prev_x = prev_y = 0xffff;
	}

	delay(20);
}
