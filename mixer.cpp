#include "mixer.hpp"

Mixer::Mixer() {
	Clear();
}

uint16_t Mixer::Count() {
	return _count;
}

void Mixer::Clear() {
	unsigned i;
	_count = 0;
	for(i = 0; i<MAX_RULE_COUNT; i++) {
		_rule[i].flag = 0;
		_rule[i].in = 0;
		_rule[i].out = 0;
		_rule[i].rate = 0;
	}
}

int Mixer::Add(rule r) {
	if (_count + 1 >= MAX_RULE_COUNT) {
		return -1;
	}
	if (r.out >= MAX_OUTPUT_COUNT) {
		return -1;
	}

	uint16_t idx = _count;
	_count += 1;

	_rule[idx] = r;

	return idx;
}

int Mixer::Add(uint8_t in, uint8_t out, float rate, bool airmode) {
	rate = rate * 8192;
	rate = (rate > 0)? rate + 0.5: rate - 0.5;
	rule r = rule { in, out, int(rate), airmode, 0 };
	return Add(r);
}

bool Mixer::Mod(uint16_t idx, rule r) {
	if (idx >= _count) {
		return false;
	}
	if (r.out >= MAX_OUTPUT_COUNT) {
		return false;
	}
	_rule[idx] = r;
	return true;
}

bool Mixer::Mod(uint16_t idx, uint8_t in, uint8_t out, float rate) {
	rate = rate * 8192;
	rate = (rate > 0)? rate + 0.5: rate - 0.5;
	rule r = rule { in, out, int(rate), 0 };
	return Mod(idx, r);
}

bool Mixer::Del(uint16_t idx) {
	if (idx >= _count) {
		return false;
	}

	_rule[idx].flag = 1;

	// Defragment
	defragment(false, idx);

	return true;
}

void Mixer::defragment(bool full, uint16_t idx) {
	unsigned i = (full)? 0: idx;
	unsigned end = (full)? MAX_RULE_COUNT: _count;
	end -= 1;

	for(; i<end; i++) {
		if ((_rule[i].flag == 1) && (_rule[i+1].flag != 1)) {
			_rule[i] = _rule[i+1];

			_rule[i+1].flag = 1; // mark remove
		}
	}

	unsigned dec = 0;
	if (full) {
		for(i=_count - 1; i<MAX_RULE_COUNT; i++) {
			if ((_rule[i].flag == 1) && (_rule[i+1].flag != 1)) {
				_rule[i].flag = 1;
				_rule[i].in = 0;
				_rule[i].out = 0;
				_rule[i].rate = 0;

				dec++;
			}
		}
	} else {
		i = end;
		if (_rule[i].flag == 1) {
			_rule[i].flag = 1;
			_rule[i].in = 0;
			_rule[i].out = 0;
			_rule[i].rate = 0;

			dec++;
		}
	}
	_count -= dec;
}

const rule* Mixer::Get(uint16_t idx) {
	if (idx >= _count) {
		return nullptr;
	}

	return &_rule[idx];
}

/*const rule* Mixer::GetD(uint16_t idx) {
	if (idx >= MAX_RULE_COUNT) {
		return nullptr;
	}

	return &_rule[idx];
}*/

int16_t Mixer::GetOut(int ch) {
	if (ch >= MAX_OUTPUT_COUNT) {
		return -32768;
	}

	return _out[ch];
}

const int16_t* Mixer::GetOut() {
	return _out;
}

void Mixer::Calc(int16_t* input) {
	unsigned i;
	int32_t _tmp[MAX_OUTPUT_COUNT] = {0};
	bool _air[MAX_OUTPUT_COUNT] = {0};
	int16_t max = 0;
	int16_t min = 0;

	for(i = 0; i<_count; i++) {
		unsigned iid = _rule[i].in;
		unsigned oid = _rule[i].out;
		_air[oid] = _air[oid] || _rule[i].airmode;
		_tmp[oid] += input[iid] * _rule[i].rate;
	}
	for(i = 0; i<MAX_OUTPUT_COUNT; i++) {
		int32_t tmp = _tmp[i] / 4096;
		tmp = (tmp > 0)? tmp+1 : tmp-1;
		_out[i] = tmp / 2;

		if (_air[i]) {
			if (_out[i] > max) max = _out[i];
			if (_out[i] < min) min = _out[i];
		}
	}

	int32_t range = max - min;
	if (range > 1000) {
		float rate = (float)1000 / range;
		for (i = 0; i<MAX_OUTPUT_COUNT; i++) {
			if (_air[i]) _out[i] = rate * _out[i];
		}
	}
	if (min < 0) {
		for (i = 0; i<MAX_OUTPUT_COUNT; i++) {
			if (_air[i]) _out[i] = _out[i] - min;
		}
	}
}


