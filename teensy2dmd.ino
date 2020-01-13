#include "Arduino.h"
#include "info.h"
#include "zmodem_config.h"
#include "zmodem.h"
#include "zmodem_zm.h"

/* 
 *  https://github.com/gi1mic/teensy2dmd
 *  
 *  
 *  This program is a merger of the arduino zmodem implementation from
 *  https://github.com/ecm-bitflipper/Arduino_ZModem and the teensey smartmatrix library
 *  from https://github.com/pixelmatix/SmartMatrix
 *  
 *  It allows the display of animated gifs and text and is intended for use as a banner on an arcade machine.
 *  
 *  It has been modified to run on a Teensey 3.6 board and uses the smartmatrix V4
 *  shield for interfacing to two 64x32 LED panels to create a 128x32 display using a HUB75 interface.
 *  
 *  https://www.adafruit.com/product/1902
 *  https://www.pjrc.com/store/teensy36.html
 *  
 *  A SD card is used to hold the animated GIF's
 *  
 *  It has been tested with Tera Term 4.96 running on a PC. This allows the user to view the contents 
 *  of the SD card, change directories, upload/download files using the zmodem protocol and display GIF
 *  files from the current active directory.
 *  
 *  You can find 600 free 128x32 animations made by eLLuiGi at http://www.neo-arcadia.com/forum/viewtopic.php?f=14&t=67065#p1233644
 *  He can provide a few thousand more animations for a small donation.
 *  
 *  More information about the origional code is detailed in info.h (Please read)
*/



#include <SmartLEDShieldV4.h>  // comment out this line for if you're not using SmartLED Shield V4 hardware (this line needs to be before #include <SmartMatrix3.h>)
#include <SmartMatrix3.h>
#include <SdFat.h>  // From https://github.com/adafruit/SdFat
#include "GifDecoder.h"
#include "FilenameFunctions.h"

SdFatSdioEX sd;

#define DISPLAY_TIME_SECONDS 10
#define error(s) sd.errorHalt(s)
#define DSERIALprintln(_p) ({ DSERIALprint(_p); DSERIAL.write("\r\n"); })
#define fname (&oneKbuf[512])
#define dir ((dir_t *)&oneKbuf[256])

extern int Filesleft;
extern long Totalleft;

SdFile fout;

int num_files;
int defaultBrightness = 255; // range 0-255

rgb24 tColour = {0xff, 0xff, 0xff};
ScrollMode  tMode = wrapForward;   //  wrapForward, bounceForward, bounceReverse, stopped, off, wrapForwardFromLeft
int tSpeed = 40;
fontChoices  tFont = font3x5; //     font3x5, font5x7, font6x10, font8x13, gohufont11, gohufont11b
char tText[50] = "Undefined text message";
int tLoopCount = 1;

const rgb24 COLOR_BLACK = { 0, 0, 0 };

/* SmartMatrix configuration and memory allocation */
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
#define refreshRate 90
const uint8_t kMatrixWidth = 128;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 2;       // known working: 2-4

const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels, or use SMARTMATRIX_HUB75_64ROW_MOD32SCAN for common 64x64 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);    // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);

// template parameters are maxGifWidth, maxGifHeight, lzwMaxBits
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

size_t DSERIALprint(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1)
  {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    if (DSERIAL.availableForWrite() > SERIAL_TX_BUFFER_SIZE / 2) DSERIAL.flush();
    if (DSERIAL.write(c)) n++;
    else break;
  }
  return n;
}


//------------------------------------------------
void screenClearCallback(void)
{
  backgroundLayer.fillScreen({0, 0, 0});
}

//------------------------------------------------
void updateScreenCallback(void)
{
  backgroundLayer.swapBuffers();
}

//------------------------------------------------
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  backgroundLayer.drawPixel(x, y, {red, green, blue});
}


//------------------------------------------------
void help(void)
{
  DSERIALprint(Progname);
  DSERIALprint(F(" - Transfer rate: ")); DSERIAL.flush();
  DSERIAL.println(ZMODEM_SPEED); DSERIAL.flush();
  DSERIALprintln(F("Available Commands:")); DSERIAL.flush();
  DSERIALprintln(F("HELP     - Print this list of commands")); DSERIAL.flush();
  DSERIALprintln(F("BRI      - Sets the brightness 0 - 255 (default)")); DSERIAL.flush();
  DSERIALprintln(F("DIR      - List files in current working directory - alternate LS")); DSERIAL.flush();
  DSERIALprintln(F("DIS <gif>- Display the gif (from the gifs directory)")); DSERIAL.flush();
  DSERIALprintln(F("PWD      - Print current working directory")); DSERIAL.flush();
  DSERIALprintln(F("CD       - Change current working directory")); DSERIAL.flush();
  DSERIALprintln(F("DEL file - Delete file - alternate RM")); DSERIAL.flush();
  DSERIALprintln(F("MD  dir  - Create dir - alternate MKDIR")); DSERIAL.flush();
  DSERIALprintln(F("RD  dir  - Delete dir - alternate RMDIR")); DSERIAL.flush();
  DSERIALprintln(F("RND      - Display random images - alternate RANDOM")); DSERIAL.flush();
  DSERIALprintln(F("SZ  file - Send file from Arduino to terminal (* = all files)")); DSERIAL.flush();
  DSERIALprintln(F("RZ       - Receive a file from terminal to Arduino (Hyperterminal sends this")); DSERIAL.flush();
  DSERIALprintln(F("           automatically when you select Transfer->Send File...)")); DSERIAL.flush();
  DSERIALprintln(F("TXT      - Define displayed text - alternate TEXT")); DSERIAL.flush();
  DSERIALprintln(F("TM       - Set text mode  - alternate TMODE")); DSERIAL.flush();
  DSERIALprintln(F("           Options - bounceForward, bounceReverse, stopped, off, wrapForwardFromLeft")); DSERIAL.flush();
  DSERIALprintln(F("TF       - Set text font (default font3x5) - alternate TFONT")); DSERIAL.flush();
  DSERIALprintln(F("           Options - font3x5, font5x7, font6x10, font8x13, gohufont11, gohufont11b")); DSERIAL.flush();
  DSERIALprintln(F("TL       - Set text loop count (default 1) - alternate TLOOP")); DSERIAL.flush();
  DSERIALprintln(F("TOL      - Set text left start offset - alternate LEFTOFFSET")); DSERIAL.flush();
  DSERIALprintln(F("TOT      - Set text top offset - alternate TOPOFFSET")); DSERIAL.flush();
}


//------------------------------------------------
//int asciiHexToInt(char c) {
//  int x = 0;
//  if (c >= "0" && c <= "9") x += c - "0";
//  else if (c >= "A" && c <= "F") x += c - "A";
//  return x;
//}

//------------------------------------------------
void setup()
{
  // NOTE: The following line needs to be uncommented if DSERIAL and ZSERIAL are decoupled again for debugging
  //  DSERIAL.begin(115200);

  ZSERIAL.begin(ZMODEM_SPEED);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);
  delay(400);

  //Initialize the SdCard.
  if (!sd.begin()) sd.initErrorHalt(&DSERIAL);
  if (!sd.chdir("/", true)) sd.errorHalt(F("sd.chdir"));

  sd.vwd()->rewind();

  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);

  // Init text
  scrollingLayer.setColor(tColour);
  scrollingLayer.setMode(tMode);
  scrollingLayer.setSpeed(tSpeed);
  scrollingLayer.setFont(tFont);

  // Initialize matrix
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.setBrightness(defaultBrightness);
  matrix.setRefreshRate(refreshRate);
  matrix.begin();

  // Clear screen
  backgroundLayer.fillScreen(COLOR_BLACK);
  backgroundLayer.swapBuffers(false);

  help();
}

//------------------------------------------------
int count_files(int *file_count, long *byte_count)
{
  *file_count = 0;
  *byte_count = 0;

  sd.vwd()->rewind();

  while (sd.vwd()->readDir(dir) == sizeof(*dir))
  {
    // read next directory entry in current working directory
    SdFile::dirName(dir, fname); // format file name

    // remember position in directory
    uint32_t pos = sd.vwd()->curPosition();

    if (!fout.open(fname, O_READ)) error(F("file.open failed"));    // open file
    else if (!sd.vwd()->seekSet(pos)) error(F("seekSet failed"));   // restore root position
    else if (!fout.isDir())
    {
      *file_count = *file_count + 1;
      *byte_count = *byte_count + fout.fileSize();
    }
    fout.close();
  }
  return 0;
}


//------------------------------------------------
void loop(void)
{
  char *cmd = oneKbuf;
  char *param;

  static unsigned long futureTime;
  char curPath[40];

  *cmd = 0;
  while (DSERIAL.available()) DSERIAL.read();

  char c = 0;
  while (1)
  {
    if (DSERIAL.available() > 0)
    {
      c = DSERIAL.read();
      if ((c == 8 or c == 127) && strlen(cmd) > 0) cmd[strlen(cmd) - 1] = 0;
      if (c == '\n' || c == '\r') break;
      DSERIAL.write(c);
      if (c != 8 && c != 127) strncat(cmd, &c, 1);
    }
    else
    {
      delay(20);
    }
    decoder.decodeFrame();
  }

  param = strchr(cmd, 32);
  if (param > 0)
  {
    *param = 0;
    param = param + 1;
  }
  else
  {
    param = &cmd[strlen(cmd)];
  }

  strupr(cmd);
  DSERIAL.println();


  if (!strcmp_P(cmd, PSTR("HELP")))
  {
    //------------------------------
    help();
  }
  else if (!strcmp_P(cmd, PSTR("DIR")) || !strcmp_P(cmd, PSTR("LS")))
  {
    //------------------------------
    char filename[40];

    sd.vwd()->getName(fname, 13);
    snprintf(curPath, sizeof(curPath), "/%s/", fname);
    curPath[strlen(curPath)] = '\0';

    DSERIALprint(F("Directory Listing for: "));
    DSERIALprintln(F(curPath)); DSERIAL.flush();
    File directory = sd.open(curPath);
    if (!directory)
    {
      DSERIALprint(F("Not a Directory")); DSERIAL.flush();
      return -1;
    }

    File file = directory.openNextFile();
    while (file)
    {
      file.getName((const char *)filename, 40);
      DSERIALprintln(F(filename)); DSERIAL.flush();
      file.close();
      file = directory.openNextFile();
    }

    file.close();
    directory.close();

    DSERIALprintln(F("End of Directory"));  DSERIAL.flush();
  }
  else if (!strcmp_P(cmd, PSTR("BRI"))) {
    //------------------------------
    DSERIALprint(F("Brightness: "));
    DSERIAL.print(param); DSERIAL.flush();
    defaultBrightness = atoi(param);
    matrix.setBrightness(defaultBrightness);

  }
  else if (!strcmp_P(cmd, PSTR("PWD")))
  {
    //------------------------------
    sd.vwd()->getName(fname, 13);
    DSERIALprint(F("Current working directory is "));
    DSERIAL.println(fname); DSERIAL.flush();

  }
  else if (!strcmp_P(cmd, PSTR("CD")))
  {
    //------------------------------
    if (!sd.chdir(param, true))
    {
      DSERIALprint(F("Directory "));
      DSERIAL.print(param);
      DSERIALprintln(F(" not found")); DSERIAL.flush();
    }
    else
    {
      DSERIALprint(F("Current directory changed to "));
      DSERIAL.println(param); DSERIAL.flush();
    }
  }
  else if (!strcmp_P(cmd, PSTR("DIS")))
  {
    //------------------------------
    char filename[40];

    sd.vwd()->getName(fname, 13);
    snprintf(filename, sizeof(filename), "/%s/%s", fname, param);
    filename[strlen(filename)] = '\0';

    if (openGifFilenameByFilename((const char *) filename) > 0)
    {
      DSERIALprint(F("Failed to display "));
      DSERIAL.println(param); DSERIAL.flush();
    }
    else
    {
      DSERIALprint(F("Displaying "));
      DSERIAL.println(param);  DSERIAL.flush();
      backgroundLayer.fillScreen(COLOR_BLACK);
      backgroundLayer.swapBuffers();
      decoder.startDecoding();

      scrollingLayer.start(tText, tLoopCount);
    }
  }
  else if (!strcmp_P(cmd, PSTR("TXT")) || !strcmp_P(cmd, PSTR("TEXT")))
  {
    //------------------------------
    DSERIALprint(F("Text: "));
    DSERIAL.println(param);  DSERIAL.flush();

    scrollingLayer.start(param, tLoopCount);
  }
  else if (!strcmp_P(cmd, PSTR("TL")) || !strcmp_P(cmd, PSTR("TLOOP")))
  {
    //------------------------------
    DSERIALprint(F("Tloop: "));
    DSERIAL.println(param);  DSERIAL.flush();

    tLoopCount = (atoi(param));
  }
  else if (!strcmp_P(cmd, PSTR("TS")) || !strcmp_P(cmd, PSTR("TSPEED")))
  {
    //------------------------------
    DSERIALprint(F("Tspeed: "));
    DSERIAL.println(param);  DSERIAL.flush();

    scrollingLayer.setSpeed(atoi(param));

  }
  else if (!strcmp_P(cmd, PSTR("TOT")) || !strcmp_P(cmd, PSTR("TOPOFFSET")))
  {
    //------------------------------
    DSERIALprint(F("Top Offest: "));
    DSERIAL.println(param);  DSERIAL.flush();

    if (atoi(param) > kMatrixHeight) {
      DSERIALprintln(F("Offset exceeds display height")); DSERIAL.flush();
    } else
      scrollingLayer.setOffsetFromTop(atoi(param));
  }
  else if (!strcmp_P(cmd, PSTR("TOL")) || !strcmp_P(cmd, PSTR("LEFTOFFSET")))
  {
    //------------------------------
    DSERIALprint(F("Left Start Offest: "));
    DSERIAL.println(param);  DSERIAL.flush();

    if (atoi(param) > kMatrixWidth) {
      DSERIALprintln(F("Offset exceeds display width")); DSERIAL.flush();
    } else
      scrollingLayer.setStartOffsetFromLeft(atoi(param));
  }
  else if (!strcmp_P(cmd, PSTR("TM")) || !strcmp_P(cmd, PSTR("TMODE")))
  {
    //------------------------------
    DSERIALprint(F("tMode: "));
    DSERIAL.println(param);  DSERIAL.flush();

    strupr(param);

    if (String(param) == PSTR("WRAPFORWARDFROMLEFT")) {
      scrollingLayer.setMode(wrapForwardFromLeft);
    }
    if (String(param) == PSTR("WRAPFORWARD")) {
      scrollingLayer.setMode(wrapForwardFromLeft);
    }
    if (String(param) == PSTR("BOUNCEFORWARD")) {
      scrollingLayer.setMode(bounceForward);
    }
    if (String(param) == PSTR("BOUNCEREVERSE")) {
      scrollingLayer.setMode(bounceReverse);
    }
    if (String(param) == PSTR("STOPPED")) {
      scrollingLayer.setMode(stopped);
    }
    if (String(param) == PSTR("OFF")) {
      scrollingLayer.setMode(off);
    }
  }
  else if (!strcmp_P(cmd, PSTR("TF")) || !strcmp_P(cmd, PSTR("TFONT")))
  {
    //------------------------------
    DSERIALprint(F("tFont: "));
    DSERIAL.println(param);  DSERIAL.flush();

    strupr(param);

    if (String(param) == PSTR("FONT3X5")) {
      scrollingLayer.setFont(font3x5);
    }
    if (String(param) == PSTR("FONT5X7")) {
      scrollingLayer.setFont(font5x7);
    }
    if (String(param) == PSTR("FONT6X10")) {
      scrollingLayer.setFont(font6x10);
    }
    if (String(param) == PSTR("FONT8X13")) {
      scrollingLayer.setFont(font8x13);
    }
    if (String(param) == PSTR("GOHUFONT11")) {
      scrollingLayer.setFont(gohufont11);
    }
    if (String(param) == PSTR("GOHUFONT11B")) {
      scrollingLayer.setFont(gohufont11b);
    }
  }
  else if (!strcmp_P(cmd, PSTR("DEL")) || !strcmp_P(cmd, PSTR("RM")))
  {
    //------------------------------
    if (!sd.remove(param))
    {
      DSERIALprint(F("Failed to delete file "));
      DSERIAL.println(param); DSERIAL.flush();
    }
    else
    {
      DSERIALprint(F("File "));
      DSERIAL.print(param);
      DSERIALprintln(F(" deleted")); DSERIAL.flush();
    }
  }
  else if (!strcmp_P(cmd, PSTR("MD")) || !strcmp_P(cmd, PSTR("MKDIR")))
  {
    //------------------------------
    if (!sd.mkdir(param, true))
    {
      DSERIALprint(F("Failed to create directory "));
      DSERIAL.println(param); DSERIAL.flush();
    }
    else
    {
      DSERIALprint(F("Directory "));
      DSERIAL.print(param);
      DSERIALprintln(F(" created")); DSERIAL.flush();
    }
  }
  else if (!strcmp_P(cmd, PSTR("RD")) || !strcmp_P(cmd, PSTR("RMDIR")))
  {
    //------------------------------
    if (!sd.rmdir(param))
    {
      DSERIALprint(F("Failed to remove directory "));
      DSERIAL.println(param); DSERIAL.flush();
    }
    else
    {
      DSERIALprint(F("Directory "));
      DSERIAL.print(param);
      DSERIALprintln(F(" removed"));  DSERIAL.flush();
    }
  }
  else if (!strcmp_P(cmd, PSTR("RND")) || !strcmp_P(cmd, PSTR("RANDOM")))
  {
    //------------------------------
    sd.vwd()->getName(fname, 13);
    //  if (sizeof(fname) > 1) {
    snprintf(curPath, sizeof(curPath), "/%s/", fname);
    //  }
    curPath[strlen(curPath)] = '\0';


    DSERIALprint(F("Displaying random gifs from "));
    DSERIALprintln(F(curPath));
    DSERIAL.flush();
    num_files = enumerateGIFFiles(curPath, (bool) false);

    randomSeed(analogRead(5));
    int index = random(num_files);

    while (1)
    {
      if (futureTime < millis())
      {
        index = random(num_files);
        if (openGifFilenameByIndex((const char *) curPath, index) >= 0)
        {
          backgroundLayer.fillScreen(COLOR_BLACK);
          backgroundLayer.swapBuffers();
          decoder.startDecoding();
          futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);
        }
      }
      decoder.decodeFrame();
      if (DSERIAL.available() > 0)
      {
        c = DSERIAL.read();
        if (c == '\n' || c == '\r') break;
      }
    };

  }
  else if (!strcmp_P(cmd, PSTR("SZ")))
  {
    //------------------------------
    //    Filcnt = 0;
    if (!strcmp_P(param, PSTR("*")))
    {
      count_files(&Filesleft, &Totalleft);
      sd.vwd()->rewind();

      if (Filesleft > 0)
      {
        ZSERIAL.print(F("rz\r"));
        sendzrqinit();
        delay(200);

        while (sd.vwd()->readDir(dir) == sizeof(*dir))
        {
          // read next directory entry in current working directory

          // format file name
          SdFile::dirName(dir, fname);

          // open file
          if (!fout.open(fname, O_READ)) error(F("file.open failed"));

          else if (!fout.isDir())
          {
            if (wcs(fname) == ERROR)
            {
              delay(500);
              fout.close();
              break;
            }
            else delay(500);
          }

          fout.close();
        }
        saybibi();
      }
      else
      {
        DSERIALprintln(F("No files found to send"));
      }
    }
    else if (!fout.open(param, O_READ))
    {
      DSERIALprintln(F("file.open failed")); DSERIAL.flush();
    }
    else
    {
      // Start the ZMODEM transfer
      Filesleft = 1;
      Totalleft = fout.fileSize();
      ZSERIAL.print(F("rz\r"));
      sendzrqinit();
      delay(200);
      wcs(param);
      saybibi();
      fout.close();
    }
  }
  else if (!strcmp_P(cmd, PSTR("RZ")))
  {
    //------------------------------
    //    DSERIALprintln(F("Receiving file..."));
    if (wcreceive(0, 0))
    {
      DSERIALprintln(F("zmodem transfer failed")); DSERIAL.flush();
    }
    else
    {
      DSERIALprintln(F("zmodem transfer successful"));
    }
    fout.flush();
    fout.sync();
    fout.close();
  }
}
