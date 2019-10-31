#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

#include <Wire.h>
#include "Adafruit_SGP30.h"

BLEClientCts  bleCTime;
Adafruit_SGP30 sgp;


/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
  return absoluteHumidityScaled;
}

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE 1
#define MINIMUM_FIRMWARE_VERSION "0.6.6"
#define MODE_LED_BEHAVIOUR "MODE"
/*=========================================================================*/

/*Create the bluefruit object using hardware SPI using 
SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST*/
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

void setup()
{
  Serial.begin(9600);
  Serial.println("PAQ Monitor BLE test");

  //Initialise the module
  Serial.println("Initialising the Bluefruit LE module: ");

  if (!ble.begin(VERBOSE_MODE))
  {
    Serial.println("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
  }
  Serial.println("OK!");

  if (FACTORYRESET_ENABLE)
  {
    //Perform a factory reset to make sure everything is in a known state
    Serial.println("Performing a factory reset");
    if (!ble.factoryReset())
    {
      Serial.println("Couldn't factory reset");
    }
  }

  //Disable command echo from Bluefruit
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");

  //Print Bluefruit information
  ble.info();

  Serial.println("Please use Adafruit Bluefruit LE app to connect in UART mode");
  Serial.println("Then Enter characters to send to Bluefruit");
  Serial.println();

  ble.verbose(false); //Debug info is a little annoying after this point

  //Wait for connection
  while (!ble.isConnected())
  {
    delay(500);
  }

  Serial.println("Connected");

  //LED Activity command is only supported from 0.6.6
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
  {
    //Change Mode LED Activity
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  //Set module to DATA mode
  Serial.println("Switching to DATA mode");
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println("DATA mode enabled");

  bleCTime.begin(); //get current time function

  if (sgp.begin())
  {
    Serial.println("SGP30 sensor enabled");
  }
}

void loop()
{
  sgp.IAQmeasure();

  if (!ble.isConnected())
  {
    bleCTime.getCurrentTime();
    bleCTime.getLocalTimeInfo();
  }

  //Print data to serial monitor
  Serial.print(sgp.TVOC);
  Serial.print(" ");
  Serial.println(sgp.eCO2);

  //Send AQ data to host via Bluefruit
  ble.print(sgp.TVOC);
  ble.print(" ");
  ble.println(sgp.eCO2);
  delay(1000);
}