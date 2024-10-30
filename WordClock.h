#ifndef WORD_CLOCK_H
#define WORD_CLOCK_H

// comment out to stop debugging output to Serial port, or #undef DEBUG
// code size WITH DEBUG - Sketch uses 807445 bytes (61%), Global variables use 45992 bytes (14%)
// code size W/O  DEBUG - Sketch uses 784921 bytes (59%), Global variables use 45364 bytes (13%)

// use #define or #undef to select ECHO (Serial.print), DEBUG and TEST_CLOCK options
#define ECHO
#undef DEBUG
#undef TEST_CLOCK

#include <Arduino.h>
#include <Math.h>               // for pow() conversion of RSSI signal strength (can discard later)
#include <WiFi.h>               // https://github.com/arduino-libraries/WiFi
#include <WiFiUDP.h>            // https://www.arduino.cc/reference/en/libraries/wifi/wifiudp/
#include <Timezone.h>           // https://github.com/JChristensen/Timezone
#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time
#include <NTPClient.h>          // https://github.com/arduino-libraries/NTPClient
#include <SunRise.h>            // https://github.com/signetica/SunRise
#include <MoonRise.h>           // https://github.com/signetica/MoonRise
#include <MoonPhase.h>          // https://github.com/signetica/MoonPhase
#include <Adafruit_NeoPixel.h>  // https://github.com/adafruit/Adafruit_NeoPixel
#include "utils.h"              // local wifi ssid/pwd etc
#include <esp_task_wdt.h>       // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html
                                // https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/

/* Note: ESP32 board 3.0.0 or over causes stack overflow in Adafruit Neopixel library for Npixel>75
   Solution: DO NOT UPGRADE ESP32 board to 3.0.0 (3.0.4) but stick with 2.17.0
   See: https://forum.arduino.cc/t/neopixel-crash-with-75-pixels-using-esp32-core-3-0-x/1273500/13
   A pull request has been made but not yet implemented.
   https://github.com/teknynja/Adafruit_NeoPixel/tree/esp32_rmt_memory_allocation_fix_safe

   This code has been successfully deployed with the following configurations:
   Arduino IDE (Windows x64) Version: 2.3.3
   Date: 2024-09-25T09:41:18.242Z
   CLI Version: 1.0.4
   Copyright © 2024 Arduino SA

   Board: FQBN: esp32:esp32:uPesy_wroom
   Platform: ESP32 version 2.0.17
   esptool.py v4.5.1

   Libraries:
   WiFi@2.0.0
   Timezone@1.2.4
   Time@1.6.1
   NTPClient@3.2.1
   SunRise@2.0.4
   MoonRise@2.0.4
   MoonPhasePlus@2.0.1 (not actually used)
   Adafruit NeoPixel@1.12.0

   This code probably runs fine with FastLED library instead of Adafruit_Neopixel library, 
   with some minor tweaking required. That probably does not change much at all in terms 
   of performance, look, feel, sound, smoke and mirrors.

   Sketch uses 787333 bytes (60%) of program storage space. Maximum is 1310720 bytes.
   Global variables use 46428 bytes (14%) of dynamic memory, leaving 281252 bytes 
   for local variables. Maximum is 327680 bytes.
 */

//30 seconds Watchdog timer
#define WDT_TIMEOUT 30

// Australia/Sydney
#define NTP_POOL "AU.POOL.NTP.ORG"
#define NTP_OFFSET 0            // we'll do timezones separately, including DST as well
#define NTP_INTERVAL 60000      // refresh every minute
#define LATITUDE -33.7          //  lat
#define LONGITUDE 151.1         //  long

#define NEO_PIXELS 144          // 12x12=144 LEDs
#define NEO_PIN 27              // neopixel data pin
#define BRIGHTNESS 196          // max intensity 0..255 -> peak LED intensity
#define CONTRAST 128            // max contrast  0..255 -> relative reduction in LED intensity, see (*) below
// (*) _LEVEL = int(BRIGHTNESS - (CONTRAST*BRIGHTNESS/255.0)*((1-cos(_phase))/2));

#define TEST_DELAY_TIME 1000    // just in case we want to test the display with chase, all words, etc.

class WordClock {
public:
  WordClock();
  int get_day();
  int get_hour();
  int get_minute();
  void begin();
  void update();
  time_t utc();
  void loop();
private:
  // private const and variables
  int _last_minute;
  int _last_hour;
  int _last_day;
  int _brightness = BRIGHTNESS;
  int _contrast = CONTRAST;
  SunRise _sunrise;
  MoonRise _moonrise;
  MoonPhase _moonphase;
  Adafruit_NeoPixel _pixels = Adafruit_NeoPixel(NEO_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
  // private methods
  void _show_sun_and_moon_info(time_t t);
  void _printDateTime();
  void _ensure_wifi();
  void _adjustBrightnessContrast();
  void _clearPixel(int p);
  void _setPixel(int p, uint32_t Color);
  void _clearDisplay();
  void _setWord(int *Word, uint32_t Color);
  void _showHourSemiExact();
  void _showMinuteSemiExact();
  void _showMinuteHasBeen();
  void _showWiFiStatus();
  void _showNTPStatus();
  void _showWarningStatus();
  void _showSunAndMoon();
  void _showLoveHeart();
  void _showEaster();
  void _showChristmas();
  void _showHalloween();
  void _showDisplay();
  void _demoChase(uint32_t Color);
  void _showMinutesAndHours();
  void _showRainbow();
  void _test_Word_Clock();
  
protected:
  // we don't have any protected stuff to pass on to children/derived classes
};

// 144 char string which mimics the clock face, in a meander pattern - used to spit out the time on the Serial interface
const std::string wordClockString = "aSZISCH!*)w@LETREIVJDLABZWEISFuFACHTFLoWZXFLEuRDSaCHSIEBaZaHKGIZNAWZaNuNHALBI&VORVABITHCABIFLoWZDRuFuFIZaHNIYIREIVWSIEWZSaCHSIEBNIGQISGPINuNIFLE";
// const String "ÄSZISCH#####LETREIVJDLABZWEISFÜFACHTFLÖWZXFLEÜRDSÄCHSIEBÄZÄHKGIZNAWZÄNÜNHALBI#VORVABITHCABIFLÖWZDRÜFÜFIZÄHNIYIREIVWSIEWZSÄCHSIEBNIGQISGPINÜNIFLE";

// gamma correction LUT
static uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255  
};

// Neopixel colours - sorry, it's a USA library so it uses Color for colours ... doh!
// we're gonna do this a little different
// declared static as they're defined outside the WordClock class but used inside the WordClock class
static unsigned long Black =  Adafruit_NeoPixel::Color(0, 0, 0);
static unsigned long Dark =   Adafruit_NeoPixel::Color(gamma8[uint8_t(1*BRIGHTNESS/8)], gamma8[uint8_t(1*BRIGHTNESS/8)], gamma8[uint8_t(1*BRIGHTNESS/8)]);
static unsigned long Grey =   Adafruit_NeoPixel::Color(gamma8[uint8_t(4*BRIGHTNESS/8)], gamma8[uint8_t(4*BRIGHTNESS/8)], gamma8[uint8_t(4*BRIGHTNESS/8)]);
static unsigned long Silver = Adafruit_NeoPixel::Color(gamma8[uint8_t(6*BRIGHTNESS/8)], gamma8[uint8_t(6*BRIGHTNESS/8)], gamma8[uint8_t(6*BRIGHTNESS/8)]);
static unsigned long White =  Adafruit_NeoPixel::Color(gamma8[uint8_t(BRIGHTNESS)], gamma8[uint8_t(BRIGHTNESS)], gamma8[uint8_t(BRIGHTNESS)]);

static unsigned long Red =    Adafruit_NeoPixel::Color(gamma8[uint8_t(BRIGHTNESS)], 0, 0);
static unsigned long Orange = Adafruit_NeoPixel::Color(gamma8[uint8_t(BRIGHTNESS)], gamma8[uint8_t(BRIGHTNESS/2)], 0);
static unsigned long Yellow = Adafruit_NeoPixel::Color(gamma8[uint8_t(BRIGHTNESS)], gamma8[uint8_t(BRIGHTNESS)], 0);
static unsigned long Green =  Adafruit_NeoPixel::Color(0, gamma8[uint8_t(BRIGHTNESS)], 0);
static unsigned long Blue =   Adafruit_NeoPixel::Color(0, 0, gamma8[uint8_t(BRIGHTNESS)]);
static unsigned long Cyan =   Adafruit_NeoPixel::Color(0, gamma8[uint8_t(BRIGHTNESS)], gamma8[uint8_t(BRIGHTNESS)]);
static unsigned long Magenta = Adafruit_NeoPixel::Color(gamma8[uint8_t(BRIGHTNESS)], 0, gamma8[uint8_t(BRIGHTNESS)]);

// Colour shortcuts for symbols
// note - these are pre-processor macros
// actual code will be compiles with Silver, Black, Orange, etc.
// Colours Black, Dark. Grey, Silver, White, Red, Green Blue etc will be adjusted for contrast and brightness
// during the diurnal cycle - bright at lunchtime, dark at midnight
#define FOREGROUNDCOLOR  White
#define BACKGROUNDCOLOR  Dark
#define TESTCOLOR        Orange
#define WIFIDISCONNECTED Red
#define WIFICONNECTING   Orange
#define WIFICONNECTED    Blue
#define NTP_NOT_SET      Blue
#define NTP_SET          Green
#define THECOLOROFLOVE   Red
#define CHRISTMASCOLOR   Green
#define EASTERCOLOR      Yellow
#define HALLOWEEN_0      White
#define HALLOWEEN_1      Yellow
#define HALLOWEEN_2      Orange
#define HALLOWEEN_3      Red
#define SUN_COLOR        Yellow
#define MOON_COLOR       Cyan
#define WARNING_COLOR    Orange

// Various symbols on the clock face
// each special symbol is mapped to an LED in the string
// note that the string zigzags back and forth over the clock
static int symbolWiFi[] = { 11, -1 };     // show [@]  when WiFi connected (blue=connecting, green=OK, red=disconnected)
static int symbolTime[] = { 10, -1 };     // show [#]  when ntp is synced 
static int symbolMoon[] = { 9, -1 };      // show [o]  at night (if !(sunrise.isVisible) ) [O]
static int symbolSun[] = { 8, -1 };       // show [*]  during daytime (if sunrise.isVisible)
static int symbolLove[] = { 77, -1 };     // show [<3] on dd/mm/yyyy only
static int symbolChristmas[] = { 41, -1 };// show [Xmas tree] on 25/12/yyyy only
static int symbolEaster[] = { 114, -1 };  // show [chicken] on Easter Sunday only
static int symbolHalloween[] = {60, -1};  // show [Ghost] on Halloween (31/10/yyyy) only
static int symbolWarning[] = { 7, -1 };   // show [!]  some sort of error display (not used yet)

// Various useful (?) words on the clock face
// static int arrayname = { series, of, LEDs, finished, with, a, -1}
static int wordNone[] = { -1 };
static int wordIt[] = { 0, 1, -1 };
static int wordIs[] = { 3, 4, 5, 6, -1 };
static int wordSoon[] = { 23, 22, 21, 20, -1 };
static int wordQuarter[] = { 18, 17, 16, 15, 14, 13, 12, -1 };
static int wordHalf[] = { 79, 80, 81, 82, 83, -1 };
static int wordTo[] = { 72, 73, 74, -1 };
static int wordPast[] = { 75, 76, -1 };
static int wordBeen[] = { 134, 133, 132, -1 };

// all the 29 minute words (past, to), including some compounds
static int wordMinuteOne[] = { 26, 27, 28, -1 };
static int wordMinuteTwo[] = { 24, 25, 26, 27, -1 };
static int wordMinuteThree[] = { 47, 46, 45, -1 };
static int wordMinuteFour[] = { 18, 17, 16, 15, -1 };
static int wordMinuteFive[] = { 29, 30, 31, -1 };
static int wordMinuteSix[] = { 48, 49, 50, 51, 52, -1 };
static int wordMinuteSeven[] = { 52, 53, 54, 55, 56, -1 };
static int wordMinuteEight[] = { 32, 33, 34, 35, -1 };
static int wordMinuteNine[] = { 71, 70, 69, -1 };
static int wordMinuteTen[] = { 57, 58, 59, -1 };
static int wordMinuteEleven[] = { 44, 43, 42, -1 };
static int wordMinuteTwelve[] = { 40, 39, 38, 37, 36, -1 };
static int wordMinuteTwenty[] = { 67, 66, 65, 64, 63, 62, 61, -1 };
static int wordMinuteTwentyOne[] = { 26, 27, 69, 68, 67, 66, 65, 64, 63, 62, 61, -1 };                // for EINaZWANZIG
static int wordMinuteTwentyTwo[] = { 24, 25, 26, 27, 68, 67, 66, 65, 64, 63, 62, 61, -1 };            // for ZweiaZWANZIG
static int wordMinuteTwentyThree[] = { 47, 46, 45, 68, 67, 66, 65, 64, 63, 62, 61, -1 };              // for DruaZWANZIG
static int wordMinuteTwentyFour[] = { 18, 17, 16, 15, 68, 67, 66, 65, 64, 63, 62, 61, -1 };           // for VIERaZWANZIG
static int wordMinuteTwentyFive[] = { 29, 30, 31, 68, 67, 66, 65, 64, 63, 62, 61, -1 };               // for FuFaZWANZIG
static int wordMinuteTwentySix[] = { 48, 49, 50, 51, 52, 68, 67, 66, 65, 64, 63, 62, 61, -1 };        // for SaCHSaZWANZIG
static int wordMinuteTwentySeven[] = { 52, 53, 54, 55, 56, 69, 68, 67, 66, 65, 64, 63, 62, 61, -1 };  // for SIEBaNaZWANZIG
static int wordMinuteTwentyEight[] = { 32, 33, 34, 35, 68, 67, 66, 65, 64, 63, 62, 61, -1 };          // for ACHTaZWANZIG
static int wordMinuteTwentyNine[] = { 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, -1 };               // for NuNaZWANZIG

/* some notes on how we count just past the hour, approaching the half hour, etc.
* m=0 (don't write minutes, just write hour)
* 0 < m < 5  -> write Minute
* (0 < m < 23) || (30 < m < 38)  -> write Past
* (37 < m ) || (22 < m < 30) -> write To
* 22 < m < 38  -> write Half
*/

// assembly of the 59 minutes words using soon, quarter, half, etc.
static int* wordMinutes[][59] = {
  { wordMinuteOne, wordNone },                  //  1
  { wordMinuteTwo, wordNone },                  //  2
  { wordMinuteThree, wordNone },                //  3
  { wordMinuteFour, wordNone },                 //  4
  { wordMinuteFive, wordNone },                 //  5
  { wordMinuteSix, wordNone },                  //  6
  { wordMinuteSeven, wordNone },                //  7
  { wordMinuteEight, wordNone },                //  8
  { wordMinuteNine, wordNone },                 //  9
  { wordMinuteTen, wordNone },                  // 10
  { wordMinuteEleven, wordNone },               // 11
  { wordMinuteTwelve, wordNone },               // 12
  { wordSoon, wordQuarter, wordNone },          // nearly Quarter
  { wordSoon, wordQuarter, wordNone },          // nearly Quarter
  { wordQuarter, wordNone },                    // 15, Quarter
  { wordMinuteSix, wordMinuteTen, wordNone },   // 16
  { wordMinuteSeven, wordMinuteTen, wordNone }, // 17
  { wordSoon, wordMinuteTwenty, wordNone },     // 18
  { wordSoon, wordMinuteTwenty, wordNone },     // 19
  { wordMinuteTwenty, wordNone },               // 20
  { wordMinuteTwentyOne, wordNone },            // 21
  { wordMinuteTwentyTwo, wordNone },            // 22
  { wordSoon, wordMinuteFive, wordNone },       // 23
  { wordSoon, wordMinuteFive, wordNone },       // 24
  { wordMinuteFive, wordNone },                 // 25
  { wordMinuteFour, wordNone },                 // 26
  { wordMinuteThree, wordNone },                // 27
  { wordMinuteTwo, wordNone },                  // 28
  { wordMinuteOne, wordNone },                  // 29
  { wordNone },                                 // 30
  { wordMinuteOne, wordNone },                  // 31
  { wordMinuteTwo, wordNone },                  // 32
  { wordMinuteThree, wordNone },                // 33
  { wordMinuteFour, wordNone },                 // 34
  { wordMinuteFive, wordNone },                 // 35
  { wordMinuteFive, wordNone },                 // 36
  { wordMinuteFive, wordNone },                 // 37
  { wordSoon, wordMinuteTwenty, wordNone },     // 38
  { wordSoon, wordMinuteTwenty, wordNone },     // 39
  { wordMinuteTwenty, wordNone },               // 40
  { wordMinuteTwenty, wordNone },               // 41
  { wordMinuteTwenty, wordNone },               // 42
  { wordSoon, wordQuarter, wordNone },          // 43
  { wordSoon, wordQuarter, wordNone },          // 44
  { wordQuarter, wordNone },                    // 45
  { wordQuarter, wordNone },                    // 46
  { wordQuarter, wordNone },                    // 47
  { wordMinuteTwelve, wordNone },               // 48
  { wordMinuteEleven, wordNone },               // 49
  { wordMinuteTen, wordNone },                  // 50
  { wordMinuteNine, wordNone },                 // 51
  { wordMinuteEight, wordNone },                // 52
  { wordMinuteSeven, wordNone },                // 53
  { wordMinuteSix, wordNone },                  // 54
  { wordMinuteFive, wordNone },                 // 55
  { wordMinuteFour, wordNone },                 // 56
  { wordMinuteThree, wordNone },                // 57
  { wordMinuteTwo, wordNone },                  // 58
  { wordMinuteOne, wordNone }                   // 59
};

/*  Matrix for the clock face. 
*   Matches the clock face character string
*   Every second line is in reverse
*   (LED string meanders)
* 
*      012345678901
*   0--aS.ISCH.####---11
*  23--BALD.VIERTEL---12
*  24--ZWEISFuFACHT---35
*  47--DRuELF.ZWoLF---36
*  48--SaCHSIEBaZaH---59
*  71--NuNaZWANZIG.---60
*  72--HALBI#VOR.AB---83
*  95--ZWoLFI.ACHTI---84
*  96--DRuFuFiZaHNI--107 
* 119--ZWEIS.VIERI.--108
* 120--SaCHSIEBNI.#--131
* 143--ELFINuNI.GSI--132
*      012345678901  */

// all 12 word hours
static int wordHourOne[] = { 117, 116, 115, -1 };
static int wordHourTwo[] = { 119, 118, 117, 116, -1 };
static int wordHourThree[] = { 96, 97, 98, -1 };
static int wordHourFour[] = { 113, 112, 111, 110, 109, -1 };
static int wordHourFive[] = { 99, 100, 101, 102, -1 };
static int wordHourSix[] = { 120, 121, 122, 123, 124, 125, -1 };
static int wordHourSeven[] = { 124, 125, 126, 127, 128, 129, -1 };
static int wordHourEight[] = { 88, 87, 86, 85, 84, -1 };
static int wordHourNine[] = { 139, 138, 137, 136, -1 };
static int wordHourTen[] = { 103, 104, 105, 106, 107, -1 };
static int wordHourEleven[] = { 143, 142, 141, 140, -1 };
static int wordHourTwelve[] = { 95, 94, 93, 92, 91, 90, -1 };

// assemble 12+1 hours
static int* wordHours[] = { wordHourTwelve, wordHourOne, wordHourTwo,
                            wordHourThree, wordHourFour, wordHourFive,
                            wordHourSix, wordHourSeven, wordHourEight,
                            wordHourNine, wordHourTen, wordHourEleven,
                            wordHourTwelve };

#endif