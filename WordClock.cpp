/*
 * This is WordClock.cpp
 */

#include "WordClock.h"

WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, NTP_POOL, NTP_OFFSET, NTP_INTERVAL);
TimeChangeRule AUDST{ "AUDST", First, Sun, Oct, 2, 660 };
TimeChangeRule AUSTD{ "AUSTD", First, Sun, Apr, 3, 600 };
Timezone Sydney(AUSTD, AUDST);
TimeChangeRule *tcr;  // pointer to the time change rule, use to get TZ abbrev

WordClock::WordClock(){
  // nothing to do here, it's just a stub
  // initialisation in WordClock::begin() below
};

void WordClock::begin() {
  // connect to WiFi
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(333);
#ifdef ECHO
    Serial.print(".");
#endif
  };
#ifdef ECHO
  Serial.println("!");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
#endif

  // We can assume that we have WiFi
  // so we're starting an NTP client, and update it
  ntpClient.begin();   // start NTP client via UDP over WiFi
  ntpClient.update();  // update the client (if necessary, see NTP_INTERVAL)
  // we now have a sync, so we're resetting the time from 1/1/1970 to UTC epoch time
  time_t t = ntpClient.getEpochTime();  // grab new updated time from client and sync with system time
  setTime(t);                           // now the internal TimeLib clock is running at UTC

  // update sunrise, moonrise, moonphase etc once an hour
  _sunrise.calculate(LATITUDE, LONGITUDE, t);   // t = EpochTime
  _moonrise.calculate(LATITUDE, LONGITUDE, t);  // now
  _moonphase.calculate(t);

// we only test the clock if we want to
#ifdef TEST_CLOCK
  // pixel display test (takes around 10s)
  _test_Word_Clock();
#endif
  // next, create some space to decide if we need to
  // so something, like once a minute or once an hour
  // force update at first ticktock loop
  _last_minute = -1;
  _last_hour = -1;
  _last_day = -1;

  // finally, start the watchdog timer
  // this will reboot the ESP32 should it get stuck for more than 30 seconds
  // WDT_TIMEOUT is defined in WordClock.h, defaults to 30s
  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                //add current thread to WDT watch
};

/*********************
 * WordClock methods *
 *********************/

// grab the day from the UTC time and convert to Sydney time
int WordClock::get_day() {
  time_t t = Sydney.toLocal(now());  // convert to local Sydney time (incl DST)
  return day(t);
};

// grab the hour from the UTC time and convert to Sydney time
int WordClock::get_hour() {
  time_t t = Sydney.toLocal(now());  // convert to local Sydney time (incl DST)
  return hour(t);
};

// grab the hour from the UTC time and convert to Sydney time
// this is an inside joke, really, unless you're in Broken Hill
int WordClock::get_minute() {
  time_t t = Sydney.toLocal(now());  // convert to local Sydney time (incl DST)
  return minute(t);
};

// the loop - this runs every ~ 1.000 second
void WordClock::loop() {

  // get current "hour" value from the clock
  int h = get_hour();

  // show sun and moon info once an hour (at hh:00)
  // and update WiFi and NTP contacts every hour
  if (h != _last_hour) {
    _ensure_wifi();
    ntpClient.update();

#ifdef DEBUG
    _show_sun_and_moon_info(now());
#endif

    // save the last hour for next round
    _last_hour = h;
  };

  // get current "minute" value from the clock
  int m = get_minute();
  // print the clock if the minute has changed
  // this is where we go and change the clock face, once per minute
  if (m != _last_minute) {

#ifdef DEBUG
    _printDateTime();
#endif

    // update and show the clock face display
    _showDisplay();
    // save the last minute for next round
    _last_minute = m;
  };
  // pause 1 second
  delay(1000);
  // reset the watchdog
  esp_task_wdt_reset();
};

void WordClock::_show_sun_and_moon_info(time_t t) {
  t = Sydney.toLocal(_sunrise.riseTime);
#ifdef ECHO
  Serial.print("Sunrise: sun rises at: ");
  Serial.println(ctime(&t));  // ctime(time_t *t) --> time_t t; ctime(&t)
#endif
  t = Sydney.toLocal(_sunrise.setTime);
#ifdef ECHO
  Serial.print("Sunrise: sun sets at: ");
  Serial.println(ctime(&t));
  if (_sunrise.isVisible) {
    Serial.println("The sun is visible right now.");
  } else {
    Serial.println("The sun is NOT visible right now.");
  };
#endif

  t = Sydney.toLocal(_moonrise.riseTime);
#ifdef ECHO
  Serial.print("Moonrise: moon rises at: ");
  Serial.println(ctime(&t));
#endif
  t = Sydney.toLocal(_moonrise.setTime);
#ifdef ECHO
  Serial.print("Moonrise: moon sets at: ");
  Serial.println(ctime(&t));
  if (_moonrise.isVisible) {
    Serial.println("The moon is visible right now.");
  } else {
    Serial.println("The moon is NOT visible right now.");
  };
#endif

  double phase = _moonphase.phase;
#ifdef ECHO
  Serial.print("The moon phase is currently [0..1] ");
  Serial.println(phase);
#endif
  double fraction = _moonphase.fraction;
#ifdef ECHO
  Serial.print("The moon fraction lit surface is currently [0..1] ");
  Serial.println(fraction);
#endif
  const char *s = _moonphase.phaseName;
#ifdef ECHO
  Serial.print("It is a ");
  Serial.print(s);
  Serial.println(" moon.");
#endif
}

void WordClock::_printDateTime() {
#ifdef ECHO
  Serial.println("*******************************");
#endif
  time_t utc_time = now();
  char *utc_str = ctime(&utc_time);
  time_t local = Sydney.toLocal(utc_time, &tcr);
  char *local_str = ctime(&local);
#ifdef ECHO
  Serial.print("utc:   ");
  Serial.println(utc_str);
  Serial.print("local: ");
  Serial.println(local_str);
#endif
};

// make sure we've got WiFi, basically just connect (again)
void WordClock::_ensure_wifi() {
#ifdef DEBUG
  Serial.println("Ensuring wifi is connected ...");
#endif

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
#ifdef DEBUG
      Serial.print(".");
#endif
    };
  };
#ifdef DEBUG
  Serial.print("WiFi.ssid is ");
  Serial.println(WiFi.SSID());
  Serial.print("WiFi.channel # is ");
  Serial.println(WiFi.channel());
  Serial.print("WiFi.localIP is ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi.rssi is ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("RF power is ");
  Serial.print(1.0E6 * pow(10.0, WiFi.RSSI() / 10.0));
  Serial.println(" nW");
#endif
};

void WordClock::_adjustBrightnessContrast() {
  int hh = get_hour();
  int mm = get_minute();
  float _phase = 2 * PI * (hh * 60.0 + mm * 1.0) / (24.0 * 60.0);

#ifdef DEBUG
  Serial.print("Phase is ... ");
  Serial.println(_phase);
#endif

  int _LEVEL = int(BRIGHTNESS - (CONTRAST * BRIGHTNESS / 255.0) * ((1 + cos(_phase)) / 2));

#ifdef DEBUG
  Serial.print("Brightness level is ");
  Serial.print(_LEVEL);
  Serial.print("/");
  Serial.print(100 * _LEVEL / 255);
  Serial.println("%");
#endif

  Black = Adafruit_NeoPixel::Color(0, 0, 0);
  Dark = Adafruit_NeoPixel::Color(gamma8[uint8_t(2 * _LEVEL / 8)], gamma8[uint8_t(2 * _LEVEL / 8)], gamma8[uint8_t(2 * _LEVEL / 8)]);
  Grey = Adafruit_NeoPixel::Color(gamma8[uint8_t(4 * _LEVEL / 8)], gamma8[uint8_t(4 * _LEVEL / 8)], gamma8[uint8_t(4 * _LEVEL / 8)]);
  Silver = Adafruit_NeoPixel::Color(gamma8[uint8_t(5 * _LEVEL / 8)], gamma8[uint8_t(6 * _LEVEL / 8)], gamma8[uint8_t(6 * _LEVEL / 8)]);
  White = Adafruit_NeoPixel::Color(gamma8[uint8_t(_LEVEL)], gamma8[uint8_t(_LEVEL)], gamma8[uint8_t(_LEVEL)]);

  Red = Adafruit_NeoPixel::Color(gamma8[uint8_t(_LEVEL)], 0, 0);
  Orange = Adafruit_NeoPixel::Color(gamma8[uint8_t(_LEVEL)], gamma8[uint8_t(_LEVEL / 2)], 0);
  Yellow = Adafruit_NeoPixel::Color(gamma8[uint8_t(_LEVEL)], gamma8[uint8_t(_LEVEL)], 0);
  Green = Adafruit_NeoPixel::Color(0, gamma8[uint8_t(_LEVEL)], 0);
  Blue = Adafruit_NeoPixel::Color(0, 0, gamma8[uint8_t(_LEVEL)]);
  Cyan = Adafruit_NeoPixel::Color(0, gamma8[uint8_t(_LEVEL)], gamma8[uint8_t(_LEVEL)]);
  Magenta = Adafruit_NeoPixel::Color(gamma8[uint8_t(_LEVEL)], 0, gamma8[uint8_t(_LEVEL)]);
}

// set a certain pixel to a certain Color
void WordClock::_setPixel(int p, uint32_t Color) {
  _pixels.setPixelColor(p, Color);
};

// clear a certain pixel (set it to the BACKGROUNDCOLOR)
void WordClock::_clearPixel(int p) {
  _setPixel(p, BACKGROUNDCOLOR);
};

// clear the display (set all pixels to the BACKGROUNDCOLOUR)
void WordClock::_clearDisplay() {
#ifdef DEBUG
  Serial.println("Clearing Display");
#endif
  for (int p = 0; p < _pixels.numPixels(); ++p) {
    _clearPixel(p);
  };
  _pixels.show();
};

// show the (hour/minute) word on the clock face and on the serial port
void WordClock::_setWord(int *Word, uint32_t Color) {
  for (int p = 0; p < _pixels.numPixels() + 1; p++) {
    if (Word[p] == -1) {
#ifdef ECHO
      Serial.print(" ");
#endif
      break;
    } else {
      _setPixel(Word[p], Color);
#ifdef ECHO
      Serial.print(wordClockString.at(Word[p]));
#endif
    };
  };
};

// show the hour on the clock face and on the serial port
void WordClock::_showHourSemiExact() {
  // grab the stored minute (from local Sydney time from UTC, it's the same)
  // grab the stored hour (from local Sydney time from UTC, use Timezone to shift)
  int _minute = get_minute();
  int _hour = get_hour();
  if (_minute < 23) {
    // show this hour if we are before 23 minutes past
    // we start going weird at "7 minutes to half ten" (this is German, yeah?)
    _setWord(wordHours[_hour % 12], FOREGROUNDCOLOR);
  } else {
    // show next hour
    _setWord(wordHours[(_hour % 12) + 1], FOREGROUNDCOLOR);
  }
}

// Show the minute on the clock face and on the serial port
// We're using a semi-exact method here. it's a bit fuzzy.
// Sometimes it shows the same time 2 mins in a row
void WordClock::_showMinuteSemiExact() {
  // grab the stored minute (from local Sydney time from UTC, it's the same)
  int _minute = get_minute();
  if (_minute != 0) {
    for (int i = 0; i < 5; ++i) {
      if (wordMinutes[_minute - 1][i] == wordNone) {
        break;
      } else {
        _setWord(wordMinutes[_minute - 1][i], FOREGROUNDCOLOR);
      }
    }

    if ((_minute > 0 && _minute < 23) || (_minute <= 37 && _minute > 30)) {
      _setWord(wordPast, FOREGROUNDCOLOR);
    }

    if (_minute > 37 || (_minute >= 23 && _minute < 30)) {
      _setWord(wordTo, FOREGROUNDCOLOR);
    }

    if (_minute >= 23 && _minute <= 37) {
      _setWord(wordHalf, FOREGROUNDCOLOR);
    }
  }
}

// if it is x minutes PAST something, let's show "past/GSI"
void WordClock::_showMinuteHasBeen() {
  int _minute = get_minute();
  if ((_minute > 15 && _minute < 18) || (_minute > 20 && _minute < 23) || (_minute > 35 && _minute < 38) || (_minute > 40 && _minute < 43) || (_minute > 45 && _minute < 48)) {
    _setWord(wordBeen, FOREGROUNDCOLOR);
  }
}

// on April 2nd (Raelene's birthday), we show a red love heart
void WordClock::_showLoveHeart() {
  time_t t = Sydney.toLocal(now());
  int m = month(t);
  int d = day(t);
  if (m == 4 && d == 2) {  // 2/4/1974
// special serial port output, too, every hour.
#ifdef ECHO
    Serial.println("Happy Birthday, Raelene Sheppard!");
#endif
    _setWord(symbolLove, THECOLOROFLOVE);
  };
};

// on Christmas Day show a Xmas Tree
void WordClock::_showChristmas() {
  // do something
  time_t t = Sydney.toLocal(now());
  int m = month(t);
  int d = day(t);
  if (m == 12 && (d == 25 || d == 26)) {  // 25.12., 26.12. -> Xmas
    _setWord(symbolChristmas, CHRISTMASCOLOR);
  };
};

// on Easter Sunday show a chicken
void WordClock::_showEaster() {
  time_t t = Sydney.toLocal(now());
  int y = year(t);
  int m = month(t);
  int d = day(t);
  // Gauss' algorithm
  float A = y % 19;  // Metonic cycle
  float B = y % 4;   // Leap years
  float C = y % 7;   // 52 weeks test
  float P = floor((float)y / 100.0);
  float Q = floor((float)(13 + 8 * P) / 25.0);
  float M = (int)(15 - Q + P - floor(P / 4)) % 30;  // M depends on the century of year Y. For 19th century, M = 23. For the 21st century, M = 24 and so on.
  float N = (int)(4 + P - floor(P / 4)) % 7;        // difference between the number of leap days between the Julian and the Gregorian calendar
  float D = (int)(19 * A + M) % 30;                 // number of days to be added to March 21 to find the date of the Paschal Full Moon
  float E = (int)(N + 2 * B + 4 * C + 6 * D) % 7;   // number of days from the Paschal full moon to the next Sunday
  int days = (int)(22 + D + E);
  int e_m;
  int e_d;
  if (D == 29 && E == 6) {  // this is a an edge case
    e_m = 4;
    e_d = 19;
  } else if (D == 28 && E == 6) {  // this is a an edge case
    e_m = 4;
    e_d = 18;
  } else {
    // If days > 31, move to April
    if (days > 31) {
      e_m = 4;
      e_d = days - 31;
    } else {
      // Otherwise, stay on March
      e_m = 3;
      e_d = days;
    }
  }
  // now make the chicken yellow if today is Easter Sunday
  if (m == e_m && d == e_d) {
    _setWord(symbolEaster, EASTERCOLOR);
  };
};

// on Halloween (31/10/yyyy) show a ghost
// it changes colour every minute, LOL
void WordClock::_showHalloween() {
  // do something
  time_t t = Sydney.toLocal(now());
  int m = month(t);
  int d = day(t);
  if (m == 10 && d == 31) {  // 31.10. -> Halloween
    int mm = abs((minute(t) % 6) - 3);
    switch (mm) {
      case 0:
        _setWord(symbolHalloween, HALLOWEEN_0);  // white
        break;
      case 1:
        _setWord(symbolHalloween, HALLOWEEN_1);  // yellow
        break;
      case 2:
        _setWord(symbolHalloween, HALLOWEEN_2);  // orange
        break;
      case 3:
        _setWord(symbolHalloween, HALLOWEEN_3);  // red
        break;
    };
  };
};

// show the clock face
// IT IS (MINUTE|QUARTER) TO/PAST (HALF) (HOUR) HASBEEN
// also show symbols
// a
void WordClock::_showDisplay() {
  // wipe the display
  _clearDisplay();

  // adjust contrast and brightness
  _adjustBrightnessContrast();

  // light up "it's" it stays on
  _setWord(wordIt, FOREGROUNDCOLOR);
  _setWord(wordIs, FOREGROUNDCOLOR);
  // Light up minutes
  _showMinuteSemiExact();
  // light up hours
  _showHourSemiExact();
  // light up has been
  _showMinuteHasBeen();
  // light up symbols
  // _showWiFiStatus();
  // _showNTPStatus();
  _showWarningStatus();
  _showSunAndMoon();
  // Warning symbols should go here, needs logic
  _showLoveHeart();
  _showEaster();
  _showChristmas();
  _pixels.show();
// new line, please
#ifdef ECHO
  Serial.println();
#endif
};

// loop 10ms over all pixels (takes 1.44s in total, per Color)
void WordClock::_demoChase(uint32_t Color) {
#ifdef ECHO
  Serial.print("Demo chase all pixels in ( ");
  Serial.print(Color, HEX);
  Serial.print(")... ");
#endif
  for (uint16_t p = 0; p < _pixels.numPixels() + 4; p++) {
    _setPixel(p, Color);                // Draw new pixel
    _setPixel(p - 4, BACKGROUNDCOLOR);  // Erase pixel a few steps back
    _pixels.show();
    delay(10);
  };
#ifdef ECHO
  Serial.println("done.");
#endif
};

void WordClock::_showRainbow() {
  _clearDisplay();
  for (uint8_t p = 0; p < _pixels.numPixels(); p++) {
    int hue = p * (65536 / _pixels.numPixels());
    int saturation = 255;
    int value = BRIGHTNESS;
    uint32_t colour = _pixels.ColorHSV(hue, saturation, value);
    _setPixel(p, colour);
  };
  _pixels.show();
};

// include WiFi.reconnect()
void WordClock::_showWiFiStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    _setWord(symbolWiFi, WIFICONNECTED);
  } else {
    _setWord(symbolWiFi, WIFIDISCONNECTED);
  }
};

void WordClock::_showNTPStatus() {
  if (ntpClient.isTimeSet()) {
    _setWord(symbolTime, NTP_SET);
  } else {
    _setWord(symbolTime, NTP_NOT_SET);
  };
};

void WordClock::_showWarningStatus() {
  int warning = 0x0;
  if (WiFi.status() != WL_CONNECTED) {
    // fail!
    _setWord(symbolWiFi, WIFIDISCONNECTED);
    warning = warning || 0x01;
  } else {
    // success!
    _setWord(symbolWiFi, WIFICONNECTED);
  };

  if (ntpClient.isTimeSet()) {
    // success!
    _setWord(symbolTime, NTP_SET);
  } else {
    // fail!
    _setWord(symbolTime, NTP_NOT_SET);
    warning = warning || 0x02;
  };

  if (warning > 0) {
    _setWord(symbolWarning, WARNING_COLOR);
  } else {
    _setWord(symbolWarning, BACKGROUNDCOLOR);
  };
  // _showDisplay() will be called in loop() right after this
};

void WordClock::_showSunAndMoon() {
  time_t t = now();
  _sunrise.calculate(LATITUDE, LONGITUDE, t);
  if (_sunrise.isVisible) {
    _setWord(symbolSun, SUN_COLOR);
  } else {
    _setWord(symbolSun, BACKGROUNDCOLOR);
  };
  _moonrise.calculate(LATITUDE, LONGITUDE, t);
  if (_moonrise.isVisible) {
    _setWord(symbolMoon, MOON_COLOR);
  } else {
    _setWord(symbolMoon, BACKGROUNDCOLOR);
  };
};

void WordClock::_showMinutesAndHours() {
#ifdef DEBUG
  Serial.println("Listing IT HAS BEEN ... ");
#endif

  _setWord(wordIt, TESTCOLOR);
  _setWord(wordIs, TESTCOLOR);
  _setWord(wordBeen, TESTCOLOR);

#ifdef DEBUG
  Serial.println("Listing wordMinutes ... ");
#endif
  for (int _minute = 1; _minute < 59; _minute++) {
    for (int i = 0; i < 5; ++i) {
      if (wordMinutes[_minute - 1][i] == wordNone) {
        break;
      } else {
        _setWord(wordMinutes[_minute - 1][i], TESTCOLOR);
      }
    }
  }
#ifdef DEBUG
  Serial.println();
#endif

#ifdef DEBUG
  Serial.println("Listing wordHours ... ");
#endif
  for (int _hour = 0; _hour < 12; _hour++) {
    _setWord(wordHours[_hour % 12], TESTCOLOR);
  };
#ifdef DEBUG
  Serial.println("done!");
#endif
};

void WordClock::_test_Word_Clock() {
#ifdef DEBUG
  Serial.println("Testing the WordClock ... !");
  Serial.println("Showing a rainbow over the entire strip");
#endif

#ifdef DEBUG
  Serial.println("Showing WiFi in Green");
#endif
  _setWord(symbolWiFi, WIFICONNECTED);
#ifdef DEBUG
  Serial.println("Showing NTP Time in Green");
#endif
  _setWord(symbolTime, NTP_SET);
#ifdef DEBUG
  Serial.println("Showing the Sun in Yellow");
#endif
  _setWord(symbolSun, SUN_COLOR);
#ifdef DEBUG
  Serial.println("Showing the Moon in Silver");
#endif
  _setWord(symbolMoon, MOON_COLOR);
#ifdef DEBUG
  Serial.println("Showing a love heart in Red");
#endif
  _setWord(symbolLove, THECOLOROFLOVE);
#ifdef DEBUG
  Serial.println("Showing a Warning in Orange");
#endif
  _setWord(symbolWarning, WARNING_COLOR);
#ifdef DEBUG
  Serial.println("Showing a Christmas tree in Green");
#endif
  _setWord(symbolWarning, CHRISTMASCOLOR);

#ifdef DEBUG
  Serial.println("Showing a Easter egg in Yellow");
#endif

  _setWord(symbolWarning, EASTERCOLOR);
  _showDisplay();

  delay(6666);  // wait ~ 6.7 seconds
  _clearDisplay();

  _showRainbow();

  delay(6666);  // wait ~ 6.7 seconds
  _clearDisplay();

  _showMinutesAndHours();
  _clearDisplay();
};