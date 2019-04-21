#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

int CasHttp = 300; // cas v sekundách
int CasNacteniTeploty = 150; // cas v sekundách

unsigned long PosledniTemp = 0;
unsigned long PosledniHTTP = 0;  

const char* host = "mainstation";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

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

void setup() {


  
   // komunikace přes sériovou linku rychlostí 115200 baud
  Serial.begin(115200);
  // zapnutí komunikace knihovny s teplotním čidlem
  senzoryDS.begin();
  senzoryDS.setResolution(tempSenzor100cm, 10);
  senzoryDS.setResolution(tempSenzor50cm, 10);
  senzoryDS.setResolution(tempSenzor20cm, 10);
  senzoryDS.setResolution(tempSenzor10cm, 10);
  senzoryDS.setResolution(tempSenzor5cm, 10);
  senzoryDS.setResolution(tempSenzorPrizemni5cm, 10);
  
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

    MDNS.addService("http", "tcp", 80);
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

}

void loop ()
{
  WiFiClient client;

  // wait for WiFi connection
  if (client.connect(server, 80))
  {
     httpServer.handleClient();
     MDNS.update();
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
          Serial.println("[Response:]");
          String line = client.readStringUntil('\n');
          Serial.println(line);
        }
      }
      PosledniHTTP = millis();
   
    }


  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  }
}
