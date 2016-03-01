#ifndef _XPT2046_h_
#define _XPT2046_h_

class TS_Point {
public:
	TS_Point(void);
	TS_Point(int16_t x, int16_t y);

	bool operator==(TS_Point);
	bool operator!=(TS_Point);

	int16_t x, y;
};

class XPT2046 {
public:
	/* The margin to the border for calibration points */
	static const uint16_t CAL_MARGIN = 20;

	/* Default screen size */
	static const uint16_t WIDTH = 240;
	static const uint16_t HEIGHT = 320;

	/* Rotation types */
	enum rotation_t : uint8_t {
		ROT_0, ROT_90, ROT_180, ROT_270
	};

	/* Mode types */
	enum adc_ref_t : uint8_t {
		MODE_SER, MODE_DFR
	};

	/**
	 * Constructor.
	 *
	 * @param cs_pin Digital pin that is connected with the CS pin.
	 * @param irq_pin Digital pin that is connected with the IRQ pin.
	 */
	XPT2046(uint8_t cs_pin, uint8_t irq_pin);

	/**
	* Initialize the XPT class.
	*
	* @param width Width of the screen without rotation.
	* @param height Height of the screen without rotation.
	*/
	void begin(uint16_t width = WIDTH, uint16_t height = HEIGHT);

	/**
	 * Sets the rotation of the screen. Should be equal with the rotation of the GUI Library.
	 *
	 * @param rotation The rotation to set.
	 */
	void setRotation(rotation_t rotation) {
		_rotation = rotation;
	}

	/**
	 * Set points for calibrating the touch screen.
	 * Read the raw position when clicking on one of these points and pass them to setCalibration.
	 *
	 * @param point1 The first point in the upper left corner.
	 * @param point2 The second point in the upper right corner.
	 * @param point3 The third point in the bottom left corner.
	 * @param point4 The fourth point in the bottom right corner.
	 */
	void getCalibrationPoints(TS_Point &point1, TS_Point &point2, TS_Point &point3, TS_Point &point4);

	/**
	 * Calibrates the touch screen with four given points for each corner.
	 * Pass the results from getCalibrationPoints when clicking on the points.
	 *
	 * @param point1 The first touch point when clicking on the upper left corner.
	 * @param point2 The second touch point when clicking on the upper right corner.
	 * @param point3 The third touch point when clicking on the bottom left corner.
	 * @param point4 The fourth touch point when clicking on the bottom right corner.
	 * @return True on sucess, false on error.
	 */
	bool setCalibration(TS_Point point1, TS_Point point2, TS_Point point3, TS_Point point4);

	/**
	 * Returns true if the screen is touched currently.
	 *
	 * @return True if touch screen is pressed currently, otherwise false.
	 */
	bool isTouching() const {
		return (digitalRead(_irq_pin) == LOW);
	}

	/**
	 * Gets the raw touch position of the current touch.
	 *
	 * @param position Point to save the touch position in.
	 * @param mode Mode to get the touch position with.
	 * @param max_samples Max samples to use.
	 */
	void getRaw(TS_Point &position, adc_ref_t mode = MODE_DFR, uint8_t max_samples = 0xff) const;

	/**
	 * Gets the calculated touch position of the current touch.
	 *
	 * @param position Point to save the touch position in.
	 * @param mode Mode to get the touch position with.
	 * @param max_samples Max samples to use.
	 */
	void getPosition(TS_Point &position, adc_ref_t mode = MODE_DFR, uint8_t max_samples = 0xff) const;

	void powerDown() const;

private:
	static const uint8_t CTRL_LO_DFR = 0b0011;
	static const uint8_t CTRL_LO_SER = 0b0100;
	static const uint8_t CTRL_HI_X = 0b1001 << 4;
	static const uint8_t CTRL_HI_Y = 0b1101 << 4;

	// 12 bits
	static const uint16_t ADC_MAX = 0x0fff;

	uint16_t _width, _height;
	rotation_t _rotation;
	uint8_t _cs_pin, _irq_pin;

	bool _is_swapped;
	int32_t _cal_dx, _cal_dy;
	TS_Point _cal_point_1, _cal_point_2, _cal_point_3, _cal_point_4;

	uint16_t _readLoop(uint8_t ctrl, uint8_t max_samples) const;
};

#endif  // _XPT2046_h_
