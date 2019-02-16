#include <Arduino.h>
#include "FreeRTOS.h"

#ifdef ESP8266 
  #include <ESP8266WiFi.h>
#else 
  #include <WiFi.h>
#endif

#include <ArduinoJson.h>
#include <MqttConnector.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

#include "init_mqtt.h"
#include "_publish.h"
#include "_receive.h"
#include "_config.h"

MqttConnector *mqtt; 

int relayPin = 15; 
int relayPinState = HIGH;
int LED_PIN = 2;
U8G2_ST7920_128X64_F_SW_SPI *u8g2;

String currentPayload = "";
String waitForInternet = "Wait for internet.";
String readyText = "Ready to render!";
bool isConnectToInternet = false;

void RenderLed( void * parameter ) {

  Serial.println("Run task one");

  int isLight;
  unsigned int hex;
  char currentHex;
  int count;
  String displayText = waitForInternet;
  for(;;)
  {
    if (currentPayload.length() != 2048)
    {
      if (isConnectToInternet)
      {
        displayText = readyText;
      }

      u8g2->firstPage();
      do
      {
        u8g2->setFont(u8g2_font_etl14thai_t);
        u8g2->setCursor(3, 20);
        u8g2->print(displayText);
      } while (u8g2->nextPage());

      continue;
    }

    u8g2->firstPage();
      do
      {
        u8g2->setFont(u8g2_font_etl14thai_t);
        u8g2->setCursor(0, 0);

        count = 0;
        for (int i = 0; i < 64 * 128; i+=4)
        {
          currentHex = currentPayload.charAt(count);
          
          hex = CharToHex(currentHex);

          for (int z = 3; z >= 0; z--)
          { 
            isLight = (hex & (0x01 << z)) >> z;

            if (isLight > 0)
            {
              u8g2->setDrawColor(1);
              u8g2->drawPixel((i + (3 - z)) % 128, (i + z) / 128);
            }
            else 
            {
              u8g2->setDrawColor(0);
              u8g2->drawPixel((i + (3 - z)) % 128, (i + z) / 128);
            }
          }
          count++; 
        }
      } while (u8g2->nextPage());

    vTaskDelay( pdMS_TO_TICKS( 500 ) );
  }

}

char myName[40];

void init_task()
{
  xTaskCreate(RenderLed,          /* Task function. */
                "TaskOne",        /* String with name of task. */
                50000,            /* Stack size in bytes. */
                NULL,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                NULL);            /* Task handle. */
}

unsigned char CharToHex( char c )
{
    switch( c ) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
    }
    return 16;
}

void init_u8g2()
{
  u8g2 = new U8G2_ST7920_128X64_F_SW_SPI(U8G2_R0, /* clock=*/ 18, /* MOSI=*/ 23, /* MISO=*/ 19);
  u8g2->begin();
  u8g2->enableUTF8Print();
}

void init_hardware()
{
  pinMode(relayPin, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(relayPin, relayPinState);;
  // serial port initialization
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Starting...");
}

IPAddress local_IP(192, 168, 1, 205);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 4, 4); //optional
IPAddress secondaryDNS(8, 8, 8, 8); //optional

void init_wifi() {
  WiFi.disconnect();
  delay(20);
  WiFi.mode(WIFI_STA);
  delay(20);
  // WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  delay(50);
  const char* ssid =  WIFI_SSID.c_str();
  const char* pass =  WIFI_PASSWORD.c_str();
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf ("Connecting to %s:%s\r\n", ssid, pass);
    delay(300);
  }
  isConnectToInternet = true;
  Serial.println("WiFi Connected.");
  digitalWrite(2, HIGH);
}

void setup()
{
  init_hardware();
  init_u8g2();
  init_task();
  init_wifi();
  init_mqtt();

  // vTaskStartScheduler();
}

void loop()
{
  mqtt->loop();
}
