/*
 * lcd-adafruit.c:
 *	Text-based LCD driver test code
 *	This is designed to drive the Adafruit RGB LCD Plate
 *	with the additional 5 buttons for the Raspberry Pi
 *
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://github.com/WiringPi/WiringPi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <wiringPi.h>
#include <mcp23017.h>
#include <lcd.h>

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (1 == 2)
#endif

// Defines for the Adafruit Pi LCD interface board

#define AF_BASE 100
#define AF_RED (AF_BASE + 6)
#define AF_GREEN (AF_BASE + 7)
#define AF_BLUE (AF_BASE + 8)

#define AF_BACKLIGHT (AF_BASE + 5)

#define AF_E (AF_BASE + 13)
#define AF_RW (AF_BASE + 14)
#define AF_RS (AF_BASE + 15)

#define AF_DB4 (AF_BASE + 12)
#define AF_DB5 (AF_BASE + 11)
#define AF_DB6 (AF_BASE + 10)
#define AF_DB7 (AF_BASE + 9)

#define AF_SELECT (AF_BASE + 0)
#define AF_RIGHT (AF_BASE + 1)
#define AF_DOWN (AF_BASE + 2)
#define AF_UP (AF_BASE + 3)
#define AF_LEFT (AF_BASE + 4)

// Global lcd handle:

static int lcdHandle;

/*
 * usage:
 *********************************************************************************
 */

int usage(const char *progName)
{
    fprintf(stderr, "Usage: %s colour line1\n", progName);
    return EXIT_FAILURE;
}


/*
 * setBacklightColour:
 *	The colour outputs are inverted.
 *********************************************************************************
 */

static void setBacklightColour(int colour)
{
    colour &= 7;

    digitalWrite(AF_RED, !(colour & 1));
    digitalWrite(AF_GREEN, !(colour & 2));
    digitalWrite(AF_BLUE, !(colour & 4));
}

/*
 * adafruitLCDSetup:
 *	Setup the Adafruit board by making sure the additional pins are
 *	set to the correct modes, etc.
 *********************************************************************************
 */

static void adafruitLCDSetup(int colour)
{
    int i;

    //	Backlight LEDs

    pinMode(AF_RED, OUTPUT);
    pinMode(AF_GREEN, OUTPUT);
    pinMode(AF_BLUE, OUTPUT);

    setBacklightColour(colour);

    //	Input buttons

    for (i = 0; i <= 4; ++i)
    {
        pinMode(AF_BASE + i, INPUT);
        pullUpDnControl(AF_BASE + i, PUD_UP); // Enable pull-ups, switches close to 0v
    }

    // Control signals

    pinMode(AF_RW, OUTPUT);
    digitalWrite(AF_RW, LOW); // Not used with wiringPi - always in write mode

    pinMode(AF_BACKLIGHT, OUTPUT);
    digitalWrite(AF_BACKLIGHT, LOW); // lcd

    // The other control pins are initialised with lcdInit ()

    lcdHandle = lcdInit(2, 16, 4, AF_RS, AF_E, AF_DB4, AF_DB5, AF_DB6, AF_DB7, 0, 0, 0, 0);

    if (lcdHandle < 0)
    {
        fprintf(stderr, "lcdInit failed\n");
        exit(EXIT_FAILURE);
    }
}


/*
 * The works
 *********************************************************************************
 */

int main(int argc, char *argv[])
{
    int colour;
    char line1[10]={0};
    //char line2[17]={0};
    
    if (argc != 3)
        return usage(argv[0]);

    strcpy(line1,argv[2]);
    //strcpy(line2,argv[3]);

    colour = atoi(argv[1]);
 
    wiringPiSetupSys();
    mcp23017Setup(AF_BASE, 0x20);
    adafruitLCDSetup(colour);

    if (strcmp(line1, "0") == 0)
    {
        lcdClear(lcdHandle);
    }
    else
    {
        lcdPosition(lcdHandle, 0, 0);
        lcdPuts(lcdHandle, line1);
        
        //if(strstr(line2, "0")==NULL)  
        //    lcdPosition(lcdHandle, 0, 1);
        //    lcdPuts(lcdHandle, line2);
    }
    
    return 0;
}
