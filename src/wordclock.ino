#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

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

SemaphoreHandle_t tx_msg_sema;

String wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:       return "Connected";
    case WL_CONNECT_FAILED:  return "Invalid Key";
    case WL_NO_SSID_AVAIL:
    case WL_CONNECTION_LOST: return "Invalid SSID";
    case WL_DISCONNECTED: 
    default:                 return "Disconnected";
    
  }
}

typedef struct WordClockState {
  String ssid;
  String key; 
  wl_status_t conn;
  IPAddress ip;
};

WordClockState state;

//////////////////////////////////////////////
// Wifi SSID/KEY EEPROM
//////////////////////////////////////////////

// Instantiate eeprom objects with parameter/argument names and sizes
EEPROMClass  wifiCredSSID("wifissid");
EEPROMClass  wifiCredKey("wifikey");

void wifiCredEEPROMInit() {
  //SSID (Max 32Bytes)
  if (!wifiCredSSID.begin(0x20)) Serial.println("EEPROM - Failed to initialise wifiCredSSID");
  //Key  (Max 63Bytes)
  if (!wifiCredKey.begin(0x40))  Serial.println("EEPROM - Failed to initialise wifiCredKey");
}

void wifiCredEEPROMLoad() {
    //SSID (Max 32Bytes)
    wifiCredSSID.get(0,state.ssid);
    //Key  (Max 63Bytes)
    wifiCredKey.get(0,state.key);
}

void wifiCredEEPROMStore() {
    //SSID (Max 32Bytes)
    wifiCredSSID.writeString(0,state.ssid);
    wifiCredSSID.commit();
    //Key  (Max 63Bytes)
    wifiCredKey.writeString(0,state.key);
    wifiCredKey.commit();
}

void wifiCredEEPROMErase() {
    Serial.println("!!! NVME Erasing... !!!");

    //SSID (Max 32Bytes)
    for( int i=0; i<wifiCredSSID.length(); i++) wifiCredSSID.writeByte(i,0);
    wifiCredSSID.end();

    //Key  (Max 63Bytes)
    for( int i=0; i<wifiCredKey.length(); i++)  wifiCredKey.writeByte(i,0);
    wifiCredKey.end();

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    Serial.println("!!! NVME Erased !!!");
    ESP.restart();
}

//////////////////////////////////////////////
// Wifi AP
//////////////////////////////////////////////
void wifiAPInit() {

  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.print("AP Started with IP: ");
  Serial.println(WiFi.softAPIP());

}

//////////////////////////////////////////////
// Wifi STA
//////////////////////////////////////////////
void wifiSTAInit() {

  //wifiCredEEPROMLoad();
  Serial.println("STA - cred: ");
  Serial.println(state.ssid.c_str());
  Serial.println(state.key.c_str());
  WiFi.disconnect();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  state.conn = WiFi.begin(state.ssid.c_str(), state.key.c_str());
  WiFi.setAutoReconnect(true);

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
    char tx_msg_str[1024];
    state.conn = WiFi.status();
    Serial.print("STA - State: ");
    Serial.print(state.conn);
    Serial.print(" - ");
    Serial.println(wl_status_to_string(state.conn));
    if( ws_client_cnt > 0) {
      state.ip = WiFi.localIP();
      if( xSemaphoreTake( tx_msg_sema, portMAX_DELAY ) == pdTRUE ) {
        tx_msg.clear();
        //memset(tx_msg_str, 0, sizeof tx_msg_str);
        //tx_msg_str[0] = '\0';
        tx_msg["type"] = "stat";
        tx_msg["payld"]["tz"]   = "??";
        tx_msg["payld"]["time"] = "??";
        tx_msg["payld"]["ssid"] = state.ssid.c_str();
        tx_msg["payld"]["conn"] = wl_status_to_string(state.conn);
        tx_msg["payld"]["ip"]   = state.ip.toString();
        serializeJson(tx_msg, tx_msg_str);
        wsNotifyClients(tx_msg_str);
        xSemaphoreGive(tx_msg_sema);
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void wsSSIDScan(void * parameter){
  int n;

  if (!ssid_scan_pend) {
    ssid_scan_pend = true;

    if( xSemaphoreTake( tx_msg_sema, portMAX_DELAY ) == pdTRUE ) {

      // If connection has failed, we need to disable WiFi before rescanning
      // appears to be a known bug https://github.com/espressif/arduino-esp32/issues/3294
      if (!(state.conn == WL_CONNECTED) || (state.conn == WL_DISCONNECTED)) {
        WiFi.disconnect();
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
        
      char tx_msg_str[1024];
      n = min(5, (int)(WiFi.scanNetworks()));
      tx_msg.clear();
      //memset(tx_msg_str, 0, sizeof tx_msg_str);
      //tx_msg_str[0] = '\0';
      tx_msg["type"] = "scan";
      if (n == 0) {
        tx_msg["payld"][0] = "None";
      } else {
        for (int i = 0; i < n; ++i) {
          tx_msg["payld"][i] = WiFi.SSID(i);
        }
      } 

      serializeJson(tx_msg, tx_msg_str);
      wsNotifyClients(tx_msg_str);
      xSemaphoreGive(tx_msg_sema);
    }
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
      const char* payld_type = rx_msg["payld"]["type"];
      if (strcmp(payld_type, "wifi") == 0) {   // WIFI Setting Update
        String ssid = rx_msg["payld"]["values"][0];
        String key  = rx_msg["payld"]["values"][1];
        state.ssid = ssid;
        state.key  = key;
        wifiCredEEPROMStore();
        wifiSTAInit();
      }
      else if (strcmp(payld_type, "tz") == 0) { // Timezone Setting Update
        Serial.println("Timezone Update Not Yet Supported!");
      }
      else {
        Serial.print("Invalid Ws Update Msg Payload Type: ");
        Serial.println(payld_type);
      }
      
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

  // Tx Msg Semaphore
  tx_msg_sema= xSemaphoreCreateBinary();
  if ( tx_msg_sema != NULL )
      xSemaphoreGive( tx_msg_sema );
}

void setup() {
  pinMode(0, INPUT); // Use BOOT Button for NVME reset, 0=Pressed, 1=Not
  Serial.begin(115200);

  wifiCredEEPROMInit();

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
     4096,             // Stack size (bytes)
     NULL,             // Parameter to pass
     1,                // Task priority
     NULL              // Task handle
  );

  // Try to connect to STA
  wifiCredEEPROMLoad();
  wifiSTAInit();

}

int reset_cnt;
void loop() {

  // If BOOT button held for 10seconds reset NVME
  reset_cnt = 0;
  while ( !digitalRead(0) ) {
    reset_cnt++;
    if ( reset_cnt >= 100 ) {
        wifiCredEEPROMErase();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
   
  ws.cleanupClients();
}
