# SSD1322-Arduino-Library
Arduino library for a SSD1322 256x64 display

The SSD1322 display controller has graphics display memory (GDRAM) of 480x128 4-bit (16 grayscale) pixels.
The GDRAM stores four 4-bit pixels in each addressable word. So, the GDRAM address range is 120x128.

This library was written for an OLED display with 256x64 display pixels using an 8-bit parallel interface.
The parallel interface permits the reading of GDRAM, so individual pixels can be read and written.

![image](https://user-images.githubusercontent.com/31147085/77266115-2c100c00-6c64-11ea-9279-2257dfec3cc4.png)

![image](https://user-images.githubusercontent.com/31147085/77266125-3500dd80-6c64-11ea-92d8-be2d4059d2c7.png)

The library was written for an Aliexpress display.
https://www.aliexpress.com/item/32988134507.html 
