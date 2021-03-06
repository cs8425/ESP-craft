#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <ESP8266WiFi.h>

////////////////////////////////////////////////////////

#define LED_PIN LED_BUILTIN // GPIO 2
#define LED_LOW_ACTIVE  1 // 1 >> low active


#define WIFI_MODE       WIFI_AP_STA // WIFI_AP, WIFI_STA, WIFI_AP_STA

#define AP_SSID         "esp-drone"
#define AP_PWD          "Aa123454321aA"
#define AP_CHANNEL      1 // 1~14
#define AP_HIDDEN       0

#define STA_SSID        "your_wifi_SSID"
#define STA_PWD         "password_for_your_wifi"

#define AES_KEY         "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92" // hex 64 Bytes, raw 32 bytes, default = sha256("123456")

////////////////////////////////////////////////////////

#define CONFIG_FILE     "/config"
#define KEY_FILE        "/key" // raw bytes
#define MIXER_FILE      "/mixer" // raw bytes
#define PIN_FILE        "/pin" // raw bytes

#define UDP_PORT        57666 // control data port for UDP server
#define WEB_PORT        8080 // port for web server
#define RES_BUF_SIZE    536 // buffer size for ResponseStream

#endif

