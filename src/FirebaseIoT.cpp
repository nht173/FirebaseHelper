#include "FirebaseIoT.h"

FirebaseData fbStream;
FirebaseData fbdo;
FirebaseConfig fbConfig;
FirebaseAuth fbAuth;

String DB_DEVICE_PATH = "";
uint32_t fb_last_millis = 0;
bool FB_FLAG_FIRST_INIT = false;
std::function<void()> fb_on_connected_cb = nullptr;

#if FBRTDB_LIB_TYPE == 1
stdGenericOutput::fbrtdb_config_t fb_dbconfig = { .fbdo = &fbdo, .path = "" };
#endif

FirebaseData::StreamEventCallback fb_rtdb_stream_dataAvailableCB = nullptr;
FirebaseData::StreamTimeoutCallback fb_rtdb_stream_timeoutCB = nullptr;

void fb_rtdb_stream_default_callback(FirebaseStream data) {

    D_Print("Stream data available: %s\n", data.dataPath().c_str());
    D_Print("Event type: %s\n", data.eventType().c_str());
    D_Print("Data type: %s\n", data.dataType().c_str());
    D_Print("Data: %s\n", data.payload().c_str());

#if FBRTDB_LIB_TYPE == 1
    D_Print("[GenericOutputBase] Data stream callback\n");
    if (attachedDBDevices.empty()) return;
    for (auto &device : attachedDBDevices)
    {
        if (device == nullptr) continue;
        device->syncState(&data);
    }
#endif
    if (fb_rtdb_stream_dataAvailableCB != nullptr) {
        fb_rtdb_stream_dataAvailableCB(data);
    }
}

void fb_setup(const String &email, const String &password, const String &api_key, const String &database_url)
{
    fbAuth.user.email = email;
    fbAuth.user.password = password;
    fbConfig.api_key = api_key;
    fbConfig.database_url = database_url;
    fbConfig.timeout.serverResponse = 10 * 1000;

    Firebase.reconnectNetwork(true);
    fbdo.setBSSLBufferSize(2048, 1024);
    fbStream.setBSSLBufferSize(2048, 1024);

#if defined(ESP32)
    fbStream.keepAlive(5, 5, 1);
#endif

    Firebase.setDoubleDigits(5);
    Firebase.begin(&fbConfig, &fbAuth);
}

String fb_get_uid(String token)
{
    if (token.length() == 0) return "";

    String data = token.substring(token.indexOf('.') + 1, token.lastIndexOf('.'));
    if (data.length() == 0) return "";

#if defined(ESP32)
    size_t olen = 0;
    std::string data_std = data.c_str();
    size_t input_len = data_std.length();
    unsigned char *output = new unsigned char[input_len]; // base64 decode sẽ không lớn hơn input
    int ret = mbedtls_base64_decode(output, input_len, &olen, (const unsigned char*)data_std.c_str(), input_len);
    if (ret != 0 || olen == 0) {
        delete[] output;
        return "";
    }
    String data_decoded_str = String((const char*)output);
    delete[] output;
#elif defined(ESP8266)
    unsigned char *data_decoded = new unsigned char[data.length() + 1];
    strncpy((char *)data_decoded, data.c_str(), data.length());
    data_decoded[data.length()] = '\0';

    unsigned char string[800];
    unsigned int string_len = decode_base64(data_decoded, string);
    delete[] data_decoded;

    if (string_len == 0) return "";

    String data_decoded_str = String((char *)string);
#endif

    int uid_index = data_decoded_str.indexOf("\"user_id\":\"");
    if (uid_index == -1) return "";
    return data_decoded_str.substring(uid_index + 11, data_decoded_str.indexOf("\"", uid_index + 11));
}

void fb_setStreamCallback(FirebaseData::StreamEventCallback dataAvailableCallback)
{
    fb_rtdb_stream_dataAvailableCB = dataAvailableCallback;
}

void fb_setStreamCallback(const String &path, FirebaseData::StreamEventCallback dataAvailableCallback)
{
    DB_DEVICE_PATH = path;
    fb_rtdb_stream_dataAvailableCB = dataAvailableCallback;
}

void fb_setStreamCallback(const String &path, FirebaseData::StreamEventCallback dataAvailableCallback, FirebaseData::StreamTimeoutCallback timeoutCallback)
{
    DB_DEVICE_PATH = path;
    fb_rtdb_stream_dataAvailableCB = dataAvailableCallback;
    fb_rtdb_stream_timeoutCB = timeoutCallback;
}

void fb_setConnectedCallback(std::function<void()> cb)
{
    fb_on_connected_cb = cb;
}

void fb_loop()
{
    if (!FB_FLAG_FIRST_INIT && Firebase.ready())
    {
        FB_FLAG_FIRST_INIT = true;
        fb_last_millis = millis();

        if (DB_DEVICE_PATH == "")
        {
            String token = Firebase.getToken();
            if (token.length() == 0)
            {
                D_Print("Token not found\n");
                return;
            }
            String uid = fb_get_uid(token);
            if (uid.length() == 0)
            {
                D_Print("UID not found\n");
                return;
            }
#if defined(ESP8266)
            DB_DEVICE_PATH = "/" + uid + "/" + ESP.getChipId() + "/data";
#elif defined(ESP32)
            DB_DEVICE_PATH = "/" + uid + "/" + String(ESP.getEfuseMac(), HEX) + "/data";
#endif
        }
        D_Print("Device path: %s\n", DB_DEVICE_PATH.c_str());

#if FBRTDB_LIB_TYPE == 1
        fb_dbconfig.path = DB_DEVICE_PATH;
#endif

        if (!Firebase.RTDB.beginStream(&fbStream, DB_DEVICE_PATH))
        {
            Serial.printf("stream begin error, %s\n\n", fbStream.errorReason().c_str());
        }
        Firebase.RTDB.setStreamCallback(&fbStream, fb_rtdb_stream_default_callback, fb_rtdb_stream_timeoutCB);

        if (fb_on_connected_cb != nullptr)
        {
            fb_on_connected_cb();
        }
    }

    Firebase.ready();
}