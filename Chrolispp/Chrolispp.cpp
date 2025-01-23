// Chrolispp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/***
  * Additional Include Paths:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Include"    [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Include"          [64Bit]
  *   "C:\Program Files\Thorlabs\upSERIES\Drivers\Instr\incl"       [No VXIPnP Folder]
  *
  * Additional Library Directories:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\lib\msc"    [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Lib_x64\msc"      [64Bit]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc"        [32Bit - No VXIPnP Folder]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\msvc64"      [64Bit - No VXIPnP Folder]
  *
  * Additional Dependencies:
  *   "TL6WL_32.lib"                                                [32Bit]
  *   "TL6WL_64.lib"                                                [64Bit]
  *
  * Library Locations:
  *   "C:\Program Files (x86)\IVI Foundation\VISA\WinNT\Bin"        [32Bit]
  *   "C:\Program Files\IVI Foundation\VISA\Win64\Bin"              [64Bit]
  *   "C:\Program Files\Thorlabs\CHROLIS\Drivers\Instr\bin"         [No VXIPnP Folder]
  *
  * Libraries:
  *   "TL6WL_32.dll"                                                [32Bit]
  *   "TL6WL_64.dll"                                                [64Bit]
 ***/

/*
* Given a csv file with the following columns in this order:
 * 1. LED index (0-5)
 * 2. Pulse length (ms)
 * 3. Time between pulses (ms)
 * 4. Number of pulses (integer)
 * 5. brightness (integer; 1000 is 100.0%, e.g. 555 is 55.5% power in Chrolis)
 * Each row with these columns corresponds to one step in the protocol.
 * These rows (steps) will be executed in order. The total time of one step is
 * n_pulses * (pulse_length + time_between_pulses) ms.
 */

#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "TL6WL.h"

void LED_PulseNTimes(ViSession instr, ViInt8 led_index,
                     ViInt16 time_between_pulses_ms, ViInt16 pulse_width_ms,
                     ViInt32 n_pulses, ViInt16 brightness) {
  int led_brightnesses[6] = {0, 0, 0, 0, 0, 0};
  ViBoolean led_states[6] = {VI_FALSE, VI_FALSE, VI_FALSE,
                             VI_FALSE, VI_FALSE, VI_FALSE};
  led_brightnesses[led_index] = brightness;
  led_states[led_index] = VI_TRUE;
  for (int i = 0; i < n_pulses; i++) {
    TL6WL_setLED_HeadBrightness(instr, led_brightnesses[0], led_brightnesses[1],
                                led_brightnesses[2], led_brightnesses[3],
                                led_brightnesses[4], led_brightnesses[5]);
    TL6WL_setLED_HeadPowerStates(instr, led_states[0], led_states[1],
                                 led_states[2], led_states[3], led_states[4],
                                 led_states[5]);
    Sleep(pulse_width_ms);
    TL6WL_setLED_HeadPowerStates(instr, VI_FALSE, VI_FALSE, VI_FALSE, VI_FALSE,
                                 VI_FALSE, VI_FALSE);
    Sleep(time_between_pulses_ms);
  }
}



int main()
{
    ViStatus err;
#ifdef WIN32
    ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x86";
#else
    ViChar bitness[TL6WL_LONG_STRING_SIZE] = "x64";
#endif
    printf("This is start of Thorlabs CHROLIS TL6WL logging output [%s]!\n\n",
           bitness);

    printf("START : Chrolis++\n");

    printf("Search for connected CHROLIS Devices\n");
    ViSession instr = 0;
    ViUInt32 rsrcCnt = 0;
    err = TL6WL_findRsrc(instr, &rsrcCnt);
    printf("Found '%lu' Devices\n" , rsrcCnt);
    if (VI_SUCCESS != err)
    {
        printf("  TL6WL_findRsrc() :\n    Error Code = %#.8lX\n" , err);
        printf("\nSample Terminated\n");
        return -1;
    }

    printf("\nGet Information of found Devices\n");
    ViChar resourceName[512] = "n/a";
    ViChar modelName[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViChar serialNumber[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViChar manufacturer[TL6WL_LONG_STRING_SIZE] = "n/a";
    ViBoolean resourceAvailable = VI_FALSE;
    ViBoolean IDQuery = VI_FALSE;
    ViBoolean resetDevice = VI_FALSE;
    for (ViInt32 i = 0; rsrcCnt > i; ++i)
    {
        err = TL6WL_getRsrcName(instr , i , resourceName);
        if (VI_SUCCESS == err)
        {
            printf("    Resource Name DeviceID[%lu] = <%s>\n" , 1 + i, resourceName);

            err = TL6WL_getRsrcInfo(instr , i , modelName , serialNumber , manufacturer , &resourceAvailable);
            if (VI_SUCCESS == err)
            {
                printf("      Model Name = <%s>\n      SerNo = <%s>\n      Manufacturer = <%s>\n      Available = <%s>\n" ,
                       modelName , serialNumber , manufacturer , resourceAvailable == VI_TRUE ? "Yes" : "No");
            }
            else
            {
                printf("  TL6WL_getRsrcInfo() :    Error Code = %#.8lX\n" , err);
            }
        }
        else
        {
            printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n" , err);
        }
    }

    printf("\nOpen first available Device found\n");
    err = TL6WL_getRsrcName(instr , 0 , resourceName);
    if (VI_SUCCESS != err)
    {
        printf("  TL6WL_getRsrcName() :\n    Error Code = %#.8lX\n" , err);
        printf("\nSample Terminated\n");
        return -2;
    }
    err = TL6WL_init(resourceName , IDQuery , resetDevice , &instr);
    if (VI_SUCCESS != err)
    {
        printf("  TL6WL_init() :\n    Error Code = %#.8lX\n" , err);
        printf("\nSample Terminated\n");
        return -3;
    }

    printf("\nRead Box Status Register\n");
    ViUInt32 boxStatus;
    err = TL6WL_getBoxStatus(instr , &boxStatus);
    int bit0 , bit1 , bit2 , bit3 , bit4 , bit5 , bit6;
    if (bit0 = (boxStatus & 0x01))
    {
        printf(" Box is open\n");
    }
    else if (bit1 = (boxStatus & 0x02))
    {
        printf(" LLG not connected\n");
    }

    else if (bit2 = (boxStatus & 0x04))
    {
        printf(" Interlock is open\n");
    }

    else if (bit3 = (boxStatus & 0x08))
    {
        printf(" Using default adjustment\n");
    }

    else if (bit4 = (boxStatus & 0x10))
    {
        printf(" Box overheated\n");
    }

    else if (bit5 = (boxStatus & 0x20))
    {
        printf(" LED overheated\n");
    }

    else if (bit6 = (boxStatus & 0x40))
    {
        printf(" Box setup invalid\n");
    }
    LED_PulseNTimes(instr, 2, 200, 100, 10, 100);

    printf("\nClose Device\n");
    err = TL6WL_close(instr);
    if (VI_SUCCESS != err)
    {
        printf("  TLUP_close() :\n    Error Code = %#.8lX\n" , err);
    }
    printf("\nProtocol End\n");
    return 0;
}



// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
