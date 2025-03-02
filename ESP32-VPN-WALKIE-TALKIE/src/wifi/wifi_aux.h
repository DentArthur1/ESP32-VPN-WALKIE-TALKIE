#include <WiFi.h>


// WiFi credentials
#define WIFI_SSID1 "x"
#define WIFI_PASS1 "x"

#define WIFI_PING_TIMER 5000 //Intervallo di controllo della connessione all' AP
#define WIFI_MAX_TIME_TO_CONNECT 10000

// Lista di coppie di SSID e password
static std::vector<std::pair<std::string, std::string>> wifi_credentials = {
    {WIFI_SSID1, WIFI_PASS1},
};
