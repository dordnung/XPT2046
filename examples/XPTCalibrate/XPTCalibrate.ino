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


static void calibratePoint(TS_Point &point, TS_Point &raw) {
	// Draw cross
	tft.drawFastHLine(point.x - 8, point.y, 16, ILI9341_WHITE);
	tft.drawFastVLine(point.x, point.y - 8, 16, ILI9341_WHITE);

	while (!touch.isTouching()) {
		delay(10);
	}

	touch.getRaw(raw);

	// Erase by overwriting with black
	tft.drawFastHLine(point.x - 8, point.y, 16, ILI9341_BLACK);
	tft.drawFastVLine(point.x, point.y - 8, 16, ILI9341_BLACK);
}

void calibrate() {
	TS_Point point1, point2, point3, point4, point5;
	TS_Point raw1, raw2, raw3, raw4, raw5;

	touch.getCalibrationPoints(point1, point2, point3, point4);

	calibratePoint(point1, raw1);
	delay(1000);
	calibratePoint(point2, raw2);
	delay(1000);
	calibratePoint(point3, raw3);
	delay(1000);
	calibratePoint(point4, raw4);
	delay(1000);

	touch.setCalibration(raw1, raw2, raw3, raw4);

	// Draw cross for circle
	TS_Point circlePoint = TS_Point(tft.width() / 4, tft.height() / 3);

	tft.drawFastHLine(circlePoint.x - 8, circlePoint.y, 16, ILI9341_WHITE);
	tft.drawFastVLine(circlePoint.x, circlePoint.y - 8, 16, ILI9341_WHITE);

	while (!touch.isTouching()) {
		delay(10);
	}

	touch.getPosition(point5);
	tft.drawCircle(point5.x, point5.y, 30, ILI9341_WHITE);

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "(%d, %d), (%d, %d), (%d, %d), (%d, %d)", raw1.x, raw1.y, raw2.x, raw2.y, raw3.x, raw3.y, raw4.x, raw4.y);

	tft.setTextColor(ILI9341_WHITE);
	tft.setCursor(0, 25);
	tft.print("setCalibration points:");
	tft.setCursor(0, 50);
	tft.print(buffer);

	Serial.println(buffer);
}

void setup() {
	delay(1000);
	tft.begin();
	touch.begin(tft.width(), tft.height());
	tft.fillScreen(ILI9341_BLACK);

	// No rotation!!
	calibrate();
}

void loop() {
	// Do nothing
	delay(1000);
}
