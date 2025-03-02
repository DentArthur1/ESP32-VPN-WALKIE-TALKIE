#include "udp.h"


bool check_udp(AsyncUDP *udp){
    if (!udp){
        return false;
    }
    return true;
}

IPAddress get_ip_address(String remote_ip){
    //Converto l'ip a IPAddress
    IPAddress remote_ipv6;

    if (!remote_ipv6.fromString(remote_ip)) {
        Serial.println("Indirizzo IPv6 non valido durante il setup udp --> " + remote_ip);
    }
    return remote_ipv6;
}

bool connect_to_ip(AsyncUDP *udp, String remote_ip, uint16_t port){
    //Converto l'ip a IPAddress
    IPAddress remote_ipv6 = get_ip_address(remote_ip);
    
    if(udp->connect(remote_ipv6, port)){
        Serial.println("(IP) Connesso all'ip: " + remote_ipv6.toString() + " sulla porta: " + port);
        return true;
    } else {
        Serial.println("(IP) Errore durante la connesione all'ip: " + remote_ipv6.toString() + " sulla porta: " + port);
        return false;
        
    }
}
bool udp_listen_ping(AsyncUDP *udp, bool* pong_received, unsigned long *pong_at_time) {
    //Chiamare prima connect_to_ip (una volta per porta) (ISTANZA PING)
    if(udp->listen(PING_PORT)){
        //Handler
        udp->onPacket([pong_received, pong_at_time](AsyncUDPPacket packet){
            //Serial.println("(PING) ricevuto");
            *pong_received = true; 
            *pong_at_time = millis();
              
        });
        return true;
    } else {
        return false;
    }
    
}




bool udp_send_ping(AsyncUDP *udp, String remote_ip, bool *ping_sent, unsigned long *ping_at_time){
    
   //Chiamare prima connect_to_ip (una volta per porta) (ISTANZA PING)
   const char *messaggio = "PING";
   IPAddress remote_ipv6 = get_ip_address(remote_ip);


   if (check_udp(udp)){
        if(udp->writeTo((uint8_t *)messaggio, strlen(messaggio), remote_ipv6, PING_PORT) > 0){
            //Serial.println("(PING) inviato");
            *ping_sent = true;
            *ping_at_time = millis();
            return true;
        } else {
            Serial.println("(PING) invio ping fallito a: " + remote_ipv6.toString() + " sulla porta:" + PING_PORT);
            *ping_sent = false;
            return false;
        }
   } else {
        Serial.println("(PING) Connessione udp non valida.");
        return false;
   }
}

bool udp_send_wav(AsyncUDP *udp, String remote_ip, int16_t *packet, int packet_len){
    //(ISTANZA WAV)
    IPAddress remote_ipv6 = get_ip_address(remote_ip);
    
    // Invia il pacchetto come array di byte
    if (check_udp(udp)){
        size_t bytes_sent = udp->writeTo((uint8_t *)packet, packet_len * sizeof(int16_t), remote_ipv6, WAV_PORT);
        if (bytes_sent > 0) {
            //Serial.printf("(UDP) Pacchetto inviato: %d byte verso %s\n", bytes_sent, remote_ip.c_str());
            return true;
        } else {
            Serial.printf("(UDP) Errore durante l'invio del pacchetto di: %d byte verso: %s\n", bytes_sent, remote_ip.c_str());
            return false;
        }
    } else {
        Serial.println("(UDP) Connessione udp non valida.");
        return false;
    }
}

bool udp_send_EOT(AsyncUDP *udp, String remote_ip)
{
    IPAddress remote_ipv6 = get_ip_address(remote_ip);

    const int total_samples = (EOT_DURATION_MS * ADC_SAMPLE_RATE) / 1000; // Numero totale di campioni
    int16_t EOT_PACKET[AUDIO_PACKET_SIZE];

    if (check_udp(udp)) {
        for (int i = 0; i < total_samples; i += AUDIO_PACKET_SIZE) {
            for (int j = 0; j < AUDIO_PACKET_SIZE; j++) {
                int sample_index = i + j;
                if (sample_index >= total_samples) break;

                // Calcolo della frequenza attuale (calante linearmente)
                float freq = EOT_START_FREQ + ((EOT_END_FREQ - EOT_START_FREQ) * sample_index / total_samples);
                float t = (float)sample_index / ADC_SAMPLE_RATE;

                // Genera un'onda sinusoidale per questa frequenza
                EOT_PACKET[j] = (int16_t)(2000 * sinf(2.0 * M_PI * freq * t)); // Ampiezza max ±2000
            }

            // Invia il pacchetto come array di byte
            size_t bytes_sent = udp->writeTo((uint8_t *)EOT_PACKET, AUDIO_PACKET_SIZE * sizeof(int16_t), remote_ipv6, WAV_PORT);
            if (bytes_sent < 0) {
                Serial.printf("(UDP) Errore durante l'invio del pacchetto di fine trasmissione di: %d byte verso: %s\n", bytes_sent, remote_ip.c_str());
                return false;
            }
        }
        return true;
    } else {
        Serial.println("(UDP) Connessione udp non valida.");
        return false;
    }
}
