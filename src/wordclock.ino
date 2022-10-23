#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>

#include "wordclockDisplay.h"

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

typedef struct EEPROMState {
  char        ssid   [32];
  char        key    [64]; 
  char        tz     [32];
  char        tz_enc [64];
};

typedef struct WordClockState {
  EEPROMState eeprom;
  wl_status_t conn;
  IPAddress   ip;
  struct tm   time;
  bool        time_vld;
};

WordClockState state;

//////////////////////////////////////////////
// Pixels (Display)
//////////////////////////////////////////////
const int pixel_pin  = 13; // GPIO13
const int pixel_w    = 16;
const int pixel_h    = 14;

wordclockDisplay pixels(pixel_w, pixel_h, pixel_pin);
uint32_t         pixels_buff [pixel_w*pixel_h];
wind_info_t      pixels_buff_win;

void pixelsInit() {
  // Setup Pixel Buffer Window Dimensions (full)
  pixels_buff_win.xMin = 0;
  pixels_buff_win.xMax = pixel_w-1;
  pixels_buff_win.yMin = 0;
  pixels_buff_win.yMax = pixel_h-1;
  //pixels_buff_win.bufferMode = true;

  // Setup Pixel Buffer Memory
  pixels.setWindowMemory(&pixels_buff_win, (color_t)pixels_buff, pixel_w*pixel_h);
  pixels.pCurrentWindow = &pixels_buff_win;

  // Clear Pixel Buffer Memory
  pixels.clear();
  
  pixels.begin();
  pixels.show();
}

//////////////////////////////////////////////
// NTP
//////////////////////////////////////////////

const char* ntpServer = "pool.ntp.org";
const char* tz_default     = "GMT";
const char* tz_enc_default = "GMT0";

void ntpInit( ) {
  configTzTime(state.eeprom.tz_enc, ntpServer);
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

void EEPROMInit() {
  if (!EEPROM.begin(sizeof(struct EEPROMState))) Serial.println("EEPROM - Failed to initialise");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  Serial.println("EEPROM - Initialised EEPROM");
}

void EEPROMLoad() {
    EEPROM.get(0, state.eeprom);

    // Set Defaults if either have a 0 byte as first byte.
    if ( (state.eeprom.tz[0]     == 0) ||
         (state.eeprom.tz_enc[0] == 0)   ) {
      strncpy(state.eeprom.tz,     tz_default,     sizeof(state.eeprom.tz));
      strncpy(state.eeprom.tz_enc, tz_enc_default, sizeof(state.eeprom.tz_enc));
    }

    Serial.print("EEPROM - SSID  : ");
    Serial.println(state.eeprom.ssid);
    Serial.print("EEPROM - KEY   : ");
    Serial.println(state.eeprom.key);
    Serial.print("EEPROM - TZ    : ");
    Serial.println(state.eeprom.tz);
    Serial.print("EEPROM - TZ Enc: ");
    Serial.println(state.eeprom.tz_enc);
}


void EEPROMStore() {
    EEPROM.put(0, state.eeprom); // SSID
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

  Serial.print("AP - SSID   : ");
  Serial.println(ap_ssid);
  Serial.print("AP - KEY    : ");
  Serial.println(ap_password);
  Serial.println("AP - STATUS : ENABLED ");
  Serial.print("AP - IP     : ");
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
  Serial.println(state.eeprom.ssid);
  Serial.print("STA - KEY  : ");
  Serial.println(state.eeprom.key);
  WiFi.disconnect();
  vTaskDelay(500 / portTICK_PERIOD_MS);
  state.conn = WiFi.begin(state.eeprom.ssid, state.eeprom.key);
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

void wsStatusUpdate() {
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
      tx_msg["type"] = "stat";
      tx_msg["payld"]["tz"]   = state.eeprom.tz;
      tx_msg["payld"]["time"] = ntpTimeStr();
      tx_msg["payld"]["ssid"] = state.eeprom.ssid;
      tx_msg["payld"]["conn"] = wl_status_to_string(state.conn);
      tx_msg["payld"]["ip"]   = state.ip.toString();
      serializeJson(tx_msg, tx_msg_str);
      wsNotifyClients(tx_msg_str);
      xSemaphoreGive(tx_msg_sema);
    }
  }
}

void statusUpdateImmediate(void * parameter) {
  wsStatusUpdate();
  vTaskDelete(NULL);
}

void statusUpdater(void * parameter) {
  for(;;){ // infinite loop
    wsStatusUpdate();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void wsSSIDScan(void * parameter) {
  int n;

  if (!ssid_scan_pend) {
    ssid_scan_pend = true;

    // If connection has failed, we need to disable WiFi before rescanning
    // appears to be a known bug https://github.com/espressif/arduino-esp32/issues/3294
    if (!(state.conn == WL_CONNECTED) || (state.conn == WL_DISCONNECTED)) {
      WiFi.disconnect();
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    n = min(5, (int)(WiFi.scanNetworks()));

    if( xSemaphoreTake( tx_msg_sema, portMAX_DELAY ) == pdTRUE ) {
        
      char tx_msg_str[1024];
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
        strncpy(state.eeprom.ssid, ssid.c_str(), sizeof(state.eeprom.ssid));
        strncpy(state.eeprom.key,  key.c_str(),  sizeof(state.eeprom.key));
        EEPROMStore();
        wifiSTAInit();
      }
      else if (strcmp(payld_type, "tz") == 0) { // Timezone Setting Update
        String tz      = rx_msg["payld"]["values"][0];
        String tz_enc  = rx_msg["payld"]["values"][1];
        strncpy(state.eeprom.tz,     tz.c_str(),     sizeof(state.eeprom.tz));
        strncpy(state.eeprom.tz_enc, tz_enc.c_str(), sizeof(state.eeprom.tz_enc));
        EEPROMStore();
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
      xTaskCreate( statusUpdateImmediate, "statusUpdateImmediate", 4096, NULL, 1, NULL );
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

/////////////////////////////////////////////
// OTA Updates
/////////////////////////////////////////////

void otaInit() {
  ArduinoOTA
    .onStart([]() {
      Serial.println("!!! Starting OTA !!!");
    })
    .onEnd([]() {
      Serial.println("\n!!! OTA Complete !!!");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.setHostname(ap_ssid);

  ArduinoOTA.begin();
}

void setup() {
  pinMode(0, INPUT); // Use BOOT Button for NVME reset, 0=Pressed, 1=Not
  Serial.begin(115200);

  // Initialize EEPROM and fetch values
  EEPROMInit();
  EEPROMLoad();

  // Initialise WiFi AP, Websocket Server, HTML Server
  wifiAPInit();
  wsInit();
  serverInit(); 
  otaInit();
  pixelsInit();
  
  // Initialise DNS Server for captive portal.
  dnsServer.start(53, "*", WiFi.softAPIP());

  // Start HTML Server
  server.begin();

  xTaskCreate(
     statusUpdater,   // Function that should be called
     "statusUpdater", // Name of the task (for debugging)
     4096,             // Stack size (bytes)
     NULL,             // Parameter to pass
     1,                // Task priority
     NULL              // Task handle
  );

  // Initialise WiFi Station, try to connect to WiFi
  wifiSTAInit();
  // Initialise NTP CLient
  ntpInit();

  uint32_t color = pixels.color(0,150,0);
  pixels.clear();
  pixels.pixel(0,0, &color );
  pixels.pixel(15,0, &color );
  pixels.pixel(0,13, &color );
  pixels.pixel(15,13, &color );
  pixels.show();

}

int reset_cnt;
void loop() {
  ArduinoOTA.handle();

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
