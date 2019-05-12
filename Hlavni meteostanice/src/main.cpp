#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <NTPClient.h> //NTP
#include <WiFiUdp.h> //NTP

int CasHttp = 300; // cas v sekundách
int CasNacteniTeploty = 60; // cas v sekundách

unsigned long PosledniTemp;
unsigned long PosledniHTTP;

const char* host = "mainstation";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const char* ssid = "Home";
const char* password = "1234567890";

WiFiUDP ntpUDP; //NTP
NTPClient timeClient(ntpUDP, "192.168.1.1", 3600, 60000); //NTP

char server [] = "pomykal.eu"; //URL adresa serveru

const int pinCidlaDS = 4; // nastavení čísla vstupního pinu pro OneWire

OneWire oneWireDS(pinCidlaDS); // vytvoření instance oneWireDS z knihovny OneWire

DallasTemperature senzoryDS(&oneWireDS); // vytvoření instance senzoryDS z knihovny DallasTemperature

float temp100cm;
float temp50cm;
float temp20cm;
float temp10cm;
float temp5cm;
float tempPrizemni5cm;
float temp200cm;

void setup() {

  // komunikace přes sériovou linku rychlostí 115200 baud
  Serial.begin(115200);
  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();

  Wire.begin(); //inicializace I2C sběrnice

  WiFi.begin(ssid, password); // wifi s heslem

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(__TIME__);
  Serial.println(__DATE__);

  //OTA sekce

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80); //OTA
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host); //OTA

  timeClient.begin(); //NTP
}


void teplota ()
{
  // adresy 1-wire čidel

  DeviceAddress Senzor100cm = {0x28, 0xAB, 0xA0, 0x77, 0x91, 0x11, 0x02, 0xB0};
  DeviceAddress Senzor50cm = {0x28, 0x89, 0x92, 0x77, 0x91, 0x11, 0x02, 0x6F};
  DeviceAddress Senzor20cm = {0x28, 0xA7, 0xE8, 0x77, 0x91, 0x09, 0x02, 0x80};
  DeviceAddress Senzor10cm = {0x28, 0xB9, 0x77, 0x77, 0x91, 0x14, 0x02, 0x75};
  DeviceAddress Senzor5cm = {0x28, 0xF4, 0xD0, 0x77, 0x91, 0x09, 0x02, 0x4D};
  DeviceAddress SenzorPrizemni5cm = {0x28, 0x30, 0xA4, 0x45, 0x92, 0x07, 0x02, 0x0B};
  DeviceAddress Senzor200cm = {0x28, 0x32, 0xD9, 0x35, 0x05, 0x00, 0x00, 0xCC};
  /*---------Proměnné-------------------------*/
  /*nastavení rozlišení čidel 9 bit  - 0,5°C
                        10 bit - 0,25°C
                        11 bit - 0,125°C
                        12 bit - 0,0625°C
  */

  senzoryDS.setResolution(Senzor100cm, 10);
  senzoryDS.setResolution(Senzor50cm, 10);
  senzoryDS.setResolution(Senzor20cm, 10);
  senzoryDS.setResolution(Senzor10cm, 10);
  senzoryDS.setResolution(Senzor5cm, 10);
  senzoryDS.setResolution(SenzorPrizemni5cm, 10);
  senzoryDS.setResolution(Senzor200cm, 10);

  float tempZcidla100cm;
  float tempZcidla50cm;
  float tempZcidla20cm;
  float tempZcidla10cm;
  float tempZcidla5cm;
  float tempZcidlaPrizemni5cm;
  float tempZcidla200cm;
  /* 1-wire sekce */ // načtení informací ze všech čidel na daném pinu dle adresy a uložení do promněných
  senzoryDS.requestTemperaturesByAddress(Senzor100cm);
  senzoryDS.requestTemperaturesByAddress(Senzor50cm);
  senzoryDS.requestTemperaturesByAddress(Senzor20cm);
  senzoryDS.requestTemperaturesByAddress(Senzor10cm);
  senzoryDS.requestTemperaturesByAddress(Senzor5cm);
  senzoryDS.requestTemperaturesByAddress(SenzorPrizemni5cm);
  senzoryDS.requestTemperaturesByAddress(Senzor200cm);

  tempZcidla100cm = senzoryDS.getTempC(Senzor100cm);
  tempZcidla50cm = senzoryDS.getTempC(Senzor50cm);
  tempZcidla20cm = senzoryDS.getTempC(Senzor20cm);
  tempZcidla10cm = senzoryDS.getTempC(Senzor10cm);
  tempZcidla5cm = senzoryDS.getTempC(Senzor5cm);
  tempZcidlaPrizemni5cm = senzoryDS.getTempC(SenzorPrizemni5cm);
  tempZcidla200cm = senzoryDS.getTempC(Senzor200cm);

  // validace teplot, teplota se do promněné uloží pouze, když je v rozsahu -50 - 70°C

  if ((-50 < tempZcidla100cm) && (tempZcidla100cm < 70)) temp100cm = tempZcidla100cm;
  if ((-50 < tempZcidla50cm) && (tempZcidla50cm < 70)) temp50cm = tempZcidla50cm;
  if (( -50 < tempZcidla20cm) && (tempZcidla20cm < 70)) temp20cm = tempZcidla20cm;
  if (( -50 < tempZcidla10cm) && (tempZcidla10cm < 70)) temp10cm = tempZcidla10cm;
  if (( -50 < tempZcidla5cm) && (tempZcidla5cm < 70)) temp5cm = tempZcidla5cm;
  if (( -50 < tempZcidlaPrizemni5cm) && (tempZcidlaPrizemni5cm < 70)) tempPrizemni5cm = tempZcidlaPrizemni5cm;
  if (( -50 < tempZcidla200cm) && (tempZcidla200cm < 70)) temp200cm = tempZcidla200cm;

  Serial.println("[Načtení teploty z čidel]");
  PosledniTemp = millis();
}

void http_push()
{
  WiFiClient client;
  // wait for WiFi connection
  if (client.connect(server, 80))
  {
    String url = "meteo/logger.php";
    String url1 = "?Teplota_100=";
    String url2 = "&Teplota_50=";
    String url3 = "&Teplota_20=";
    String url4 = "&Teplota_10=";
    String url5 = "&Teplota_5=";
    String url6 = "&Teplota_prizemni=";
    String url7 = "&Teplota_200=";
    String host = "pomykal.eu";

    client.print(String("GET ") + url + url1 + temp100cm + url2 + temp50cm + url3 + temp20cm + url4 + temp10cm + url5 + temp5cm + url6 + tempPrizemni5cm + url7 + temp200cm + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Odeslaná teplota skrze HTTP");

    while (client.connected())
    {
      if (client.available())
      {
        Serial.println("[Response:]");
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    PosledniHTTP = millis()+61000;;
  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  }
}

void loop ()
{
  httpServer.handleClient(); //OTA
  MDNS.update(); //OTA
  timeClient.update(); //NTP

  if (millis() > PosledniTemp + CasNacteniTeploty * 1000)
  {
    teplota ();
  }

  else if (timeClient.getMinutes() % 5 == 0 && timeClient.getSeconds() == 00 && (millis() > PosledniHTTP))
  {
    http_push();
  }
}