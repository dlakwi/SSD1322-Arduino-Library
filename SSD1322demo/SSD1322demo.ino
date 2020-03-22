// SSD1322demo.ino
// 
// 2020-03-22
// dlakwi Ensign Core
// Donald Johnson
// 
// 3.12" 256x64 OLED display - SSD1322 480x128 4-bit grayscale
// https://www.aliexpress.com/item/32988134507.html
// 
// 4 bits   / pixel
// 4 pixels / word
// 256 x 64 pixels
//  64 x 64 words
// 
// column page = x / 4  ~  x >> 2
//        quad = x % 4  ~  x & x3
// row         = y
// 
// ----------------

#include <Arduino.h>
#include <avr/pgmspace.h>

// ----------------

// Wiring
//
// Arduino UNO pin    OLED display          
//                                          
//   GND               1   GND              
//   3V3               2   VDD                NOT 5V!
//                     3   NC               
//     8               4   D0               
//     9               5   D1               
//     2               6   D2               
//     3               7   D3               
//     4               8   D4               
//     5               9   D5               
//     6              10   D6               
//     7              11   D7               
//    A0              12   RD  Read         
//    A1              13   WR  Write        
//    A2              14   DC  Data/Command 
//    A3              15   RES Reset        
//    A4              16   CS  Chip Select  
//
//                    --   BS1              
//                    --   BS2              

// ----------------

// Commands see data sheet SSD1322      // + number of byte arguments
// see initDisp() ..

#define CMD_SET_COLUMN_ADDR             0x15 // +2B start,end (default 0x00,0x3F) (OLED 0x1C,0x5B)
#define CMD_SET_ROW_ADDR                0x75 // +2B start,end (default 0x00,0x7F) (OLED 0x00,0x3F)

#define CMD_SET_REMAP                   0xA0 // +2B (default 0x00,0x01)
#define CMD_SET_START_LINE              0xA1 // +1B (default 0x00)
#define CMD_SET_OFFSET                  0xA2 // +1B (default 0x00)

#define CMD_WRITE_RAM                   0x5C
#define CMD_READ_RAM                    0x5D

#define CMD_ENTIRE_DISPLAY_OFF          0xA4
#define CMD_ENTIRE_DISPLAY_ON           0xA5
#define CMD_NORMAL_DISPLAY              0xA6
#define CMD_INVERSE_DISPLAY             0xA7

#define CMD_ENABLE_PARTIAL_DISPLAY      0xA8 // +2B start,end
#define CMD_EXIT_PARTIAL_DISPLAY        0xA9

#define CMD_DISPLAY_OFF                 0xAE // SLEEP_MODE_ON  ==> display off
#define CMD_DISPLAY_ON                  0xAF // SLEEP_MODE_OFF ==> display on

#define CMD_GRAYSCALE_TABLE             0xB8 // +15B
#define CMD_ENABLE_GRAYSCALE_TABLE      0x00
#define CMD_SELECT_DEF_GRAYSCALE_TABLE  0xB9

#define CMD_FUNCTION_SELECTION          0xAB // +1B (0x00 ext VDD, 0x01 int VDD)

#define CMD_SET_PHASE_LENGTH            0xB1 // +1B (default 0x53)
#define CMD_CLKDIV_OSCFREQ              0xB3 // +1B (default 0x41)
#define CMD_DISPLAY_ENH_A               0xB4 // +2B
#define CMD_DISPLAY_ENH_B               0xD1 // +1B
#define CMD_SETGPIO                     0xB5 // +1B
#define CMD_SET_SEC_RECH_PERIOD         0xB6 // +1B
#define CMD_SET_PRECHARGE_VOLT          0xBB // +1B
#define CMD_SET_VCOMH_VOLTAGE           0xBE // +1B
#define CMD_SET_CONTRAST_CURRENT        0xC1 // +1B (default 0x40)
#define CMD_MASTER_CONTRAST_CURRENT     0xC7 // +1B
#define CMD_SET_MUX_RATIO               0xCA // +1B
#define CMD_SET_COMMAND_LOCK            0xFD // +1B

// ----------------

// OLED control pins - these are active LO, so "0" is the on/active state

#define RD_pin       A0
#define RD_on        digitalWrite( RD_pin, 0 )
#define RD_off       digitalWrite( RD_pin, 1 )
#define WR_pin       A1
#define WR_on        digitalWrite( WR_pin, 0 )
#define WR_off       digitalWrite( WR_pin, 1 )
#define DC_pin       A2
#define DC_command   digitalWrite( DC_pin, 0 )
#define DC_data      digitalWrite( DC_pin, 1 )
#define RST_pin      A3
#define RST_on       digitalWrite( RST_pin, 0 )
#define RST_off      digitalWrite( RST_pin, 1 )
#define CS_pin       A4
#define CS_select    digitalWrite( CS_pin, 0 )
#define CS_deselect  digitalWrite( CS_pin, 1 )

// OLED data pins 0~7 ==> Arduino UNO pins 8~9,2~7

byte pin[] = { 8, 9, 2, 3, 4, 5, 6, 7 };  // do not use 0 and 1 (Rx and Tx)

// set the Arduino data pins to inputs

void setAsInput( void )
{
  for ( int i = 0; i < 8; i++ )
    pinMode( pin[i], INPUT );
}

// set the Arduino data pins to outputs

void setAsOutput( void )
{
  for ( int i = 0; i < 8; i++ )
    pinMode( pin[i], OUTPUT );
}

// initialize the Arduino control and data pins

void initIO( void )
{
  // all command pins are set to be outputs
  pinMode( RD_pin,  OUTPUT );
  pinMode( WR_pin,  OUTPUT );
  pinMode( DC_pin,  OUTPUT );
  pinMode( RST_pin, OUTPUT );
  pinMode( CS_pin,  OUTPUT );
  // all data pins are initially set to be outputs
  setAsOutput();
  setData( 0x00 );

  // Set the ports to reasonable starting states
  CS_select;
  RST_off;
  DC_command;
  WR_off;
  RD_off;
  CS_deselect;
}

// set the Arduino data pins to byte d

void setData( byte d )
{
  for ( int i = 0; i < 8; i++ )
    digitalWrite( pin[i], ( ( d >> i ) & 0x01 ) );
}

// get a data byte from the Arduino data pins

byte getData( void )
{
  byte d = 0x00;
  for ( int i = 0; i < 8; i++ )
    d |= ( digitalRead( pin[i] ) << i );
  return d;
}

// --------

// read and write commands and data

#define del delayMicroseconds(5)

// write the command byte c to the OLED display

void writeCommand( byte c )
{
  RD_off;                // not reading
  CS_select;             // enable OLED control
  DC_command;            // command

  setData( c );          // present command
  del;
  WR_on;
  del;
  WR_off;                // OLED command is written as WR goes off

  CS_deselect;           // disable OLED control
}

// this is only used for writing data for commands, not for display pixels

// write the data byte d to the OLED display

void writeDataByte( byte b )
{
  RD_off;                // not reading
  CS_select;             // enable OLED control
  DC_data;               // data

  setData( b );          // present data
  del;
  WR_on;
  del;
  WR_off;                // OLED data is written as WR goes off

  CS_deselect;           // disable OLED control
}

// write the data word w to the OLED display

void writeDataWord( word w )
{
  byte b0, b1;

  RD_off;                // not reading
  CS_select;             // enable OLED control
  DC_data;               // data

  b1 = ( w >> 8 );
  b0 = ( w & 0x00FF );

  setData( b1 );         // present data
  del;
  WR_on;
  del;
  WR_off;                // OLED data is written as WR goes off
  del;

  setData( b0 );         // present data
  del;
  WR_on;
  del;
  WR_off;                // OLED data is written as WR goes off

  CS_deselect;           // disable OLED control
}

// write N words to the OLED display

void writeDataWords( word w, word N )
{
  byte b0, b1;

  RD_off;                // not reading
  CS_select;             // enable OLED control
  DC_data;               // data

  b1 = ( w >> 8 );
  b0 = ( w & 0x00FF );

  for ( word i = 0; i < N; i++ )
  {
    setData( b1 );         // present data
    del;
    WR_on;
    del;
    WR_off;                // OLED data is written as WR goes off
    del;

    setData( b0 );         // present data
    del;
    WR_on;
    del;
    WR_off;                // OLED data is written as WR goes off
  }

  CS_deselect;           // disable OLED control
}

// do a dummy read only after setting the column address

boolean need_dummy_read = false;

// read a data word from the OLED display

word readDataWord( void )
{
  word r0 = 0x0000,
       r1 = 0x0000;
  word response;

  setData( 0x00 );
  setAsInput();          // set the Arduino data pins to inputs

  WR_off;                // not writing
  CS_select;             // enable OLED control
  DC_data;               // data

  if ( need_dummy_read )
  {
    RD_on;
    del;
    RD_off;
    del;
    need_dummy_read = false;
  }
  RD_on;
  del;
  RD_off;                // OLED data is presented as RD goes off
  del;
  r0 = getData();        // read the OLED into the Arduino data pins

  del;
  RD_on;
  del;
  RD_off;                // OLED data is presented as RD goes off
  del;
  r1 = getData();        // read the OLED into the Arduino data pins

  CS_deselect;           // disable OLED control

  setAsOutput();         // set the Arduino data pins back to outputs

  response = ( r0 << 8 ) | r1;
  return response;
}

// read a data byte from the OLED display

byte readDataByte( void )
{
  byte r0 = 0x00;
  byte response;

  setData( 0x00 );
  setAsInput();          // set the Arduino data pins to inputs

  WR_off;                // not writing
  CS_select;             // enable OLED control
  DC_data;               // data

  if ( need_dummy_read )
  {
    RD_on;
    del;
    RD_off;
    del;
    need_dummy_read = false;
  }
  RD_on;
  del;
  RD_off;                // OLED data is presented as RD goes off
  del;
  r0 = getData();        // read the OLED into the Arduino data pins

  CS_deselect;           // disable OLED control

  setAsOutput();         // set the Arduino data pins back to outputs

  response = r0;
  return response;
}

// ----------------

// get and set pixels in a GDRAM word

// 
// SSD1322 controller GDRAM is 480 columns wide and 128 rows high
// The Samiore/Aliexpress display module is 256 columns wide and 128 rows wide
// 
// GDRAM is organized as 64 pages wide by 64 rows high - each page is 1 word / 2 bytes / 4 pixels
// 
// the pixel at [x][y] is found in [x/4][y] as bits (x%4) << ( (x%4) << 1 )
// 
//              one page
//          +---------------------------+
//     col: |  0      1      2      3   |
//          +------+------+------+------+
//  row:    | 3210 | 7654 | 3210 | 7654 |
//          +------+------+------+------+
// 
//  note that the location of the pixels on the display are left to right
//  while within the byte they are right to left
// 
// r     = row
// p     = col / 4   (0 ~ ..)
// q     = col % 4   (0 ~  3)
// sh    = q << 2
// mask  = 0x0F << sh
// nask  = ~mask
// pixel = ( [r][p] & mask ) >> sh
// 
// 0000 = white
// .... = grays
// 1111 = black
// 

word mask[4] = { 0b0000000000001111, 0b0000000011110000, 0b0000111100000000, 0b1111000000000000 };
word nask[4] = { 0b1111111111110000, 0b1111111100001111, 0b1111000011111111, 0b0000111111111111 };  // == ~mask[]

// get the 4-bit pixel q within the word w
word get_p( word w, word q )
{
  q &= 0x0003;             // q = 0~3
  q = 3-q;
  word p = w & mask[q];    // get the pixel p (bits q3 ~ q0)
  return ( p >> (q<<2) );  // shift the bits, returning a value 0~F
}

void set_p( word& w, word q, word p )
{
  q &= 0x0003;             // q = 0~3
  q = 3-q;
  w &= nask[q];            // clear the pixel q-bits within w
  p &= 0x000F;             // p = 0~F
  w |= ( p << (q<<2) );    // set the pixel value p within w
}

// ----------------

void initDisp( void )
{
  RST_on;
  delay( 200 );
  RST_off;
  delay( 200 );

  writeCommand( CMD_SET_COMMAND_LOCK );
  writeDataByte( 0x12 );

  writeCommand( CMD_DISPLAY_OFF );

  writeCommand( CMD_CLKDIV_OSCFREQ );           // Set Clock as 80 Frames/Sec (default 0x50)
  writeDataByte( 0x91 );
  writeCommand( CMD_SET_MUX_RATIO );            // 1/64 Duty (0x0F~0x7F) (default 0x7F)
  writeDataByte( 0x3F );
  writeCommand( CMD_SET_OFFSET );               // Shift Mapping RAM Counter (0x00~0x5F)
  writeDataByte( 0x00 );
  writeCommand( CMD_SET_START_LINE );           // Set Mapping RAM Display Start Line (0x00~0x5F)
  writeDataByte( 0x00 );
  writeCommand( CMD_FUNCTION_SELECTION );       // function selection 0xAB
  writeDataByte( 0x01 );

  writeCommand( CMD_SET_REMAP );                // set re-map 0xA0
  writeDataByte( 0x14 );                        //   A[0] = 0, Horizontal address increment [default/reset]
                                                //   A[0] = 1, Vertical address increment
                                                //   A[1] = 0, Disable Column Address Re-map [default/reset]
                                                //   A[1] = 1, Enable Column Address Re-map
                                                //   A[2] = 0, Disable Nibble Re-map [default/reset]
                                                //   A[2] = 1, Enable Nibble Re-map
                                                //   A[4] = 0, Scan from COM0 to COM[N �1] [default/reset]
                                                //   A[4] = 1, Scan from COM[N-1] to COM0, where N is the Multiplex ratio
                                                //   A[5] = 0, Disable COM Split Odd Even [default/reset]
                                                //   A[5] = 1, Enable COM Split Odd Even
  writeDataByte( 0x01 );                        //   B[4] = 0, Disable Dual COM mode [default/reset]
                                                //   B[4] = 1, Enable Dual COM mode (MUX = 63) --> A[5] must be = 0

  writeCommand( CMD_MASTER_CONTRAST_CURRENT );  // master contrast current - scaling factor
  writeDataByte( 0x0F );                        // 0x00~0x0F
  writeCommand( CMD_SET_CONTRAST_CURRENT );     // set contrast current
  writeDataByte( 0x9F );                        // 0x00~0xFF --> larger values for brighter pixels
  writeCommand( CMD_SET_PHASE_LENGTH );         // set phase length
  writeDataByte( 0x74 );
  writeCommand( CMD_SET_PRECHARGE_VOLT );       // set pre-charge voltage
  writeDataByte( 0x1F );
  writeCommand( CMD_DISPLAY_ENH_A );            // display enhancemant A
  writeDataByte( 0xA2 );                        // set VSL to internal
  writeDataByte( 0xFD );                        // improve GS display quality
  writeCommand( CMD_SET_VCOMH_VOLTAGE );        // set VCOMH
  writeDataByte( 0x04 );
  writeCommand( CMD_EXIT_PARTIAL_DISPLAY );

  // set gray scales
  writeCommand( CMD_GRAYSCALE_TABLE );  //            0~180
  //                        //   Gray Scale Level  0    0   - cannot be changed
  writeDataByte(   8 );     //   Gray Scale Level  1    0
  writeDataByte(  16 );     //   Gray Scale Level  2    8
  writeDataByte(  20 );     //   Gray Scale Level  3   16
  writeDataByte(  24 );     //   Gray Scale Level  4   24
  writeDataByte(  28 );     //   Gray Scale Level  5   32
  writeDataByte(  32 );     //   Gray Scale Level  6   40
  writeDataByte(  36 );     //   Gray Scale Level  7   48
  writeDataByte(  40 );     //   Gray Scale Level  8   56
  writeDataByte(  46 );     //   Gray Scale Level  9   64
  writeDataByte(  54 );     //   Gray Scale Level 10   72
  writeDataByte(  64 );     //   Gray Scale Level 11   80
  writeDataByte(  76 );     //   Gray Scale Level 12   88
  writeDataByte(  88 );     //   Gray Scale Level 13   96
  writeDataByte( 100 );     //   Gray Scale Level 14  104
  writeDataByte( 112 );     //   Gray Scale Level 15  112
  writeCommand( CMD_ENABLE_GRAYSCALE_TABLE );  // Enable Gray Scale Table

  writeCommand( CMD_NORMAL_DISPLAY );

  fillDisplay( 0x0000 );                        // clear display before turning it on

  writeCommand( CMD_DISPLAY_ON );               // display ON 0xAF
}

// ----------------

// the 256x64 pixel OLED is offset within the GDRAM

const byte offset = 0x1C;  // 112/4

// there seems to a problem setting pixels ~ page/row addresses
// sometimes the program stops
// could there be some interaction between controller addressing increment and address range?
// try adding 1 to to the end addresses to avoid 1x1 address ranges: page end = 0x40, row end = 0x40

// set GDRAM page (x) and row (y) for following read or write

void setPageAddr( word p )
{
  byte addr = offset + ( p & 0x3F );
  writeCommand( CMD_SET_COLUMN_ADDR );
  writeDataByte( addr );
  writeDataByte( offset + 0x40 );
  need_dummy_read = true;  // after the column address is set, a dummy read is needed in readDataWord()
}

void setRowAddr( word r )
{
  byte addr = r & 0x3F;
  writeCommand( CMD_SET_ROW_ADDR );
  writeDataByte( addr );
  writeDataByte( 0x40 );
}

// p and w are page values (4-nibble) ~ 4x
// r and h are row  values            ~  y

void setAddresses( word p, word r, word w, word h )
{
  byte addr;
  addr = offset + ( p & 0x3F );
  writeCommand( CMD_SET_COLUMN_ADDR );
  writeDataByte( addr );
  writeDataByte( addr + w-1 );
  need_dummy_read = true;  // after the column address is set, a dummy read is needed in readDataWord()

  addr = ( r & 0x3F );
  writeCommand( CMD_SET_ROW_ADDR );
  writeDataByte( addr );
  writeDataByte( addr + h-1 );
}

// ----------------

// fill the OLED display RAM

void fillDisplay( word w )
{
  setAddresses( 0, 0, 64, 64 );
  writeCommand( CMD_WRITE_RAM );
  writeDataWords( w, 64*64 );
}

// ----------------

static boolean checkXY = true;

// set the pixel at (x,y) to colour
// read-modify-write method

void drawPixel( word x, word y, word colour )
{
  if ( checkXY && ( ( x > 255 ) || ( y > 63 ) ) )
    return;

  word page = x >> 2,
       quad = x & 3,
       w    = 0x0000;

  colour &= 0x000F;

  setPageAddr( page );
  setRowAddr( y );
  writeCommand( CMD_READ_RAM );
  w = readDataWord();

  set_p( w, quad, colour );  // the pixel is one of 4 nibbles - we want to change only pixel (x,y)

  setPageAddr( page );
  setRowAddr( y );
  writeCommand( CMD_WRITE_RAM );
  writeDataWord( w );
}

word getPixel( word x, word y )
{
  if ( checkXY && ( ( x > 255 ) || ( y > 63 ) ) )
    return 0x0000;

  word page = x >> 2,
       quad = x & 3,
       w    = 0x0000,
       p    = 0x0000;

  setPageAddr( page );
  setRowAddr( y );
  writeCommand( CMD_READ_RAM );
  w = readDataWord();
  p = get_p( w, quad );
  return p;
}

// ----------------

// Since the SSD1322 has 4 pixel column-pages, a font glyph that is a multiple of 4 makes for simpler character drawing
// (x,y) positions are in pixels
// both x and y should be multiples of 8
// x could be a multiple of 4
// y could be any value, but should be at most 56 (64-8)

#include "font_h8x8.h"
// all glyphs from 0~255 are defined

// drawChar
// 8x8 character
// blank rows and columns are coded into the glyph - no need to write blank ones

word dots[4] = { 0x0000, 0x00F0, 0x000F, 0x00FF };

void drawChar( word x, word y, byte ch )
{
  word col = x / 4;
  word j, i;
  byte b;
  word d0, d1, d2, d3;
  word w;

  // row and page addresses (start and end) are set so that writes wrap within the 8x8 character box

  setAddresses( col, y, 2, 8 );
  writeCommand( CMD_WRITE_RAM );

  for ( j = 0; j < 8; j++ )
  {
    b = pgm_read_byte( &font_h8x8[ch][j] );  // font codes 1 bit / pixel

    d3 = dots[ ( ( b >> 6 ) & 0x3 ) ];       // expand 4 pairs of bits to 4 bytes ~ 2 words
    d2 = dots[ ( ( b >> 4 ) & 0x3 ) ];
    d1 = dots[ ( ( b >> 2 ) & 0x3 ) ];
    d0 = dots[ ( ( b >> 0 ) & 0x3 ) ];

    writeDataWord( ( d0 << 8 ) | d1 );
    writeDataWord( ( d2 << 8 ) | d3 );
  }
}

// draw a string

void drawString( word x, word y, const char* aString )
{
  const char* Src_Pointer = aString;  // copy the pointer, because we will increment it

  while ( *Src_Pointer != '\0' )      // until string end
  {
    drawChar( x, y, (byte)*Src_Pointer );
    Src_Pointer++;
    x += 8;
  }
}

// draw a string residing in PROGMEM

void drawString_P( word x, word y, const char* aString )
{
  byte tmp;
  const char* Src_Pointer = aString;  // copy the pointer, because we will increment it

  tmp = pgm_read_byte( Src_Pointer );
  while ( tmp != '\0' )
  {
    drawChar( x, y, tmp );
    Src_Pointer++;
    tmp = pgm_read_byte( Src_Pointer );
    x += 8;
  }
}

// ----------------

// draw a vertical line

// original code decremented y1 ...
//   while ( y1 >= y0 )
//   {
//     drawPixel( x0, y1, colour );
//     y1--;
//   }
// that code hung, so increment y0 ...

void drawVLine( word x0, word y0, word y1, word colour )
{
  word tmp = 0;
  if ( y0 > y1 )
  {
    word tmp = y0; y0 = y1; y1 = tmp;
  }
  while ( y0 <= y1 )
  {
    drawPixel( x0, y0, colour );
    y0++;
  }
}

// draw a horizontal line

void drawHLine( word x0, word y0, word x1, word colour )
{
  word tmp = 0;
  if ( x0 > x1 )
  {
    word tmp = x0; x0 = x1; x1 = tmp;
  }
  while ( x0 <= x1 )
  {
    drawPixel( x0, y0, colour );
    x0++;
  }
}

// draw a line

void drawLine( word x0, word y0, word x1, word y1, word colour )
{
  int dx       = 0;
  int dy       = 0;
  int stepx    = 0;
  int stepy    = 0;
  int fraction = 0;

  if ( x0 == x1 )
  {
    if ( y0 == y1 )
      drawPixel( x0, y0, colour );
    else
      drawVLine( x0, y0, y1, colour );
    return;
  }
  if ( y0 == y1 )
  {
    drawHLine( x0, y0, x1, colour );
    return;
  }

  dy = ( y1 - y0 );
  dx = ( x1 - x0 );
  if ( dy < 0 )
  {
    dy    = -dy;
    stepy = -1;
  }
  else
  {
    stepy = 1;
  }
  if ( dx < 0 )
  {
    dx    = -dx;
    stepx = -1;
  }
  else
  {
    stepx = 1;
  }

  dx <<= 1;
  dy <<= 1;

  drawPixel( x0, y0, colour );

  if ( dx > dy )
  {
    fraction = ( dy - ( dx >> 1 ) );
    while ( x0 != x1 )
    {
      if ( fraction >= 0 )
      {
        y0       += stepy;
        fraction -= dx;
      }
      x0       += stepx;
      fraction += dy;

      drawPixel( x0, y0, colour );
    }
  }
  else
  {
    fraction = ( dx - ( dy >> 1 ) );
    while ( y0 != y1 )
    {
      if ( fraction >= 0 )
      {
        x0       += stepx;
        fraction -= dy;
      }
      y0       += stepy;
      fraction += dx;

      drawPixel( x0, y0, colour );
    }
  }
}

// draw a rectangle
//   if fill > 0 then fill the rectangle

void drawRectangle( word x0, word y0, word x1, word y1, word colour, byte fill )
{
  word xmin = 0;
  word xmax = 0;
  word ymin = 0;
  word ymax = 0;

  if ( x0 < x1 )
  {
    xmin = x0;
    xmax = x1;
  }
  else
  {
    xmin = x1;
    xmax = x0;
  }
  if ( y0 < y1 )
  {
    ymin = y0;
    ymax = y1;
  }
  else
  {
    ymin = y1;
    ymax = y0;
  }

  if ( x0 == x1 )
  {
    if ( y0 == y1 )
      drawPixel( x0, y0, colour );
    else
      drawVLine( x0, y0, y1, colour );
    return;
  }
  if ( y0 == y1 )
  {
    drawHLine( x0, y0, x1, colour );
    return;
  }

  if ( xmax == ( xmin + 1 ) )
  {
    if ( ymax == ( ymin + 1 ) )
    {
      drawPixel( x0, y0, colour );
      drawPixel( x0, y1, colour );
      drawPixel( x1, y0, colour );
      drawPixel( x1, y1, colour );
    }
    else
    {
      drawVLine( xmin, ymin, ymax, colour );
      drawVLine( xmax, ymin, ymax, colour );
    }
    return;
  }
  if ( ymax == ( ymin + 1 ) )
  {
    drawHLine( xmin, ymin, xmax, colour );
    drawHLine( xmin, ymax, xmax, colour );
    return;
  }

  // filled rectangle
  if ( fill != 0 )
  {
#ifdef _FILL_V
    for ( ; xmin <= xmax; ++xmin )
    {
      drawVLine( xmin, ymin, ymax, colour );
    }
#else
    for ( ; ymin <= ymax; ++ymin )
    {
      drawHLine( xmin, ymin, xmax, colour );
    }
#endif
  }
  // rectangle (not filled)
  else
  {
    drawVLine( xmin,   ymin,         ymax,   colour );  // left side
    drawVLine( xmax,   ymin,         ymax,   colour );  // right side
    drawHLine( xmin+1, ymin, xmax-1,         colour );  // bottom side
    drawHLine( xmin+1, ymax, xmax-1,         colour );  // top side
  }
}

// --------

// Bresenham's circle drawing algorithm
// Algorithm copied from Wikipedia

void drawCircle( int xc, int yc, int radius, boolean fill, word colour )
{
  int f     =  1 - radius;
  int ddF_x =  0;
  int ddF_y = -2 * radius;
  int x     =  0;
  int y     =  radius;

  if ( fill )
  {
    drawPixel( xc, yc + radius, colour );
    drawPixel( xc, yc - radius, colour );
    drawHLine( xc - radius, yc, xc + radius, colour );
  }
  else
  {
    drawPixel( xc,          yc + radius, colour );
    drawPixel( xc,          yc - radius, colour );
    drawPixel( xc + radius, yc,          colour );
    drawPixel( xc - radius, yc,          colour );
  }

  while ( x < y )
  {
    if ( f >= 0 )
    {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x + 1;
    if ( fill )
    {
      drawHLine( xc - x, yc + y, xc + x, colour );
      drawHLine( xc - x, yc - y, xc + x, colour );
      drawHLine( xc - y, yc + x, xc + y, colour );
      drawHLine( xc - y, yc - x, xc + y, colour );
    }
    else
    {
      drawPixel( xc + x, yc + y, colour );
      drawPixel( xc - x, yc + y, colour );
      drawPixel( xc + x, yc - y, colour );
      drawPixel( xc - x, yc - y, colour );
      drawPixel( xc + y, yc + x, colour );
      drawPixel( xc - y, yc + x, colour );
      drawPixel( xc + y, yc - x, colour );
      drawPixel( xc - y, yc - x, colour );
    }
  }
}

// ----------------

// test routines

// draw some text

void textSamples( void )
{
  for ( byte i = 0; i < 20; i++ )
  {
    drawChar( i*6, 0, (0x21+i) );
  }
  drawString( 0,  0, "SSD1322 256x64 4-bit OLED" );
  drawString( 8, 16, "2020-03-22" );
  for ( byte i = 0; i < 32; i++ )
  {
    drawChar( i*8, 32, (0x20+i) );
    drawChar( i*8, 40, (0x40+i) );
  }
}

// draw some checkerboard patterns

void checkerBoard( void )
{
  word i, j;
  word colour;
  setAddresses( 0, 0, 64, 32 );
  writeCommand( CMD_WRITE_RAM );
  colour = 0xF0F0;
  for ( i = 0; i < 8; i++ )
  {
    colour = ~colour;
    for ( j = 0; j < 64; j++ )
    {
      writeDataWord( colour );
    }
  }
  colour = 0xE1E1;
  for ( i = 0; i < 8; i++ )
  {
    colour = ~colour;
    for ( j = 0; j < 64; j++ )
    {
      writeDataWord( colour );
    }
  }
  colour = 0xD2D2;
  for ( i = 0; i < 8; i++ )
  {
    colour = ~colour;
    for ( j = 0; j < 64; j++ )
    {
      writeDataWord( colour );
    }
  }
  colour = 0xC3C3;
  for ( i = 0; i < 8; i++ )
  {
    colour = ~colour;
    for ( j = 0; j < 64; j++ )
    {
      writeDataWord( colour );
    }
  }
}

// test pixel read and write
// 
// writes a row of checkerboard and some text
// reads the pixels and write them reversed
// *note that the 2 pixels in each byte need to be reversed to properly mirror the original drawing

byte reverseNibbles( byte b )
{
  byte n0 = b & 0x0F,
       n1 = b & 0xF0;
  return ( n0 << 4 ) | ( n1 >> 4 );
}

byte readArray[128] = {};

void readDemo( void )
{
  word i, j,
       colour = 0xC3C3;
  byte b;

  // clear the display
  fillDisplay( 0x0000 );

  // draw 1 page (8 rows) of checkerboard
  setAddresses( 0, 0, 64, 8 );
  writeCommand( CMD_WRITE_RAM );
  for ( i = 0; i < 8; i++ )
  {
    colour = ~colour;
    for ( j = 0; j < 64; j++ )
    {
      writeDataWord( colour );
    }
  }

  drawString(  8,  8, "S S D 1 3 2 2 " );
  drawString( 16, 16, "4-bit  O L E D" );

  delay( 500 );

  // copy top page to bottom
  for ( i = 0; i < 24; i++ )
  {
    setPageAddr( 0 );
    setRowAddr( i );
    writeCommand( CMD_READ_RAM );
    for ( j = 0; j < 128; j++ )
    {
      readArray[j] = readDataByte();
    }

    setPageAddr( 0 );
    setRowAddr( i+32 );
    writeCommand( CMD_WRITE_RAM );
    for ( j = 0; j < 128; j++ )
    {
      b = reverseNibbles( readArray[127-j] );
      writeDataByte( b );  // reverse
    }
  }
}

// draw some shaded box patterns

void shades( void )
{
  word i, j, k, r, g, w;

  for ( k = 0; k < 4; k++ )
  {
    r = 4 + 8*k;
    setAddresses( 0, r, 32, 4 );
    writeCommand( CMD_WRITE_RAM );
    for ( j = 0; j < 3; j++ )
    {
      for ( i = 0; i < 16; i++ )
      {
        g = i;
        w = g << (k<<2);
        w = ~w;
        writeDataWord( 0 );
        writeDataWord( w );
      }
    }
  }
}

// draw individual pixels

void pixels( void )
{
  word x, y, g,
       p = random( 300 ) + 16;

  for ( word i = 0; i < p; i++ )
  {
    x = random( 256 );
    y = random(  64 );
    g = random(  15 ) + 1;
    drawPixel( x, y, g );
    delay( 2 );
  }
}

// draw some lines

void drawLines( void )
{
  word x0, y0,
       x1, y1,
       g,
       lines   = random( 16 ) + 4;

  for ( byte i = 0; i < lines; i++ )
  {
    x0 = random( 256 );
    y0 = random(  64 );
    x1 = random( 256 );
    y1 = random(  64 );
    g = random( 15 ) + 1;
    drawLine( x0, y0, x1, y1, g );
  //delay( 2 );
  }
}

// draw a cross-shaped figure

void drawCross( void )
{
  word x, y0, y1, g, k;
  for ( k = 0; k < 256; k++ )
  {
    x  = k;
    y0 = 63 - ( k >> 2 );
    y1 = k >> 2;
    g  = k & 0xF;
    drawVLine( x, y0, y1, g );
  //delay( 2 );
  }
}

// draw some rectangles

void drawRectangles( void )
{
  word x0, y0,
       x1, y1,
       g,
       rects   = random( 8 ) + 2;

  for ( byte i = 0; i < rects; i++ )
  {
    x0 = random( 256 );
    y0 = random(  64 );
    x1 = random( 256 );
    y1 = random(  64 );
    g = random( 15 ) + 1;
    drawRectangle( x0, y0, x1, y1, g, i & 1 );
  //delay( 2 );
  }
}

// draw some circles

void drawCircles( void )
{
  word xc, yc,
       r,
       g,
       circles = random( 15 ) + 2;

  for ( byte i = 0; i < circles; i++ )
  {
    xc = random( 192 ) + 32;
    yc = random(  32 ) + 16;
    r  = random(  22 ) +  2;
    g  = random(  15 ) +  1;
    drawCircle( xc, yc, r, ( ( i & 1 ) == 0 ), g );
  //delay( 100 );
  }
}

// ----------------

#define waittime  5000

void setup( void )
{
  // initialize ports/pins
  initIO();

  // debug console - uncomment the serial stuff if you want to debug
//Serial.begin( 9600 );
//Serial.println( "setup()" );

  // Fire up the OLED
//Serial.println( "initDisplay()" );

  initDisp();
  delay( 1000 );

  randomSeed( analogRead( A5 ) );
}

void loop( void )
{
//Serial.println( "loop()" );

//Serial.println( "textSamples()" );
  textSamples();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "checkerboard()" );
  checkerBoard();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "shades()" );
  shades();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "readDemo()" );
  readDemo();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "pixels()" );
  pixels();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "drawLines()" );
  drawLines();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "drawCross()" );
  drawCross();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "drawRectangles()" );
  drawRectangles();
  delay( waittime );
  fillDisplay( 0x0000 );

//Serial.println( "drawCircles()" );
  drawCircles();
  delay( waittime );
  fillDisplay( 0x0000 );

}

//#define _READ_TEST 1

#ifdef _READ_TEST

char strbuf[48];

void loop()
{
  word i, j, k, g, h, w, b[4];

  // 4 blocks
  setAddresses( 2, 8, 2, 8 );
  writeCommand( CMD_WRITE_RAM );
  w = 0x0123;  writeDataWord( w );  writeDataWord( w );
  w = 0x4567;  writeDataWord( w );  writeDataWord( w );
  w = 0x89AB;  writeDataWord( w );  writeDataWord( w );
  w = 0xCDEF;  writeDataWord( w );  writeDataWord( w );
  w = 0x0123;  writeDataWord( w );  writeDataWord( w );
  w = 0x4567;  writeDataWord( w );  writeDataWord( w );
  w = 0x89AB;  writeDataWord( w );  writeDataWord( w );
  w = 0xCDEF;  writeDataWord( w );  writeDataWord( w );

  delay( 500 );

  setAddresses( 2, 8, 1, 4 );
  writeCommand( CMD_READ_RAM );
  for ( k = 0; k < 4; k++ )
  {
    b[k] = readDataWord();
  //sprintf( strbuf, "%2d = %04X", k, b[k] );
  //Serial.println( strbuf );
  }

  setAddresses( 8, 8, 8, 32 );
  writeCommand( CMD_WRITE_RAM );
  for ( k = 0; k < 8; k++ )
  {
    w = b[k%4];
    for ( j = 0; j < 8; j++ )
    {
      for ( i = 0; i < 4; i++ )
      {
        g = get_p( w, i );
        h = ( g << 12 ) | ( g << 8 ) | ( g << 4 ) | g;
        writeDataWord( h );
      }
    }
  }

  while ( 1 ) { }
}

#endif

// ----------------
