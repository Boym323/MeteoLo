#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

int CasSpanku = 60; // cas v sekundách
const char* ssid = "Home";
const char* password = "1234567890";

// připojení knihoven
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <BH1750.h> // knihovna luxmetr
BH1750 lightMeter;

char server [] = "boym.cz"; //URL adresa serveru


const int pinCidlaDS = 4; // nastavení čísla vstupního pinu pro OneWire

OneWire oneWireDS(pinCidlaDS); // vytvoření instance oneWireDS z knihovny OneWire

DallasTemperature senzoryDS(&oneWireDS); // vytvoření instance senzoryDS z knihovny DallasTemperature


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

  // pauza pro přehlednější výpis
  // wait for WiFi connection
  if (client.connect(server, 80)) {
    delay(1000);
    senzoryDS.requestTemperatures();   // načtení informací ze všech připojených čidel na daném pinu
    int sensorValue = analogRead(A0); // čtení baterie
    float Voltage = sensorValue * (5.0 / 1023.0); // čtení baterie

    String url = "/logger.php";
    String url1 = "?Teplota_1=";
    float url2 = senzoryDS.getTempCByIndex(0);
    String url3 = "&Teplota_2=";
    float url4 = senzoryDS.getTempCByIndex(1);
    String url5 = "&VoltageBat=";
    float url6 = Voltage;
    String host = "boym.cz";


    client.print(String("GET ") + url + url1 + url2 + url3 + url4 + url5 + url6 + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");


    Serial.println("[Response:]");   // není potřeba jenom pro ladění
    while (client.connected())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  } // až po sem není třeba

  ESP.deepSleep(CasSpanku*1000000);
}
