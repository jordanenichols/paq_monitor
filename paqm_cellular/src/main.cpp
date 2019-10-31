// Required libs
#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include "fonahelper.cpp"

// Assigned pins
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX); // FONA software serial connection
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);             // FONA library connection

// Wifi access point:
// Optionally configure a GPRS APN, username, and password.
// You might need to do this to access your network's GPRS/data
// network.  Contact your provider for the exact APN, username,
// and password values.  Username and password are optional and
// can be removed, but APN is required.
#define FONA_APN "wholesale"
#define FONA_USERNAME ""
#define FONA_PASSWORD ""

// Adafruit.io setup
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "yeetypete"
#define AIO_KEY "821a0b74bf0a4459afab437d0c77e561"

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab in the include folder
boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

// Setup feeds for for publishing.
Adafruit_MQTT_Publish methanedata = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/methanedata");
Adafruit_MQTT_Publish codata = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/codata");
Adafruit_MQTT_Publish flamgasdata = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/flamgasdata");
Adafruit_MQTT_Publish batterystatus = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/batterystatus");
Adafruit_MQTT_Publish gpsloc = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/gpsloc/csv");

// How many transmission failures in a row we're willing to be ok with before reset
uint8_t txfailures = 0;
#define MAXTXFAILURES 3;

// MQTT Connect function
void MQTT_connect();

// Function protoypes
void logMethane(float, Adafruit_MQTT_Publish &publishFeed);
void logCo(float, Adafruit_MQTT_Publish &publishFeed);
void logFlamgas(float, Adafruit_MQTT_Publish &publishFeed);
void logBattery(float, Adafruit_MQTT_Publish &publishFeed);
void logLocation(float, float, float, Adafruit_MQTT_Publish &publishFeed);
float getMQ2(int);
float getMQ4(int);
float getMQ7(int);
// Declare gps related vars
float latitude, longitude, speed_kph, heading, altitude, speed_mph;

void setup()
{
  Watchdog.enable(8000);
  Serial.begin(115200);
  Serial.println(F("Initializing MESP..."));

  //mqtt.subscribe(&onoffbutton);

  Watchdog.reset();
  delay(1000);
  Watchdog.reset();
  
  //Initializa FONA module
  fonaSS.begin(4800);
  if (!fona.begin(fonaSS))
  {
    Serial.println(F("Couldn't find FONA"));
  }

  Serial.println(F("FONA is OK"));

  Serial.println(F("Checking for network..."));
  while (!FONAconnect(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD)))
  {
    delay(1000);
    Serial.println(F("Connecting..."));
  }

  Serial.println(F("Connected to Cellular"));

  Watchdog.reset();
  delay(5000); // wait a few seconds to stabilize connection
  Watchdog.reset();
  
  int8_t ret = mqtt.connect();

  if (ret != 0)

  {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("MQTT connection failed, resetting in setup"));
  }
  
  Serial.println(F("MQTT Connected in setup"));

  // Make the initial GPS Reading
  Serial.println(F("Enabling GPS in setup"));
  fona.enableGPS(true);
}

void loop()
{
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  MQTT_connect(); //Ensure the device is connected to MQTT
  Serial.println(F("MQTT Connected in loop"));
  Watchdog.reset();

  // Grab battery reading
  uint16_t vbat;
  fona.getBattPercent(&vbat);

  float mq2Vals = getMQ2(A2);
  float mq4Vals = getMQ4(A1);
  float mq7Vals = getMQ7(A0);

  boolean gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  Watchdog.reset();
  delay(1000);
  Watchdog.reset();

  if (gpsFix)
  {
    Serial.print(F("GPS fix value:"));
    Serial.println(gpsFix);
    Serial.print(F("GPS latitide:"));
    Serial.println(latitude);
    Serial.print(F("GPS longitude:"));
    Serial.println(longitude);
    Serial.print(F("GPS speed KPH:"));
    Serial.println(speed_kph);
    Serial.print(F("GPS speed MPH:"));
    speed_mph = speed_kph * 0.621371192;
    Serial.println(speed_mph);
    Serial.print(F("GPS heading:"));
    Serial.println(heading);
    Serial.print(F("GPS altitude:"));
    Serial.println(altitude);
  }
  else
  {
    Serial.println(F("Waiting for GPS..."));
  }
  Serial.println(F("GPS Aquired"));

  // Publish data
  Serial.println(F("Publishing data..."));

  logMethane(mq4Vals, methanedata);
  logCo(mq7Vals, codata);
  logFlamgas(mq2Vals, flamgasdata);
  logBattery(vbat, batterystatus);
  if (latitude || longitude || altitude)
  {
    logLocation(latitude, longitude, altitude, gpsloc);
  }

  Watchdog.disable();
  delay(10000); //So as not to overload data limit on adafruit.io
  Watchdog.enable();
}

//////////////////////////////////////////////////////////////////

// Function to connect and reconnect as necessary to the MQTT server. Should be called in loop
void MQTT_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
  {
    return;
  }

  Serial.println(F("Connecting to MQTT in loop"));

  while ((ret = mqtt.connect()) != 0)
  { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println(F("Retrying MQTT connection in 5 seconds..."));
    mqtt.disconnect();
    delay(5000); // wait 5 seconds
  }
  Serial.println(F("MQTT Connected!"));
}

// Log methane function
void logMethane(float indicator, Adafruit_MQTT_Publish &publishFeed)
{

  // Publish
  Serial.print(F("Publishing methane data: "));
  Serial.println(indicator);
  if (!publishFeed.publish(indicator))
  {
    Serial.println(F("Publish failed!"));
    txfailures++;
  }
  else
  {
    Serial.println(F("Publish succeeded!"));
    txfailures = 0;
  }
}

// Log flammable gas and smoke function
void logFlamgas(float indicator, Adafruit_MQTT_Publish &publishFeed)
{

  // Publish
  Serial.print(F("Publishing flammable gas data: "));
  Serial.println(indicator);
  if (!publishFeed.publish(indicator))
  {
    Serial.println(F("Publish failed!"));
    txfailures++;
  }
  else
  {
    Serial.println(F("Publish succeeded!"));
    txfailures = 0;
  }
}

// Log carbon monoxide function
void logCo(float indicator, Adafruit_MQTT_Publish &publishFeed)
{

  // Publish
  Serial.print(F("Publishing CO data: "));
  Serial.println(indicator);
  if (!publishFeed.publish(indicator))
  {
    Serial.println(F("Publish failed!"));
    txfailures++;
  }
  else
  {
    Serial.println(F("Publish succeeded!"));
    txfailures = 0;
  }
}

// Log battery function
void logBattery(float indicator, Adafruit_MQTT_Publish &publishFeed)
{

  // Publish
  Serial.print(F("Publishing battery percentage: "));
  Serial.println(indicator);
  if (!publishFeed.publish(indicator))
  {
    Serial.println(F("Publish failed!"));
    txfailures++;
  }
  else
  {
    Serial.println(F("Publish succeeded!"));
    txfailures = 0;
  }
}

// Function to send gps location data
void logLocation(float latitude, float longitude, float altitude, Adafruit_MQTT_Publish &publishFeed)
{
  // Initialize a string buffer to hold the data that will be published.
  char sendBuffer[120];
  memset(sendBuffer, 0, sizeof(sendBuffer));
  int index = 0;

  // Start with '0,' to set the feed value.  The value isn't really used so 0 is used as a placeholder.
  sendBuffer[index++] = '0';
  sendBuffer[index++] = ',';

  // Now set latitude, longitude, altitude separated by commas.
  dtostrf(latitude, 2, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(longitude, 3, 6, &sendBuffer[index]);
  index += strlen(&sendBuffer[index]);
  sendBuffer[index++] = ',';
  dtostrf(altitude, 2, 6, &sendBuffer[index]);

  // Finally publish the string to the feed.
  Serial.print(F("Publishing location: "));
  Serial.println(sendBuffer);
  if (!publishFeed.publish(sendBuffer))
  {
    Serial.println(F("Publish failed!"));
    txfailures++;
  }
  else
  {
    Serial.println(F("Publish succeeded!"));
    txfailures = 0;
  }
}

float getMQ7(int analogPin)
{
  float RS_gas = 0;
  float ratio = 0;
  float sensorValue = 0;
  float sensor_volt = 0;
  float R0 = 35000;

  sensorValue = analogRead(analogPin);
  sensor_volt = sensorValue / 1024 * 5.0;
  RS_gas = (5.0 - sensor_volt) / sensor_volt;
  ratio = RS_gas / R0; //Replace R0 with the value found using the sketch above
  float x = 1538.46 * ratio;
  float ppm = pow(x, -1.709);
  Serial.print(F("CO PPM is: "));
  Serial.println(ppm);
  return ppm;
}

float getMQ4(int analogPin)
{
  float m = -0.318; //Slope
  float b = 1.133;  //Y-Intercept
  float R0 = 2.26;  //Sensor Resistance in fresh air from previous code

  float sensor_volt;                            //Define variable for sensor voltage
  float RS_gas;                                 //Define variable for sensor resistance
  float ratio;                                  //Define variable for ratio
  float sensorValue = analogRead(analogPin);    //Read analog values of sensor
  sensor_volt = sensorValue * (5.0 / 1023.0);   //Convert analog values to voltage
  RS_gas = ((5.0 * 10.0) / sensor_volt) - 10.0; //Get value of RS in a gas
  ratio = RS_gas / R0;                          // Get ratio RS_gas/RS_air
  double ppm_log = (log10(ratio) - b) / m;      //Get ppm value in linear scale according to the the ratio value
  double ppm = pow(10, ppm_log);                //Convert ppm value to log scale
  Serial.print(F("Methane PPM is: "));
  Serial.println(ppm);
  return ppm;
}

float getMQ2(int analogPin)
{
  float sensorValue = analogRead(analogPin);
  return sensorValue;
}