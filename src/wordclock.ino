#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Index HTML Minified and converted into a index_html const string.
#include "index.h"

// Replace with your network credentials
const char* ap_ssid = "wordclock";
const char* ap_password = "helloworld";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Number of currently connected clients
int  ws_client_cnt = 0;

// SSID Scan in progress, dont allow multiple
bool ssid_scan_pend = false;

DynamicJsonDocument rx_msg(512);
DynamicJsonDocument tx_msg(1024);

typedef struct WordClockState {
  String ssid;
  String key; 
  String conn;
};

WordClockState state;
//{'type':'stat', 'payld':STATE}
//STATE['tz']   = 'London (GMT)'
//STATE['time'] = None
//STATE['ssid'] = 'helloworldAP'
//STATE['conn'] = 'Connected'

//////////////////////////////////////////////
// Wifi SSID/KEY EEPROM
//////////////////////////////////////////////

//////////////////////////////////////////////
// Wifi AP
//////////////////////////////////////////////
void wifiAPInit() {

  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP Started with IP: ");
  Serial.println(WiFi.softAPIP());

}

/////////////////////////////////////////////
// Websocket Handling
/////////////////////////////////////////////
void wsNotifyClients(char * json) {
  Serial.print("TX: ");
  Serial.println(json);    
  ws.textAll(json);
}

void wsStatusUpdate(void * parameter) {
  for(;;){ // infinite loop
    if( ws_client_cnt > 0) {
      //ws.textAll("Status");
      Serial.println("Status");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void wsSSIDScan(void * parameter){
  char output[1024];
  int n;

  if (!ssid_scan_pend) {
    ssid_scan_pend = true;
    n = min(5, (int)(WiFi.scanNetworks()));

    tx_msg.clear();
    tx_msg["type"] = "scan";
    if (n == 0) {
      tx_msg['payld'][0] = "None";
    } else {
      for (int i = 0; i < n; ++i) {
        tx_msg["payld"][i] = WiFi.SSID(i);
      }
    } 

    serializeJson(tx_msg, output);
    wsNotifyClients(output);
    ssid_scan_pend = false;
  }
  vTaskDelete(NULL);
}


void wsRxParse(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.print("RX: ");
    Serial.println((char*)data);    
    deserializeJson(rx_msg, (char*)data);
    const char* type = rx_msg["type"];
    if (strcmp(type, "scan") == 0) {
      xTaskCreate( wsSSIDScan, "wsSSIDScan", 4096, NULL, 1, NULL );
    }
    else if (strcmp(type, "update") == 0) {
      Serial.println("Update");
    }
    else {
      Serial.print("Invalid Ws Msg Type: ");
      Serial.println(type);
    }
    rx_msg.clear();
  }
}

void wsRxEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ++ws_client_cnt;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      --ws_client_cnt;
      break;
    case WS_EVT_DATA:
      wsRxParse(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void wsInit() {
  ws.onEvent(wsRxEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  wifiAPInit();

  wsInit();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  // Start server
  server.begin();

  xTaskCreate(
     wsStatusUpdate,   // Function that should be called
     "wsStatusUpdate", // Name of the task (for debugging)
     1000,             // Stack size (bytes)
     NULL,             // Parameter to pass
     1,                // Task priority
     NULL              // Task handle
  );

}

void loop() {
  ws.cleanupClients();
}
