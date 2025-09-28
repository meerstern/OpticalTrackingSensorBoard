// PAT9125EL: Miniature Optical Navigation Chip reference code.
// Version: 1.2
// Latest Revision Date: 23 July 2018
// By PixArt Imaging Inc.
// Primary Engineer: Vincent Yeh (PixArt USA)

// Copyright [2018] [Vincent Yeh]
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0

#include <Arduino.h>
#include <Wire.h>

#define WIRE Wire

const uint8_t initialize[][2] = {
    { 0x09, 0x5A },     //Disables write-protect.
    { 0x0D, 0xB5 },     //Sets X-axis resolution (necessary value here will depend on physical design and application).
    { 0x0E, 0xFF },     //Sets Y-axis resolution (necessary value here will depend on physical design and application).
    { 0x19, 0x04 },     //Sets 12-bit X/Y data format.
    //{ 0x4B, 0x04 },     //Needed for when using low-voltage rail (1.7-1.9V): Power saving configuration.
};
#define initialize_size (sizeof(initialize)/sizeof(initialize[0]))

#define I2C_Slave_ID 0x75                           //ONLY WHEN ID PIN IS TIED TO GROUND! ID changes when tied to VDD (0x73) or when left floating (0x79).

int8_t deltaX_low, deltaY_low;                      //Stores the low-bits of movement data.
int16_t deltaX_high, deltaY_high;                   //Stores the high-bits of movement data.
int16_t deltaX, deltaY;                             //Stores the combined value of low and high bits.
int16_t totalX, totalY = 0;                         //Stores the total deltaX and deltaY moved during runtime.

uint8_t readRegister(uint8_t addr)
{
    uint8_t data;
    WIRE.beginTransmission(I2C_Slave_ID);
    WIRE.write(addr);
    WIRE.endTransmission(0);
    WIRE.requestFrom((uint8_t)I2C_Slave_ID,(uint8_t)1);
    data = WIRE.read();
    return(data);
}

void writeRegister(uint8_t addr, uint8_t data)
{
    WIRE.beginTransmission(I2C_Slave_ID);
    WIRE.write(addr);
    WIRE.write(data);
}

void load(const uint8_t array[][2], uint8_t arraySize)
{
    for(uint8_t q = 0; q < arraySize; q++)
    {
        writeRegister(array[q][0], array[q][1]);    //Writes the given array of registers/data.
    }
}

void grabData(void)
{
    deltaX_low = readRegister(0x03);        //Grabs data from the proper registers.
    deltaY_low = readRegister(0x04);
    deltaX_high = (readRegister(0x12)<<4) & 0xF00;      //Grabs data and shifts it to make space to be combined with lower bits.
    deltaY_high = (readRegister(0x12)<<8) & 0xF00;
    
    if(deltaX_high & 0x800)
        deltaX_high |= 0xf000; // 12-bit data convert to 16-bit (two's comp)
        
    if(deltaY_high & 0x800)
        deltaY_high |= 0xf000; // 12-bit data convert to 16-bit (2's comp)
        
    deltaX = deltaX_high | deltaX_low;      //Combined the low and high bits.
    deltaY = deltaY_high | deltaY_low;
}

void printData(void)
{
    if((deltaX != 0) || (deltaY != 0))      //If there is deltaX or deltaY movement, print the data.
    {
        totalX += deltaX;
        totalY += deltaY;
        Serial.print("deltaX: " + String(deltaX) + "\t deltaY:" + String(deltaY)+"\t\t");    //Prints each individual count of deltaX and deltaY.
        Serial.print("X-axis Counts: " + String(totalX) + "\t Y-axis Counts: " + String(totalY) + "\n\r");  //Prints the total movement made during runtime.
    }
    
    deltaX = 0;                             //Resets deltaX and Y values to zero, otherwise previous data is stored until overwritten.
    deltaY = 0;
}

void setup()
{

  Serial.begin(9600);
  WIRE.begin();
  delay(10);
  Serial.println("Optical Tracking Sensor Test Start");

  
  if(readRegister(0x00) != 0x31)
  {
    Serial.println("Communication protocol error! Terminating program.\n\r");
    return 0;
  }
  
  writeRegister(0x06, 0x97);          //Software reset (i.e. set bit7 to 1)
  delay(1);                           //Delay 1 ms for chip reset timing.
  writeRegister(0x06, 0x17);          //Ensure software reset is done and chip is no longer in that state.
  
  load(initialize, initialize_size);  //Load register settings from the "initialize" array
  
  
  
  if(readRegister(0x5E) == 0x04)      //These unlisted registers are used for internal recommended settings.
  {
      writeRegister(0x5E, 0x08);
      
      if(readRegister(0x5D) == 0x10)
          writeRegister(0x5D, 0x19);
  }
  
  writeRegister(0x09, 0x00);  // enable write protect.
}



void loop()
{
  if(readRegister(0x02) & 0x80)
  {
      grabData();
      printData();
  }
  delay(100);
  
}
