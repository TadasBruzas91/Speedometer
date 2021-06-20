#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "Free_Fonts.h"
#include "RTClib.h"

int addressOdo = 0;
int addressTrip = 9;

int screenRotation = 1;

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

Adafruit_BME280 bme;

String temp;

int input;

unsigned long start, finished;
unsigned long elapsed;
unsigned long tempUpdated;
unsigned int circMetric = 205; // wheel circumference (in centimeters)
unsigned int speedk;           // holds calculated speed vales in metric
unsigned int distance = 0;
unsigned int odometer = 0;
unsigned long distanceRstTime;

bool savedToEeprom = false;

RTC_DS3231 rtc;

void calcSpeed();
void resetSpeed();
void resetDistance();
void displaySpeed();
void displayTime();
void displayTemp();
void displayOdo();
void displayTrip();
void getDataFromEeprom();
void writeDataToEeprom();
void changeView();

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
  pinMode(PB3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PB3), calcSpeed, FALLING);
  pinMode(PB4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PB5), changeView, FALLING);
}

void loop()
{

  displayTime();
  displayTemp();
  displaySpeed();
  displayOdo();
  displayTrip();
  writeDataToEeprom();

  resetDistance();
  resetSpeed();
  delay(500);
}

void calcSpeed()
{
  bool x = digitalRead(PB3);
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
  bool btn = digitalRead(PB4);
  if (!btn)
  {
    if (millis() - distanceRstTime > 3000)
    {
      distance = 0;
      savedToEeprom = false;
    }
  }
  else
  {
    distanceRstTime = millis();
  }
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

void displayTime()
{
  DateTime now = rtc.now();
  String time = String(now.hour()) + ":" + String(now.minute());
  tft.setCursor(5, 5);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(time);
}

void displayTemp()
{
  if (millis() - tempUpdated > 5000)
  {
    temp = String(bme.readTemperature(), 1);
    tft.setCursor(230, 5);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(temp);
    tft.print("c");
    tempUpdated = millis();
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

void getDataFromEeprom()
{
  EEPROM.get(addressOdo, odometer);
  EEPROM.get(addressTrip, distance);
}

void writeDataToEeprom()
{
  if (speedk / 100 > 5)
    savedToEeprom = false;
  if (!savedToEeprom && speedk == 0)
  {
    EEPROM.put(addressOdo, odometer);
    EEPROM.put(addressTrip, distance);
    savedToEeprom = true;
  }
}

void changeView()
{
}