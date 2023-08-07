## wally - ESP32 WiFi/Serial Control of a Shorekarn Message Maker LED Wallboard

This code provides current weather and a clock over RS232 to the wallboard display.  The specific wallboard I used supports "VT100" escape codes to control color of the text and other functions such as "clear screen".


<img width="1515" alt="Screen Shot 2022-12-17 at 9 48 39 AM" src="https://user-images.githubusercontent.com/66791904/208247622-8f06dd40-4752-423a-a5da-bf6840bb193b.png">

Initial setup of the device uses WiFiManager and optional Adafruit IO integration to send text to the display remotely.  On first start - connect to the provided hotspot for configuration.  To reset the device, long-press button 0.

NOTE: I found that when powering from an AC-USB adapter that an additional ground wire is needed from the ESP32 to the wallbaord device in addition to the ground wire on the serial port.
