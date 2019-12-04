#include <WiFi.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "SWS_free";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

//Web Server address to read/write from 
const char *host = "https://api.netatmo.com/api/getstationsdata?access_token=581b6d82e8ede1964c8b462d|f3f0749ba919fa7742579f591b833a2a&device_id=70:ee:50:1f:47:12";

void setup() 
{
  // We start by connecting to a WiFi network
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid); //  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop() 
{

  HTTPClient http;    //Declare object of class HTTPClient
  Serial.print("Request Link:");
  Serial.println(host); //Specify the URL and certificate
  
  http.begin(host);     //Specify request destination
  
  int httpCode = http.GET();            //Send the request
  String payload = http.getString();    //Get the response payload from server

  Serial.print("Response Code:"); //200 is OK
  Serial.println(httpCode);   //Print HTTP return code

  Serial.print("Returned data from Server:");
  Serial.println(payload);    //Print request response payload

if(httpCode == 200)
   {
    // Allocate JsonBuffer
    // Use arduinojson.org/assistant to compute the capacity.
    const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 21;
    DynamicJsonDocument doc(capacity);
  
   // Parse JSON object
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
   
const char* sensor = doc["sensor"]; // "gps"
long time = doc["time"]; // 1351824120

float data_0 = doc["data"][0]; // 48.75608
float data_1 = doc["data"][1]; // 2.302038

Serial.println(F("Response:"));
Serial.println(sensor); 
Serial.println(time); 
Serial.println(data_0); 
Serial.println(data_1); 
  }
  else
  {
    Serial.println("Error in response");
  }

  http.end();  //Close connection
  
  delay(10000);  //GET Data at every 5 seconds
}