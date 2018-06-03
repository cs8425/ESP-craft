#ifndef __MIXER_HPP_
#define __MIXER_HPP_

#include <stdint.h>
//#include <FS.h>

//#include "config.h"

#define MAX_RULE_COUNT    32

#define MAX_INPUT_COUNT   8
#define MAX_OUTPUT_COUNT  8

enum {
	INPUT_RC_ROLL = 0,
	INPUT_RC_PITCH,
	INPUT_RC_YAW,
	INPUT_RC_THROTTLE,
	INPUT_RC_AUX1,
	INPUT_RC_AUX2,
	INPUT_RC_AUX3,
	INPUT_RC_AUX4,
	INPUT_STABILIZED_ROLL,
	INPUT_STABILIZED_PITCH,
	INPUT_STABILIZED_YAW,
	INPUT_STABILIZED_THROTTLE,
	INPUT_SOURCE_COUNT
} inputSource_e;

struct rule {
	// input ch 0~7
	unsigned int in : 3;

	// output ch 0~7
	unsigned int out : 3;

	// rate -8191 ~ 8191, scale = 8192
	int rate : 15;

	// prevent saturation
	unsigned int airmode : 1;

	// flag
	unsigned int flag : 2;

//	unsigned int en : 1;

} __attribute__((packed));

class Mixer {
	public:
		Mixer();

		int Add(rule r); // start with 0, -1 >> error
		int Add(uint8_t in, uint8_t out, float rate); // start with 0, -1 >> error
		bool Mod(uint16_t idx, rule r);
		bool Mod(uint16_t idx, uint8_t in, uint8_t out, float rate);
		bool Del(uint16_t idx);
		const rule* Get(uint16_t idx);
		uint16_t Count();
		void Clear();

		// AERT1234
		void Calc(int16_t* input); // uint16_t* array has MAX_INPUT_COUNT, RAE value -500 ~ 500, T1234 value 0 ~ 1000
		int16_t GetOut(int ch);
		const int16_t* GetOut();


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
		rule _rule[MAX_RULE_COUNT];

		int16_t _out[MAX_OUTPUT_COUNT];
};

static Mixer mixer;

#endif //__MIXER_HPP_

