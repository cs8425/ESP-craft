/*
ESP-Craft

Simple WiFi Controlled plane based on esp-01

*/

#include <ESP8266WiFi.h>
#include <FS.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


#include "config.h"
#include "setting.hpp"
#include "util.hpp"

#include "auth.hpp"
#include "web.hpp"

ADC_MODE(ADC_VCC); // test battery

AsyncWebServer server(WEB_PORT);

void setup() {
	Serial.begin(115200);

	if (!SPIFFS.begin()) {
		Serial.println("Could not mount SPIFFS!");
	}

	// load config
	config.Load();

	Serial.printf("WiFi Mode = %d\n", config.WiFi_mode);
	Serial.printf("AP_SSID = %s, channel = %d, hidden = %d\n", config.AP_ssid.c_str(), config.AP_chan, config.AP_hidden);
	Serial.printf("STA_SSID = %s\n", config.STA_ssid.c_str());

	WiFi.persistent(false); // !!! less flash write for WiFi setting !!!
	WiFi.mode(config.WiFi_mode);
	switch (config.WiFi_mode) {
		case WIFI_STA:
			WiFi.begin(config.STA_ssid.c_str(), config.STA_pwd.c_str());
			break;
		case WIFI_AP:
			WiFi.softAP(config.AP_ssid.c_str(), config.AP_pwd.c_str(), config.AP_chan, config.AP_hidden);
			break;
		default:
		case WIFI_AP_STA:
			WiFi.begin(config.STA_ssid.c_str(), config.STA_pwd.c_str());
			WiFi.softAP(config.AP_ssid.c_str(), config.AP_pwd.c_str(), config.AP_chan, config.AP_hidden);
			break;
	}
	auth.setKey((uint8_t*)config.KEY.c_str());

	// LED for debug
	pinMode(LED_PIN, OUTPUT);


	// ESP-01
	// GPIO3	RX
	// GPIO1	TX
	// GPIO0	left motor	out_ch0
	// GPIO2	right motor	out_ch1
	// load from SPIFFS
	mixer.Load();
	if (pwmout.Load() != 0) { // USE GPIO0, GPIO2 as pwm output on ESP-01
		pwmout.SetDefMotor(0, 0);
		pwmout.SetDefMotor(2, 1);
		pwmout.SetFreq(20000, 1000);
	}
	pwmout.InitPin();


	setupServer(server);

	// for debug
	server.on("/dump", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain", RES_BUF_SIZE);

		res->printf("heap:%u\n", ESP.getFreeHeap());
		res->printf("url:%s\n", req->url().c_str());
		res->printf("EspId:%x\n", ESP.getChipId());
		res->printf("flashId:%x\n", ESP.getFlashChipId());
		res->printf("CpuFreq:%x\n", ESP.getCpuFreqMHz());

		res->end();
		req->send(res);
	});

	const char * buildTime = __DATE__ " " __TIME__ " GMT";
	server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html").setLastModified(buildTime);

	server.onNotFound([](AsyncWebServerRequest *request){
		request->send(404);
	});

	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
	server.begin();
}


uint32_t now_ms;
uint32_t next_ms = 0;
uint32_t ext_cmd = 0;

void loop() {

	now_ms = millis();
	if (next_ms == 0) next_ms = now_ms;
	if (now_ms - next_ms >= 1000) {
		uint32_t dt = now_ms - next_ms;
		next_ms = now_ms + 1000 - dt;

		static int o = 0;
		digitalWrite(LED_PIN, LED_LOW_ACTIVE - o);
		o = 1 - o;

		Serial.print("wifi status: ");
		Serial.print(WiFi.status());
		Serial.print(", wifi SSID: ");
		Serial.print(WiFi.SSID());
		Serial.print(", IP Address: ");
		Serial.println(WiFi.localIP());

		Serial.print("Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);
		Serial.print("\n\n\n");

	}

	// 5 times 'R' to reset config
	if (Serial.available() > 0) {
		static uint8_t count = 0;
		int inByte = Serial.read();
		if (inByte == 'R') {
			count++;
			if (count >= 5) {
				Serial.println("reset all config...");
				config.Reset();
				mixer.Reset();
			}
		} else {
			count = 0;
		}
	}

	if (ext_cmd) {
		switch (ext_cmd) {
		case 1: // reboot
			Serial.println("reboot...");
			ESP.restart();
			break;
		case 2: // flush
			Serial.println("flush...");
			mixer.Save();
			pwmout.Save();
			break;
		case 3: // load
			Serial.println("load...");
			mixer.Load();
			pwmout.Load();
			break;
		}
		ext_cmd = 0;
	}

}

void setupServer(AsyncWebServer& server) {

	// get & processing setting
	server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *req){

		// TODO: encrypt before send
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);
		res->printf("{\"mode\":%d,", config.WiFi_mode);
		res->printf("\"ap\":\"%s\",", config.AP_ssid.c_str());
		res->printf("\"ch\":%d,", config.AP_chan);
		res->printf("\"hide\":%d,", config.AP_hidden);
		res->printf("\"sta\":\"%s\",", config.STA_ssid.c_str());
		res->printf("\"pwr\":%d}", config.PWR_SLEEP);

		res->end();
		req->send(res);
	});
	server.on("/setting", HTTP_POST, [](AsyncWebServerRequest *req){
		if(!req->hasParam("c", true)) {
			WRET_PARAM_ERR(req);
			return;
		}
		String c = req->getParam("c", true)->value();
		if (c.length() > MAX_POST_LEN) {
			WRET_PARAM_ERR(req);
			return;
		}

		String buf = auth.CodeHex2Byte((uint8_t*)c.c_str(), c.length());
		auth.SetGenerate(true);

		String magic = readStringUntil(buf, '\n');
		if (!magic.equals(REQ_MAGIC)) {
			WRET_AUTH_ERR(req);
			return;
		}

		int mode = readStringUntil(buf, '\n').toInt();
		String ap_ssid = readStringUntil(buf, '\n');
		String ap_pwd = readStringUntil(buf, '\n');
		int chan = readStringUntil(buf, '\n').toInt();
		int hidden = readStringUntil(buf, '\n').toInt();
		String sta_ssid = readStringUntil(buf, '\n');
		String sta_pwd = readStringUntil(buf, '\n');

		String new_key = readStringUntil(buf, '\n');


		if (mode >= 1 && mode <= 3) config.WiFi_mode = (WiFiMode_t) mode;

		if (ap_ssid.length() > 0 && ap_ssid.length() < 32) {
			config.AP_ssid = ap_ssid;
			if (ap_pwd.length() >= 8 && ap_pwd.length() < 64) config.AP_pwd = ap_pwd;
		}

		if (chan > 0 && chan <= 14) config.AP_chan = chan;

		if (hidden >= 0 && hidden <= 1) config.AP_hidden = hidden;

		if (sta_ssid.length() > 0 && sta_ssid.length() < 32) {
			config.STA_ssid = sta_ssid;
			if (sta_pwd.length() >= 8 && sta_pwd.length() < 64) config.STA_pwd = sta_pwd;
		}

		config.Save();

		if (new_key.length() == 64) {
			config.SetKeyHex(new_key);
			config.SaveKey();
			auth.setKey((uint8_t*)config.KEY.c_str());
		}

		req->send(200, "text/plain", "ok");
	});

	server.on("/sys", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("c");
		unsigned c = PARAM_GET_INT("c");
		// 0 : nop
		// 1 : reboot
		// 2 : flush schedule data to SPIFFS
		// 3 : load schedule data from SPIFFS

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		ext_cmd = c; // set event, and do in loop()

		req->send(200, "text/plain", String(c));
	});

	server.on("/token", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", auth.GetIVHex());
	});

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("{\"mid\":%d,\"md\":[%d,%d],\"count\":%u,", mid, m->on, m->off, count);

		const log_t* data = &newest_log;
		res->printf("\"sen\":[%d,%d,%d],", data->temp, data->hum, data->press);


		res->printf("\"pin\":%u,", pin.GetOutput());
		res->printf("\"log\":%u,", logs.Count());
		res->printf("\"heap\":%u}", ESP.getFreeHeap());

		res->end();
		req->send(res);
	});

	server.on("/mix/ls", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);

		res->printf("{\"def\":[\n");
		unsigned count = sch.Count();
		for(unsigned i = 0; i < count; i++){
			const schedule* data = sch.Get(i);
			res->printf("[%u,%u,%u,%u,%u]", data->start.week, data->start.sec, data->end.sec, data->m.on, data->m.off);
			if(i < count - 1) res->printf(",\n");
		}
		res->printf("],");

		int16_t mid = sch.GetModeId();
		res->printf("\"mid\":%d,\"count\":%u}\n", mid, count);

		res->end();
		req->send(res);
	});

	server.on("/mix/add", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("i");
		PARAM_CHECK("o");
		PARAM_CHECK("r");
		PARAM_CHECK("f");

		// TODO: check input
		unsigned chI = PARAM_GET_INT("i") - 1; // 0 ~ 8
		unsigned chO = PARAM_GET_INT("o") - 1; // 0 ~ 8
		signed rate = PARAM_GET_INT("r") - 1; // +-8191
		unsigned flag = PARAM_GET_INT("f") - 1;


		if(chI >= 9) WERRC(req, 400);
		if(chO >= 9) WERRC(req, 400);
		if(rate >= 8192 || rate <= -8192) WERRC(req, 400);
		if(flag >= 16) WERRC(req, 400);

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		int idx = mixer.Add(rule{chI, chO, rate, flag});
		req->send(200, "text/plain", String(idx));
	});

	server.on("/mix/mod", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("i");
		PARAM_CHECK("w");
		PARAM_CHECK("as");
		PARAM_CHECK("bs");
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		// TODO: check input
		unsigned i = PARAM_GET_INT("i") - 1;
		unsigned w = PARAM_GET_INT("w") - 1; // 0~6
		unsigned as = PARAM_GET_INT("as") - 1; // 0 ~ 86399
		unsigned bs = PARAM_GET_INT("bs") - 1; // 0 ~ 86399
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		if(w >= 7) WERRC(req, 400);
		if(as >= 86400) WERRC(req, 400);
		if(bs >= 86400) WERRC(req, 400);

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		bool ret = sch.Mod(i, daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(ret));
	});

	server.on("/mix/rm", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("i");
		unsigned idx = PARAM_GET_INT("i") - 1;

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		req->send(200, "text/plain", String(sch.Del(idx)));
	});

}



