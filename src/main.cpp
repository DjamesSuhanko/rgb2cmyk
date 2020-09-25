/*
 No User_setup.h, deixe de acordo aos parâmetros seguintes para a placa
 AFSmartControl com ESP32:

Instale a biblioteca TFT_eSPI

Edite o arquivo User_setup.h
Confirme o modello ILI9341

procure por EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP e defina os pinos:
#define TOUCH_CS 33 
#define TFT_MISO 19 
#define TFT_MOSI 23 
#define TFT_SCLK 18
#define TFT_CS   12       
#define TFT_DC    2           
#define TFT_RST   4   
#define TFT_RST  -1   

#########################################################################
###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
#########################################################################


http://www.javascripter.net/faq/rgb2cmyk.htm
https://www.rapidtables.com/convert/color/rgb-to-cmyk.html


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

resultado é a porcentagem de cada cor
 */

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>

#define TFT_GREY 0x5AEB

#define SSID "SuhankoFamily"
#define PASSWD "fsjmr112"

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

TFT_eSPI tft = TFT_eSPI();

String vol          = "0";

float one_ml        = 2.141;
float ltx           = 0;              // Coordenada x do ponteiro analógico

uint16_t osx        = 120, osy = 120; // Guarda coordenadas x e y

uint32_t updateTime = 0;              // intervalo para update

int old_analog      =  -999; // Value last displayed
int old_digital     = -999; // Value last displayed
int value[4]        = {0, 0, 0, 0}; //variável que armazena os valores CMYK exibidos no display
int old_value[4]    = { -1, -1, -1, -1};
int vol_in_ml       = 10; //TODO: colocar em 0 depois de criar a função que alimenta essa variável

boolean SwitchOn    = false;

char print_str[7];

//construtores
void plotPointer(void);
void plotLinear(char *label, int x, int y);
void plotNeedle(int value, byte ms_delay);
void analogMeter();
void rgb2cmyk(uint8_t R, uint8_t G, uint8_t B);
void getTouch();


void setup(void) {
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

  updateTime = millis();
  rgb2cmyk(80,30,50);
  Serial.println(" ");
  Serial.println("MCU started.");

  WiFi.begin(SSID,PASSWD);
    //...e aguardamos até que esteja concluída.
    while (WiFi.status() != WL_CONNECTED) {

        delay(1000);

    }
    Serial.println("Wifi started.");
}


void loop() {
  getTouch();
  if (updateTime <= millis()) {
    updateTime = millis() + 25; // limitador da velocidade de update
 
    //d += 4; if (d >= 360) d = 0; //usado para fazer a senoide de exemplo, será descartado
    
    //Será necessário uma função que receba o RGB para passar à função rgb2cmyk (TODO)
    rgb2cmyk(80,30,50);
    //renderiza os ponteiros vermelhos das colunas
    plotPointer(); // It takes aout 3.5ms to plot each gauge for a 1 pixel move, 21ms for 6 gauges
     
    //renderiza o ponteiro analógico
    //plotNeedle(value[0], 0); // It takes between 2 and 12ms to replot the needle with zero delay
    plotNeedle(vol_in_ml, 10); //TODO: testar com 10 no segundo parâmetro
  }
}

void getTouch(){
  //TODO: implementar os meters
  uint16_t x,y = 0;
  if (tft.getTouch(&x,&y)){
    // 320 é Y, 240 é X
    //se x <= 50 & y <= 100
    if (x <= FRAME_W && y <= FRAME_H){
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString(print_str, 120, 70, 4); 
      vol_in_ml = vol_in_ml >14 ? vol_in_ml-5 : vol_in_ml;
    }
    //x >= 240-50 & y <= 100
    else if (x >= ML_PLUS_X && (y <= FRAME_H)){
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString(print_str, 120, 70, 4); 
      vol_in_ml = vol_in_ml < 96 ? vol_in_ml+5 : vol_in_ml;
    }
      //tft.fillCircle(x, y, 2, TFT_BLACK);
      Serial.println(x);
      Serial.println(y);
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
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
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

  plotNeedle(0, 0); // Ponteiro começa no 0, simples desse jeito
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
    tft.setTextColor(TFT_BLACK);
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
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
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
  
  tft.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
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
    }
  }
}
