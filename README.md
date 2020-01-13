# teensy2dmd
Teensy LED Smartmatrix display for an animated GIF Arcade Marquee or text display


This is a project to create a low cost animated 128x32 LED display for the marquee of a full-size MAME Arcade cabinet. The provided code allows the Teensy to process a simple command line protocol to browse the contents of the Teensy SD card, upload and download files using the ZMODEM protocol, change directories, display text messages and display animated GIF's on the attached LED panels. Displayed GIF's should have a resolution of 132x64 to match the attached panels.


# Background

During the Christmas period I had planned to update my full-size upright Arcade cabinet with the latest version of MAME and a new GUI front-end. By accident I stumbled on Pixelcade, a LED marquee for displaying animated GIF's. It was integrated with MAME to display different graphics depending on what you were doing. Unfortunately they where in a different country and given it was Christmas there was no way to get one in a reasonable time period.  Also they used a raspberrypi with a WEB based REST interface for control which I did not want to use. 

Lucky I keep a number of Teensy boards and LED panels to hand and a quick web search lead me to the Smartmatrix library. Unfortunately I did not have the smartmatrixv4 hardware but I was able to order it with next day delivery for ~£18 from a UK supplier. Starting with a Teensy3.2 and the SmartmatrixV4 hardware it only took a few minutes to get LED panels showing animated GIFS using the examples. To be honest it took me longer to find suitably formatted animated GIF's than it did to get the code running. 

The smartmatrix library example only shows random animated GIFs from a specific folder while I wanted to drive the display via serial commands. I also quickly discovered having to extract the SD card every time I wanted to add a file to it was a major pain-in-the-ass. It was also not practical to remove the SD card in my intended application as the board would be inaccessible once inside my arcade cabinet. I was also keen to drive my planned display via BASH or BAT scripts.

I did look at using the Teensy as an MTP device, i.e. it would appear on a windows machine like a mobile phone device, but I could not get this to work. So then I had a look for serial transfer protocols. A few minutes searching and I found a ZMODEM implementation offering serial commands, file upload, download and directory listing of the SD card. A few hours later and I had the code hacked together with the Smartmatrix animated GIF example. 

Since then I have implemented a few changes and added a few new features such as switching to the SDFAT libs for long filename support, implementing commands to allow for overlay text display, changed it to display files from the current working directory and a random display function that uses the currently active directory.

Since starting this project I discovered there are various similar devices called NAME-YOUR-PROCESSOR2dmd systems. Most based around the raspberry-pi. Some of these implement clocks using animated backgrounds which would be an easy feature to add to this code using the overlay text feature but its not something I need.


While the code is configured for a 128x32 display it will work at 64x32 using a single panel and a #define change. I have also tested the code using a 256x32 pixel display created from four 64x32 panels. This requires changing the uint8_t defining "kMatrixWidth" to a uint16_t (the rest of the code seemed to work fine).

Original sources:

	https://github.com/pixelmatix/SmartMatrix
	
	https://github.com/ecm-bitflipper/Arduino_ZModem


# Physical requirements

  2 x 64x32 P5 Led panels
  
  1 x 5V 3A mains power supply (to power the panels)
  
  1 x Teensy3.6 (Smaller boards don't have enough RAM)
 
  1 x SmartmatrixV4 interface
 
  1 x micro SD card




Example static GIF displayed on the panels
![alt text](https://github.com/gi1mic/teensy2dmd/blob/master/photos/street-fighter.jpg " Example static GIF displayed on the panels")


# Implemented Commands (may change):

For testing you can use the Arduino IDE serial monitor to communicate with the programmed board at 57600 baud. But I highly recommend using "Tetra Term" for windows for more advanced testing. This is a free terminal emulator which supports uploading and downloading files using the ZMODEM protocol.

Once connected type "help" and you should see the following:

	
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

	TM       - Set text mode - alternate TMODE

			Options - bounceForward, bounceReverse, stopped, off, wrapForwardFromLeft

	TF       - Set text font (default font3x5) - alternate TFONT

			Options - font3x5, font5x7, font6x10, font8x13, gohufont11, gohufont11b

	TL       - Set text loop count (default 1) - alternate TLOOP

	TOL      - Set text left start offset - alternate LEFTOFFSET

	TOT      - Set text top offset - alternate TOPOFFSET


# GIF's for your SD card
As a starting point there are 600 excellent free 128x32 animations made by eLLuiGi available from http://www.neo-arcadia.com/forum/viewtopic.php?f=14&t=67065#p1233644
He has a lot more that can be purchased for a small donation.

You can also use Googles advanced search feature to find animated GIF's that match a specific resolution.

# Building the Hardware
Ok there is now building, this is basically plug and play. You simply plug the Teensy 3.6 into the SmartmatrixV4 card. Then plug the Smartmatrix card into the HUB75 input socket on the rear of the LED panel. Then connect the HUB75 output socket from the first panel to the input socket on the second panel using the ribbon cable which came with the panels.
Finally power the both panels using a 5V 3A power supply using the power cable that came with the panels. It is fine if the power supply provides more amps, but it must be at 5V.  

Installed Teensy 3.6 and SmartMatrixV4 HW
![alt text](https://github.com/gi1mic/teensy2dmd/blob/master/photos/Installed%20Hardware.JPG " Installed Teensy 3.6 and SmartMatrixV4 HW")

Power and HUB75 Interconnect
![alt text](https://github.com/gi1mic/teensy2dmd/blob/master/photos/Pwr%20%26%20interconnect.jpg " Power and HUB75 Interconnect")


# Notes
The Teensy 3.6 is required due to the extra memory it provides. You can get away with a Teensy3.2 if you only using
a single 64x32 panel but you will need to physically attach a SD card and modify the code for the correct chip select (CS). The Teensy4.0 is not compatible with the Smartmatrix library.

