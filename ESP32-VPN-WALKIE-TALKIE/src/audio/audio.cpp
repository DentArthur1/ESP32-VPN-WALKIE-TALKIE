#include "audio.h"


void audio_debug(int16_t *audio_packet, size_t packet_len) {

    // Stampa i primi 10 valori
    Serial.println("Contenuto del pacchetto (i primi 10 valori):");
    for (size_t i = 0; i < packet_len && i < 10; i++) {
        Serial.printf("audio_packet[%d]: %d\n", i, audio_packet[i]);
    }

    // Stampa la lunghezza totale dell'array
    Serial.printf("Numero totale di int16_t nel pacchetto: %zu\n", packet_len);
}