//playing music sketch on teensy3.2 
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>
//check if there is a missing library for bounce check 
//https://github.com/PaulStoffregen/Audio
//https://www.pjrc.com/teensy/td_libs_Audio.html

AudioPlaySdWav          playSDWav1;
AudioOutputI2S          i2s1;
AudioConnection         patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection         patchCord2(playSdWav1, 1, i2s1, 1);
AudioControlSGTL5000    sgtl5000_1;

Bounce button0 = Bounce(0, 15);
Bounce butt78on2 = Bounce(2, 15); // 15 = 15 ms debounce time 

void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgt15000_1.enable();
  sgt15000_1.volume(0.45);
  SPI.setMOSI(7);
  SPI.setSCK(14);[
  
  if (!(SD.begin(10)))  {
    while (1) {
      Serial.println("Can't access microSD card");
      delay(250); //default 500, 1000 is 1 second
    }
  }
  
int filenumber = 0; //while file to play
//rename your audio files
//BREATH EDITS (15 sec).wav        ->LOOP1.WAV
//ELLIOTT_DISDAIN_VM_SAMPLE002.wav ->LOOP2.WAV
//ELLIOTT_DISDAIN_VM_SAMPLE009.wav ->LOOP3.WAV
//ELLIOTT_DISDAIN_VM_SAMPLE011.wav ->LOOP4.WAV
//ELLIOTT_DISDAIN_VM_SAMPLE012.wav ->LOOP5.WAV
//VM_LOOP_PROTOTYPE_BLADELEG.wav   ->LOOP6.WAV
const char * filelist[6] = {
  "LOOP1.WAV", "LOOP2.WAV", "LOOP3.WAV", "LOOP4.WAV", "LOOP5.WAV", "LOOP6.WAV"
};

elapsedMillis blinkTime;

void loop() {
  
  if (playSdWav1.isPlaying() == false {
    const char *filename = filelist[filenumber];
    filenumber = filenumber + 1;
    if (filenumber >= 4) filenumber = 0;
    Serial.print("Start playing ");
    Serial.println(filename);
    playSdWav1.play(filename);
    delay(10); // wait for library to go through WAV info
    }
//blink the LED with no delay
if (blinkTime < 250) {
  digitalWrite(13, LOW);
  } else if (blinkTime < 500) {
    digitalWrite(13, HIGH);
  } else  {
    blinkTime = 0; // start blink cycle over again
}

//read button
//if buttons are added
button0.update();
  if (button0.fallingEdge()) {
  playSdWav1.stop();
  }
  button2.update();
  if (button2.fallingEdge()) {
    playSdWav1.stop();
    filenumber = filebnumber - 2;
    if (filenumber < 0) filebumber = filenumber + 6;



}



  
//  pinMode(13, OUTPUT); //LED ON PIN 13
//  delay(500); //default 1000

  //print out the offset playtime
  Serial.print("Timeline at");
  Serial.print(playSdWav1.positionMillis());
  Serial.println(" ms");

//  //blink LED and print info during play
//  digitalWrite(13, HIGH);
//  delay(250);
//  digitalWrite(13, LOW);
//  delay(250);



  }

  //read the knob position (analog input A2)
  //Nouk, connect your softpot to get a better feel
//  int knob = analogRead(A2);
//  float vol = (float)knob / 1280.0;
//  sgtl5000_1.volume(vol);
//  Serial.print("volume = ");
//  Serial.println(vol);

}
  

}
