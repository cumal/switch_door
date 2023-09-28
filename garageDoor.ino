#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <user_interface.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>

const char *ssid = "[EXPOSED_WIFI_NAME]"; // wifi ssid
const char *password = "[WIFI_PASSW]"; // wifi passw
const char* DomainName = "[DOMAIN]";  // set domain name domain.local
const byte DNS_PORT = 53;

//DEFINITIONS
//#define LED1pin 2 //D4
#define LED2pin 16 //D0 board pin
#define RelPin 4 //D2

//VARIABLES
bool pulseDoor = false;
//String responseHTML = ""
//  "<!DOCTYPE html><html><head><title>Captive Portal</title>"
//  "<style>html {font-family: Arial; text-align: center; background-color: DimGray;}"
//  "body {margin-top: 50px; min-height: 10vh;}"
//  "button {background-color: #1abc9c;border: 2px solid black;color: black;padding: 10px 30px;text-decoration: none;font-size: 20px;border-radius: 4px;margin: 50px 40px 20px;}"
//  "a {font-size: 2.0rem; text-decoration:none; decoration:none; color:Goldenrod}"
//  "</style></head><body>"
//  "<a href=\"http://garaje.local\">Home Page</a></p>"
//  "<form action=\"http://garaje.local/opendoor\" method=\"post\"><button type=\"submit\">Abrir puerta</button></form>"
//  "</body></html>" ;

/* Put IP Address details */
IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
HTTPClient http;
DNSServer dnsServer;

String processor(const String &var){
  return String();
}

void server_start(){
  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  while (!WiFi.softAP(ssid, password)){ delay(1000); }
  while (!MDNS.begin(DomainName, WiFi.softAPIP())){ delay(1000); }
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    //AsyncResponseStream *response = request->beginResponseStream("text/html");
    //response->print(responseHTML);
    //request->send(response);
    request->send(SPIFFS, "/index.html", String(), false);
  }
};

void setup(){
  if (!SPIFFS.begin()) {return;}

  pinMode(RelPin, OUTPUT);
  pinMode(LED2pin, OUTPUT);
  digitalWrite(LED2pin, HIGH);
  digitalWrite(RelPin, HIGH);
  
  server_start();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/opendoor", HTTP_POST, [](AsyncWebServerRequest *request) {
    pulseDoor = true;
    request->send(200, "text/plain", "OK");
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Not Found");
  });
  dnsServer.start(DNS_PORT, "*", local_ip);
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
  server.begin();
  MDNS.addService("http", "tcp", 80);
}

void loop(){
  MDNS.update();
  dnsServer.processNextRequest();
  
  if (WiFi.softAPgetStationNum() > 0) { digitalWrite(LED2pin, LOW); }
  else { digitalWrite(LED2pin, HIGH); }
  if (pulseDoor) {
    digitalWrite(RelPin, LOW);
    delay(100);
    digitalWrite(RelPin, HIGH);
    pulseDoor = false;
  }
  
}
