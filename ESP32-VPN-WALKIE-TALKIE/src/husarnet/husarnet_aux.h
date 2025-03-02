#include <Arduino.h>

//Husarnet credentials
#define JOIN_CODE "x"
//Stringa di base per ogni peer collegato alla rete
#define BASE_PEER "ESP-32-PEER"

#define HUSARNET_TIMEOUT_JOIN 30000

//se non ricevo una risposta di pong al mio ping da più di
//PEER_CONNECTION_TIMEOUT riavvio
#define PEER_CONNECTION_TIMEOUT 100000
#define PEER_FIND_TIMEOUT 50000
#define HUSARNET_CHECK_TIME 3000
//il valore è grande per evitare riavvi continui da entrambe le parti

//variabili ereditate dal main per gestire il 
//mantenimento della connessione con il peer
static bool ping_sent = false;
static bool pong_received = false;
static unsigned long ping_time;
static unsigned long pong_time;

//intervallo tra un ping e un pong, millis() - hanshake_interval non deve mai superare PEER_CONNECTION_TIMEOUT
static unsigned long handshake_interval;
static bool handshake_timer_started = false;

