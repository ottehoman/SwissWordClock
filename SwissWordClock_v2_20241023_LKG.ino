/*
 * WordClock.ino
 * 
 * inspired by https://meister.io/blog/zyt/
 * code modified from https://meister.io/blog/zyt/
 * expanded with code from
 *   https://github.com/signetica/SunRise
 *   https://github.com/signetica/MoonRise
 *   https://github.com/signetica/MoonPhase
 *   https://github.com/JChristensen/Timezone
 *   https://github.com/PaulStoffregen/Time
 *   https://github.com/arduino-libraries/NTPClient
 *   https://iotassistant.io/esp32/enable-hardware-watchdog-timer-esp32-arduino-ide/
 * 
 * TimeLib etc. - the code basically runs everything in UTC, takes now() from NTPClient,
 * and converts time to Sydney local timezone and DST where and when required.
 *
 * Character pattern kept at 12x12=144 LEDs, but changed from Bern Swiss German 
 * to Zurich Swiss German, and a few gimmicks with sun, moon, time and WiFI status
 *
 * Something special happens on April 2nd, every year: Happy Birthday Mrs. S. :-)
 * 
 * NEW: 02/09/2024 
 * - add Xmas (25.12/26.12)
 *   Xmas tree is LED 7,4 (n=41)
 * - add Easter (Gauss Algorithm)
 *   Easter is LED 6,10 (n=114)
 * - add Halloween ghost
 *   Halloween is LED 12,6 (n=60)
 *
 * 20241017 fixed HALBI <-> VOR.AB grammar bug (thank you Seb)
 *
 * All functionality defined in WordClock class definition, with some bulk 
 * constants and variables offloaded into utils.h
 *
 **************************************************************************
 * ~   60 lines of code and comments in *.ino sketch
 * ~  650 lines of code and comments in *.cpp class definition
 * ~  340 lines of code and comments in *.h header definition
 * ~   55 lines of code and comments in utils.h helper file
 * ~ 1100 lines of code and comments in total overall
 * Sketch uses 787337 bytes (60%) of program storage space. Maximum is 1310720 bytes.
 * Global variables use 46452 bytes (14%) of dynamic memory, leaving 281228 bytes for local variables. Maximum is 327680 bytes.
 **************************************************************************
 */

#include "WordClock.h"
WordClock wordClock;

/* main setup routine
 * set's Serial speed to high, then calls
 * WordClock::begin() for the wordClock object.
 */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // 115200, 921600, 9600, 1200 (old skool #AT)
  wordClock.begin();
};

/* main loop routine
 * just calls WordClock::loop() method, which does all the magic.
 */
void loop() {
  wordClock.loop();
};

// EOF - not bad for less than 60 lines of "code"