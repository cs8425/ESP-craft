#ifndef __PINOUT_HPP_
#define __PINOUT_HPP_

#include <stdint.h>
#include <FS.h>

#include "config.h"

// esp8266 GPIO 0~16
#define MAX_PIN_COUNT     17


// TODO: different period/output type for each pin
struct pin_s {
	// source channel 0~31
	unsigned int ch : 5;

	// using pin
	unsigned int en : 1;

	// servo default, brushed default
	unsigned int center : 13; // 1500, 0
	unsigned int min : 13; // 1000, 0
	unsigned int max : 13; // 2000, 1000
	unsigned int period : 19; // 50, 20000

} __attribute__((packed));

class Pinout {
	public:
		Pinout() {
			Clear();
		}

		bool Mod(uint16_t idx, pin_s p);

		// servo: SetFreq(50, 20000)
		// motor: SetFreq(20000, 1000)
		void SetFreq(uint32_t _freq, uint32_t _range) {
			analogWriteFreq(_freq);
			analogWriteRange(_range);
		}

		void InitPin() {
			for(unsigned i=0; i<MAX_PIN_COUNT; i++){
				const pin_s* p = &(_pin[i]);
				if (p->en) {
					pinMode(i, OUTPUT);
				} else {
					analogWrite(i, 0);
				}
			}
		}

		bool SetDefServo(uint16_t idx, uint16_t ch) {
			if (idx >= MAX_PIN_COUNT) {
				return false;
			}

			pin_s* p = &(_pin[idx]);
			p->en = 1;
			p->ch = ch;
			p->center = 1500;
			p->min = 1000;
			p->max = 2000;
			p->period = 50;
			return true;
		}

		bool SetDefMotor(uint16_t idx, uint16_t ch) {
			if (idx >= MAX_PIN_COUNT) {
				return false;
			}

			pin_s* p = &(_pin[idx]);
			p->en = 1;
			p->ch = ch;
			p->center = 0;
			p->min = 0;
			p->max = 1000;
			p->period = 20000;
			return true;
		}

		void Clear() {
			for(unsigned i=0; i<MAX_PIN_COUNT; i++){
				//SetDefServo(i);
				//SetDefMotor(i);

				pin_s* p = &(_pin[i]);
				p->en = 0;
				p->center = 1500;
				p->min = 1000;
				p->max = 2000;
				p->period = 50;
			}
		}

		void Output(int16_t* chdata) { // int16_t* array has MAX_OUTPUT_COUNT, value 0 ~ 1000
			for(unsigned i=0; i<MAX_PIN_COUNT; i++){
				const pin_s* p = &(_pin[i]);
				if (p->en) {
					int value = chdata[p->ch] + p->center;
					if(value < p->min) value = p->min;
					if(value > p->max) value = p->max;
					analogWrite(i, value);
				}
			}
		}

		int Load() {
			if (!SPIFFS.exists(PIN_FILE)) return 1;
			File f = SPIFFS.open(PIN_FILE, "r");
			if (!f) return 1;

			return Parse(&f);
		}

		int Parse(File* f) {
			Clear();
			pin_s d;
			while(f->readBytes((char*) &d, sizeof(d))) {
				_pin[i] = d;
			}

			f->close();
			return 0;
		}

		int Save() {
			File f = SPIFFS.open(PIN_FILE, "w+");
			if (!f) return 1;

			for(unsigned i=0; i<MAX_PIN_COUNT; i++){
				const pin_s* data = &(_pin[i]);
				f.write((uint8_t*) data, sizeof(pin_s));
			}

			f.close();
			return 0;
		}

		void Reset() {
			SPIFFS.remove(MIXER_FILE);
		}
	private:
		pin_s _pin[MAX_PIN_COUNT];
};

static Pinout pwmout;

#endif //__PINOUT_HPP_

