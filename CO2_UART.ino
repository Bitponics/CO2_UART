//running Arduino 1.0 IDE
//MH-Z14 CO2 Module from Zhengzhou Winsen Electronics Technology Co., Ltd
//Seeeduino Stalker v2.3

//Michael Doherty - Bitponics
//Crys Moore
//2.7.12
//updated 4.5.12



// sketch to get sensor readings via UART and print them to Adafruit 7 segment backpack.
//Sensor readings are time stamped and logged to the SD card. 

/*************************************************** 
 * This is a library for our I2C LED Backpacks
 * 
 * Designed specifically to work with the Adafruit LED 7-Segment backpacks 
 * ----> http://www.adafruit.com/products/881
 * ----> http://www.adafruit.com/products/880
 * ----> http://www.adafruit.com/products/879
 * ----> http://www.adafruit.com/products/878
 * 
 * These displays use I2C to communicate, 2 pins are required to 
 * interface. There are multiple selectable I2C addresses. For backpacks
 * with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
 * with 3 Address Select pins: 0x70 thru 0x77
 * 
 * Adafruit invests time and resources providing this open source code, 
 * please support Adafruit and open-source hardware by purchasing 
 * products from Adafruit!
 * 
 * Written by Limor Fried/Ladyada for Adafruit Industries.  
 * BSD license, all text above must be included in any redistribution
 ****************************************************/


#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
//#include <SD.h>
#include <SdFat.h>



//using softserial to send/recieve data over analog pins 0 & 1
SoftwareSerial mySerial(A0, A1); // RX, TX respectively
Adafruit_7segment matrix = Adafruit_7segment();
RTC_DS1307 RTC;
//File dataFile = SD.open("datalog1.csv", FILE_WRITE);
SdFat sd;
SdFile myFile;



//VARIABLES  
byte cmd[9] = {
  0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
char response[9]; 
const int chipSelectPin = 10;
unsigned long time;
unsigned long calibrationTime = 60000; //warm up time for C02 sensor (3min = 180sec)
float lipoCalibration=.685; //was 1.1
float voltage;
int BatteryValue;
float threshold = 4.0; //battery threshold
boolean logging = true; 
char userCmd;
char timeStamp[20];
String ppmString = " ";
//////////////////////////////////////////////////////////// SETUP
void setup() 
{
   sdCardSetup();
  analogReference(INTERNAL); 
  pinMode(A2,INPUT);
  mySerial.begin(9600);
  matrix.begin(0x70);
  Wire.begin();
  RTC.begin();
  matrix.setBrightness(5); //0-15 (11 drawing 50mA of current from the NCP1402-5V Step-Up Breakout)
  pinMode(10, OUTPUT);

  //ADJUST THE DATE AND INITIALIZE SD CARD
  if (! RTC.isrunning())
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }


}
////////////////////////////////////////////////////////////END SETUP


void loop() 
{
  time = millis(); //1s = 1000
  DateTime now = RTC.now();

  //PING CO2, READ RESPONSE, GET READINGS AND CONVERT TO PPM
  mySerial.write(cmd,9);
  mySerial.readBytes(response, 9);
  int responseHigh = (int) response[2];
  int responseLow = (int) response[3];
  int ppm = (256*responseHigh)+responseLow;

  //READ VALUE FROM LIPO, CONVERT TO VOLTAGE
  BatteryValue = analogRead(A2);
  voltage = BatteryValue * (lipoCalibration / 1024)* (10+2)/2;  //Voltage divider 
  //  Serial.print("Battery Voltage -> ");
  //  Serial.print(voltage);
  //  Serial.print("V   ");
  //  Serial.println();

  if (voltage < threshold)
  {
    //display the battery low msg 'Batt'
    matrix.print(0xBAFF, HEX);
    matrix.writeDisplay();
  }
  else if (millis() < calibrationTime)
  { 
    matrix.print(0xB00B, HEX);
    matrix.println((calibrationTime - millis())/1000);
    matrix.writeDisplay(); 
    Serial.println((calibrationTime - millis())/1000);
  }
  //if the battery is good and the calibration period is done...
  else
  {  
    ppmString = String(ppm); //cast ppm from int to string
    matrix.println(ppm);  //print c02 readings to SD
    matrix.writeDisplay();

    if (logging )
    {

      sprintf(timeStamp,"%04i-%02i-%02iT%02i:%02i:%02i", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()); 
      logData();
    }
    else 
    {
      // Serial.println("error opening datalog1.csv");
    }
  }
}



void serialEvent() {

  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 

    if (inChar == '\n') {
      logging = !logging;
      switch(userCmd){
      case 'r': 
        readSd(); 
        break;
      case 'f': //delete the SD
        //formatCard();
        break;
      }
    } 
    else userCmd = inChar;
  }
}

/*void logData()
  {

  dataFile = SD.open("datalog1.csv", FILE_WRITE);
  if (dataFile)
  {
    dataFile.print(ppmString);
    dataFile.print(',');
    dataFile.print(timeStamp);
    dataFile.print(','); 
    dataFile.print(voltage);
    dataFile.println();
    dataFile.close();   
  }
}*/

void logData()
{

  //dataFile = SD.open("datalog1.csv", FILE_WRITE);
  if (!myFile.open("datalog1.csv", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening datalog1.csv for write failed");
  }
  myFile.print(ppmString);
  myFile.print(',');
  myFile.print(timeStamp);
  myFile.print(','); 
  myFile.print(voltage);
  myFile.println();
  myFile.close();   
}


/*  void readSd()
  {
    File dataFile = SD.open("datalog1.csv");

    if (dataFile) {
      while (dataFile.available()) {
        Serial.write(dataFile.read());
      }
      dataFile.close();
    }  
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog1.csv");
    }
  }
  */
  void readSd()
{
  //  File dataFile = SD.open("datalog1.csv");
  if (!myFile.open("datalog1.csv", O_READ)) {
    sd.errorHalt("opening datalog1.csv for read failed");

    /*if (dataFile) {
     while (dataFile.available()) {
     Serial.write(dataFile.read());
     }
     dataFile.close();
     }  
     // if the file isn't open, pop up an error:
     else {
     Serial.println("error opening datalog1.csv");
     }*/

    // read from the file until there's nothing else in it:
    int data;
    while ((data = myFile.read()) > 0) Serial.write(data);
    // close the file:
    myFile.close();


  }
}



