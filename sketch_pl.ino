/* 
Sketch to water our olive plant called "Princess Lea"
Stefan Sunaert - v1.0 - dd Jan 24, 2024 - treanus@gmail.com

Using:
  - a DFRobot Analog Waterproof Capacitive Soil Moisture Sensor
      see: https://wiki.dfrobot.com/Waterproof_Capacitive_Soil_Moisture_Sensor_SKU_SEN0308
  - a water pump recycled from the water basin of our cat called "Remi"
  - a relay switch that controls that pump
  - 2 push buttons to set the desired moisture level
  - all this (for now) connected on a breadboard

Logic is as follows:
  water Princess Lea if she is thirsty
  do so by monitoring the moisture sensor above
  wait at least x minutes that the water sensor senses dry soil, not to trigger the pump on a false reading
  add a bit of water: switch on the pump for y seconds, wait another x minutes before adding more...
    and not constantly until the moisture sensor fails the water added, since then - maybe we add too much water
    thus add sips of water for y seconds, every x minutes,
    until the mositure sensor picks it up
  Then wait again... it might rain and Princess Lea has enough water, or she might dry out in the sun...

Implementation:
  1/ Sensor has been calibrated according to the above website
  - variable "soilMoisturePercent" a float value varies between 0 and 100% (from completely dry to soaking wet)
  - variabale "thresholdAddWater" is the hard coded value threshold for adding H2O

  2/ Wait for x minutes of dry soil; then start adding H2O
    - use dryTime and millis()

  3/ Add water for y seconds
    - use addWater

Also use the LCD display to monitor according to:
 https://docs.arduino.cc/learn/electronics/lcd-displays

Also include 2 buttons to change the moisture threshold value
  https://arduinogetstarted.com/tutorials/arduino-button-library
*/

// include the library code:
#include <LiquidCrystal.h> // library for the LCD screen.
#include <ezButton.h> // library for the up/down push buttons

// LCD
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// BUTTONS
// initialize the moisture up/down buttons
ezButton button1(7);  // create ezButton object that attach to pin 6;
ezButton button2(6);  // create ezButton object that attach to pin 7;

// DFRobot Analog Waterproof Capacitive Soil Moisture Sensor
//  Here, we did a calibration of the moisture sensor according to it's webpage
//  Pin A0 is used as measurement pin
const float AirValue = 620;   //you need to change this value that you had recorded in the air
const float WaterValue = 35;  //you need to change this value that you had recorded in the water
int soilMoistureValue = 0;
float soilMoisturePercent = 0;

// The (initial) threshold of moisture that triggers addition of water
//  this can be changed by using the up/down buttons
//  important to optimise this one for each soil and/or plant sort! (currently for an olive tree initialised at 37% moisture)
float thresholdAddWater = 37.00;

// We use a small water pump to feed the plant, it's controlled via a relay
// The arduino pin that controls the water pump
const int RELAY_PIN = 8;  // the Arduino pin, which connects to the IN pin of relay


// Set up monitoring and addition of H2O intervals
// dryTime is the length of time in ms before we add water when dry, set to 3 minutes
const long dryTime = 180000; // 1000ms * 60 * 3 = 180.000 ms
// addWater is the lenght of time in ms that we activate the pump
// now set to 20s 
const long addWater = 20000; // 1000ms * 20s = 20000 ms

// Ohter variables used in the sketch
unsigned long previousMillisSense = 0;  // will store last time was updated
unsigned long intervalSense = 1000;  // interval at which to poll the soil moisture (milliseconds)
unsigned long dryTimeSum = 0; // each time it's dry add some time
unsigned long addWaterSum = 0; // each time it's dry add some time
unsigned long timeAddH20 = 0; // time to add next H20
String lcd_line1 = "Hello Lea"; // Set up the LCD display - string on top line
String lcd_line2 = "1st measurement      "; // string on bottom line
String diff = ""; // a variable for the lcd string (either > or >=)

// Lets do the setup of the arduino
void setup() {
  Serial.begin(9600); // open serial port, set the baud rate to 9600 bps

  // initialize digital pin as an output.
  pinMode(RELAY_PIN, OUTPUT);

  // initialize the up/down moisture buttons
  button1.setDebounceTime(20); // set debounce time to 50 milliseconds
  button2.setDebounceTime(20); // set debounce time to 50 milliseconds

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print the initial message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print(lcd_line1);
  lcd.setCursor(0, 1);
  lcd.print(lcd_line2);
}

void loop() {

  // Read the buttons continuously
  button1.loop(); 
  button2.loop(); 
  // Depending the button press increase or lower the moisture threshold for adding water
  if(button1.isPressed()) {
    Serial.println("The button 1 is pressed");
    thresholdAddWater = thresholdAddWater + 1;
  }
  if(button2.isPressed()) {
    Serial.println("The button 2 is pressed");
    thresholdAddWater = thresholdAddWater - 1;
  }

  // Sense moisture every intervalSense
  // We use the millis() fn to do this only according to the above defined timings
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisSense >= intervalSense) {
    previousMillisSense = currentMillis; 

    soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
    //Serial.println(soilMoistureValue); // uncomment if you want to see the raw sensor value

    // Convert the raw value to % moisture
    soilMoisturePercent = 100 - (soilMoistureValue - WaterValue) / (AirValue - WaterValue) * 100;
    Serial.println("Measured moisture level: " + String(soilMoisturePercent) + "%"); 

    // Switch on pump for addwater if:
    //  - soil is dry for dryTime
    if(soilMoisturePercent < thresholdAddWater ) {
      dryTimeSum = dryTimeSum + intervalSense;
      Serial.println("It's dry now for " + String(dryTimeSum) + " ms");

      if(dryTimeSum >= dryTime) {
        if (addWaterSum < addWater) {
          addWaterSum = addWaterSum + intervalSense;
          Serial.print("Going to add H2O for ");
          Serial.println(" " + String(intervalSense) + " ms; busy with " +  String(addWaterSum));
          digitalWrite(RELAY_PIN, HIGH);
          lcd_line2 = "Add H20 for " + String(addWater / 1000) + "s   ";
        }
        else { // temporarily stop adding water, let it soak in and sense further
          addWaterSum = 0; 
          dryTimeSum = 0;
          digitalWrite(RELAY_PIN, LOW);
          //lcd_line2 = "W8 soak for " + String(dryTime / 1000) + "s   ";
        }
      }
      else {
        timeAddH20 = dryTime - dryTimeSum;
        lcd_line2 = "H20 add in " + String(timeAddH20 / 1000) + "s   ";
      }
    }
    else { 
      Serial.println("Princess Lea is happy");
      digitalWrite(RELAY_PIN, LOW);
      lcd_line2 = "Lea is Happy    ";
    }
    Serial.println("dryTimeSum: " + String(dryTimeSum));
    Serial.println("addWaterSum: " + String(addWaterSum));
  }

  // Print on the LED screen
  if(soilMoisturePercent < thresholdAddWater) {
   diff = "<";
  } 
  else {
    diff = ">=";
  }
  lcd_line1 = "M: " + String(soilMoisturePercent) + "% " + diff + " " + String(int(thresholdAddWater)) + "%  ";
  lcd.setCursor(0, 0);
  lcd.print(lcd_line1);
  lcd.setCursor(0, 1);
  lcd.print(lcd_line2);
}
