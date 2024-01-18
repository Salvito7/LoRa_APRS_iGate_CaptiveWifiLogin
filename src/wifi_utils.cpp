#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wpa2.h>
#include <esp_wifi.h>
#include "configuration.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "display.h"
#include "wpa2entCERT.h"

extern Configuration  Config;
extern WiFi_AP        *currentWiFi;
extern int            myWiFiAPSize;
extern int            stationMode;
extern uint32_t       previousWiFiMillis;

int             localWiFiAPIndex = 0;


namespace WIFI_Utils {

  void wpa2ent() {
/*  //todo finish implementing this
  //uint8_t masterCustomMac[] = {0x24, 0x0A, 0xC4, 0x9A, 0xA5, 0xA1}; // 24:0a:c4:9a:a5:a1

    //esp_wifi_set_mac(ESP_IF_WIFI_STA, &masterCustomMac[0]);
    Serial.print("MAC >> ");
    Serial.println(WiFi.macAddress());
    Serial.printf("Connecting to WiFi: %s ", currentWiFi->ssid.c_str());
    esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)incommon_ca, strlen(incommon_ca) + 1);
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
    esp_wifi_sta_wpa2_ent_enable(&config);
    WiFi.begin(currentWiFi->ssid.c_str());
*/
  }

  void captiveLogin() {
    currentWiFi = &Config.wifiAPs[localWiFiAPIndex];

  if (currentWiFi->captiveLoginBool) { //not needed???
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
      if (currentWiFi->captiveLoginBool) {
        captiveLogin();
      }
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
    
    if (currentWiFi->wpa2ent) {
      	wpa2ent();
    } else {
      WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
    }

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

        if (currentWiFi->wpa2ent) {
            wpa2ent();
        } else {
          WiFi.begin(currentWiFi->ssid.c_str(), currentWiFi->password.c_str());
        }
 
      }
    }
    #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
    digitalWrite(internalLedPin,LOW);
    #endif
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected as ");
        Serial.println(WiFi.localIP());
        show_display("", "", "     Connected!!", "" , "     loading ...", 1000);

      if (currentWiFi->captiveLoginBool) {
        captiveLogin();
      }
      
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