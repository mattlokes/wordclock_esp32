#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

// Index HTML Minified and converted into a index_html const string.
#include "index.h"

// Replace with your network credentials
const char* ap_ssid = "wordclock";
const char* ap_password = "helloworld";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

int ws_client_cnt = 0;

String message = "";
//String sliderValue1 = "0";
//String sliderValue2 = "0";
//String sliderValue3 = "0";
//
//Json Variable to Hold Slider Values
//JSONVar sliderValues;
//
////Get Slider Values
//String getSliderValues(){
//  sliderValues["sliderValue1"] = String(sliderValue1);
//  sliderValues["sliderValue2"] = String(sliderValue2);
//  sliderValues["sliderValue3"] = String(sliderValue3);
//
//  String jsonString = JSON.stringify(sliderValues);
//  return jsonString;
//}

// Initialize WiFi
void initWiFiAP() {
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP Started with IP: ");
  Serial.println(WiFi.softAPIP());
}

void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    Serial.println(message);
    //if (message.indexOf("1s") >= 0) {
    //  sliderValue1 = message.substring(2);
    //  dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 255);
    //  Serial.println(dutyCycle1);
    //  Serial.print(getSliderValues());
    //  notifyClients(getSliderValues());
    //}
    //if (message.indexOf("2s") >= 0) {
    //  sliderValue2 = message.substring(2);
    //  dutyCycle2 = map(sliderValue2.toInt(), 0, 100, 0, 255);
    //  Serial.println(dutyCycle2);
    //  Serial.print(getSliderValues());
    //  notifyClients(getSliderValues());
    //}    
    //if (message.indexOf("3s") >= 0) {
    //  sliderValue3 = message.substring(2);
    //  dutyCycle3 = map(sliderValue3.toInt(), 0, 100, 0, 255);
    //  Serial.println(dutyCycle3);
    //  Serial.print(getSliderValues());
    //  notifyClients(getSliderValues());
    //}
    //if (strcmp((char*)data, "getValues") == 0) {
    //  notifyClients(getSliderValues());
    //}
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ++ws_client_cnt;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
      --ws_client_cnt;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  initWiFiAP();

  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  // Start server
  server.begin();

}

void loop() {
  ws.cleanupClients();
}
