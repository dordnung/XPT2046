#include <Arduino.h>
#include <SPI.h>

#include "XPT2046.h"


// If the SPI library has transaction support, these functions
// establish settings and protect from interference from other
// libraries.  Otherwise, they simply do nothing.
#ifdef SPI_HAS_TRANSACTION
static inline void spi_begin(void) __attribute__((always_inline));
static inline void spi_begin(void) {
	SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));
}
static inline void spi_end(void) __attribute__((always_inline));
static inline void spi_end(void) {
	SPI.endTransaction();
}
#else
#define spi_begin()
#define spi_end()
#endif


// Swap variable a with variable b
inline static void swap(int16_t &a, int16_t &b) {
	int16_t tmp = a;
	a = b;
	b = tmp;
}

XPT2046::XPT2046(uint8_t cs_pin, uint8_t irq_pin)
	: _cs_pin(cs_pin), _irq_pin(irq_pin), _is_swapped(false) {
}

void XPT2046::begin(uint16_t width, uint16_t height) {
	pinMode(_cs_pin, OUTPUT);
	pinMode(_irq_pin, INPUT_PULLUP);

	_width = width;
	_height = height;

	// Delta x and delta y of the calibration points
	_cal_dx = _width - 2 * CAL_MARGIN;
	_cal_dy = _height - 2 * CAL_MARGIN;

	// Default calibration (map 0..ADC_MAX -> 0..width|height)
	setCalibration(
		TS_Point(CAL_MARGIN * ADC_MAX / width, CAL_MARGIN * ADC_MAX / height),
		TS_Point(width - CAL_MARGIN * ADC_MAX / width, CAL_MARGIN * ADC_MAX / height),
		TS_Point(CAL_MARGIN * ADC_MAX / width, height - CAL_MARGIN * ADC_MAX / height),
		TS_Point(width - CAL_MARGIN * ADC_MAX / width, height - CAL_MARGIN * ADC_MAX / height)
		);

	// TODO(?) Use the following empirical calibration instead? -- Does it depend on VCC??
	// setCalibration(TS_Point(192, 1728), TS_Point(194, 275), TS_Point(1744, 1707), TS_Point(1744, 288));

	// Start SPI
	SPI.begin();
	#ifndef SPI_HAS_TRANSACTION
		SPI.setBitOrder(MSBFIRST);
		SPI.setDataMode(SPI_MODE0);
	#endif

	// Make sure PENIRQ is enabled
	powerDown();
}

void XPT2046::getCalibrationPoints(TS_Point &point1, TS_Point &point2, TS_Point &point3, TS_Point &point4) {
	point1 = TS_Point(CAL_MARGIN, CAL_MARGIN);
	point2 = TS_Point(_width - CAL_MARGIN, CAL_MARGIN);
	point3 = TS_Point(CAL_MARGIN, _height - CAL_MARGIN);
	point4 = TS_Point(_width - CAL_MARGIN, _height - CAL_MARGIN);
}

bool XPT2046::setCalibration(TS_Point point1, TS_Point point2, TS_Point point3, TS_Point point4) {
	_cal_point_1 = point1;
	_cal_point_2 = point2;
	_cal_point_3 = point3;
	_cal_point_4 = point4;

	int16_t delta_x = point2.x - point1.x;
	int16_t delta_y = point2.y - point1.y;

	// x and y are swapped, if the delta of x is in x direction less then the delta of y
	if (abs(delta_x) < abs(delta_y)) {
		delta_x = point3.x - point1.x;
		delta_y = point3.y - point1.y;

		// x and y are only swapped, if the delta of y is in y direction higher then the delta of x
		if (abs(delta_x) > abs(delta_y)) {
			_is_swapped = true;
		} else {
			// This can't be possible! Maybe wrong click?
			return false;
		}
	} else {
		_is_swapped = false;
	}

	// Calculate the average of the x and y
	int16_t average_x_1, average_x_2, average_y_1, average_y_2;

	if (_is_swapped) {
		average_x_1 = (_cal_point_1.x + _cal_point_2.x) / 2;
		average_x_2 = (_cal_point_3.x + _cal_point_4.x) / 2;
		_cal_point_1.x = _cal_point_2.x = average_x_1;
		_cal_point_3.x = _cal_point_4.x = average_x_2;

		average_y_1 = (_cal_point_1.y + _cal_point_3.y) / 2;
		average_y_2 = (_cal_point_2.y + _cal_point_4.y) / 2;
		_cal_point_1.y = _cal_point_3.y = average_y_1;
		_cal_point_2.y = _cal_point_4.y = average_y_2;
	} else {
		average_x_1 = (_cal_point_1.x + _cal_point_3.x) / 2;
		average_x_2 = (_cal_point_2.x + _cal_point_4.x) / 2;
		_cal_point_1.x = _cal_point_3.x  = average_x_1;
		_cal_point_2.x = _cal_point_4.x = average_x_2;

		average_y_1 = (_cal_point_1.y + _cal_point_2.y) / 2;
		average_y_2 = (_cal_point_3.y + _cal_point_4.y) / 2;
		_cal_point_1.y = _cal_point_2.y = average_y_1;
		_cal_point_3.y = _cal_point_4.y = average_y_2;
	}

	return true;
}

uint16_t XPT2046::_readLoop(uint8_t ctrl, uint8_t max_samples) const {
	uint16_t prev = 0xffff, cur = 0xffff;
	uint8_t i = 0;

	do {
		prev = cur;
		cur = SPI.transfer(0);

		// 16 clocks -> 12-bits (zero-padded at end)
		cur = (cur << 4) | (SPI.transfer(ctrl) >> 4);
	} while ((prev != cur) && (++i < max_samples));

	return cur;
}

// TODO: Caveat - MODE_SER is completely untested!!
//       Need to measure current draw and see if it even makes sense to keep it as an option
void XPT2046::getRaw(TS_Point &point, adc_ref_t mode, uint8_t max_samples) const {
	// Implementation based on TI Technical Note http://www.ti.com/lit/an/sbaa036/sbaa036.pdf
	uint8_t ctrl_lo = ((mode == MODE_DFR) ? CTRL_LO_DFR : CTRL_LO_SER);

	digitalWrite(_cs_pin, LOW);
	
	spi_begin();

	// Send first control byte
	SPI.transfer(CTRL_HI_X | ctrl_lo);

	point.x = _readLoop(CTRL_HI_X | ctrl_lo, max_samples);
	point.y = _readLoop(CTRL_HI_Y | ctrl_lo, max_samples);

	if (mode == MODE_DFR) {
		// Turn off ADC by issuing one more read (throwaway)
		// This needs to be done, because PD=0b11 (needed for MODE_DFR) will disable PENIRQ

		// Maintain 16-clocks/conversion; _readLoop always ends after issuing a control byte
		SPI.transfer(0);
		SPI.transfer(CTRL_HI_Y | CTRL_LO_SER);
	}

	// Flush last read, just to be sure
	SPI.transfer16(0);
	spi_end();

	digitalWrite(_cs_pin, HIGH);
}

void XPT2046::getPosition(TS_Point &position, adc_ref_t mode, uint8_t max_samples) const {
	if (!isTouching()) {
		position.x = position.y = 0xffff;
		return;
	}

	// First get raw position
	TS_Point rawPosition;
	getRaw(rawPosition, mode, max_samples);

	// Calculate x and y
	if (_is_swapped) {
		position.x = CAL_MARGIN + _cal_dx * (rawPosition.y - _cal_point_1.y) / (_cal_point_2.y - _cal_point_1.y);
		position.y = CAL_MARGIN + _cal_dy * (rawPosition.x - _cal_point_1.x) / (_cal_point_3.x - _cal_point_1.x);
	} else {
		position.x = CAL_MARGIN + _cal_dx * (rawPosition.x - _cal_point_1.x) / (_cal_point_2.x - _cal_point_1.x);
		position.y = CAL_MARGIN + _cal_dy * (rawPosition.y - _cal_point_1.y) / (_cal_point_3.y - _cal_point_1.y);
	}

	// Transform based on current rotation setting
	switch (_rotation) {
		case ROT_90:
			position.x = _width - position.x;
			swap(position.x, position.y);
			break;
		case ROT_180:
			position.x = _width - position.x;
			position.y = _height - position.y;
			break;
		case ROT_270:
			position.y = _height - position.y;
			swap(position.x, position.y);
			break;
		case ROT_0:
		default:
			// Do nothing
			break;
	}
}

void XPT2046::powerDown() const {
	digitalWrite(_cs_pin, LOW);
	// Issue a throw-away read, with power-down enabled (PD{1,0} == 0b00)
	// Otherwise, ADC is disabled
	spi_begin();
	SPI.transfer(CTRL_HI_Y | CTRL_LO_SER);
	SPI.transfer16(0);  // Flush, just to be sure
	spi_end();
	digitalWrite(_cs_pin, HIGH);
}


TS_Point::TS_Point(void) {
	x = y = 0;
}

TS_Point::TS_Point(int16_t x0, int16_t y0) {
	x = x0;
	y = y0;
}

bool TS_Point::operator==(TS_Point p1) {
	return  ((p1.x == x) && (p1.y == y));
}

bool TS_Point::operator!=(TS_Point p1) {
	return  ((p1.x != x) || (p1.y != y));
}