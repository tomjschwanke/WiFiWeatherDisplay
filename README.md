# Important:
Due to limitations / bugs with the ESP8266, I have moved this to the ESP32, you can find it at [https://github.com/tomjschwanke/WiFiWeatherDisplayEsp32](https://github.com/tomjschwanke/WiFiWeatherDisplayEsp32)

# WiFiWeatherDisplay
Adapted jconenna's WiFi weather display for my own needs and ported it to the ESP8266. This is basically a complete recode, but I still wanna give him credit for the existing animations.

## What is this?
A WiFi weather station giving you a short animation on the current weather, the temperature and humidity.

## History.
A few years back I saw the video by jconenna where he showed off his WiFi weather station. It was an Arduino communicating with an ESP8266 via Serial. I built it and programmed it, however things didn't work as expected. The serial communication and JSON parsing was prone to errors and timeouts and things weren't made for metric units. That's why I decided to change a lot of stuff. I included negative temperatures etc, but in the end, the serial communication was a major issue. I kinda abandoned it. Recently however, I thought of how cool that thing was and decided I wanted it back. Unfortunately I fried my Arduino, so I decided for a rewrite, only using the ESP8266 and getting rid of a lot of unneccessary features I built in. That had the neat side-effect of solving the serial communications issue entirely and a new JSON library takes care of all the parsing now.

## Features
### API
- Data requested from OpenWeatherMap API (you need your own key)
- Define location and units in [settings.hpp](settings.hpp)
### Weather animations
- clear day / night
- light / moderate / heavy rain
- lightning
- light / moderate / heavy thunderstorm
- moderate / heavy snowfall
- few / scattered / broken clouds
- fog
- if a condition is missing, there will be a blinking X
### Temperature
- -99° to 199°
- Celsius or Farenheit defined in the OpenWeatherMap API call
### Humidity
- 0% to 100% RH
### Automatic dimming
- Dims after sunset (info from OpenWeatherMap API)
### OTA
- Set a hostname and password to update the ESP8266 over WiFi without connecting the USB cable
- has its own neat progress animation (can be a bit laggy, but excuse the chip, its updating itself)
### LED indicators
- LED on D1 for indicating a failed request
- LED on D2 for indicating lost connectivity
### Should something fail
- If a request or the WiFi connection fails for whatever reason, it fails and returns to displaying the data it got before. The LED indicators will show the status.
### Requests
- After the display has cycled through all available data, it loads new data (you can change this in the code, though OpenWeatherMap allows up to 60 requests per minute / 1,000,000 per month, I haven't run into any limit yet)

## Parts
- 1 ESP8266
- 1 8x8 LED Matrix + MAX7219 controller
- 2 LEDs + resistor
- Some sort of powersupply

## Connections
| ESP8266    | Other part            |
|------------|-----------------------|
| ESP9266 D1 | Failed request LED    |
| ESP8266 D2 | Lost connectivity LED |
| ESP8266 D5 | MAX7219 CLK           |
| ESP8266 D7 | MAX7219 DIN           |
| ESP8266 D8 | MAX7219 CS            |

Power should be self-explanatory (VCC to 5V, GND to GND)

