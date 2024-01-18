#include <WiFi.h>
#include <HTTPClient.h>
#include "configuration.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "display.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            myWiFiAPSize;
extern int            stationMode;
extern uint32_t       previousWiFiMillis;

int             localWiFiAPIndex;
bool            captiveLoginBool;


namespace WIFI_Utils {

void captiveLogin() {
currentWiFi = &Config.wifiAPs[localWiFiAPIndex];

if (currentWiFi->captiveLoginBool) { 
  show_display("", " CaptiveLogin: true", "  WiFi Connected!!", "" , "     logging in ...", 1500);

  HTTPClient http;
  Serial.println("WifiIndex:" + localWiFiAPIndex);
  Serial.print("captiveURL: "); 
  Serial.println(currentWiFi->captiveURL);
  http.begin(currentWiFi->captiveURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(currentWiFi->captiveQuery);
  http.end(); 

  // httpCode will be negative on error
  if (httpCode > 0) {
    if (httpCode == 200) {
      // String payload = http.getString();   //uncommment this if you want to see the Servers HTML response (could lead to memory problems)
      // Serial.println(payload);
      Serial.printf("[HTTP] POST response code: %d\n", httpCode); 
      show_display("HTTPlogin:  OK", "", "",   "   If no APRS-IS" , "  check URL/Query", "", 2000);  //wenn die credentials falsch sein nor kimp die normale login seite mit OK response 
      
      } else {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST response code: %d\n", httpCode);
        show_display("", " HTTP POST response: "+httpCode, "     Connected!!", "" , "     loading ...", 2000);
      }
    } else {
      Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
      show_display("HTTPlogin:   FAILED", "", "%s\n", http.errorToString(httpCode).c_str(), "" , "Check Serial Monitor", "", 4000);
    }
  
} else {
  show_display("", "", "     Connected!!", "" , "     loading ...", 1000);
}
}

  void checkWiFi() {
    if ((WiFi.status() != WL_CONNECTED) && ((millis() - previousWiFiMillis) >= 30*1000)) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousWiFiMillis = millis();
    }
  }

  void startWiFi() {
    int wifiCounter = 0;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);
    unsigned long start = millis();
    show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
    Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
    WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    while (WiFi.status() != WL_CONNECTED && wifiCounter<2) {
      delay(500);
      #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
      digitalWrite(internalLedPin,HIGH);
      #endif
      Serial.print('.');
      delay(500);
      #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
      digitalWrite(internalLedPin,LOW);
      #endif
      if ((millis() - start) > 6000){
        delay(1000);
        if(localWiFiAPIndex >= (myWiFiAPSize-1)) {
          localWiFiAPIndex = 0;
          if (stationMode==5) {
            wifiCounter++;
          }
        } else {
          localWiFiAPIndex++;
        }
        currentWiFi = &Config.wifiAPs[localWiFiAPIndex];
        start = millis();
        Serial.print("\nConnecting to WiFi '"); Serial.print(currentWiFi->ssid); Serial.println("' ...");
        show_display("", "", "Connecting to Wifi:", "", currentWiFi->ssid + " ...", 0);
        WiFi.disconnect();
        WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
      }
    }
    #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
    digitalWrite(internalLedPin,LOW);
    #endif
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected as ");
        Serial.println(WiFi.localIP());
        show_display("", "", "     Connected!!", "" , "     loading ...", 1000);
        captiveLogin();
    } else if (WiFi.status() != WL_CONNECTED && stationMode==5) {
        Serial.println("\nNot connected to WiFi! (DigiRepeater Mode)");
        show_display("", "", " WiFi Not Connected!", "  DigiRepeater MODE" , "     loading ...", 2000);
    }
  }

  void setup() {
    if (stationMode==1 || stationMode==2) {
      if (stationMode==1) {
        Serial.println("stationMode ---> iGate (only Rx)");
      } else {
        Serial.println("stationMode ---> iGate (Rx + Tx)");
      }
      startWiFi();
      btStop();
    } else if (stationMode==3 || stationMode==4) {
      if (stationMode==3) {
        Serial.println("stationMode ---> DigiRepeater (Rx freq == Tx freq)");
      } else {
        Serial.println("stationMode ---> DigiRepeater (Rx freq != Tx freq)");
      }
      WiFi.mode(WIFI_OFF);
      btStop();
    } else if (stationMode==5) {
      Serial.println("stationMode ---> iGate when Wifi/APRS available (DigiRepeater when not)");
    } else { 
      Serial.println("stationMode ---> NOT VALID, check '/data/igate_conf.json'");
      show_display("------- ERROR -------", "stationMode Not Valid", "change it on : /data/", "igate_conf.json", 0);
      while (1);
    }
  }

}