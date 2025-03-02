#include <Arduino.h> 
#include "led/led.h"
#include "AsyncUDP.h"
#include "audio/audio.h"


#define PING_PORT 12345
#define WAV_PORT 12346
#define PING_SEND_TIMER 750
#define PING_UP_TIME 1000


#define AUDIO_PACKET_SIZE 128
#define AUDIO_BUFFER_SIZE 32


// Parametri per il suono di fine trasmissione
#define EOT_DURATION_MS  500 // Durata del suono in millisecondi
#define EOT_START_FREQ 1000   // Frequenza iniziale (Hz)
#define EOT_END_FREQ  200      // Frequenza finale (Hz)

//packet size e sending frequency devono essere direttamente proporzionali

//Led di connessione riuscita (Accesso su entrambi in caso di connessione stabilita)
#define CONNECTION_STATUS_LED GPIO_NUM_2 

bool check_udp(AsyncUDP *udp);
IPAddress get_ip_address(String remote_ip);
bool connect_to_ip(AsyncUDP *udp, String remote_ip, uint16_t port);
bool udp_listen_ping(AsyncUDP *udp, bool* pong_received, unsigned long *pong_at_time);
bool udp_send_ping(AsyncUDP *udp, String remote_ip, bool *ping_sent, unsigned long *ping_at_time);
bool udp_send_wav(AsyncUDP *udp, String remote_ip, int16_t *packet, int packet_len);
bool udp_send_EOT(AsyncUDP *udp, String remote_ip);


    