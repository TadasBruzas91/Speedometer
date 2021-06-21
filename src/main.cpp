#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "Free_Fonts.h"
#include "RTClib.h"
#include "ms_to_time.h" // library convert ms to normal time

#define HALL PB3
#define TRIP_RESET PB4
#define DISPLAY_CHANGE PB5
#define SCREEN_UPDATE_TIME 250

int addressOdo = 0;
int addressTrip = 5;
int addressTripDriveTime = 10;
int addressTripAvgSpeed = 15;
int addressTripIdleTime = 20;
int addressTripStartTime = 25;

int screenRotation = 1;

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

Adafruit_BME280 bme;

unsigned long start, finished;
unsigned long elapsed;
unsigned long tempUpdated;
unsigned int circMetric = 206; // wheel circumference (in centimeters)
unsigned int speedk;           // holds calculated speed vales in metric
unsigned int distance = 0;
unsigned int odometer = 0;
unsigned long tripStartTime = 0;
unsigned long tripDriveTime = 0;
float tripDriveAvgSpeed = 0.00f;
unsigned long tripIdleTime = 0;
unsigned long distanceRstTime;
unsigned long screenChangeTime;
unsigned long screenRstTime;
int screenSelector = 1;
int scrensAvailable = 2;

bool savedToEeprom = false;
bool afterStartTemp = true;

RTC_DS3231 rtc;

void calcSpeed();
void resetSpeed();
void resetDistance();
void calculateDriveTime();
void calculateIdleTime();
void displaySpeed();
void displayTime();
void displayTemp();
void displayOdo();
void displayTrip();
void displayTripStart();
void displayTripDriveTime();
void displayTripDriveAvgSpeed();
void displayTripIdleTime();
void getDataFromEeprom();
void writeDataToEeprom();
// Available screens
void mainScreen();
void tripDataScreen();

void changeView();
void screenReset();
void displayView();

void setup()
{

  start = millis();
  //Set up the display
  tft.init();
  tft.setRotation(screenRotation);
  for (int i = 0; i < 2; i++)
  {
    tft.fillScreen(TFT_BLACK);
  }

  // Set up bme280
  bme.begin(0x76);

  // Set up ds3231
  if (rtc.begin())
  {
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  getDataFromEeprom();

  // Set up pins
  pinMode(HALL, INPUT_PULLUP); // Hall sensor input
  attachInterrupt(digitalPinToInterrupt(HALL), calcSpeed, FALLING);
  pinMode(TRIP_RESET, INPUT_PULLUP);     // Trip reset button
  pinMode(DISPLAY_CHANGE, INPUT_PULLUP); // Change view button
}

void loop()
{
  displayView();
  changeView();

  calculateDriveTime();
  calculateIdleTime();
  writeDataToEeprom();
  resetDistance();
  resetSpeed();
  delay(SCREEN_UPDATE_TIME);
}

void calcSpeed()
{
  bool x = digitalRead(HALL);
  if (!x)
  {
    //calculate elapsed
    elapsed = millis() - start;
    //reset start
    start = millis();
    //calculate speed in cm/h
    speedk = (3600 * circMetric) / elapsed;

    distance += circMetric;
    odometer += circMetric;
  }
}

void resetSpeed()
{
  if (millis() - start > 3000)
    speedk = 0;
}

void resetDistance()
{
  bool btn = digitalRead(TRIP_RESET);
  if (!btn)
  {
    if (millis() - distanceRstTime > 3000)
    {
      DateTime now = rtc.now();
      distance = tripDriveTime = tripIdleTime = 0;
      tripDriveAvgSpeed = 0.00f;
      tripStartTime = (now.hour() * 3600000) + (now.minute() * 60000) + (now.second() * 1000);
      savedToEeprom = false;
    }
  }
  else
  {
    distanceRstTime = millis();
  }
}

void calculateDriveTime()
{
  if (speedk / 100 > 4)
  {
    tripDriveTime += SCREEN_UPDATE_TIME;
  }
}

void calculateIdleTime()
{
  if (speedk / 100 < 4)
  {
    tripIdleTime += SCREEN_UPDATE_TIME;
  }
}

void displayTime()
{
  DateTime now = rtc.now();
  tft.setCursor(5, 5);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.printf("%02d:%02d", now.hour(), now.minute());
}

void displaySpeed()
{
  int speedPos = 100;
  int kmph = speedk / 100;
  kmph = constrain(kmph, 0, 99);
  int meterph = speedk / 10 - (kmph * 10);
  if (kmph >= 99)
    meterph = 9;
  String space;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (kmph < 10)
    space = "  ";
  tft.drawString(space + String(kmph), speedPos, 80, 8);
  tft.drawString(String(meterph), speedPos + 114, 80, 6);
}

void displayTemp()
{
  if (millis() - tempUpdated > 5000 || afterStartTemp)
  {
    String temp = String(bme.readTemperature(), 1);
    tft.setCursor(230, 5);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextFont(4);
    tft.print(temp);
    tft.print(" C");
    tempUpdated = millis();
    afterStartTemp = false;
  }
}

void displayOdo()
{
  int odoPos = 5;
  int km = odometer / 100000;
  if (km >= 10000)
    odometer = 0;
  int m100 = odometer / 10000 - (odometer / 100000 * 10);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(odoPos, 200);
  tft.setTextFont(4);
  tft.printf("%04d,", km);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.printf("%d", m100);
}

void displayTrip()
{
  int odoPos = 200;
  int km = distance / 100000;
  if (km >= 10000)
    distance = 0;
  int m100 = distance / 100 - (distance / 100000 * 1000);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(odoPos, 200);
  tft.setTextFont(4);
  tft.printf("%04d,", km);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.printf("%03d", m100);
}

void displayTripStart()
{
  MsConverter time(tripStartTime);

  tft.setCursor(5, 5);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.print("Trip start ");
  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.print(time.getTimeString());
}

void displayTripDriveTime()
{
  MsConverter time(tripDriveTime);

  tft.setCursor(5, 45);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.print("Drive time ");
  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.print(time.getTimeString());
}

void displayTripDriveAvgSpeed()
{
  float time = tripDriveTime / 3600000.0;
  float avgSpeed = (distance / 100000.0) / time;
  if(distance == 0) avgSpeed = 0.0;
  tft.setCursor(5, 85);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.print("Avg speed ");
  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.print(avgSpeed, 1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(" km/h");
}

void displayTripIdleTime()
{
  MsConverter time(tripIdleTime);

  tft.setCursor(5, 125);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextFont(4);
  tft.print("Idle time ");
  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.print(time.getTimeString());
}

void getDataFromEeprom()
{
  EEPROM.get(addressOdo, odometer);
  EEPROM.get(addressTrip, distance);
  EEPROM.get(addressTripDriveTime, tripDriveTime);
  EEPROM.get(addressTripAvgSpeed, tripDriveAvgSpeed);
  EEPROM.get(addressTripIdleTime, tripIdleTime);
  EEPROM.get(addressTripStartTime, tripStartTime);
}

void writeDataToEeprom()
{
  if (speedk / 100 > 5)
    savedToEeprom = false;
  if (!savedToEeprom && speedk == 0)
  {
    EEPROM.put(addressOdo, odometer);
    EEPROM.put(addressTrip, distance);
    EEPROM.put(addressTripDriveTime, tripDriveTime);
    EEPROM.put(addressTripAvgSpeed, tripDriveAvgSpeed);
    EEPROM.put(addressTripIdleTime, tripIdleTime);
    EEPROM.put(addressTripStartTime, tripStartTime);
    savedToEeprom = true;
  }
}

void mainScreen()
{
  displayTime();
  displayTemp();
  displaySpeed();
  displayOdo();
  displayTrip();
}

void tripDataScreen()
{
  displayTripStart();
  displayTripDriveTime();
  displayTripDriveAvgSpeed();
  displayTripIdleTime();
  displayOdo();
  displayTrip();
}

void changeView()
{
  bool btn = digitalRead(DISPLAY_CHANGE);
  if (!btn)
  {
    if (millis() - screenChangeTime > 1000)
    {
      screenSelector++;
      if (screenSelector > scrensAvailable)
        screenSelector = 1;
      if (screenSelector < 1)
        screenSelector = scrensAvailable;
      screenChangeTime = millis();
      tft.fillScreen(TFT_BLACK);
      displayView();
    }
  }
  else
  {
    screenChangeTime = millis();
  }
}

// void screenReset()
// {
//   bool btn = digitalRead(DISPLAY_CHANGE);
//   if (!btn)
//   {
//     if (millis() - screenRstTime > 3000)
//       screenSelector = 0;
//   }
//   else
//   {
//     screenRstTime = millis();
//   }
// }

void displayView()
{
  if (screenSelector == 1)
    mainScreen();
  else if (screenSelector == 2)
    tripDataScreen();

  // screenReset();
}