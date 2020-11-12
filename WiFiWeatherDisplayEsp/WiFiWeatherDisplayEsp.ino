// Look into settings.hpp!

#include "LedController.hpp"
#include "Frames.hpp"
#include "settings.hpp"
#include <LinkedList.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#define CS 15
#define Segments 1

LedController lc = LedController();

const String uri  = "https://api.openweathermap.org/data/2.5/weather?q=" + String(location) + "&appid=" + String(apikey) + "&units=" + String(units) + "";

// Variables
int weatherId       = 0;
float currentTemp   = 0;
int humidity        = 0;
long currentTime    = 0;
long sunrise        = 0;
long sunset         = 0;
boolean dataIsFresh = false;
boolean started     = false;

LinkedList<byte> buffer = LinkedList<byte>();

void setup() {
  // Init LEDs
  pinMode(D1, OUTPUT);    // Red - no connectivity LED
  pinMode(D2, OUTPUT);    // Yellow - last request failed LED
  
  // Setup 8x8 matrix
  lc.init(CS, Segments);
  lc.setIntensity(0);

  // Prefill cloud buffer
  for (int i = 0; i < 8; i++)
    buffer.add(empty);

  setupWiFi();
  wifiLed();

  setupOta();

  int timeout = 0;
  while(!dataIsFresh) {
    requestData();
    if(dataIsFresh) {
      break;
    }
    for(int i = 0; i < 7; i++) {
      displayImage(reqData[i]);
      newDelay(50);
    }
    if(timeout > 100) {
      ESP.restart();
    }
    timeout++;
  }
  started = true;
}

void loop() {
  // Main loop(
  requestData();
  if(dataIsFresh == false) {
    requestData();
  }
  displayAnimation(weatherId);
  displayTemp(currentTemp);
  newDelay(5000);
  displayImage(HU);
  newDelay(2000);
  displayHumidity(humidity);
  newDelay(6000);
}

void newDelay(unsigned long ms) {
  unsigned long currmillis = millis();
  unsigned long timeFuture = currmillis + ms;
  while(currmillis < timeFuture) {
    currmillis = millis();
    // Task that should be done while Arduino is idling
    if(started == true) {
      wifiLed();
      adjustBrightness();
      ArduinoOTA.handle();
    }
    delay(0);
  }
}

void wifiLed() {
  if(WiFi.status() == WL_CONNECTED) {
    digitalWrite(D1, LOW);
  }else {
    digitalWrite(D1, HIGH);
  }
}

void adjustBrightness() {
  if((currentTime >= sunrise) && (currentTime <= sunset))
    lc.setIntensity(2);
  else 
    lc.setIntensity(0);
}

void displayTemp(int temp) {
  if (temp < 100 && temp > -1)
    displayUnderHundred(temp);
  else if (temp < 0 && temp > -10)
    displayNegative(temp);
  else
    displayOverHundred(temp);
    newDelay(100);
}

void displayNegative(int temp) {
  temp = -temp;
  lc.setRow(0, 0, B00001000);
  lc.setRow(0, 1, B00001000);
  lc.setRow(0, 2, B00001000);
  lc.setRow(0, 3, B00000000);

  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 4, numUnderHundred[temp][i]);
  
  lc.setRow(0, 7, B00000001);
}

void displayUnderHundred(int temp) {
  if (temp < 10)
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[0][i]);
  else
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[temp / 10][i]);

  lc.setRow(0, 3, B00000000);

  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 4, numUnderHundred[temp % 10][i]);

  lc.setRow(0, 7, B00000001);
}

void displayOverHundred(int temp) {
  if (temp < 110)
    for (unsigned int i = 0; i < 5 ; i++)
      lc.setRow(0, i, hundredOnes[i]);
  else
    for (unsigned int i = 0; i < 5 ; i++)
      lc.setRow(0, i, hundredTeens[i]);
  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 5, numOverHundred[temp % 10][i]);
}

void displayAnimation(int code) {
  if (code == 200 || (code <= 232 && code >= 230)) { // thunderstorm with light rain
    for (unsigned int i = 0; i < 3; i ++) {
      for (unsigned int j = 0; j < 3; j ++) {
        displayImage(moderateRain[j]);
        newDelay(400);
      }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++) {
          displayImage(lightning[k]);
          if (k == 1)
            newDelay(200);
          else
            newDelay(30);
        }
    }
  }

  else if (code == 201) { // thunderstorm with moderate rain
    for (unsigned int i = 0; i < 4; i ++) {
      for (unsigned int j = 0; j < 4; j ++)
        for (unsigned int k = 0; k < 3; k ++) {
          displayImage(moderateRain[k]);
          newDelay(100);
        }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++) {
          displayImage(lightning[k]);
          if (k == 1)
            newDelay(200);
          else
            newDelay(30);
        }
    }
  }

  else if (code == 202) { // thunderstorm with heavy rain
    for (unsigned int i = 0; i < 4; i ++) {
      for (unsigned int j = 0; j < 4; j ++)
        for (unsigned int k = 0; k < 3; k ++) {
          displayImage(heavyRain[k]);
          newDelay(100);
        }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++) {
          displayImage(lightning[k]);
          if (k == 1)
            newDelay(200);
          else
            newDelay(30);
        }
    }
  }

  else if (code >= 210 && code <= 221) { // lightning storm
    for (unsigned int i = 0; i < 4; i ++) {
      for (unsigned int j = 0; j < 3; j ++) {
        displayImage(lightning[j]);
        if (j == 0)
          newDelay(700);
        else if (j == 1)
          newDelay(200);
        else
          newDelay(30);
      }

      for (unsigned int j = 1; j < 3; j ++) {
        displayImage(lightning[j]);
        if (j == 1)
          newDelay(200);
        else
          newDelay(30);
      }
    }
  }

  else if ((code >= 300 && code <= 321) || code == 500 || code == 520) { // display light rain or drizzle
    for (unsigned int i = 0; i < 5; i ++)
      for (unsigned int j = 0; j < 3; j ++) {
        displayImage(moderateRain[j]);
        newDelay(400);
      }
  }

  else if (code == 501 || code == 521) { // display moderate rain
    for (unsigned int i = 0; i < 20; i ++)
      for (unsigned int j = 0; j < 3; j ++) {
        displayImage(moderateRain[j]);
        newDelay(100);
      }
  }

  else if ((code >= 502 && code <= 511) || code == 522 || code == 531 || code == 771 || code == 906 || code == 901 || code == 902 || code > 958) { // display heavy rain
    for (unsigned int i = 0; i < 20; i ++)
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(heavyRain[j]);
        newDelay(100);
      }
  }

  else if (code == 801) // display few clouds
    cloudDriver(fewClouds);

  else if (code == 802) // display scattered clouds
    cloudDriver(scatteredClouds);

  else if (code == 803 || code == 804) // display overcast or broken clouds
    cloudDriver(brokenClouds);
    
  else if (code == 600 || code == 601 || code == 611 || code == 612 || code == 615 || code == 616 || code == 620 || code == 621) {
      for (unsigned int i = 0; i < 5; i++) {     // Added for light to normal snowfall
        displayImage(snowfall[0]);
        newDelay(1000);
        displayImage(snowfall[1]);
        newDelay(1000);
      }
    }

  else if (code == 602 || code == 622) {          // Added for heavy snowfall
    for (unsigned int i = 0; i < 5; i++) {
      for (unsigned int j = 0; j < 4; j++) {
        displayImage(heavySnowfall[j]);
        newDelay(500);
      }
    }
  }

  else if (code == 701 || code == 741 || code == 711 || code == 721 || code == 731 || code == 751 || code == 761 || code == 762 || code == 771) {           // Fog and mist
    cloudDriver(fog);
  }

  else if (code == 800)
    dayOrNight();

  else
    for (unsigned int i = 0; i < 5; i++) {
      displayImage(notFound[0]);
      newDelay(500);
      displayImage(notFound[1]);
      newDelay(500);
    }
}

void dayOrNight() {
  if((currentTime >= sunrise) && (currentTime <= sunset))
    displayClearDay();
  else 
    displayClearNight();
}

void displayClearDay() {
  // display clear day
  for (unsigned int i = 0; i < 5; i ++)
    for (unsigned int j = 0; j < 3; j ++) {
      displayImage(clearDay[j]);
      newDelay(400);
    }
}

void displayClearNight() {
  // display clear night
  for (unsigned int i = 0; i < 5; i ++)
    for (unsigned int j = 0; j < 2; j ++) {
      displayImage(clearNight[j]);
      newDelay(600);
    }
}

void cloudDriver(byte b[]) {
  for (unsigned int i = 0; i < 2; i++) {
    for (unsigned int j = 0 ; j < 16; j++) {
      passInByte(b[j]);
      displayBuffer();
      newDelay(187);
    }
    passInByte(empty);
    displayBuffer();
    newDelay(187);
  }
  clearBuffer();
}

void passInByte(byte b) {
  buffer.shift();
  buffer.add(b);
}

void displayBuffer() {
  for (unsigned int i = 0; i < 8; i++) {
    lc.setRow(0, i, buffer.get(i) );
  }
}

void clearBuffer() {
  for (unsigned int i = 0; i < 16; i++) {
    buffer.shift();
    buffer.add(B00000000);
  }
}

void displayHumidity(int h) {
  if(h == 100)
    displayImage(maxHumidity);
  else {
    if (h < 10)
      for (unsigned int i = 0; i < 3 ; i++)
        lc.setRow(0, i, numUnderHundred[0][i]);
    else if (h == 10 || (h > 10 && h != 100))
      for (unsigned int i = 0; i < 3 ; i++)
        lc.setRow(0, i, numUnderHundred[h / 10][i]);

    lc.setRow(0, 3, B00000000);

    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i + 4, numUnderHundred[h % 10][i]);

    lc.setRow(0, 7, B00000000);
  }
}

void requestData() {
  dataIsFresh = false;
  if(WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if(http.begin(client, uri)) {
    // Success connecting
    int httpCode = http.GET();
    if(httpCode > 0 && httpCode == HTTP_CODE_OK) {
      // Successful request
      String payload = http.getString();
      // Parse JSON
      DynamicJsonDocument doc(1000);
      deserializeJson(doc, payload);
      JsonObject weather_0  = doc["weather"][0];
      weatherId             = weather_0["id"];
      JsonObject main       = doc["main"];
      currentTemp           = main["temp"];
      humidity              = main["humidity"];
      currentTime           = doc["dt"];
      JsonObject sys        = doc["sys"];
      sunrise               = sys["sunrise"];
      sunset                = sys["sunset"];

      dataIsFresh = true;
      digitalWrite(D2, LOW);
    }else {
      // Fail
      digitalWrite(D2, HIGH);
      http.end();
      return;
    }
    http.end();
  }else {
    // Fail
    digitalWrite(D2, HIGH);
    return;
  }
}

void setupWiFi() {
  wifiLed();
  if(WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int timeout = 0;
    while(WiFi.status() != WL_CONNECTED) {
      for(int i = 0; i < 14; i++) {
        displayImage(wifi[i]);
        newDelay(50);
      }
      timeout++;
      if(timeout > 500) {
        //ESP.restart();
        break;
      }
    }
  }
  displayImage(wifi[14]);
  wifiLed();
}

void displayImage(byte image[]) {
  for (unsigned int i = 0; i < 8; i++) {
    lc.setRow(0, i, image[i]);
  }
}

void displayImageOverlay(byte image1[], byte image2[]) {
  for(unsigned int i = 0; i < 8; i++) {
    lc.setRow(0, i, image1[i] | image2[i]);
  }
}

void setupOta() {
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(otapassw);

  ArduinoOTA.onStart([] () {
    for(int i = 0; i < 12; i++) {
      displayImage(load[i]);
      newDelay(50);
    }
  });
  ArduinoOTA.onEnd([] () {
    displayImage(progressAni[20]);
  });
  ArduinoOTA.onProgress([] (unsigned int progress, unsigned int total) {
    if(progress / (total / 100) == 100) {
      displayImage(progressAni[20]);
    }else {
      displayImageOverlay(progressAni[progress / (total / 20)], load[(millis() / 100) % 12]);
    } 
  });
  ArduinoOTA.onError([] (ota_error_t error) {
    displayImage(notFound[0]);
  });
  ArduinoOTA.begin();
}
