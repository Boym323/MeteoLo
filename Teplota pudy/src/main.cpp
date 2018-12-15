#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266httpUpdate.h>

int CasHttp = 60; // cas v sekundách
int CasNacteniTeploty = 10; // cas v sekundách
int CasOTA = 60; // cas v sekundách

unsigned long PosledniTemp = 0;
unsigned long PosledniHTTP = 0;
unsigned long PosledniOTA = 0;

const char* ssid = "Home";
const char* password = "1234567890";

// připojení knihoven
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>

char server [] = "pomykal.eu"; //URL adresa serveru

const int pinCidlaDS = 4; // nastavení čísla vstupního pinu pro OneWire

OneWire oneWireDS(pinCidlaDS); // vytvoření instance oneWireDS z knihovny OneWire

DallasTemperature senzoryDS(&oneWireDS); // vytvoření instance senzoryDS z knihovny DallasTemperature
// adresy 1-wire čidel//

 DeviceAddress tempSenzor100cm = {0x28, 0xAB, 0xA0, 0x77, 0x91, 0x11, 0x02, 0xB0};
 DeviceAddress tempSenzor50cm = {0x28, 0x89, 0x92, 0x77, 0x91, 0x11, 0x02, 0x6F};
 DeviceAddress tempSenzor20cm = {0x28, 0xA7, 0xE8, 0x77, 0x91, 0x09, 0x02, 0x80};
 DeviceAddress tempSenzor10cm = {0x28, 0xB9, 0x77, 0x77, 0x91, 0x14, 0x02, 0x75};
 DeviceAddress tempSenzor5cm = {0x28, 0xF4, 0xD0, 0x77, 0x91, 0x09, 0x02, 0x4D};
 DeviceAddress tempSenzorPrizemni5cm = {0x28, 0x30, 0xA4, 0x45, 0x92, 0x07, 0x02, 0x0B};

/*---------Proměnné-------------------------*/

float temp100cm;
float temp50cm;
float temp20cm;
float temp10cm;
float temp5cm;
float tempPrizemni5cm;

void setup(void) {

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

}

void loop ()
{
  WiFiClient client;

  // wait for WiFi connection
  if (client.connect(server, 80))
  {
    /* 1-wire sekce */ // načtení informací ze všech čidel na daném pinu dle adresy a uložení do promněných
    if (millis() > PosledniTemp + CasNacteniTeploty * 1000)
    {
      senzoryDS.requestTemperaturesByAddress(tempSenzor100cm);
      senzoryDS.requestTemperaturesByAddress(tempSenzor50cm);
      senzoryDS.requestTemperaturesByAddress(tempSenzor20cm);
      senzoryDS.requestTemperaturesByAddress(tempSenzor10cm);
      senzoryDS.requestTemperaturesByAddress(tempSenzor5cm);
      senzoryDS.requestTemperaturesByAddress(tempSenzorPrizemni5cm);

      temp100cm = senzoryDS.getTempC(tempSenzor100cm);
      temp50cm = senzoryDS.getTempC(tempSenzor50cm);
      temp20cm = senzoryDS.getTempC(tempSenzor20cm);
      temp10cm = senzoryDS.getTempC(tempSenzor10cm);
      temp5cm = senzoryDS.getTempC(tempSenzor5cm);
      tempPrizemni5cm = senzoryDS.getTempC(tempSenzorPrizemni5cm);

      Serial.println("[Načtení teploty z čidel]");
      PosledniTemp = millis();
    }
    /*konec 1-wire sekce*/
    else if (millis() > PosledniHTTP + CasHttp * 1000)
    {

      String url = "meteo/logger.php";
      String url1 = "?Teplota_100=";
      String url2 = "&Teplota_50=";
      String url3 = "&Teplota_20=";
      String url4 = "&Teplota_10=";
      String url5 = "&Teplota_5=";
      String url6 = "&Teplota_prizemni=";

      String host = "pomykal.eu";

      client.print(String("GET ") + url + url1 + temp100cm + url2 + temp50cm + url3 + temp20cm + url4 + temp10cm + url5 + temp5cm + url6 + tempPrizemni5cm + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      Serial.println("Odeslaná teplota skrze HTTP");
      
      while (client.connected())
      {
        if (client.available())
        {
          Serial.println("Response:");
          String line = client.readStringUntil('\n');
          Serial.println(line);
        }
      }
      PosledniHTTP = millis();
   
    }
    else if (millis() > PosledniOTA + CasOTA * 1000)
    {

      t_httpUpdate_return ret = ESPhttpUpdate.update("boym.cz", 80, "/esp/update/update.php", "");
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.println("[update] Update failed.");
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("[update] Update no Update.");
          break;
        case HTTP_UPDATE_OK:
          Serial.println("[update] Update ok."); // may not called we reboot the ESP
          break;
      }
      PosledniOTA = millis();
    }
  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  }
}
