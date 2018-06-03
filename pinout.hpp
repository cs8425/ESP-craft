#ifndef __PINOUT_HPP_
#define __PINOUT_HPP_

#include <stdint.h>
//#include <FS.h>

//#include "config.h"

#define MAX_PIN_COUNT     16

struct pin_s {
	// gpio 0~31
	unsigned int pin : 5;

	// type: 0 >> brushed, 1 >> servo
	unsigned int type : 2;

	// using pin
	unsigned int en : 1;

	// for brushed


	// for servo
	unsigned int center : 13;
	unsigned int min : 13;
	unsigned int max : 13;

	// for servo and brushed
	// period frequency
	unsigned int period : 16;

} __attribute__((packed));

class Pinout {
	public:
		Pinout();

		int Add(pin_s p); // start with 0, -1 >> error
		bool Mod(uint16_t idx, pin_s p);
		bool Del(uint16_t idx);
		const pin_s* Get(uint16_t idx);
		uint16_t Count();
		void Clear();

		void Output(int16_t* pwm); // uint16_t* array has MAX_OUTPUT_COUNT, RAE value -500 ~ 500, T1234 value 0 ~ 1000

/*		int Load() {
			if (!SPIFFS.exists(MIXER_FILE)) return 0;
			File f = SPIFFS.open(MIXER_FILE, "r");
			if (!f) return 1;

			return Parse(&f);
		}

		int Parse(File* f) {
			Clear();
			rule d;
			while(f->readBytes((char*) &d, sizeof(d))) {
				int ret = Add(d);
				if (ret == -1) break;
			}

			f->close();
			return 0;
		}

		int Save() {
			File f = SPIFFS.open(MIXER_FILE, "w+");
			if (!f) return 1;

			unsigned count = Count();
			for(unsigned i=0; i<count; i++){
				const rule* data = Get(i);
				f.write((uint8_t*) data, sizeof(rule));
			}

			f.close();
			return 0;
		}

		void Reset() {
			SPIFFS.remove(MIXER_FILE);
		}*/

		//void defragment(bool full = true, uint16_t idx = 0);
	private:
		void defragment(bool full = true, uint16_t idx = 0);

		uint16_t _count;
		pin_s _pin[MAX_PIN_COUNT];
};

static Pinout pwmout;

#endif //__PINOUT_HPP_

