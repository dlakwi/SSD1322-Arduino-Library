# SSD1322-Arduino-Library
Arduino library for a SSD1322 256x64 display

The SSD1322 display controller has graphics display memory (GDRAM) of 480x128 4-bit (16 grayscale) pixels.
The GDRAM store 4 pixels in each addressable word. So, the GDRAM address range is 120x128.

This library was written for an OLED display with 256x64 display pixels using an 8-bit parallel interface.
The parallel interface permits the reading of GDRAM, so individual pixels can be read and written.

The library was written for an Aliexpress display.
https://www.aliexpress.com/item/32988134507.html 
