/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

#if defined (ARDUINO)
#include "zmodem_config.h"
#include <SdFat.h>
extern SdFatSdioEX sd;
#endif

File file;

char filename[40];
int numberOfFiles;

bool fileSeekCallback(unsigned long position)
{
    return file.seek(position);
}

unsigned long filePositionCallback(void)
{
    return file.position();
}

int fileReadCallback(void)
{
    return file.read();
}

int fileReadBlockCallback(void *buffer, int numberOfBytes)
{
    return file.read((uint8_t *)buffer, numberOfBytes);
}

bool isAnimationFile(const char filename [])
{
    String filenameString(filename);

    DSERIAL.print(filenameString);
    DSERIAL.flush();

    if ((filenameString[0] == '_') || (filenameString[0] == '~') || (filenameString[0] == '.'))
    {
        DSERIAL.println(" ignoring: leading _/~/. character");
        DSERIAL.flush();
        return false;
    }

    filenameString.toUpperCase();
    if (filenameString.endsWith(".GIF") != 1)
    {
        DSERIAL.println(" ignoring: doesn't end of .GIF");
        DSERIAL.flush();
        return false;
    }

    DSERIAL.println();

    return true;
}

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, bool displayFilenames)
{
    numberOfFiles = 0;

    File directory = sd.open(directoryName);
    if (!directory)
    {
        return -1;
    }

    File file = directory.openNextFile();
    while (file)
    {
        file.getName((const char *)filename, 40);
        if (isAnimationFile(filename))
        {
            numberOfFiles++;
            if (displayFilenames == true)
            {
                file.getName((const char *)filename, 40);
                DSERIAL.println(filename);
            }
        }
        file.close();
        file = directory.openNextFile();
    }

    file.close();
    directory.close();

    return numberOfFiles;
}

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer)
{
    // Make sure index is in range
    if ((index < 0) || (index >= numberOfFiles))
        return;

    File directory = sd.open(directoryName);
    if (!directory)
        return;

    File file = directory.openNextFile();
    while (file && (index >= 0))
    {
        file.getName((const char *)pnBuffer, 40);

        if (isAnimationFile(pnBuffer))
        {
            index--;
        }

        file.close();
        file = directory.openNextFile();
    }

    file.close();
    directory.close();
}


int openGifFilenameByIndex(const char *directoryName, int index)
{
    char pathname[40];

    getGIFFilenameByIndex(directoryName, index, pathname);

    DSERIAL.print("Pathname: ");
    DSERIAL.println(pathname);
    DSERIAL.flush();

    if(file)
        file.close();

    // Attempt to open the file for reading
    file = sd.open(pathname);
    if (!file)
    {
        DSERIAL.println("Error opening GIF file");
        DSERIAL.flush();
        return -1;
    }
    return 0;
}


int openGifFilenameByFilename(const char *pathname)
{
    if(file)
        file.close();

    // Attempt to open the file for reading
    file = sd.open(pathname);
    if (!file)
    {
        DSERIAL.print("Error opening GIF file: ");
        DSERIAL.println(pathname);
        DSERIAL.flush();
        return -1;
    }

    DSERIAL.print("file: ");
    DSERIAL.println(pathname);
    DSERIAL.println(">");
    DSERIAL.flush();
    return 0;
}
