#include <BMI160.h>
#include <CurieIMU.h>

#include <BLEAttribute.h>
#include <BLECentral.h>
#include <BLECharacteristic.h>
#include <BLECommon.h>
#include <BLEDescriptor.h>
#include <BLEPeripheral.h>
#include <BLEService.h>
#include <BLETypedCharacteristic.h>
#include <BLETypedCharacteristics.h>
#include <CurieBLE.h>


/////////////////////////////////////////////////////////////////////////////
//////////////////////////// MIDI STUFF /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//Buffer to hold 5 bytes of MIDI data. Note the timestamp is forced
uint8_t midiData[] = {0x80, 0x80, 0x00, 0x00, 0x00};

//Loads up buffer with values for note On
void noteOn(char chan, char note, char vel) //channel 1
{
  midiData[2] = 0x90 + chan;
  midiData[3] = note;
  midiData[4] = vel;
}

//Loads up buffer with values for note Off
void noteOff(char chan, char note) //channel 1
{
  midiData[2] = 0x80 + chan;
  midiData[3] = note;
  midiData[4] = 0;
}

unsigned count = 0;

/////////////////////////////////////////////////////////////////////////////
//////////////////////////// SAMPLE STUFF ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct Sample
{
  bool playing; // is samples playing
  unsigned pin; // pin number for sensor
  unsigned midiNote; // midi note
};

const unsigned NUM_SAMPLES = 6;
const unsigned THRESHOLD = 450;

Sample samples[NUM_SAMPLES] = 
{
  {false, A0, 39},
  {false, A1, 36},
  {false, A2, 38},
  {false, A3, 46},
  {false, A4, 37},
  {false, A5, 42},
  //{false, 8, 56},
  //{false, 9, 43}
};

/////////////////////////////////////////////////////////////////////////////
//////////////////////////// BLUETOOTH //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
BLEPeripheral midiDevice; // create peripheral instance

BLEService midiSvc("03B80E5A-EDE8-4B33-A751-6CE34EC4C700"); // create service 
//Service about is specifically MIDI over BLE Primary
// create switch characteristic and allow remote device to read and write
BLECharacteristic midiChar("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLEWrite | BLEWriteWithoutResponse | BLENotify | BLERead, 5);


int fsrAnalogPin = 0; // FSR is connected to analog 0
int LEDpin = 11; // connect Red LED to pin 11 (PWM pin)
int fsrReading; // the analog reading from the FSR resistor divider
int LEDbrightness;


void setup(void) 
{
  Serial.begin(9600); // We'll send debugging information via the Serial monitor
  pinMode(LEDpin, OUTPUT);

  BLESetup();
}



void loop(void)
{
  for (unsigned i = 0; i < NUM_SAMPLES; ++i)
  {
    int val = analogRead(samples[i].pin);
    bool newIsPlaying = val > THRESHOLD;
    /*if (count % 100 == 0)
    {
      Serial.print("pin ");
      Serial.print(samples[i].pin);
      Serial.print(" midi ");
      Serial.print(samples[i].midiNote);
      Serial.print(" val ");
      Serial.println(val);
    }*/

    // if note playing status has changed send to BLE
    if (newIsPlaying != samples[i].playing)
    {
      samples[i].playing = !samples[i].playing;
      if (samples[i].playing)
      {
        Serial.print("sending note on ");
        Serial.print(samples[i].midiNote);
        Serial.print(" pin ");
        Serial.print(samples[i].pin);
        Serial.print(" val ");
        Serial.println(val);
        noteOn(0, samples[i].midiNote, 127);
      }
      else
      {
        Serial.print("sending note off ");
        Serial.print(samples[i].midiNote);
        Serial.print(" pin ");
        Serial.print(samples[i].pin);
        Serial.print(" val ");
        Serial.println(val);
        noteOff(0, samples[i].midiNote);
      }
      midiChar.setValue(midiData, 5); //send over BLE
    }
  }
  ++count;
  /*
  fsrReading = analogRead(fsrAnalogPin);
  Serial.print("Analog reading = ");
  Serial.println(fsrReading);
  
  // we'll need to change the range from the analog reading (0-1023) down to the range
  // used by analogWrite (0-255) with map!
  LEDbrightness = map(fsrReading, 0, 1023, 0, 255);
  // LED gets brighter the harder you press
  analogWrite(LEDpin, LEDbrightness);
  
  delay(100);*/
}






void BLESetup()
{
  // set the local name peripheral advertises
  midiDevice.setLocalName("Ard101");
  midiDevice.setDeviceName("Ard101");
  //I'll add a clearcache next time.

  // set the UUID for the service this peripheral advertises
  midiDevice.setAdvertisedServiceUuid(midiSvc.uuid());

  // add service and characteristic
  midiDevice.addAttribute(midiSvc);
  midiDevice.addAttribute(midiChar);

  // assign event handlers for connected, disconnected to peripheral
  midiDevice.setEventHandler(BLEConnected, midiDeviceConnectHandler);
  midiDevice.setEventHandler(BLEDisconnected, midiDeviceDisconnectHandler);

  // assign event handlers for characteristic
  midiChar.setEventHandler(BLEWritten, midiCharacteristicWritten);
  // set an initial value for the characteristic
  midiChar.setValue(midiData, 5);

  // advertise the service
  midiDevice.begin();
}

void midiDeviceConnectHandler(BLECentral & central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void midiDeviceDisconnectHandler(BLECentral & central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void midiCharacteristicWritten(BLECentral & central, BLECharacteristic & characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("Characteristic event, written: ");
}
