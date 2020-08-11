#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include <SFE_BMP180.h>
#include <EasyNTPClient.h> //NTP
#include <WiFiUdp.h>       //NTP

#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);

const char *mqttServer = "192.168.1.2";
const int mqttPort = 1883;
const char *mqttUser = "mqtt username";
const char *mqttPassword = "mqtt password";

WiFiUDP udp;                                 // NTP
EasyNTPClient ntpClient(udp, "192.168.1.1"); //

#define I2C_SCL 12
#define I2C_SDA 13
SFE_BMP180 pressure;
#define ALTITUDE 425 // nadmořská výška stanice

Adafruit_Si7021 sensor = Adafruit_Si7021();

int CasDat = 60;   // cas v sekundách
int CasHttp = 300; // cas v sekundách

unsigned long PosledniHTTP;
unsigned long PosledniDataMeteotemplate;

double T, P, p0, a; // BMP180 T - teplota. P - tlak, P0 - tlak u hladiny moře

const char *host = "mainstation";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const char *ssid = "Home";
const char *password = "1234567890";

char server[] = "pomykal.eu"; //URL adresa serveru
char serverMeteotemplate[] = "pocasi-loucka.eu";
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
float OutHumidity;

void setup()
{
  // komunikace přes sériovou linku rychlostí 115200 baud
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL); //inicializace I2C sběrnice, SDA pin 13 - D7 ,SCL pin 12 - D6
  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();
  pressure.begin(); //BMP180

  WiFi.begin(ssid, password); // wifi s heslem

  while (WiFi.status() != WL_CONNECTED)
  {
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

  MDNS.addService("http", "tcp", 80);                                                           //OTA
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host); //OTA
}

void teplota()
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

  if ((-50 < tempZcidla100cm) && (tempZcidla100cm < 70))
    temp100cm = tempZcidla100cm;
  if ((-50 < tempZcidla50cm) && (tempZcidla50cm < 70))
    temp50cm = tempZcidla50cm;
  if ((-50 < tempZcidla20cm) && (tempZcidla20cm < 70))
    temp20cm = tempZcidla20cm;
  if ((-50 < tempZcidla10cm) && (tempZcidla10cm < 70))
    temp10cm = tempZcidla10cm;
  if ((-50 < tempZcidla5cm) && (tempZcidla5cm < 70))
    temp5cm = tempZcidla5cm;
  if ((-50 < tempZcidlaPrizemni5cm) && (tempZcidlaPrizemni5cm < 70))
    tempPrizemni5cm = tempZcidlaPrizemni5cm;
  if ((-50 < tempZcidla200cm) && (tempZcidla200cm < 70))
    temp200cm = tempZcidla200cm;

  Serial.println("Načtení teploty z čidel");
}

void vlhkost()
{

  OutHumidity = sensor.readHumidity();

  Serial.println("Načtení vlhkosti");
}

void tlak()
{
  char status;

  status = pressure.startTemperature();
  delay(status);
  status = pressure.getTemperature(T);
  status = pressure.startPressure(3);
  delay(status);
  status = pressure.getPressure(P, T);
  p0 = pressure.sealevel(P, ALTITUDE); // we're at 1655 meters (Boulder, CO)
}

void http_meteotemplate()
{
  WiFiClient client;
  // wait for WiFi connection
  if (client.connect(serverMeteotemplate, 80))
  {
    String page = "/api.php";
    String web = "pocasi-loucka.eu";
    String hesloAPI = "fWhdtbA3";

    client.print(String("GET ") + page +
                 "?U=" + ntpClient.getUnixTime() +
                 "&T=" + temp200cm +
                 "&H=" + OutHumidity +
                 "&P=" + p0 +
                 "&T1=" + tempPrizemni5cm +
                 "&TS1=" + temp5cm + "&TSD1=5" +
                 "&TS2=" + temp10cm + "&TSD2=10" +
                 "&TS3=" + temp20cm + "&TSD3=20" +
                 "&TS4=" + temp50cm + "&TSD4=50" +
                 "&TS5=" + temp100cm + "&TSD5=100" +
                 "&PASS=" + hesloAPI +

                 " HTTP/1.1\r\n" +
                 "Host: " + web + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Odeslaná teplota skrze HTTP do meteotemplate");

    while (client.connected())
    {
      if (client.available())
      {
        Serial.println("Response:");
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
  }
  else
  {
    Serial.println("connection failed meteotemplate!");
    client.stop();
  }
}

void mqtt()
{
  client.setServer(mqttServer, mqttPort);
  client.connect("ESP32Client", mqttUser, mqttPassword);
  int waitmqqt = 50;
  client.publish("meteostanice/extraTemp1", String(temp200cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/outHumidity", String(OutHumidity).c_str(), true);
  //delay(waitmqqt);
  //client.publish("meteostanice/pressure", String(P).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/extraTemp2", String(tempPrizemni5cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/extraTemp3", String(temp5cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/soilTemp1", String(temp10cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/soilTemp2", String(temp20cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/soilTemp3", String(temp50cm).c_str(), true);
  delay(waitmqqt);
  client.publish("meteostanice/soilTemp4", String(temp100cm).c_str(), true);
}


void loop()
{
  httpServer.handleClient(); //OTA
  MDNS.update();             //OTA

  if (millis() > PosledniDataMeteotemplate + CasDat * 1000)
  {
    teplota();
    vlhkost();
    tlak();
    http_meteotemplate();
    mqtt();
    PosledniDataMeteotemplate = millis();
  }
}