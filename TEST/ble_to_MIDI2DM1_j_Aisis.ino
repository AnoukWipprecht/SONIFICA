/*
 * ble to cc
 * 
 * convert curie acclerometer data to midi cc messages via bluetooth le
 * updated with BLE indicateor on LEDpin 13
 * updated with easy calibration code block
 * corrected typo. Controller 16,17 and 18 works.
 * 
 * J. Adams <jna@retina.net>
 * 11/2016
 * 
 */
#include <CurieBLE.h>
#include <CurieIMU.h>

#define MAX_ACCEL_RANGE           64
#define TXRX_BUF_LEN              20 //max number of bytes
#define RX_BUF_LEN                20 //max number of bytes

#define MIDI_CC_X                 36 // the midi cc # number for x motion
#define MIDI_CC_Y                 37 // the midi cc # number for y motion
#define MIDI_CC_Z                 38 // the midi cc # number for z motion

#define STATUS_LED_PIN            13 // I think this is the status led on the curie.

#define MIDI_CHANNEL              1  // channel to transmit on 

uint8_t rx_buf[RX_BUF_LEN];
int rx_buf_num, rx_state = 0;
uint8_t rx_temp_buf[20];
uint8_t outBufMidi[128];

//Buffer to hold 5 bytes of MIDI data. Note the timestamp is forced
uint8_t midiData[] = {0x80, 0x80, 0x00, 0x00, 0x00};

boolean BEConnected = FALSE;

//Loads up buffer with values for note On
void noteOn(uint8_t chan, uint8_t note, uint8_t vel) //channel 1
{
  midiData[2] = 0x90 + chan;
  midiData[3] = note;
  midiData[4] = vel;
}

//Loads up buffer with values for note Off
void noteOff(uint8_t chan, uint8_t note) 
{
  midiData[2] = 0x80 + chan;
  midiData[3] = note;
  midiData[4] = 0;
}

void midiCC(uint8_t chan, uint8_t ccnum, uint8_t val) 
{
  midiData[2] = 0xb0 + chan;
  midiData[3] = ccnum;
  midiData[4] = val; // restricted to 0-127, 128-255 reserved in midi spec, do not use
} 

uint8_t scaleAccelValue(float f) {
  // "64" is our dead-center value, what would normally be 0 on the device.
  // if we're less than 0, return inverse of 64 to zero. (so -2 == 0)
  // if > 0, return (64 + f)

  // we are essentially remapping -2 <= x <= 2  to 0 <= x <= 127 

  if (f <= 0) { 
    return (uint8_t) (64 - ((abs(f) / 4) * 64));
  } else {
    return (uint8_t) (64 + ((abs(f) / 4) * 64));
  }
}

BLEPeripheral midiDevice; // create peripheral instance

BLEService midiSvc("03B80E5A-EDE8-4B33-A751-6CE34EC4C700"); // create service

// create switch characteristic and allow remote device to read and write
BLECharacteristic midiChar("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLEWrite | BLEWriteWithoutResponse | BLENotify | BLERead, 5);

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing IMU device...");
  CurieIMU.begin();

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);


  // Set the accelerometer range to 2 
  CurieIMU.setAccelerometerRange(MAX_ACCEL_RANGE);
  
  BLESetup();
  Serial.println(("Bluetooth device active, waiting for connections..."));

  // uncomment the next line to do midi mapping. 
  calibrate();
}

void BLESetup()
{
  // set the local name peripheral advertises
  midiDevice.setLocalName("SONIFICA");
  midiDevice.setDeviceName("SONIFICA");

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

void calibrate() { 
  // send each of the controllers with a delay so that you can do midi-mapping.
  // easier than recompiling.


  while (BEConnected == FALSE) { 
    Serial.println("Waiting on Bluetooth connection to start setup...");
    delay(1000);
  }

  Serial.println("Sending controller MIDI_CC_X in 10 seconds...");
  delay(10000);
  noteOn(MIDI_CHANNEL, MIDI_CC_X, 10);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  
  
  Serial.println("Sending controller MIDI_CC_Y in 5 seconds...");
  delay(5000);
  noteOn(MIDI_CHANNEL, MIDI_CC_Y, 10);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  

  Serial.println("Sending controller MIDI_CC_Z in 5 seconds...");
  delay(5000);
  noteOn(MIDI_CHANNEL, MIDI_CC_Z, 10);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes

  Serial.println("Starting to send data in 10 seconds. close midi map now!");
  delay(10000);
  
}


void loop() {

  /*Simple randome note player to test MIDI output
     Plays random note every 400ms
  */
  float ax, ay, az;   //scaled accelerometer values
  
  // read accelerometer measurements from device, scaled to the configured range
  CurieIMU.readAccelerometerScaled(ax, ay, az);

  // display tab-separated accelerometer x/y/z values
  Serial.print("a:\t");
  Serial.print(ax);
  Serial.print("\t");
  Serial.print(ay);
  Serial.print("\t");
  Serial.print(az);
  Serial.println("\n");

  Serial.print("scaled:\t");
  Serial.print(scaleAccelValue(ax));
  Serial.print("\t");
  Serial.print(scaleAccelValue(ay));
  Serial.print("\t");
  Serial.print(scaleAccelValue(az));
  Serial.println();

  // transmit the CC values over midi
  // scale floats down to midi 0-127
  noteOn(MIDI_CHANNEL, MIDI_CC_X, scaleAccelValue(ax));
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  noteOff(0,note);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  
  noteOn(MIDI_CHANNEL, MIDI_CC_Y, scaleAccelValue(ay));
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  noteOff(0,note);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  
  noteOn(MIDI_CHANNEL, MIDI_CC_Z, scaleAccelValue(az));
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  noteOff(0,note);
  midiChar.setValue(midiData, 5);//midiData); //posts 5 bytes
  
  delay(250);
}


void midiDeviceConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  BEConnected = TRUE;
  digitalWrite(STATUS_LED_PIN, HIGH);
}

void midiDeviceDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  BEConnected = FALSE;
  digitalWrite(STATUS_LED_PIN, LOW);

}

void midiCharacteristicWritten(BLECentral& central, BLECharacteristic& characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("Characteristic event, written: ");
}
