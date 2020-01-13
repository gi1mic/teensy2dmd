// Arghhh. These three links have disappeared!
// See this page for the original code:
// http://www.raspberryginger.com/jbailey/minix/html/dir_acf1a49c3b8ff2cb9205e4a19757c0d6.html
// From: http://www.raspberryginger.com/jbailey/minix/html/zm_8c-source.html
// docs at: http://www.raspberryginger.com/jbailey/minix/html/zm_8c.html
// The minix files here might be the same thing:
// http://www.cise.ufl.edu/~cop4600/cgi-bin/lxr/http/source.cgi/commands/zmodem/


// It DOES NOT handle interruptions of the Tx or Rx lines so it
// will NOT work in a hostile environment.
 
/*
  Originally was an example by fat16lib of reading a directory
  and listing its files by directory entry number.
See: http://forum.arduino.cc/index.php?topic=173562.0

  Heavily modified by Pete (El Supremo) to recursively list the files
  starting at a specified point in the directory structure and then
  use zmodem to transmit them to the PC via the ZSERIAL port

  Further heavy modifications by Dylan (monte_carlo_ecm, bitflipper, etc.)
  to create a user driven "file manager" of sorts.
  Many thanks to Pete (El Supremo) who got this started.  Much work remained
  to get receive (rz) working, mostly due to the need for speed because of the
  very small (64 bytes) Serial buffer in the Arduino.

  I have tested this with an Arduino Mega 2560 R3 interfacing with Windows 10
  using Hyperterminal, Syncterm and TeraTerm.  All of them seem to work, though
  their crash recovery (partial file transfer restart) behaviours vary.
  Syncterm kicks out a couple of non-fatal errors at the beginning of sending
  a file to the Arduino, but appears to always recover and complete the transfer.

  This sketch should work on any board with at least 30K of flash and 2K of RAM.
  Go to zmodem_config.h and disable some of the ARDUINO_SMALL_MEMORY_* macros
  for maximum peace of mind and stability if you don't need all the features
  (send, receive and file management).

V2.1.2
2018-05-11
  - Fixes for Arduino IDE 1.8.5
  - Attempted to patch for use on Teensy

V2.1
2015-03-06
  - Large scale code clean-up, reduction of variable sizes where they were
    unnecessarily large, sharing variables previously unshared between sz and
    rz, and creative use of the send/receive buffer allowed this sketch to
    BARELY fit and run with all features enabled on a board with 30K flash and
    2K of RAM.  Uno & Nano users - enjoy.
  - Some boards were unstable at baud rates above 9600.  I tracked this back
    to overrunning the SERIAL_TX_BUFFER_SIZE to my surprise.  Added a check
    if a flush() is required both in the help and directory listings, as well
    as the sendline() macro.

V2.0
2015-02-23
  - Taken over by Dylan (monte_carlo_ecm, bitflipper, etc.)
  - Added Serial based user interface
  - Added support for SparkFun MP3 shield based SDCard (see zmodem_config.h)
  - Moved CRC tables to PROGMEM to lighten footprint on dynamic memory (zmodem_crc16.cpp)
  - Added ZRQINIT at start of sz.  All terminal applications I tested didn't strictly need it, but it's
    super handy for getting the terminal application to auto start the download
  - Completed adaptation of rz to Arduino
  - Removed directory recursion for sz in favour of single file or entire current directory ("*") for sz
  - Optimized zdlread, readline, zsendline and sendline
      into macros for rz speed - still only up to 57600 baud
  - Enabled "crash recovery" for both sz and rz.  Various terminal applications may respond differently
      to restarting partially completed transfers; experiment with yours to see how it behaves.  This
      feature could be particularly useful if you have an ever growing log file and you just need to
      download the entries since your last download from your Arduino to your computer.

V1.03
140913
  - remove extraneous code such as the entire main() function
    in sz and rz and anything dependent on the vax, etc.
  - moved purgeline, sendline, readline and bttyout from rz to zm
    so that the the zmodem_rz.cpp file is not required when compiling
    sz 
    
V1.02
140912
  - yup, sz transfer still works.
    10 files -- 2853852 bytes
    Time = 265 secs
    
V1.01
140912
This was originally working on a T++2 and now works on T3
  - This works on a T3 using the RTC/GPS/uSD breadboard
    It sent multiple files - see info.h
  - both rz and sz sources compile together here but have not
    yet ensured that transmit still works.
    
V1.00
130630
  - it compiles. It even times out. But it doesn't send anything
    to the PC - the TTYUSB LEDs don't blink at all
  - ARGHH. It does help to open the Serial1 port!!
  - but now it sends something to TTerm but TTerm must be answering
    with a NAK because they just repeat the same thing over
    and over again.

V2.00
130702
  - IT SENT A FILE!!!!
    It should have sent two, but I'll take it!
  - tried sending 2012/09 at 115200 - it sent the first file (138kB!)
    but hangs when it starts on the second one. The file is created
    but is zero length.
    
  - THIS VERSION SENDS MULTIPLE FILES

*/
