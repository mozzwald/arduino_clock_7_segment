/*
  7 Segment Arduino Clock v1.1
  
  Copyright (C) 2015  Joe Honold  
  http://mozzwald.com/articles/iminixie-imitation-nixie-arduino-clock

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  Uses the following additional libraries not included in the
  default arduino installation:
  https://github.com/adafruit/Adafruit-LED-Backpack-Library
  https://github.com/adafruit/Adafruit-GFX-Library
  https://www.pjrc.com/teensy/td_libs_Encoder.html
  
  
  TODO LIST: 
  * Add date set option
  * Add date display if knob turn in time mode
*/


#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <RTClib.h>

RTC_DS1307 rtc;

#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif

Adafruit_7segment seg7disp = Adafruit_7segment();

long knobKount = -999; // Encoder counter
long newKount; // Encoder counter check
int knobA = 3; // Encoder Input A Pin
int knobB = 2; // Encoder Input B Pin
int knobLEDred = 5; // Knob LED red Pin
int knobLEDgreen = 6; // Knob LED green Pin
int bttn = 4; // Knob Button Pin
Encoder knob(knobB, knobA); // Create encoder object
boolean optMode = 0; // Options menu mode
boolean brightMode = 0; // Brightness option mode
boolean setMode = 0; // Time set option mode
boolean setHour = 0; // Time set hours mode
boolean setMin = 0; // Time set minutes mode
boolean setAMPM = 0; // Time set AM/PM mode
boolean timeMode = 1; // Display the time (start in this mode)
boolean drawDots = 0; // Display colon or not
int dotCounter = 0; // 10 times per sec (colon blink delay)
int hours = 9; // Hours variable
int mins = 34; // Minutes variable
boolean ampm = 1; // Dot for AM / PM
int menuDisplay = 0; // Menu item to display
int maxMenu = 2; // Highest menu item
int brightness = 7; // Set default brightness
long startDelay = 0; // Menu delay counter
int menuDelay = 5000; // Milliseconds delay before exiting option menu
DateTime now; // Holds the current time

void setup(){
  Wire.begin();
  rtc.begin();
  Serial.begin(9600); // Open serial port
  Serial.println("Loading clock...");
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, loading default time...");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }else{
    Serial.println("RTC is running, loading time from RTC...");
  }
  // Print the current date and time to the serial port
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  pinMode(bttn, INPUT_PULLUP); // Setup input button
  pinMode(knobLEDgreen, OUTPUT); // Setup green knob LED
  digitalWrite(knobLEDgreen, LOW); // Turn off green knob LED
  pinMode(knobLEDred, OUTPUT); // Setup red knob LED
  digitalWrite(knobLEDred, LOW); // Turn off red knob LED
  /* Setup I2C 7 Segment Display */
  seg7disp.begin(0x70);  // Set 7 Seg LED Address
}

void loop(){
  /* Get the time */
  now = rtc.now();

  if(digitalRead(bttn) == LOW){
    // Button pressed, what are we gunna do?
    digitalWrite(knobLEDgreen, HIGH); // Turn on knob LED
    drawDots = 0; // Turn off the colon when entering some other mode
    seg7disp.drawColon(drawDots);
    seg7disp.writeDisplay();
    if(!optMode && !setMode && !brightMode){ // We need to display the menu
      Serial.println("Show me the menu!");
      optMode = 1;
      timeMode = 0;
      startDelay = millis();
    }else if(optMode && menuDisplay == 1){
      Serial.println("We just entered set time mode, hours first");
      optMode = 0;
      timeMode = 0;
      setMode = 1;
      brightMode = 0;
      setHour = 1;
      drawDots = 1; // Turn on colon for time set
      seg7disp.drawColon(drawDots);
      if(now.hour() > 12){
        hours = now.hour()-12;
      }else{
        hours = now.hour();
      }
      seg7disp.print(hours*100);
      seg7disp.writeDigitRaw(3, B01000000);
      seg7disp.writeDigitRaw(4, B01000000);
      seg7disp.writeDisplay();
      knob.write(hours);
    }else if(setMode && setHour){
      // Save RTC Hours Here
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, now.minute(), 0));
      Serial.println("Hours are set, moving to minutes");
      setHour = 0;
      setMin = 1;
      seg7disp.print(now.minute());
      seg7disp.writeDigitRaw(0, B01000000);
      seg7disp.writeDigitRaw(1, B01000000);
      seg7disp.writeDisplay();
      knob.write(now.minute());
    }else if(setMode && setMin){
      // Save RTC Minutes Here
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), mins, 0));
      Serial.println("Minutes are set, moving to AM/PM");
      setHour = 0;
      setMin = 0;
      setAMPM = 1;
      seg7disp.writeDigitRaw(0, B00000000); // blank digit 0
      seg7disp.writeDigitRaw(1, B00000000); // blank digit 1
      if(ampm){
        seg7disp.writeDigitRaw(3,115); // P
        seg7disp.writeDigitRaw(4, B10000000); // Dot
        knob.write(1);
      }else{
        seg7disp.writeDigitRaw(3,119); // A
        seg7disp.writeDigitRaw(4, B00000000); // No Dot
        knob.write(0);
      }
      seg7disp.writeDisplay();
    }else if(setMode && setAMPM){
      // Update RTC Hours for am/pm
      if(ampm){ // If true, it's PM so add 12 to hour
        hours = now.hour()+12;
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, now.minute(), 0));
      }
      Serial.println("Time is set, return to time mode");
      optMode = 0;
      timeMode = 1;
      setMode = 0;
      brightMode = 0;
      setMin = 0;
      setAMPM = 0;
    }else if(optMode && menuDisplay == 2){
      Serial.print("We just entered brightness mode");
      optMode = 0;
      timeMode = 0;
      setMode = 0;
      brightMode = 1;
      seg7disp.print(brightness);
      if(brightness > 9){
        // pad digit 1 and two
        seg7disp.writeDigitRaw(0, B00111111);
        seg7disp.writeDigitRaw(1, B00111111);
      }else{
        // pad digits 1-3
        seg7disp.writeDigitRaw(0, B00111111);
        seg7disp.writeDigitRaw(1, B00111111);
        seg7disp.writeDigitRaw(3, B00111111);
      }
      seg7disp.writeDisplay();
      knob.write(brightness);
    }else if(brightMode){
      /* Button press in brightness mode means return to time mode */
      optMode = 0;
      timeMode = 1;
      setMode = 0;
      brightMode = 0;
    }
    delay(500); // Wait so we don't catch multiple button presses
  }

  /* Display our options menu */
  if(optMode){
    newKount = knob.read();
    if(newKount > (knobKount + 5)){
      // Show next menu item
      if(menuDisplay == maxMenu){
        // move menu back to beginning
        menuDisplay = 1;
      }else{
        menuDisplay++;
      }
      knobKount = newKount;
      startDelay = millis(); // reset menu timeout
    }else if(newKount < (knobKount - 5)){
      // Show prev menu item
      if(menuDisplay == 1){
        // move menu to the end
        menuDisplay = maxMenu;
      }else{
        menuDisplay--;
      }
      knobKount = newKount;
      startDelay = millis(); // reset menu timeout
    }
    showMenu(menuDisplay);
    if(millis() - startDelay >= menuDelay){
      // Menu timeout, go back to time mode
      optMode = 0;
      timeMode = 1;
      startDelay = 0;
    }
  }

  /* Set the time */
  if(setMode){
    newKount = knob.read();
    if(setHour){ // Set the Hours
      if(newKount != knobKount && newKount < 13 && newKount > 0){
        hours = newKount;
        seg7disp.print(hours*100);
        // pad digit 3 and 4
        seg7disp.writeDigitRaw(3, B01000000);
        seg7disp.writeDigitRaw(4, B01000000);
        seg7disp.writeDisplay();
      }else if(newKount != knobKount && newKount > 12){
        knob.write(12);
        hours = 12;
      }else if(newKount != knobKount && newKount < 1){
        knob.write(1);
        hours = 1;
      }
    }else if(setMin){ // Set the minutes
      if(newKount != knobKount && newKount < 60 && newKount >= 0){
        mins = newKount;
        if(mins < 10){
          // pad digit 3 with a 0
          seg7disp.writeDigitRaw(3, B00111111);
        }
        seg7disp.print(mins);
        // pad digit 0 and 1
        seg7disp.writeDigitRaw(0, B01000000);
        seg7disp.writeDigitRaw(1, B01000000);
        seg7disp.writeDisplay();
      }else if(newKount != knobKount && newKount > 59){
        knob.write(59);
        mins = 59;
      }else if(newKount != knobKount && newKount < 1){
        knob.write(0);
        mins = 0;
      }
    }else if(setAMPM){ // Set AM/PM
      if(newKount != knobKount && newKount <= 1 && newKount >= 0){
        ampm = newKount;
        if(!ampm){
          seg7disp.writeDigitRaw(3,119); // A
          seg7disp.writeDigitRaw(4, B00000000); // No Dot
          knob.write(0);
          seg7disp.writeDisplay();
          knobKount = newKount;
        }else{
          seg7disp.writeDigitRaw(3,115); // P
          seg7disp.writeDigitRaw(4, B10000000); // Dot
          knob.write(1);
          seg7disp.writeDisplay();
          knobKount = newKount;
        }
      }else if(newKount != knobKount && newKount < 0){
        knob.write(0);
        ampm = 0;
      }else if(newKount != knobKount && newKount > 1){
        knob.write(1);
        ampm = 1;
      }
    }
    knobKount = newKount;
  }

  /* Set the brightness 1 - 16 */
  if(brightMode){
    newKount = knob.read();
    if(newKount != knobKount && newKount < 17 && newKount > 0){
      brightness = newKount;
      seg7disp.setBrightness(brightness);
      seg7disp.print(brightness);
      if(brightness > 9){
        // pad digit 1 and 2 with Zeros
        seg7disp.writeDigitRaw(0, B00111111);
        seg7disp.writeDigitRaw(1, B00111111);
      }else{
        // pad digits 1-3 with Zeros
        seg7disp.writeDigitRaw(0, B00111111);
        seg7disp.writeDigitRaw(1, B00111111);
        seg7disp.writeDigitRaw(3, B00111111);
      }
      seg7disp.writeDisplay();
      knobKount = newKount;
    }
    if(newKount < 1){ 
      knob.write(1);
    }else if(newKount > 16){
      knob.write(16);
    }    
  }

  /* Display the time */
  if(timeMode){
    digitalWrite(knobLEDgreen, LOW); // Turn off knob LED
    newKount = knob.read();
    if(newKount != knobKount){
      // TODO: if knob turned, display date
    }
    if(now.hour() == 0){
      hours = 12;
      ampm = 0;
    }else if(now.hour() > 12){
      hours = now.hour() - 12;
      ampm = 1;
    }else if(now.hour() == 12){
      hours = now.hour();
      ampm = 1;
    }else{
      hours = now.hour();
      ampm = 0;
    }
    mins = now.minute();
    dotCounter++;
    if(dotCounter == 9){
      if(drawDots){
        drawDots = 0;
      }else{
        drawDots = 1;
      }
      dotCounter = 0;
    }
    seg7disp.print((hours*100)+mins);
    if(ampm){
      int pm;
      if(mins > 9){
        // if minutes is > 9 we need to get the rightmost digit
        pm = mins-((mins/10)*10);
      }else{
        pm = mins;
      }
      //Serial.println(pm);
      seg7disp.writeDigitNum(4, pm, true);
    }
    seg7disp.drawColon(drawDots);
    seg7disp.writeDisplay();
    delay(100);
  }
}

void showMenu(int opt){
  if(opt == 1){ // Display "tSEt"
    seg7disp.drawColon(0);
    seg7disp.writeDigitRaw(0, B01111000);
    seg7disp.writeDigitRaw(1,109);
    seg7disp.writeDigitRaw(3,121);
    seg7disp.writeDigitRaw(4, B01111000);
    seg7disp.writeDisplay();
  }else if(opt == 2){ // Display "brt"
    seg7disp.drawColon(0);
    seg7disp.writeDigitRaw(0,124);
    seg7disp.writeDigitRaw(1,80);
    seg7disp.writeDigitRaw(3, B01111000);
    seg7disp.writeDigitRaw(4, B00000000);
    seg7disp.writeDisplay();
  }
}
