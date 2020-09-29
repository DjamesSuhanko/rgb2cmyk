#include <WiFi.h>
#include <Wire.h>
#include "heltec.h"
#include "TCS34725.h"

#define OLED_UPDATE_INTERVAL 500  

const char* ssid       = "colorMixer"; 
const char* password   = "dobitaobyte"; 

const uint16_t port = 1234;
const char * host = "192.168.4.1"; // TODO: passar para o modo AP

TCS34725 tcs;
unsigned long interval = millis();
int timeout            = 3000;
int values[4]       = {0,0,0,0};
unsigned char RGBsample[3]   = {0,0,0};
 
void rgb2cmyk(uint8_t R, uint8_t G, uint8_t B){
  float Rfrac = (float)R/255.0;
  float Gfrac = (float)G/255.0;
  float Bfrac = (float)B/255.0;

  float K = 1-max({Rfrac,Gfrac,Bfrac});

  float C = (1-Rfrac-K)/(1-K);
  float M = (1-Gfrac-K)/(1-K);
  float Y = (1-Bfrac-K)/(1-K);

  values[0] = C*100;
  values[1] = M*100;
  values[2] = Y*100;
  values[3] = K*100/2; //GAMBIARRA PARA ACERTAR O NIVEL DE PRETO. PRECISA DE CORREÇÃO.
}

void setupWIFI()
{
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Connecting...");
  Heltec.display->drawString(0, 10, String(ssid));
  Heltec.display->display();

  WiFi.disconnect(true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  //WiFi.onEvent(WiFiEvent);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);    
  //WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid, password);

  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }

  Heltec.display->clear();
  if(WiFi.status() == WL_CONNECTED){
    Heltec.display->drawString(0, 0, "Conectado");
    Heltec.display->drawString(0, 10, "Ate logo");
    Heltec.display->display();
    delay(5000);
    Heltec.display->clear();
     Heltec.display->display();
  }
  else{
    Heltec.display->drawString(0, 0, "Connect False");
    Heltec.display->display();
  }

  Heltec.display->clear();
}

void setup()
{
    pinMode(0,INPUT_PULLDOWN);
    Serial.begin(115200);
    Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
    pinMode(25, OUTPUT);

    while (!Serial) {
        ;
    }

    setupWIFI();
  
    Wire.begin(23,22);
    if (!tcs.attach(Wire)) Serial.println("ERROR: TCS34725 NOT FOUND !!!");

    tcs.integrationTime(33); // ms
    tcs.gain(TCS34725::Gain::X01);

}

void loop(){
    if (!digitalRead(0)){
        WiFiClient client;
        Serial.println("INTERRUPCAO");
        digitalWrite(25,HIGH);
        delay(1500);
        digitalWrite(25,LOW);

        Heltec.display->clear();
        Heltec.display->setFont(ArialMT_Plain_10);
        Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      
        Heltec.display->drawString(0, 0, String("Address"));
        Heltec.display->display();

        if (tcs.available()){
           TCS34725::Color color = tcs.color();
          if (millis()-interval > timeout){
              RGBsample[0] = color.r;
              RGBsample[1] = color.g;
              RGBsample[2] = color.b;

              rgb2cmyk(RGBsample[0],RGBsample[1],RGBsample[2]);
              
              Serial.print("R          : "); Serial.println(RGBsample[0]);
              Serial.print("G          : "); Serial.println(RGBsample[1]);
              Serial.print("B          : "); Serial.println(RGBsample[2]);
              interval = millis();

              if (!client.connect(host, port)){
                  Serial.println("Connection to host failed");
              }
 
              unsigned char msg[6];
              memset(msg,0,sizeof(msg));
              msg[0] = 0x5e;
              msg[1] = values[0];
              msg[2] = values[1];
              msg[3] = values[2];
              msg[4] = values[3];
              msg[5] = '$';
              Serial.print("msg: ");
              for (uint8_t i=0;i<6;i++){
                  Serial.write(msg[i]);  
              }
              Serial.println(" ");
              
              Serial.print("CMYK: ");
              for (uint8_t j=0;j<4;j++){
                Serial.print(values[j]);
                Serial.print(" ");
              }
              Serial.println(" ");

              for (uint8_t j=0;j<6;j++){
                client.write(msg[j]);
              }
              //client.write(&msg[0],7);
              delay(3000);
              client.stop();
          }
       }
    }
}

void printRGB(){
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0, "RGB");
  Heltec.display->drawString(0, 10, "CMYK");
  Heltec.display->display();  
}

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event)
    {
        case SYSTEM_EVENT_WIFI_READY:               /**< ESP32 WiFi ready */
            break;
        case SYSTEM_EVENT_SCAN_DONE:                /**< ESP32 finish scanning AP */
            break;

        case SYSTEM_EVENT_STA_START:                /**< ESP32 station start */
            break;
        case SYSTEM_EVENT_STA_STOP:                 /**< ESP32 station stop */
            break;

        case SYSTEM_EVENT_STA_CONNECTED:            /**< ESP32 station connected to AP */
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:         /**< ESP32 station disconnected from AP */
            break;

        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:      /**< the auth mode of AP connected by ESP32 station changed */
            break;

        case SYSTEM_EVENT_STA_GOT_IP:               /**< ESP32 station got IP from connected AP */
        case SYSTEM_EVENT_STA_LOST_IP:              /**< ESP32 station lost IP and the IP is reset to 0 */
            break;

        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:       /**< ESP32 station wps succeeds in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:        /**< ESP32 station wps fails in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:       /**< ESP32 station wps timeout in enrollee mode */
        case SYSTEM_EVENT_STA_WPS_ER_PIN:           /**< ESP32 station wps pin code in enrollee mode */
            break;

        case SYSTEM_EVENT_AP_START:                 /**< ESP32 soft-AP start */
        case SYSTEM_EVENT_AP_STOP:                  /**< ESP32 soft-AP stop */
        case SYSTEM_EVENT_AP_STACONNECTED:          /**< a station connected to ESP32 soft-AP */
        case SYSTEM_EVENT_AP_STADISCONNECTED:       /**< a station disconnected from ESP32 soft-AP */
        case SYSTEM_EVENT_AP_PROBEREQRECVED:        /**< Receive probe request packet in soft-AP interface */
        case SYSTEM_EVENT_AP_STA_GOT_IP6:           /**< ESP32 station or ap interface v6IP addr is preferred */
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            break;

        case SYSTEM_EVENT_ETH_START:                /**< ESP32 ethernet start */
        case SYSTEM_EVENT_ETH_STOP:                 /**< ESP32 ethernet stop */
        case SYSTEM_EVENT_ETH_CONNECTED:            /**< ESP32 ethernet phy link up */
        case SYSTEM_EVENT_ETH_DISCONNECTED:         /**< ESP32 ethernet phy link down */
        case SYSTEM_EVENT_ETH_GOT_IP:               /**< ESP32 ethernet got IP from connected AP */
        case SYSTEM_EVENT_MAX:
            break;
    }
}
