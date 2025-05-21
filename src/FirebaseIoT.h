#ifndef FIREBASEIOT_H
#define FIREBASEIOT_H

/**
 * This is wrapper for Firebase ESP8266 Client library
 * mobizt/Firebase Arduino Client Library for ESP8266 and ESP32@^4.4.14
 *
 *
 * Usage:
 * // Call in setup()
 * fb_setup("email", "password", "api_key", "database_url");
 * fb_setStreamCallback([](FirebaseData &data) {});
 * fb_setStreamCallback("/path/to/stream", [](FirebaseData &data) {});
 *
 * // Call in loop()
 * fb_loop();
 */


/**
 * Add new stream callback
 * 
 * FirebaseData newStream;
 * Firebase.RTDB.beginStream(&newStream, path);
 * Firebase.RTDB.setStreamCallback(&newStream, dataAvailableCallback, timeoutCallback);
 * 
 */


#ifndef FIREBASEIOT_CLIENT
#define FIREBASEIOT_CLIENT
#define FIREBASEIOT_CLIENT_VERSION 1
#endif

#include <Firebase_ESP_Client.h>

#if defined(ESP32)
#include "mbedtls/base64.h"
#elif defined(ESP8266)
#include "base64.hpp"
#endif

#define DEBUG

#if defined(DEBUG)
#define D_Print(...) Serial.printf(__VA_ARGS__)
#else
#define D_Print(...)
#endif




// Firebase Objects
extern FirebaseData fbStream;
extern FirebaseData fbdo;
extern FirebaseConfig fbConfig;
extern FirebaseAuth fbAuth;

extern String DB_DEVICE_PATH;
extern uint32_t fb_last_millis;
extern bool FB_FLAG_FIRST_INIT;
extern std::function<void()> fb_on_connected_cb;

extern FirebaseData::StreamEventCallback fb_rtdb_stream_dataAvailableCB;
extern FirebaseData::StreamTimeoutCallback fb_rtdb_stream_timeoutCB;

#if __has_include(<GenericOutputBase.h>)
#include "GenericOutputBase.h"
extern std::vector<stdGenericOutput::GenericOutputBase *> attachedDBDevices;
extern stdGenericOutput::fbrtdb_config_t fb_dbconfig;
#endif // __has_include(<GenericOutputBase.h>)

void fb_rtdb_stream_default_callback(FirebaseStream data);

void fb_setup(const String &email, const String &password, const String &api_key, const String &database_url);

String fb_get_uid(String token);

void fb_setStreamCallback(FirebaseData::StreamEventCallback dataAvailableCallback);

void fb_setStreamCallback(const String &path, FirebaseData::StreamEventCallback dataAvailableCallback);

void fb_setStreamCallback(const String &path, FirebaseData::StreamEventCallback dataAvailableCallback, FirebaseData::StreamTimeoutCallback timeoutCallback);

void fb_setConnectedCallback(std::function<void()> cb);

void fb_loop();

#endif // FIREBASEIOT_H
