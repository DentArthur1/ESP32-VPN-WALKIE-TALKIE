#include "adc_sampler.h"

// Inizializza l'adc
adc_continuous_handle_t initialize_adc() {
    adc_continuous_handle_t handle = NULL;

    // Configurazione dell'handle
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 256,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    // Configurazione del pattern dell'ADC
    adc_digi_pattern_config_t my_digi_config[1] = {
        {
            .atten = ATTEN,                 // Attenuazione (definisci ATTEN altrove)
            .channel = ADC_CHANNEL,         // Canale ADC (definisci ADC_CHANNEL altrove)
            .unit = ADC_UNIT_1,             // Unitario ADC1
            .bit_width = ADC_BITWIDTH,      // Risoluzione (definisci ADC_BITWIDTH altrove)
        }
    };

    // Configurazione del modo di acquisizione e altre impostazioni
    adc_continuous_config_t my_config = {
        .pattern_num = 1,                // Numero di pattern da leggere
        .adc_pattern = my_digi_config,   // I pattern da usare
        .sample_freq_hz = ADC_SAMPLE_RATE,   // Frequenza di campionamento (definisci SAMPLE_RATE altrove)
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,  // Modalità di conversione
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1 // Formato di uscita
    };

    // Applica la configurazione
    ESP_ERROR_CHECK(adc_continuous_config(handle, &my_config));

    // Restituisci l'handle pronto per lo start
    return handle;
}
// Fa partire l 'adc
bool start_adc(adc_continuous_handle_t handle){
   
    esp_err_t result = adc_continuous_start(handle);
    if (result == ESP_OK){
        return true;
    } else {
        Serial.println("(ADC) Errore durante l'inizializzazione dell'ADC");
        return false;
    }
}

bool stop_adc(adc_continuous_handle_t handle) {
    // Ferma l'acquisizione continua
    esp_err_t result = adc_continuous_stop(handle);
    if (result != ESP_OK) {
        Serial.println("(ADC) Errore durante l'arresto dell'acquisizione ADC");
        return false;
    }

    // Cancella l'handle e libera le risorse
    result = adc_continuous_deinit(handle);
    if (result != ESP_OK) {
        Serial.println("(ADC) Errore durante la deinizializzazione dell'ADC");
        return false;
    }

    return true;
}


// Legge un array di campioni grezzi dall'ADC
bool read_raw_from_adc(adc_continuous_handle_t handle, uint8_t *buffer, size_t buffer_size_bytes, uint32_t *bytes_letti){
    return adc_continuous_read(handle, buffer, buffer_size_bytes, bytes_letti, ADC_MAX_DELAY) == ESP_OK;
}

// Funzione per calcolare la tensione a partire dal valore grezzo ADC Dout
float convert_raw_to_voltage(uint16_t Dout) {
    return ((Dout * VMAX) / DMAX);  // Formula Vout = Dout * Vmax / Dmax
}

// Funzione per mappare correttamente il valore di tensione in PCM
int16_t convert_voltage_to_pcm(float voltage) {
    // Centra la tensione rispetto a VMAX/2 per ottenere valori positivi e negativi
    float centered_voltage = voltage - (VMAX / 2);

    // Mappa il valore centrato a PCM 16 bit (-32768 a 32767)
    int16_t pcm_value = (int16_t)((centered_voltage / (VMAX / 2)) * PCM_MAX_VALUE);

    // Limita ai valori PCM massimo e minimo
    if (pcm_value > PCM_MAX_VALUE) pcm_value = PCM_MAX_VALUE;
    if (pcm_value < PCM_MIN_VALUE) pcm_value = PCM_MIN_VALUE;
    return pcm_value;
}

// Funzione per leggere un array di campioni grezzi dall'ADC e convertirli in PCM
bool read_and_convert_to_pcm(adc_continuous_handle_t handle, int16_t *pcm_buffer, size_t buffer_len) {
    uint8_t raw_buffer[buffer_len * 2];  // Buffer per i campioni grezzi (2 byte per campione a 12 bit)
    uint32_t bytes_letti = 0;

    // Leggi i dati grezzi dall'ADC
    if (!read_raw_from_adc(handle, raw_buffer, sizeof(raw_buffer), &bytes_letti)) {
        Serial.println("(ADC) Errore nella lettura dell'ADC!");
        return false;
    }

    // Converti i dati grezzi a 12 bit e riempi il buffer PCM
    for (size_t i = 0; i < buffer_len; i++) {
        // Ricostruzione del campione a 12 bit (2 byte)
        uint16_t Dout = ((raw_buffer[i * 2 + 1] << 8) | raw_buffer[i * 2]) & 0xFFF;

        // Converti il valore grezzo in tensione
        float voltage = convert_raw_to_voltage(Dout);

        // Converti la tensione in valore PCM
        pcm_buffer[i] = convert_voltage_to_pcm(voltage) / VOLUME_DIVIDER;
    }
    return true;
}

