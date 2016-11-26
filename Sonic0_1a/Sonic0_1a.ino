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
#include <BLEUuid.h>
#include <CurieBLE.h>

/* Sonic- version 1

   MIDI over BLE info from: https://developer.apple.com/bluetooth/Apple-Bluetooth-Low-Energy-MIDI-Specification.pdf


  ===============================================
*/



//Set tempo, key, and scale
#define BPM        500
#define MainKey    G
#define FlamencoScale G2

//Setting up buffers for MIDI stuff
#define TXRX_BUF_LEN              20 //max number of bytes
#define RX_BUF_LEN                20 //max number of bytes
uint8_t rx_buf[RX_BUF_LEN];
int rx_buf_num, rx_state = 0;
uint8_t rx_temp_buf[20];
uint8_t outBufMidi[128];

//Defining Progression versions
#define FlamencoA 0
#define FlamencoB 1
#define FlamencoC 2
#define FlamencoD 3
#define FlamencoE 4
#define FlamencoF 5
//Defining notes
#define G2 0
#define Gs2 1
#define As2 2
#define B2 3
#define C3 4
#define D3 5
#define Ds3 6
#define G 7
#define F3 8
#define A 9

//Defining the progressions
uint8_t FlamencoAScale[] = {0, 1, 3, 5, 6, 8, 9};
uint8_t FlamencoBScale[] = {0, 1, 3, 5, 6, 7, 9};
uint8_t FlamencoCScale[] = {0, 3, 6, 7, 8, 8, 9};
uint8_t FlamencoDScale[] = {0, 1, 2, 3, 4, 5, 9};
uint8_t FlamencoEScale[] = {0, 2, 3, 4, 5, 6, 7};
uint8_t FlamencoFScale[] = {0, 2, 3, 5, 7, 8, 9};
const float alpha = 0.25;

//Variables for IMU stuff
int rollNote = 0;
int pitchNote = 0;
int newNote = 0;
int accelFlag = 0;
int calibrateOffsets = 1; // int to determine whether calibration takes place or not
double fXg = 0;
double fYg = 0;
double fZg = 0;
double roll, pitch, yaw; // Roll and pitch are calculated using the accelerometer while yaw is calculated using the magnetometer
int ax, ay, az;         // accelerometer values
int gx, gy, gz;         // gyrometer values

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

BLEPeripheral midiDevice; // create peripheral instance

BLEService midiSvc("03B80E5A-EDE8-4B33-A751-6CE34EC4C700"); // create service 
//Service about is specifically MIDI over BLE Primary
// create switch characteristic and allow remote device to read and write
BLECharacteristic midiChar("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLEWrite | BLEWriteWithoutResponse | BLENotify | BLERead, 5);

void setup() {
  Serial.begin(9600);
  IMUSetup();
  BLESetup();
  Serial.println(("Bluetooth device active, waiting for connections..."));
}

static void eventCallback(void)
{
  accelFlag = 1;
}

void IMUSetup()
{
  // initialize device
  Serial.println("Initializing IMU device...");
  CurieIMU.begin();

  // verify connection
  Serial.println("Testing device connections...");
  if (CurieIMU.begin()) {
    Serial.println("CurieIMU connection successful");
  } else {
    Serial.println("CurieIMU connection failed");
  }

  // use the code below to calibrate accel/gyro offset values
  if (calibrateOffsets == 1) {
    Serial.println("Internal sensor offsets BEFORE calibration...");
    Serial.print(CurieIMU.getAccelerometerOffset(X_AXIS));
    Serial.print("\t"); // -76
    Serial.print(CurieIMU.getAccelerometerOffset(Y_AXIS));
    Serial.print("\t"); // -235
    Serial.print(CurieIMU.getAccelerometerOffset(Z_AXIS));
    Serial.print("\t"); // 168
    Serial.print(CurieIMU.getGyroOffset(X_AXIS));
    Serial.print("\t"); // 0
    Serial.print(CurieIMU.getGyroOffset(Y_AXIS));
    Serial.print("\t"); // 0
    Serial.println(CurieIMU.getGyroOffset(Z_AXIS));

    Serial.println("About to calibrate. Make sure your board is stable and upright");
    delay(5000);

    // The board must be resting in a horizontal position for
    // the following calibration procedure to work correctly!
    Serial.print("Starting Gyroscope calibration and enabling offset compensation...");
    CurieIMU.autoCalibrateGyroOffset();
    Serial.println(" Done");

    Serial.print("Starting Acceleration calibration and enabling offset compensation...");
    CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
    CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
    CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
    Serial.println(" Done");

    Serial.println("Internal sensor offsets AFTER calibration...");
    Serial.print(CurieIMU.getAccelerometerOffset(X_AXIS));
    Serial.print("\t"); // -76
    Serial.print(CurieIMU.getAccelerometerOffset(Y_AXIS));
    Serial.print("\t"); // -2359
    Serial.print(CurieIMU.getAccelerometerOffset(Z_AXIS));
    Serial.print("\t"); // 1688
    Serial.print(CurieIMU.getGyroOffset(X_AXIS));
    Serial.print("\t"); // 0
    Serial.print(CurieIMU.getGyroOffset(Y_AXIS));
    Serial.print("\t"); // 0
    Serial.println(CurieIMU.getGyroOffset(Z_AXIS));
  }
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


/* This quantizer takes in the unquantized not number,
    the scale, and the key. It then figures out what note
    to 'snap' to based off the scale definitions. This is probably
    not the most efficient way to do this, but I needed something
    quick and it works.
*/
uint8_t quantize(int note, char scale, char key)
{
  int modNote = 0;
  uint8_t tempScale[8];
  modNote = note % 12;
  if (scale == FlamencoA)
  {
    for (int i = 0; i < sizeof(FlamencoAScale); i++)
    {
      if (modNote == FlamencoAScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoAScale[i] + key)
      {
        return note + 1;
      }
    }
  }
  else if (scale == FlamencoB)
  {
    for (int i = 0; i < sizeof(FlamencoBScale); i++)
    {
      if (modNote == FlamencoBScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoBScale[i] + key)
      {
        return note + 1;
      }
    }
  }
  else if (scale == FlamencoC)
  {
    for (int i = 0; i < sizeof(FlamencoCScale); i++)
    {
      if (modNote == FlamencoCScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoCScale[i] + key)
      {
        return note + 1;
      }
    }
  }
  else if (scale == FlamencoD)
  {
    for (int i = 0; i < sizeof(FlamencoDScale); i++)
    {
      if (modNote == FlamencoDScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoDScale[i] + key)
      {
        return note + 1;
      }
    }
  }
  else if (scale == FlamencoE)
  {
    for (int i = 0; i < sizeof(FlamencoEScale); i++)
    {
      if (modNote == FlamencoEScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoEScale[i] + key)
      {
        return note + 1;
      }
    }
  }
  else if (scale == FlamencoF)
  {
    for (int i = 0; i < sizeof(FlamencoFScale); i++)
    {
      if (modNote == FlamencoFScale[i] + key)
      {
        return note;
      }
      else if (modNote + 1 == FlamencoFScale[i] + key)
      {
        return note + 1;
      }
    }
  }
}

void loop() {
  // read raw accel/gyro measurements from device
  CurieIMU.readMotionSensor(ax, ay, az, gx, gy, gz);
  roll = atan2(ay, az) * RAD_TO_DEG; //calculate the roll
  pitch = atan(-ax / sqrt(ay * ay + az * az)) * RAD_TO_DEG; //calculate the pitch

  rollNote = roll / 4 + 80; //convert the raw roll to a MIDI note value
  pitchNote = pitch / 4 + 80; //convert the raw pitch to a MIDI note value

  if (pitch > 55) //filter out notes that are too low 
  {
    newNote = quantize(rollNote, FlamencoScale, MainKey); //Quantize the note
    Serial.println(newNote); //print it out so we can see what it plays on the term
    noteOn(0, newNote, 127); //turn the note on
    midiChar.setValue(midiData, 5); //send over BLE

    delay(8);
    noteOff(0, newNote); //turn the note off
    midiChar.setValue(midiData, 5); //send over BLE
  }
  delay(BPM);
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
