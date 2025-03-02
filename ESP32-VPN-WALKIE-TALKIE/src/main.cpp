#include "wifi/wifi_aux.h"
#include "husarnet/husarnet_aux.h"
#include "husarnet.h"
#include "udp/udp.h"
#include "led/led.h"
#include "audio/audio.h"
#include <rom/ets_sys.h>
#include <nvs_flash.h>

//Husarnet
HusarnetClient husarnet;
String IPV6_PEER;
bool peer_found = false;
String my_name;

//Audio
adc_continuous_handle_t my_adc_handle = NULL; 
i2s_chan_handle_t my_i2s_handle = NULL;
int16_t packet[AUDIO_PACKET_SIZE];
bool recording = false;

//udp
AsyncUDP *ping_udp = new AsyncUDP();
AsyncUDP *wav_udp = new AsyncUDP();

//Recording button
#define GPIO_BUTTON GPIO_NUM_4

//Serial
#define BAUD_RATE 115200

void restart(){
   Serial.println("(ESP) Riavvio...");
   write_rgb(255,0,0);
   delay(3000);
   ESP.restart();
}

void setup_connection(){
    //Da chiamare ogni volta che l'ip cambia, gli handler sono già configurati in automatico
    if (connect_to_ip(ping_udp, IPV6_PEER, PING_PORT) && connect_to_ip(wav_udp, IPV6_PEER, WAV_PORT)){
         Serial.println("(UDP) Nuova connessione stabilita con: " + IPV6_PEER);
         peer_found = true;
         write_rgb(0,255,255);
         delay(2000);
    } else {
         Serial.println("(UDP) impossibile stabilire una connessione con: " + IPV6_PEER);
    }
}
bool udp_listen_wav(AsyncUDP *udp){
   //Chiamare prima connect_to_ip (uva volta per porta) (ISTANZA WAV)
   if (udp->listen(WAV_PORT)){
        //Handler
        udp->onPacket([](AsyncUDPPacket packet){
                if(my_i2s_handle != NULL && packet.length() > 0 && !recording){
                  //se non sto registrando, l'istanza i2s è attiva e il pacchetto è valido
                  //OK   
                  int16_t* data = (int16_t*)packet.data();
                  write_pcm(data, packet.length() / sizeof(int16_t), my_i2s_handle);
                  //Attivo il blink di ricezione
                  write_rgb(0, 255, 0);
                  transmit_led = true;
                  transmit_on_time = millis();

                }

            });
        return true;
        
   } else {
        return false;
   }
}
void enable_udp_handlers(){
   
   if(udp_listen_ping(ping_udp, &pong_received, &pong_time) &&  udp_listen_wav(wav_udp)){
      Serial.println("(UDP) Handler configurati correttamente");
   } else {
      Serial.println("(UDP) Errore durante la configurazione degli handler");
      restart();
   }
   
}

void find_peer(){
   //Funzione per trovare il peer sulla rete husarnet a cui inviare le richieste-->Ritorna IPV6 del peer
   //list_peers fornisce una lista di peers sulla rete tra cui:
   //Master, Local, Server attuale, peer, peer
   //A noi server l'ip del peer diverso da quello del server attuale
   std::vector<HusarnetPeer> peers = husarnet.listPeers();
   std::string ip_address = husarnet.getIpAddress();
   if (peers.size() >= 5){ //Devono essere almeno 5

      for (auto const& peer : peers) {
         if (peer.second != ip_address) { 
            String new_ipv6 = String(peer.second.c_str());

            if (new_ipv6 != IPV6_PEER){ //Se cambio l'ip, mi riconnetto
               IPV6_PEER = new_ipv6;
               setup_connection();
            }
            break;
      }
      }
   } 
   
}

void reset_wifi() {
   WiFi.disconnect(true);  // Disconnette e "dimentica" la rete
   delay(100);             // Breve pausa per assicurarsi che la disconnessione sia completa
   WiFi.mode(WIFI_OFF);    // Spegne il modulo WiFi
   delay(100);
   WiFi.mode(WIFI_STA);    // Riporta in modalità stazione
}

bool try_wifi(String ssid, String pass) {
   reset_wifi();

   WiFi.setSleep(WIFI_PS_NONE); // no risparmio energetico
   WiFi.begin(ssid, pass);
   
   Serial.printf("(WIFI) Connecting to WiFi: %s\n", ssid.c_str());
   if (WiFi.waitForConnectResult(WIFI_MAX_TIME_TO_CONNECT) != WL_CONNECTED) {
      Serial.printf("(WIFI) WiFi connection failure\n", WiFi.status());
      return false;
   }
   
   Serial.print("(WIFI) Connected! Local IP: ");
   Serial.println(WiFi.localIP());
   return true;
}

void setup_wifi() {
   bool connected = false;
   for (const auto &credential: wifi_credentials){
      const char* ssid = credential.first.c_str();
      const char* password = credential.second.c_str();
      if (connected = try_wifi(ssid, password)){
         break;
      }
   }
   if(!connected){
      Serial.println("(WIFI) Impossibile stabilire una connessione WIFI");
      restart();
   }
}

bool check_wifi(){
   //Per controllare se sono connesso a un AP
   return WiFi.isConnected();
}

void manage_wifi(){
   //Controllo se ho perso connessione, in caso posivo provo a connettermi di nuovo
   static unsigned long last_wifi_time = 0;
   if (millis() - last_wifi_time > WIFI_PING_TIMER){ //Controllo ogni WIFI_PING_TIMER microsecondi
      last_wifi_time = millis();
      if (!check_wifi()){
        Serial.println("(WIFI) Persa la connessione con l'AP.");
        restart(); // per semplicità riavviamo
      }
   }
}


void setup_husarnet(){
   // Join the Husarnet network
   String MAC_ADDRESS = WiFi.macAddress();
   my_name = BASE_PEER + MAC_ADDRESS.substring(MAC_ADDRESS.length() - 2);
   

   husarnet.join(my_name.c_str(), JOIN_CODE);

   Serial.print("\n(HUSARNET) Waiting for Husarnet network.");
   unsigned long startTime = millis(); // Registra il tempo di inizio
   while(!husarnet.isJoined()) {
      Serial.print(".");
      delay(1000);
      // Controlla se il tempo limite è stato superato
      if (millis() - startTime > HUSARNET_TIMEOUT_JOIN) {
         Serial.println("\n(HUSARNET) Impossibile unirsi al network.");
         restart();
      }
   }
   Serial.println("\n(HUSARNET) Husarnet network joined");

   Serial.print("(HUSARNET) Husarnet IP: ");
   Serial.println(husarnet.getIpAddress().c_str());
}


void switch_to_tx()
{ 

    if (my_i2s_handle != NULL){
      if(stop_i2s_output(my_i2s_handle)){
         Serial.println("(I2S) Ascolto terminato.");
         my_i2s_handle = NULL;
      } else{
         Serial.println("(I2S) Disattivazione fallita");
         restart();
      }
   }
   
   //Posso attivare liberamente l'adc (in teoria)
   if (my_adc_handle == NULL){ //TX non ancora attivo
      //Attivo TX
      my_adc_handle = initialize_adc();
      if(start_adc(my_adc_handle)){
         Serial.println("(ADC) inizio a registrare.");
      } else {
         Serial.println("(ADC) Attivazione fallita");
         restart();
      }
   }
}

void switch_to_rx(){

   if (my_adc_handle != NULL){
      if(stop_adc(my_adc_handle)){
         Serial.println("(ADC) Registrazione finita");
         my_adc_handle = NULL;
      } else {
         Serial.println("(ADC) Fine registrazione fallita");
         restart();
      }
   }

   if (my_i2s_handle == NULL){
      my_i2s_handle = initialize_i2s_output();
      if(start_i2s_output(my_i2s_handle)){
         Serial.println("(I2S) Attivo e pronto ad ascoltare.");
      } else {
         Serial.println("(I2S) Attivazione fallita");
         restart();
      }
   }
}

bool check_connection_with_peer(){
   //Per capire se il ping funziona e le due esp32 sono connesse
   return connection_led;
}

void send_ping(){
   static unsigned long last_ping_time = 0;
   if (millis() - last_ping_time > PING_SEND_TIMER) { //Ogni PEER_PING_TIMER microsecondi
      last_ping_time = millis();
      if (IPV6_PEER != ""){
          Serial.println("---------------------------------------------------");
         udp_send_ping(ping_udp, IPV6_PEER, &ping_sent, &ping_time);
      }
   }
}

void manage_husarnet(){
   static unsigned long last_husarnet_time = 0;
   if (millis() - last_husarnet_time > HUSARNET_CHECK_TIME){ 
      last_husarnet_time = millis();
      if (!husarnet.isJoined()){
         Serial.println("(HUSARNET) Persa la connessione a husarnet");
         restart();
      }
   }
}

void manage_ping(){

   //Se ho appena inviato il ping, non ho ancora ricevuto un pong 
   //e il timer per l'handshake non è ancora partito
   if(ping_sent && !pong_received && !handshake_timer_started){
      handshake_interval = millis();
      handshake_timer_started = true;
      //Se ho inviato un ping da poco e ho appena ricevuto un pong
      //handshake effettuato
   } else if (ping_sent && pong_received){
      Serial.println("(PEER) Handshake effettuato");
      //attivo led handshake
      if (!recording){ //per mostrare il led di recording 
         write_rgb(0, 0, 255);
      }
      connection_led = true;
      no_connection_led = false;
      connection_led_time = millis();
      
      //termino il timer dell'handshake
      handshake_timer_started = false;
   } else {
      //Faccio partire il blink di attesa connessione
      //solo se il led di connessione è spento e se il no_connection_led
      //non è già acceso
      if(!no_connection_led && !connection_led && ((millis() - no_connection_led_off_time) >= NO_CONNECTION_LED_BLINK_TIME)){
         no_connection_led = true;
         no_connection_led_time = millis();
         write_rgb(255, 255, 255); 
      }
   }
   
   //Se il timer è partito, controllo se è oltre il limite di attesa e non sto ricevendo
   if(handshake_timer_started && ((millis() - handshake_interval) >= PEER_CONNECTION_TIMEOUT)){
      Serial.println("(HUSARNET) Limite di intervallo ping-pong superato.");
      restart();
   }

   //disattivo la flag di ping inviato se vado oltre PING_UP_TIME millisecondi
   if(ping_sent && ((millis() - ping_time) >= PING_UP_TIME)){
      ping_sent = false;
   }
   
   //qualsiasi cosa abbia fatto devo disattivare questa flag
   pong_received = false; //potremmo fare la stessa cosa del ping anche per il pong
   //ma a quel punto forse i led non funzionerebbero come prima
   //(in quanto il blocco if(ping && pong) verrebbe eseguito ripetutamente sovrascrivendo molto
   //velocemente lo stato del led)   
}

void record(){
   
   switch_to_tx();
   write_rgb(128, 0, 128);
   recording = true;
   
   while(gpio_get_level(GPIO_BUTTON) == 0){
   
      //invio il ping 
      send_ping();
      //controllo i timer
      manage_ping();
      //Chiamo la funzione che riempe il pacchetto dati con i dati presi dal microfono
      read_and_convert_to_pcm(my_adc_handle, packet, AUDIO_PACKET_SIZE);
      //Invio il pacchetto al peer
      udp_send_wav(wav_udp, IPV6_PEER, packet, AUDIO_PACKET_SIZE);
   } 
   //pacchetto di fine trasmissione
   write_rgb(0,0,0);
   udp_send_EOT(wav_udp, IPV6_PEER);
   recording = false;

}

void led_manager(){
   //gestisce il blink dei led

   //RECEIVE
   if (transmit_led && ((millis() - transmit_on_time) >= RECEIVE_LED_TIMEOUT)) {
        write_rgb(0 ,0 ,0);
        transmit_led = false;          // Resetta lo stato del LED
   }

   //PING
   if(connection_led  && ((millis() - connection_led_time) >= CONNECTION_LED_TIMEOUT)){
         write_rgb(0, 0, 0);
         connection_led = false;
   }

   //NO PING
   if(no_connection_led  && ((millis() - no_connection_led_time) >= NO_CONNECTION_LED_BLINK_TIME)){
         write_rgb(0, 0, 0);
         no_connection_led = false;
         no_connection_led_off_time = millis();
   }


}

void listen(){
   switch_to_rx();
   
   while((gpio_get_level(GPIO_BUTTON) == 1)){ //finchè non premo il tasto di trasmissione rimango in modalità ascolto
       send_ping();
       manage_wifi();
       manage_husarnet();
       manage_ping(); 
       led_manager();
   }

}


void manage_recording(){
   //controllo se ho premuto il bottone e avvio una registrazione nuova SOLO
   //se è stata stabilita una connessione con il peer e sto premendo il bottone
   if ((gpio_get_level(GPIO_BUTTON) == 0) && 
       check_connection_with_peer()){

      record();
   }
}

void peer_finder_loop(){
   //Per ora è possibile chiamarlo solo all'inizio
   //Nel caso si "perda" il peer durante l'esecuzione il programma continuerà
   //Questo si basa sul fatto che i due ip dei peer non cambino mai
   static unsigned long peer_timer_start = millis();
   while (!peer_found){
      if ((millis() - peer_timer_start) >= PEER_FIND_TIMEOUT){
         Serial.println("(HUSARNET) Tempo massimo per trovare il peer raggiunto.");
         restart();
      }
      find_peer();
      delay(10);
   }
}

void manage_comm(){
   manage_recording();
   listen();
}


void setup_nvs(){
   if(nvs_flash_init() == ESP_OK){
      Serial.println("(NVS) Partizione NVS inizializzata");
   } else {
      Serial.println("(NVS) Partizione NVS non inizializzata correttamente");
   }
}

void setup_pins(){
   //Setup pin di connessione
   gpio_set_direction(CONNECTION_STATUS_LED, GPIO_MODE_OUTPUT);  // Imposta il pin 2 come uscita
   //Setup pin bottone
   gpio_set_direction(GPIO_BUTTON, GPIO_MODE_INPUT);
   gpio_set_pull_mode(GPIO_BUTTON, GPIO_PULLUP_ONLY);
   Serial.println("(GPIO) PIN setup completato.");
}


void setup() { 
  Serial.begin(BAUD_RATE);
  //Setup vari
  setup_led();
  setup_pins();
  setup_nvs();
  setup_wifi();
  enable_udp_handlers();
  setup_husarnet();
  peer_finder_loop();
}

void loop() {
  manage_comm();
}