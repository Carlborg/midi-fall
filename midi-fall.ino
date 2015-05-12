#include <Adafruit_NeoPixel.h> //led
#include <avr/power.h> //led
#include <Bounce.h> //midi

#define PIN              11 //pin number for step sequencers
#define NUMPIXELS        128 //number of neopixels in stepsequencer
#define COLUMNLENGTH     16 //number of neopixels in one sequence column

#define INNOTE1          36
#define INNOTE2          37
#define INNOTE3          38
#define INNOTE4          39
#define INNOTE5          40
#define INNOTE6          41
#define INNOTE7          42
#define INNOTE8          43

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


// the MIDI channel number to send messages
const int channel = 1;

// Stuff for the midi clock
byte counter; 
byte CLOCK = 248; 
byte START = 250; 
byte CONTINUE = 251; 
byte STOP = 252;

//an array of bytes to store each notes last velocity
byte momentaryVelocities[8];

//arrays to store each columns all steps
byte velocitySequence1[COLUMNLENGTH];
byte velocitySequence2[COLUMNLENGTH];

void setup() {
  
  pixels.begin(); // This initializes the NeoPixel library.
  
    Serial.begin(115200);
    usbMIDI.setHandleNoteOff(OnNoteOff);
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleVelocityChange(OnVelocityChange);
    usbMIDI.setHandleControlChange(OnControlChange);
    usbMIDI.setHandleProgramChange(OnProgramChange);
    usbMIDI.setHandleAfterTouch(OnAfterTouch);
    usbMIDI.setHandlePitchChange(OnPitchChange);
    
    usbMIDI.setHandleRealTimeSystem(RealTimeSystem); 
}

void loop() {
  usbMIDI.read(); // USB MIDI receive
 
}



void OnNoteOn(byte channel, byte note, byte velocity) {
  
  if(note==INNOTE1){
    momentaryVelocities[0] = velocity * 2;
  }
  
  if(note==INNOTE2){
    momentaryVelocities[1] = velocity * 2;
  }
}

void OnNoteOff(byte channel, byte note, byte velocity) {
  if(note==INNOTE1){
    momentaryVelocities[0] = 0;
  }
  
  if(note==INNOTE2){
    momentaryVelocities[1] = 0;
  }

}

void OnVelocityChange(byte channel, byte note, byte velocity) {

}

void OnControlChange(byte channel, byte control, byte value) {

}

void OnProgramChange(byte channel, byte program) {
 
}

void OnAfterTouch(byte channel, byte pressure) {

}

void OnPitchChange(byte channel, int pitch) {

}


void RealTimeSystem(byte realtimebyte) { 
  if(realtimebyte == CLOCK) {
    counter++; 
    if(counter == 3) { //set to 3 means 32th note steps, set to 6 means 16th note steps
      counter = 0;
      for (int i=0; i < COLUMNLENGTH; i++){
        pixels.setPixelColor(i, pixels.Color(velocitySequence1[i],0,0));
      }
      for (int i=COLUMNLENGTH-1; i>0 ; i--){          
        velocitySequence1[i]=velocitySequence1[i-1];
      }
      velocitySequence1[0]=momentaryVelocities[0];
      
      for (int i=0; i < COLUMNLENGTH; i++){
        pixels.setPixelColor(i+32, pixels.Color(velocitySequence2[i],velocitySequence2[i],0));
      }
      for (int i=COLUMNLENGTH-1; i>0 ; i--){
        velocitySequence2[i]=velocitySequence2[i-1];
      }
      velocitySequence2[0]=momentaryVelocities[1];
      
      pixels.show();
    } 
    
  } 
  
  if(realtimebyte == START || realtimebyte == CONTINUE) { 
    counter = 0; 
 
  } 
  
  if(realtimebyte == STOP) { 
    
  } 

} 




