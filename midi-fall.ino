#include <Adafruit_NeoPixel.h> //led
#include <avr/power.h> //led
#include <Bounce.h> //midi

#define LEDMATRIXPIN     11 //pin number for matrix leds
#define LEDBUTTONPIN     6 // pin number for button leds
#define ROWS             16 //number of neopixels in one sequence column
#define COLS             8 //number of columns


byte drumkitNotes[8] = {
  36, //C1
  37, //C#1
  38, //D1
  39, //D#1
  40, //E1
  41, //F1
  42, //F#1
  43, //G1
};

//byte (*buttonNotes)[8];
byte *buttonNotes;

Adafruit_NeoPixel buttonPixels = Adafruit_NeoPixel(COLS, LEDBUTTONPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrixPixels = Adafruit_NeoPixel(ROWS*COLS, LEDMATRIXPIN, NEO_GRB + NEO_KHZ800);


// the MIDI channel number to send messages
const int channel = 1;

// Stuff for the midi clock
byte counter; 
byte CLOCK = 248; 
byte START = 250; 
byte CONTINUE = 251; 
byte STOP = 252;

//an array of bytes to store each notes last velocity
//byte momentaryVelocities[COLS];

//arrays to store each columns all steps
byte velocitySequences[ROWS][COLS];

//Matrix containing colors for all columns
float colors[3][COLS]={
  {0.5,0.1,0.9,0.0,0.3,0.0,0.0,0.0},
  {0.0,0.5,0.8,0.5,0.0,0.5,0.2,0.9},
  {0.7,0.6,0.3,0.6,0.2,0.0,0.0,0.3}
};

void setup() {
  
  matrixPixels.begin(); // Initialize the NeoPixels
  buttonPixels.begin();

  Serial.begin(115200);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleVelocityChange(OnVelocityChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleProgramChange(OnProgramChange);
  usbMIDI.setHandleAfterTouch(OnAfterTouch);
  usbMIDI.setHandlePitchChange(OnPitchChange);
  
  usbMIDI.setHandleRealTimeSystem(RealTimeSystem); 

  buttonNotes = drumkitNotes; //Make current button notes that scale
}

void loop() {
  usbMIDI.read(); // USB MIDI receive
 
}



void OnNoteOn(byte channel, byte note, byte velocity) {
  
  if(note==buttonNotes[0]){ //if the incoming midi note matches the set scales 1st value
    velocitySequences[0][0] = velocity * 2; //save that velocity in the momentaryVelocity array (*2 to match LEDs scale)
  }
  if(note==buttonNotes[1]){
    velocitySequences[0][1] = velocity * 2;
  }
  if(note==buttonNotes[2]){
    velocitySequences[0][2] = velocity * 2;
  }  
  if(note==buttonNotes[3]){
    velocitySequences[0][3] = velocity * 2;
  }
  if(note==buttonNotes[4]){
    velocitySequences[0][4] = velocity * 2;
  }
  if(note==buttonNotes[5]){
    velocitySequences[0][5] = velocity * 2;
  }
  if(note==buttonNotes[6]){
    velocitySequences[0][6] = velocity * 2;
  }
  if(note==buttonNotes[7]){
    velocitySequences[0][7] = velocity * 2;
  }  
}

void OnNoteOff(byte channel, byte note, byte velocity) {

  // if(note==drumkitNotes[0]){ //if the incoming midi note matches the set scales 1st value
  //   momentaryVelocities[0] = 0;
  // }
  // if(note==drumkitNotes[1]){
  //   momentaryVelocities[1] = 0;
  // }
  // if(note==drumkitNotes[2]){
  //   momentaryVelocities[2] = 0;
  // }  
  // if(note==drumkitNotes[3]){
  //   momentaryVelocities[3] = 0;
  // }
  // if(note==drumkitNotes[4]){
  //   momentaryVelocities[4] = 0;
  // }
  // if(note==drumkitNotes[5]){
  //   momentaryVelocities[5] = 0;
  // }
  // if(note==drumkitNotes[6]){
  //   momentaryVelocities[6] = 0;
  // }
  // if(note==drumkitNotes[7]){
  //   momentaryVelocities[7] = 0;
  // }  

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
      for (int j=0; j < COLS; j++){
        for (int i=0; i < ROWS; i++){
          if(j==0||j%2==0){ //if it is column 0,2,4,6 do it normal
            matrixPixels.setPixelColor(i+ROWS*j, matrixPixels.Color(velocitySequences[i][j]*colors[0][j],velocitySequences[i][j]*colors[1][j],velocitySequences[i][j]*colors[2][j])); //adress pixels directly from model and get color multiplyer from matrix
          }
          else{ //otherwise adress pixels in reverse due to hardware setup
            matrixPixels.setPixelColor(ROWS-1-i+ROWS*j, matrixPixels.Color(velocitySequences[i][j]*colors[0][j],velocitySequences[i][j]*colors[1][j],velocitySequences[i][j]*colors[2][j])); //adressing pixels inverted as in model and get color multiplyer from matrix
          }
        }
        for (int i=ROWS-1; i>0 ; i--){          
          velocitySequences[i][j]=velocitySequences[i-1][j]; //shift the values down in the models columns, starting at the bottom to not overwrite important data
        }      
        velocitySequences[0][j]=0; //Set first position to 0 again  
      }
      matrixPixels.show();
    } 
    
  } 
  
  if(realtimebyte == START || realtimebyte == CONTINUE) { 
    counter = 0; 
 
  } 
  
  if(realtimebyte == STOP) { 
    
  } 

} 


