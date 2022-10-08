#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>

// Index HTML Minified and converted into a index_html const string.
#include "index.h"

// Replace with your network credentials
const char* ap_ssid = "Wordclock";
const char* ap_password = "helloworld";

// Create DNS Server for captive portal
DNSServer dnsServer;

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
  String      ssid;
  String      key; 
  wl_status_t conn;
  IPAddress   ip;
  String      tz;
  String      tz_enc;
  struct tm   time;
  bool        time_vld;
};

WordClockState state;

//////////////////////////////////////////////
// NTP
//////////////////////////////////////////////

const char*  ntpServer = "pool.ntp.org";
const String tz_default     = "GMT";
const String tz_enc_default = "GMT0";

void ntpInit( ) {
  configTzTime(state.tz_enc.c_str(), ntpServer);
}

void ntpUpdate() {
 if(!getLocalTime(&state.time)){
   state.time_vld = false;
 }
 state.time_vld = true;
}

String ntpTimeStr() {
 char timeStringBuff[50];
 if(!state.time_vld){
   return "??";
 }
 //strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &state.time);
 strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &state.time);
 String asString(timeStringBuff);
 return timeStringBuff;
}


//////////////////////////////////////////////
// Wifi SSID/KEY EEPROM
//////////////////////////////////////////////

// Instantiate eeprom objects with parameter/argument names and sizes
const int eeprom_ssid_len   = 32;
const int eeprom_key_len    = 64;
const int eeprom_tz_len     = 32;
const int eeprom_tz_enc_len = 64;
const int eeprom_len        = eeprom_ssid_len + eeprom_key_len + eeprom_tz_len + eeprom_tz_enc_len;

const int eeprom_ssid_addr   = 0;
const int eeprom_key_addr    = eeprom_ssid_addr + eeprom_ssid_len;
const int eeprom_tz_addr     = eeprom_key_addr  + eeprom_key_len;
const int eeprom_tz_enc_addr = eeprom_tz_addr   + eeprom_tz_len; 

void EEPROMInit() {
  if (!EEPROM.begin(512)) Serial.println("EEPROM - Failed to initialise");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  Serial.println("EEPROM - Initialised EEPROM");
}

void wifiCredEEPROMLoad() {
    EEPROM.get(eeprom_ssid_addr, state.ssid); // SSID
    EEPROM.get(eeprom_key_addr,  state.key);  // KEY

    Serial.print("EEPROM - SSID  : ");
    Serial.println(state.ssid);
    Serial.print("EEPROM - KEY   : ");
    Serial.println(state.key);
}

void wifiCredEEPROMStore() {
    EEPROM.put(eeprom_ssid_addr, state.ssid); // SSID
    EEPROM.put(eeprom_key_addr,  state.key);  // KEY
    EEPROM.commit();
}

void tzEEPROMLoad() {
    EEPROM.get(eeprom_tz_addr,     state.tz);     // TZ
    EEPROM.get(eeprom_tz_enc_addr, state.tz_enc); // TZ_ENC

    // Set Defaults if either have a 0 byte as first byte.
    if ( ((char)state.tz[0]     == 0) ||
         ((char)state.tz_enc[0] == 0)   ) {
      state.tz     = tz_default;
      state.tz_enc = tz_enc_default;
    }

    Serial.print("EEPROM - TZ    : ");
    Serial.println(state.tz);
    Serial.print("EEPROM - TZ Enc: ");
    Serial.println(state.tz_enc);

}

void tzEEPROMStore() {
    EEPROM.put(eeprom_tz_addr,     state.tz);     // TZ
    EEPROM.put(eeprom_tz_enc_addr, state.tz_enc.c_str()); // TZ_ENC
    EEPROM.commit();
}

void EEPROMErase() {
    Serial.println("!!! NVME Erasing... !!!");

    //SSID (Max 32Bytes)
    for( int i=0; i<EEPROM.length(); i++) EEPROM.write(i,0);
    EEPROM.end();

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
// WIFI AP - Captive Portal
//////////////////////////////////////////////

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html); 
    //request->send_P(200, "text/html", index_html); 
  }
};

//////////////////////////////////////////////
// Wifi STA
//////////////////////////////////////////////
void wifiSTAInit() {

  Serial.print("STA - SSID : ");
  Serial.println(state.ssid.c_str());
  Serial.print("STA - KEY  : ");
  Serial.println(state.key.c_str());
  WiFi.disconnect();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  state.conn = WiFi.begin(state.ssid.c_str(), state.key.c_str());
  WiFi.setAutoReconnect(true);

}

/////////////////////////////////////////////
// HTML Handling
/////////////////////////////////////////////

void serverInit() {

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

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
    ntpUpdate();
    //Serial.print("STA - State: ");
    //Serial.print(state.conn);
    //Serial.print(" - ");
    //Serial.println(wl_status_to_string(state.conn));
    if( ws_client_cnt > 0) {
      state.ip = WiFi.localIP();
      if( xSemaphoreTake( tx_msg_sema, portMAX_DELAY ) == pdTRUE ) {
        tx_msg.clear();
        //memset(tx_msg_str, 0, sizeof tx_msg_str);
        //tx_msg_str[0] = '\0';
        tx_msg["type"] = "stat";
        tx_msg["payld"]["tz"]   = state.tz;
        tx_msg["payld"]["time"] = ntpTimeStr();
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

void wsSSIDScan(void * parameter) {
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
        String tz      = rx_msg["payld"]["values"][0];
        String tz_enc  = rx_msg["payld"]["values"][1];
        state.tz     = tz;
        state.tz_enc = tz_enc;
        tzEEPROMStore();
        ntpInit();
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

  // Initialize EEPROM and fetch values
  EEPROMInit();
  wifiCredEEPROMLoad();
  tzEEPROMLoad();

  // Initialise WiFi AP, Websocket Server, HTML Server
  wifiAPInit();
  wsInit();
  serverInit(); 
  
  // Initialise DNS Server for captive portal.
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Start HTML Server
  server.begin();

  xTaskCreate(
     wsStatusUpdate,   // Function that should be called
     "wsStatusUpdate", // Name of the task (for debugging)
     4096,             // Stack size (bytes)
     NULL,             // Parameter to pass
     1,                // Task priority
     NULL              // Task handle
  );

  // Initialise WiFi Station, try to connect to WiFi
  wifiSTAInit();
  // Initialise NTP CLient
  ntpInit();

}

int reset_cnt;
void loop() {

  // If BOOT button held for 10seconds reset NVME
  reset_cnt = 0;
  while ( !digitalRead(0) ) {
    reset_cnt++;
    if ( reset_cnt >= 100 ) {
        EEPROMErase();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
 
  dnsServer.processNextRequest();  
  ws.cleanupClients();
}
