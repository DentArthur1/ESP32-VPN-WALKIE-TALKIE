#include "esp_adc/adc_continuous.h"
#include <Arduino.h>

//ADC SETTINGS
#define ADC_SAMPLE_RATE 44100
#define ATTEN ADC_ATTEN_DB_12
#define ADC_CHANNEL ADC_CHANNEL_7
#define ADC_BITWIDTH ADC_BITWIDTH_12

#define VMAX 3.9f         // Tensione massima che l'ADC può misurare (es. 3.9V) --> dipende da ATTEN
#define DMAX 4095          // Valore massimo per un ADC a 12 bit (0-4095) --> dipende da DC_BITWIDTH
#define PCM_MAX_VALUE 32767  // Massimo valore per PCM a 16 bit (signed) --> dipende dal formato i2s in output per il MAX98357
#define PCM_MIN_VALUE -32768 // Minimo valore per PCM a 16 bit (signed) --> dipende dal formato i2s in output per il MAX98357


#define VOLUME_DIVIDER 3


adc_continuous_handle_t initialize_adc();
bool start_adc(adc_continuous_handle_t handle);
bool stop_adc(adc_continuous_handle_t handle);
bool read_raw_from_adc(adc_continuous_handle_t handle, uint8_t *buffer, size_t buffer_size_bytes, uint32_t *bytes_letti);
float convert_raw_to_voltage(uint16_t Dout);
int16_t convert_voltage_to_pcm(float voltage);
bool read_and_convert_to_pcm(adc_continuous_handle_t handle, int16_t *pcm_buffer, size_t buffer_len);
