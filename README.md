# teensy2dmd
Teensy LED Smartmatrix display for an animated gif Arcade Marquee or text display



This is a project to create a low cost animated 128x32 LED display for the marquee of a MAME Arcade cabinet.

The hardware consists of:

  two daisy chained 64x32 P5 Led panels,
  a 5V 3A mains power supply for the panels,
  a Teensy3.6,
  a SmartmatrixV4 board providing the interconnect between the teensey3.6 and the HUB75 interface used by the LED panels
  and a SD card
 
The software is  a merger of the Animated GIF example from the smartmatrixV4 library and the Arduino zmodem example see:
	https://github.com/pixelmatix/SmartMatrix
	https://github.com/ecm-bitflipper/Arduino_ZModem
for more info.


The code allows the Teensy to process a simple command line protocol allowing a user to browse the contents of
a SD card fitted in the Teensy SD slot, upload and download files using the ZMODEM protocol and to display
animated GIF's on the attached LED panels. Displayed GIF files should have a resolution of 132x64 to match the 
display panels.

You can also add animated text messages over the animated GIF's if you so desire. Hardware such as the raspberrypi2dmd 
use this feature to create a digital clock function. This would be pretty easy to implement but I'll leave that for another
day. 


Implemented Commands:
	
Teensy2DMD V1.0 - Transfer rate: 57600
Available Commands:
HELP     - Print this list of commands
BRI      - Sets the brightness 0 - 255 (default)
DIR      - List files in current working directory - alternate LS
DIS <gif>- Display the gif (from the gifs directory)
PWD      - Print current working directory
CD       - Change current working directory
DEL file - Delete file - alternate RM
MD  dir  - Create dir - alternate MKDIR
RD  dir  - Delete dir - alternate RMDIR
RND      - Display random images - alternate RANDOM
SZ  file - Send file from Teensy SD to terminal (* = all files)
RZ       - Receive a file from terminal to Teensy SD(Hyperterminal sends this
           automatically when you select Transfer->Send File...)
TXT      - Define displayed text - alternate TEXT
TM       - Set text mode  - alternate TMODE
           Options - bounceForward, bounceReverse, stopped, off, wrapForwardFromLeft
TF       - Set text font (default font3x5) - alternate TFONT
           Options - font3x5, font5x7, font6x10, font8x13, gohufont11, gohufont11b
TL       - Set text loop count (default 1) - alternate TLOOP
TOL      - Set text left start offset - alternate LEFTOFFSET
TOT      - Set text top offset - alternate TOPOFFSET
 
