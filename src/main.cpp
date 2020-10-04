/*
 No User_setup.h, deixe de acordo aos parâmetros seguintes para a placa
 AFSmartControl com ESP32:

Instale a biblioteca TFT_eSPI

Edite o arquivo User_setup.h
Confirme o modello ILI9341

Procure por EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP e defina os pinos:
#define TOUCH_CS 33 
#define TFT_MISO 19 
#define TFT_MOSI 23 
#define TFT_SCLK 18
#define TFT_CS   12       
#define TFT_DC    2           
#define TFT_RST   4   
#define TFT_RST  -1   

Essa definição é a que deve ser usada na AFSmartControl:
https://www.afeletronica.com.br/pd-6ec52e-esp32-afsmartcontrol.html?ct=&p=1&s=1

#########################################################################
###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
#########################################################################


Referência: https://www.rapidtables.com/convert/color/rgb-to-cmyk.html


Conversão de RGB 24 bits para CMYK:
The R,G,B values are divided by 255 to change the range from 0..255 to 0..1:
R' = R/255
G' = G/255
B' = B/255

The black key (K) color is calculated from the red (R'), green (G') and blue (B') colors:
K = 1-max(R', G', B')

The cyan color (C) is calculated from the red (R') and black (K) colors:
C = (1-R'-K) / (1-K)

The magenta color (M) is calculated from the green (G') and black (K) colors:
M = (1-G'-K) / (1-K)

The yellow color (Y) is calculated from the blue (B') and black (K) colors:
Y = (1-B'-K) / (1-K)

resultado é a porcentagem de cada cor em relação ao volume.
 */

//TODO: trocar para modo AP ou AP/STA

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <initializer_list>

#define TFT_GREY 0x5AEB

#define SSID "colorMixer"
#define PASSWD "dobitaobyte"

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 320

#define REPEAT_CAL     false
#define BLACK_SPOT

#define FRAME_X        0
#define FRAME_Y        0
#define FRAME_W        50
#define FRAME_H        116

#define ML_MINUS_X     FRAME_X
#define  ML_MINUS_Y    FRAME_Y
#define ML_MINUS_W     FRAME_W
#define ML_MINUS_H     FRAME_H

#define ML_PLUS_X      DISPLAY_WIDTH-FRAME_W
#define ML_PLUS_Y      FRAME_H
#define ML_PLUS_W      FRAME_W
#define ML_PLUS_H      FRAME_H

#define RGB_MAX        255.0
#define ONE_DOT        1.0
#define HUNDRED        100.0

#define PCF_ADDR       0x27

#define PUMP_CYAN      0
#define PUMP_MAGENT    1
#define PUMP_YELLOW    2
#define PUMP_BLACK     3

SemaphoreHandle_t myMutex = NULL;

TaskHandle_t task_zero    = NULL;
TaskHandle_t task_one     = NULL;
TaskHandle_t task_two     = NULL;
TaskHandle_t task_three   = NULL;


TFT_eSPI tft = TFT_eSPI();

char *labels[4] = {"cyan","magent","yellow","black"};
String vol              = "0";

float one_ml            = 2141;
float ltx               = 0;                  // Coordenada x do ponteiro analógico

uint8_t RGBarray[3]     = {0}; 

uint16_t osx            = 120, osy = 120;     // Guarda coordenadas x e y

uint32_t updateTime     = 0;                  // intervalo para update

int old_analog          =  -999;              // Value last displayed
int old_digital         = -999;               // Value last displayed
int value[4]            = {0, 0, 0, 0};       //variável que armazena os valores CMYK exibidos no display
int old_value[4]        = { -1, -1, -1, -1};
int vol_in_ml           = 1; 

boolean pump_is_running = false;

char print_str[7];

String hexaColor = "ColorMixer";

struct pump_t {
    uint8_t pcf_value       = 255;                                      // estados dos pinos
    uint8_t pumps_bits[4]   = {7,6,5,4};                                // apenas para ordenar da esquerda para direita logicamente  
    TaskHandle_t handles[4] = {task_zero,task_one,task_two,task_three}; //manipuladores das tasks
    uint8_t running         = 0;                                        //cada task incrementa e decrementa. 0 é parado.
    unsigned long  times[4] = {0,0,0,0};                                
} pump_params;


//construtores
void plotPointer(void);
void plotLinear(char *label, int x, int y);
void plotNeedle(int value, byte ms_delay);
void analogMeter();
void rgb2cmyk(uint8_t R, uint8_t G, uint8_t B);
void cmyk2rgb(uint8_t C, uint8_t M, uint8_t Y, uint8_t K);
void getTouch();
void btnStart();
void fromPicker(void *pvParameters);
void pump(void *pvParameters);
void rgbToHexaString();

WiFiServer server(1234); 

void setup(void) {
  //cria o mutex. Fácil de esquecer, mas sem isso é reset na certa, quando colidir a requisição.
  vSemaphoreCreateBinary(myMutex);

  tft.init();
  tft.setRotation(0); // 0 ou 2? Depende da posição do display, mas tem que ser vertical
  uint16_t calData[5] = { 331, 3490, 384, 3477, 6 };
  tft.setTouch(calData);
  Serial.begin(9600); 
  tft.fillScreen(TFT_BLACK);

  memset(print_str,0,sizeof(print_str));

  analogMeter(); // Draw analogue meter

  // Traça 4 meters. Divisão de 240/4 = 60
  byte distance = 60;
  //label, x, y - título dos meters
  plotLinear("C", 0, 160);
  plotLinear("M", 1 * distance, 160);
  plotLinear("Y", 2 * distance, 160);
  plotLinear("K", 3 * distance, 160);

  btnStart();

  updateTime = millis();
  //rgb2cmyk(0,0,0);
  //Serial.println(" ");
  //Serial.println("MCU started.");

  /*WiFi.begin(SSID,PASSWD);
    //...e aguardamos até que esteja concluída.
    while (WiFi.status() != WL_CONNECTED) {

        delay(1000);

    }*/
    WiFi.softAP(SSID,PASSWD);
    //Serial.println("Wifi started.");
    //Serial.println(WiFi.localIP());
    IPAddress myIp = WiFi.localIP();
    hexaColor = "";
    /*
    for (uint8_t a=0;a<4;a++){
        hexaColor += String() + myIp[a];
        if (a < 3){
            hexaColor += ".";
        }    
    } */

    server.begin();
    //Serial.println("Socket started.");

    Wire.begin(21,22);
    Wire.beginTransmission(PCF_ADDR);
    Wire.write(0xFF);
    Wire.endTransmission();
    cmyk2rgb(0, 0, 0, 0);
    tft.setTextColor(TFT_WHITE);
    rgbToHexaString();
    tft.drawCentreString(hexaColor, 240/2, 130, 4);

    /* Essa tarefa recebe os valores CMYK do picker e atribui à variável
    values[n]. Fazendo isso, automaticamente a interface será atualizada.
    O início da mistura só pode ser feito pelo botão iniciar para não ter
    risco de ataque pela rede.
    */
    xTaskCreatePinnedToCore(fromPicker,"fromPicker",10000,NULL,0,NULL,0);
}

void loop() {
  getTouch();
  if (updateTime <= millis()) {
    updateTime = millis() + 25; // limitador da velocidade de update
    
    //renderiza os ponteiros vermelhos das colunas
    plotPointer(); // It takes aout 3.5ms to plot each gauge for a 1 pixel move, 21ms for 6 gauges
     
    //renderiza o ponteiro analógico
    plotNeedle(vol_in_ml, 10); 
  }
}

//pegar o parametro como definição da bomba a acionar
void pump(void *pvParameters){
   int color_bit = (int) pvParameters;

   vTaskDelay(pdMS_TO_TICKS(200)); //apenas para entrar em fila com as outras tasks

   xSemaphoreTake(myMutex,portMAX_DELAY);
   if (value[color_bit] > 0){
        pump_params.running += 1; //a partir de agora nenhuma alteração é permitida até voltar a 0.
        pump_params.times[color_bit] = vol_in_ml*(value[color_bit])*one_ml/100; //tempo de execução da bomba

        pump_params.pcf_value = pump_params.pcf_value&~(1<<pump_params.pumps_bits[color_bit]); //baixa o bit (liga com 0)
   
        Wire.beginTransmission(PCF_ADDR); //inicia comunicação i2c no endereço do PCF
        Wire.write(pump_params.pcf_value); //escreve o valor recém modificado
        Wire.endTransmission(); //finaliza a transmissão
        //Serial.println(pump_params.times[color_bit]);
   }
   xSemaphoreGive(myMutex);

   

    vTaskDelay(pdMS_TO_TICKS(pump_params.times[color_bit])); //executa o delay conforme calculado
    
    xSemaphoreTake(myMutex,portMAX_DELAY);
    if (value[color_bit] > 0){
        pump_params.pcf_value = pump_params.pcf_value|(1<<pump_params.pumps_bits[color_bit]);

        Wire.beginTransmission(PCF_ADDR);
         Wire.write(pump_params.pcf_value); 
         Wire.endTransmission();

         pump_params.running -= 1;
         pump_params.times[color_bit] = 0;
    }
    xSemaphoreGive(myMutex);

   vTaskDelete(NULL); //finaliza a task e se exclui
}

void rgbToHexaString(){
  char colorBuffer[9];
  snprintf(colorBuffer, sizeof(colorBuffer), "%02X %02X %02X", RGBarray[0], RGBarray[1], RGBarray[2]);
  hexaColor = "#" + String(colorBuffer);
}

void btnStart(){
  uint16_t x,y = 0;
  if (!tft.getTouch(&x,&y)){
  tft.fillRect(240/2-48, 93, 80, 20, tft.color565(127,90,80));
  tft.fillRect(240/2-45, 95, 80, 25, tft.color565(0x8d,0xa6,0xb4));
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Iniciar", 240/2-35, 95, 4);
  }
}

void cmyk2rgb(uint8_t C, uint8_t M, uint8_t Y, uint8_t K){
/*
A reversa também é simples e será utilizada para manipular o CMYK direto no display.
O RGB é inteiro, mas o CMYK precisa ser passado de 0 à 1 novamente. No artigo tem uma
imagem de exemplo do cálculo na calculadora, mas segue um exemplo:
C = 43
M = 30
Y = 10
K = 10

R = 255 * (1-(43/100))*(1-(10/100)) = 130,815 ; arredondar para cima quando > 0.5

==================================
The R,G,B values are given in the range of 0..255.

The red (R) color is calculated from the cyan (C) and black (K) colors:
R = 255 × (1-C) × (1-K)

The green color (G) is calculated from the magenta (M) and black (K) colors:
G = 255 × (1-M) × (1-K)

The blue color (B) is calculated from the yellow (Y) and black (K) colors:
B = 255 × (1-Y) × (1-K)
*/
    //uint8_t exemplo = round(255.0 * (1.0-((float)43/100.0))*(1.0-((float)10/100.0)));
    memset(RGBarray,0,sizeof(RGBarray));
    RGBarray[0] = round(RGB_MAX * (ONE_DOT-((float)C/HUNDRED)) * (ONE_DOT-((float)K/HUNDRED)));
    RGBarray[1] = round(RGB_MAX * (ONE_DOT-((float)M/HUNDRED)) * (ONE_DOT-((float)K/HUNDRED)));
    RGBarray[2] = round(RGB_MAX * (ONE_DOT-((float)Y/HUNDRED)) * (ONE_DOT-((float)K/HUNDRED)));
}

void fromPicker(void *pvParameters){
    //Serial.println("Start listening...");

   /* Lógica invertida: o GND é o PCF, portanto o pino deve ser colocado em 0 para 
   acionar os relés (addr: 0x27).
   As tasks são tCyan, tMagent, tYellow e tBlack

   RELES:
    128    64     32     16
   [ C ]  [ M ]  [ Y ]  [ K ]
     7      6      5      4
   */
    uint8_t i = 0;
    uint8_t result[6];
    memset(result,0,sizeof(result));

    while (true){
        WiFiClient client = server.available();
        if (client){
            //Serial.print("RGBarray[0]: ");
            //Serial.println(RGBarray[2]);
            i = 0;
            while (client.connected()){
                //avalia se tem dados e controla o buffer
                if (client.available() && i<6){
                    result[i] = client.read();
                    //Serial.println(result[i]);
                    i = result[0] == 94 ? i+1 : 0;
                    ////Serial.println(result[0]);
                }
            }
            client.stop();
        }

        if (result[0] == 0x5e && pump_params.running == 0){
           for (uint k=0;k<4;k++){
               value[k] = result[k+1]; //esse incremento é porque a msg começa na posição 1 (^CMYK$)
           }
            memset(result,0,sizeof(result));
        }    
    }
}

void getTouch(){
  uint16_t x,y = 0;
  if (tft.getTouch(&x,&y) && pump_params.running == 0){
    // 320 é Y, 240 é X
    //se x <= 50 & y <= 100
    //VOLUME (ml) esquerda e direita
    if (x <= FRAME_W && y <= FRAME_H){
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString(print_str, 120, 70, 4); 
      vol_in_ml = vol_in_ml >0 ? vol_in_ml-1 : 0;
    }
    //x >= 240-50 & y <= 100
    else if (x >= ML_PLUS_X && (y <= FRAME_H)){
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString(print_str, 120, 70, 4); 
      vol_in_ml = vol_in_ml < 100 ? vol_in_ml+1 : 100;
    }
    /* Os meters estão assim:
     C   M   Y   K
    --- --- --- ---
    | | | | | | | |
    --- --- --- ---
     1   2   3   4

    O display tem 240 x 320. 240/4 = 60, portanto cada meter tem 60 de espaço.
    As posições são passadas na função plotLinear(label,x,y) em setup(). Ex:
    byte distance = 60;
    plotLinear("C", 0, 160);
    plotLinear("M", 1 * distance, 160);
    plotLinear("Y", 2 * distance, 160);
    plotLinear("K", 3 * distance, 160);

    Deve-se deixar uma borda para não invadir o próximo meter no toque, portanto:
    meter0: x=0   w=55 ; y=160 h=320-160
    meter1: x=60  w=55 ; y=160 h=320-160
    meter2: x=120 w=55 ; y=160 h=320-160
    meter3: x=180 w=55 ; y=160 h=320-160
    */
    else if (x<56 && (y>160 && y<180)){
        value[0] = value[0] <100 ? value[0]+1 : value[0];
        //Serial.println("C+");
    }
    else if ((x>=60 && x<60+55) && (y>160 && y<180)){
        value[1] = value[1] <100 ? value[1]+1 : value[1];
        //Serial.println("M+");
    }
    else if ((x>=120 && x<120+55) && (y>160 && y<180)){
        value[2] = value[2] <100 ? value[2]+1 : value[2];
        //Serial.println("Y+");
    }
    else if ((x>=180 && x<180+55) && (y>160 && y<180)){
        value[3] = value[3] <100 ? value[3]+1 : value[3];
        //Serial.println("K+");
    }
    //Agora, os últimos 20 px são para decremento.
    else if (x<56 && y>=300){
      value[0] = value[0] >0 ? value[0]-1 : value[0];
      //Serial.print("M: ");
      //Serial.println(value[0]);
    }
    else if ((x>=60 && x<60+55) && y>=300){
      value[1] = value[1] >0 ? value[1]-1 : value[1];
      //Serial.print("M: ");
      //Serial.println(value[1]);
    }
    else if ((x>=120 && x<120+55) && y>300){
      value[2] = value[2] >0 ? value[2]-1 : value[2];
      //Serial.print("Y: ");
      //Serial.println(value[2]);
    }
    else if((x>=180 && x<180+55) && y>300){
      value[3] = value[3] >0 ? value[3]-1 : value[3];
      //Serial.print("K: ");
      //Serial.println(value[3]);
    }
    // botão de iniciar
    else if ((x>240/2-45 && x<240/2+45) && (y>95 && y<95+25) && pump_params.running == 0){
        tft.fillRect(240/2-45, 95, 80, 20, tft.color565(255,0,0));
        tft.setTextColor(TFT_WHITE);
        tft.drawString("Iniciar", 240/2-35, 95, 4);
        delay(1000);
        tft.fillRect(240/2-45, 95, 80, 25, tft.color565(0x8d,0xa6,0xb4));
        tft.drawString("Iniciar", 240/2-35, 95, 4);

        for (int j=0; j<4;j++){
            xTaskCreatePinnedToCore(pump,labels[j],10000,(void*) j,0,&pump_params.handles[j],0);
        }      
    }
  }
}

void rgb2cmyk(uint8_t R, uint8_t G, uint8_t B){
  float Rfrac = (float)R/(float)255;
  float Gfrac = (float)G/(float)255;
  float Bfrac = (float)B/(float)255;

  float K = 1-max({Rfrac,Gfrac,Bfrac});

  float C = (1-Rfrac-K)/(1-K);
  float M = (1-Gfrac-K)/(1-K);
  float Y = (1-Bfrac-K)/(1-K);

  value[0] = C*100;
  value[1] = M*100;
  value[2] = Y*100;
  value[3] = K*100;
}

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Traça o retângulo do meter analógico
  tft.fillRect(0, 0, 239, 126, TFT_GREY);
  tft.fillRect(5, 3, 230, 119, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Cor do texto para os ticks

  // Ticks de -50 à +50 a cada 5 graus.
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 120;
    uint16_t y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;

    // Green zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, tft.color565(0x8d,0xa6,0xb4));
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, tft.color565(0x8d,0xa6,0xb4));
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120;
    y1 = sy * 100 + 140;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Cálculo da posição dos labels a cada 25%
      x0 = sx * (100 + tl + 10) + 120;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: tft.drawCentreString("25", x0, y0 - 9, 2); break;
        case 0: tft.drawCentreString("50", x0, y0 - 6, 2); break;
        case 1: tft.drawCentreString("75", x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString("100", x0, y0 - 12, 2); break;
      }
    }

    // Enfeite: traça o arco da escala
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  tft.drawString("ml", 5 + 230 - 40, 119 - 20, 2); // Unidade no canto direito inferior
  tft.drawCentreString(" ", 120, 70, 4); // Não vi efeito, mas mantido
  tft.drawRect(5, 3, 230, 119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Ponteiro começa no 0.
}

// #########################################################################
// Update needle position (posição do ponteiro)
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  tft.drawRightString(buf, 40, 119 - 20, 2);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;
    if (vol_in_ml <1){
      vol_in_ml = 0;
    }
    else if (vol_in_ml > 100){
      vol_in_ml = 100;
    }
    if (ms_delay == 0) old_analog = value; // Update immediately id delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(tft.color565(0x8d,0xa6,0xb4));
    String vol_str = String(vol_in_ml);
    vol_str = vol_str + " ml";
    
    memset(print_str,0,sizeof(print_str));
    vol_str.toCharArray(print_str,sizeof(vol_str),0U);
    tft.drawCentreString(print_str, 120, 70, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_RED);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_MAGENTA);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    btnStart();
    delay(ms_delay);
  }
}

// #########################################################################
//  Draw a linear meter on the screen (C,M,Y,K)
// #########################################################################
void plotLinear(char *label, int x, int y)
{
  //largura das colunas das setas vermelhas?
  int w = 60;
  tft.drawRect(x, y, w, 155, TFT_GREY);
  tft.fillRect(x+2, y + 19, w-3, 155 - 38, TFT_WHITE);
  tft.setTextColor(tft.color565(0x8d,0xa6,0xb4), TFT_BLACK);
  tft.drawCentreString(label, x + w / 2, y + 2, 2);

  //linhas percentuais da coluna
  for (int i = 0; i < 110; i += 10)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }

  for (int i = 0; i < 110; i += 50)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }
  
  tft.drawCentreString("--%", x + w / 2, y + 155 - 18, 2);
}

// #########################################################################
//  Adjust 4 linear meter pointer positions (eram 6 no código de exemplo, modificado para CMYK)
// #########################################################################
void plotPointer(void)
{

  //dimensoes do triangulo vermelho
  int dy = 87;
  byte pw = 16;

  //espaços entre cada coluna
  uint8_t intervals = 60;

  //texto do rodape
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // Move the 4 pointers one pixel towards new value
  for (int i = 0; i < 4; i++)
  {
    char buf[8]; dtostrf(value[i], 4, 0, buf);
    tft.drawRightString(buf, i * intervals + 36 - 5, 187 - 27 + 155 - 18, 2);

    //posicao de plotagem das setas
    int dx = 3 + intervals * i;
    if (value[i] < 0) value[i] = 0; // Limit value to emulate needle end stops
    if (value[i] > 100) value[i] = 100;

    while (!(value[i] == old_value[i])) {
      dy = 187 + 100 - old_value[i];
      if (old_value[i] > value[i])
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_value[i]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_value[i]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
      cmyk2rgb(value[0],value[1],value[2],value[3]);
      tft.fillRect(5, 130, 230, 20, tft.color565(RGBarray[0],RGBarray[1],RGBarray[2]));
      tft.setTextColor(TFT_WHITE);
      rgbToHexaString();
      tft.drawCentreString(hexaColor, 240/2, 130, 4);
    }
  }
}
