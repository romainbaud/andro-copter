// Arduino sketch for the AndroCopter project.
// Romain Baud, 2013.

// This code has been tested for the Arduino ADK board only.
// The ADK library is necessary to compile the sketch. Follow the instructions
// at http://developer.android.com/tools/adk/index.html

#include <Wire.h>
#include <Servo.h>

#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory.h>

const int BATTERY_LEVEL_PIN = A0;
const int NW_PWM_PIN = 3;
const int NW_GND_PIN = 4;
const int NE_PWM_PIN = 6;
const int NE_GND_PIN = 7;
const int SE_PWM_PIN = 8;
const int SE_GND_PIN = 9;
const int SW_PWM_PIN = 11;
const int SW_GND_PIN = 12;

const int PULSE_MIN = 1000; // Min time of a pulse sent to an ESC [us].
const int PULSE_MAX = 2000; // Max time of a pulse sent to an ESC[us].
const unsigned long MAX_TIME_WITHOUT_RECEPTION = 500; // If no data is received during this time, stop all motors [ms].
const unsigned long PULSES_PERIOD = 2222; // Period of the main loop (450 Hz) [us].
const unsigned int BATT_VOLT_TX_PERIOD = 500; // Sending period of the battery voltage [ms].

Servo nwMotor, neMotor, seMotor, swMotor;
int nwPower, nePower, sePower, swPower; // Impulses duration.
unsigned long currentTimeMs, lastRxTimeMs, lastTxTimeMs, lastPulseTimeUs, currentPulseTimeUs;
uint8_t rxBuffer[4];

AndroidAccessory acc("Romain Baud",
		     "AndroCopterADK",
		     "AndroCopter ADK board",
		     "1.0",
		     "randroprog.blogspot.com",
		     "0000000012345678");

void setup()
{
  // Setup the serial communication with the computer (debug).
  Serial.begin(115200);
  Serial.print("\r\nStart");
  
  // Start the Android accessory object.
  acc.powerOn();
  
  // Variables init.
  currentTimeMs = millis();
  lastRxTimeMs = currentTimeMs;
  lastTxTimeMs = currentTimeMs;
  lastPulseTimeUs = micros();
  
  nwPower = PULSE_MIN;
  nePower = PULSE_MIN;
  sePower = PULSE_MIN;
  swPower = PULSE_MIN;
  
  // Associate the Servo objects to the pins.
  nwMotor.attach(NW_PWM_PIN);
  neMotor.attach(NE_PWM_PIN);
  seMotor.attach(SE_PWM_PIN);
  swMotor.attach(SW_PWM_PIN);
  
  // Set the GND pins for each ESC.
  pinMode(NW_GND_PIN, OUTPUT); digitalWrite(NW_GND_PIN, LOW);
  pinMode(NE_GND_PIN, OUTPUT); digitalWrite(NE_GND_PIN, LOW);
  pinMode(SE_GND_PIN, OUTPUT); digitalWrite(SE_GND_PIN, LOW);
  pinMode(SW_GND_PIN, OUTPUT); digitalWrite(SW_GND_PIN, LOW);
}

void loop()
{
  currentTimeMs = millis();
  
  // Check if the phone is connected.
  if (acc.isConnected()) // Yes, try to read it.
  {
    int len = acc.read(rxBuffer, sizeof(rxBuffer), 4);

    if(len == 4)
    {
      // Update the motor signals.
      // The received powers for the motors (data[]) are between 0 and 255, so
	  // a transformation to the ESC range (1000-2000 us) is performed.
      nwPower = map(rxBuffer[0], 0, 255, PULSE_MIN, PULSE_MAX);
      nePower = map(rxBuffer[1], 0, 255, PULSE_MIN, PULSE_MAX);
      sePower = map(rxBuffer[2], 0, 255, PULSE_MIN, PULSE_MAX);
      swPower = map(rxBuffer[3], 0, 255, PULSE_MIN, PULSE_MAX);
    
      lastRxTimeMs = currentTimeMs;
    }
  }
  else // Phone disconnected: emergency stop!
  {
    nwPower = PULSE_MIN;
    nePower = PULSE_MIN;
    sePower = PULSE_MIN;
    swPower = PULSE_MIN;
  }
  
  // If the phone does not send updates for some time, this mean
  // something could have go wrong (maybe the app crashed...), and
  // all motors must be stopped.
  if(currentTimeMs - lastRxTimeMs > MAX_TIME_WITHOUT_RECEPTION)
  {
    nwPower = PULSE_MIN;
    nePower = PULSE_MIN;
    sePower = PULSE_MIN;
    swPower = PULSE_MIN;
  }
  
  // Send a pulse to each ESC.
  nwMotor.writeMicroseconds(nwPower);
  neMotor.writeMicroseconds(nePower);
  seMotor.writeMicroseconds(sePower);
  swMotor.writeMicroseconds(swPower);
  
  // Sometimes, send the battery level to the phone.
  if(currentTimeMs - lastTxTimeMs >= BATT_VOLT_TX_PERIOD)
  {
    lastTxTimeMs = currentTimeMs;
    uint16_t batteryLevel = analogRead(BATTERY_LEVEL_PIN);
    acc.write((uint8_t*)&batteryLevel, 2);
  }
  
  // Delay to get a 450 Hz main loop.
  currentPulseTimeUs = micros();
  delayMicroseconds(PULSES_PERIOD - (currentPulseTimeUs - lastPulseTimeUs));
  lastPulseTimeUs = currentPulseTimeUs;
}
