#include <Adafruit_NeoPixel.h> //led
#include <avr/power.h> //led
#include <Bounce.h> //debounce
#include <math.h>

//to do
//  sequenced notes miss the bright attack note sometimes
// change midi channel
// custom colors / drum mode



//Offline Memory
#define EEsize           4096 //number of bytes // use EEPROM.read(address); & EEPROM.write(i, zz); to read and write to it

//MODEL related stuff-------------------------------------------------------
#define ROWS                   16 //number of rows in the model matrix, same as the view
#define COLS                   8 //number of columnsin the model matrix, same as the view 
#define MATRIXAMBIENTVALUE     5//5 //velocity level of inactive cells in the matrix // currently in model but would make more sense to have in view
#define BUTTONAMBIENTVALUE     15//14            

byte currentTriggerVelocities[COLS]={0,0,0,0,0,0,0,0}; //this keeps track of the current trigger state (both buttons and sequenced midi)
byte matrixValues[ROWS][COLS]; //The model of the matrix containing all values

// SCALES----------------
//if you want to add more scales please update all these three constants
const int numberOfScales = 26;
const byte scales[][numberOfScales] = { //define scales on the form 'semitones added to tonic'
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, // 0 'Chromatic'
  {0, 2, 4, 5, 7, 9, 11}, // 1 ‘Major’
  {0, 2, 3, 5, 7, 8, 10}, // 2 ‘Minor’
  {0, 2, 3, 5, 7, 9, 10}, // 3 ‘Dorian’
  {0, 2, 4, 5, 7, 9, 10}, // 4 ‘Mixolydian’
  {0, 2, 4, 6, 7, 9, 11}, // 5 ‘Lydian’
  {0, 1, 3, 5, 7, 8, 10}, // 6 ‘Phrygian’
  {0, 1, 3, 4, 7, 8, 10}, // 7 ‘Locrian’
  {0, 1, 3, 4, 6, 7, 9, 10}, // 8 ‘Diminished’
  {0, 2, 3, 5, 6, 8, 9, 11}, // 9 ‘Whole-half’
  {0, 2, 4, 6, 8, 10}, // 10 ‘Whole Tone’
  {0, 3, 5, 6, 7, 10}, // 11 ‘Minor Blues’
  {0, 3, 5, 7, 10}, // 12 ‘Minor Pentatonic’
  {0, 2, 4, 7, 9}, // 13 ‘Major Pentatonic’
  {0, 2, 3, 5, 7, 8, 11}, // 14 ‘Harmonic Minor’
  {0, 2, 3, 5, 7, 9, 11}, // 15 ‘Melodic Minor’
  {0, 1, 3, 4, 6, 8, 10}, // 16 ‘Super Locrian’
  {0, 1, 4, 5, 7, 8, 11}, // 17 ‘Bhairav’
  {0, 2, 3, 6, 7, 8, 11}, // 18 ‘Hungarian Minor’
  {0, 1, 4, 5, 7, 8, 10}, // 19 ‘Minor Gypsy’
  {0, 2, 3, 7, 8}, // 20 ‘Hirojoshi’
  {0, 1, 5, 7, 10}, // 21 ‘In-Sen’
  {0, 1, 5, 6, 10}, // 22 ‘Iwato’
  {0, 2, 3, 7, 9}, // 23 ‘Kumoi’
  {0, 1, 3, 4, 7, 8}, // 24 ‘Pelog’
  {0, 1, 3, 4, 5, 6, 8, 10} // 25 ‘Spanish’
};
const byte scaleSizes[numberOfScales] = {12, 7, 7, 7, 7, 7, 7, 7, 8, 8, 6, 6, 5, 5, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 6, 8}; //havn't found a way to do this prettier yet

//Scale variables-----------
byte tonic = 36; //store cc value of what midi note is currently the tonic
byte scale = 12; //store cc value of what scale to use

bool mirror = false; // mirror the way the model is represented on the controller // since you can use it from two ways
int scaleTranspose = 2; // move the layout of the buttons


//VIEW related stuff--------------------------------------------------------
#define LEDMATRIXPIN     12 //pin number for matrix leds
#define LEDBUTTONPIN     11 // pin number for button leds

#define UPPERNOTELIMIT   127 //if trying to access anything beyond, columns are deactivated instead of generating wierd stuff
#define LOWERNOTELIMIT   0

Adafruit_NeoPixel buttonPixels = Adafruit_NeoPixel(COLS, LEDBUTTONPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel matrixPixels = Adafruit_NeoPixel(ROWS*COLS, LEDMATRIXPIN, NEO_GRB + NEO_KHZ800);

//Arrays containing colors for all columns // if user wants to make a custom look
//not giving users access to the brightness, since it is used for showing velocities
byte hues[COLS] = {100,100,100,100,100,100,100,100};
byte saturations[COLS] = {120,120,120,120,120,120,120,120};




//CONTROL related stuff-----------------------------------------------------
const int buttonPins[8] = {21, 20, 19, 18, 17, 16, 15, 14}; //array with pin numers for the buttons, guess i soldered them in reverse
const int bounceTime = 50; //debounce time in milliseconds
//Button Stuff--------------------
Bounce bouncers[8] = {
  Bounce( buttonPins[0], bounceTime),
  Bounce( buttonPins[1], bounceTime),
  Bounce( buttonPins[2], bounceTime),
  Bounce( buttonPins[3], bounceTime),
  Bounce( buttonPins[4], bounceTime),
  Bounce( buttonPins[5], bounceTime),
  Bounce( buttonPins[6], bounceTime),
  Bounce( buttonPins[7], bounceTime),
};

//MIDI
const int setupChannel = 13; // the MIDI channel on which system settings are sent, such as what the operating channel should be
int operatingChannel = 1; // the MIDI channel on which it currently sends and recieves notes

const int tonicCC = 22; // according to standard cc table cc 22-31 are undefined to aviod interfering with other stuff
const int scaleCC = 23;
const int mirrorCC = 24;
const int transposeCC=25;


// Stuff for the midi clock--------
bool clockIsTicking = false;
byte counter;
byte CLOCK = 248;
byte START = 250;
byte CONTINUE = 251;
byte STOP = 252;

//flags------------
bool buttonChange = false;



//----------------------------------SETUP-----------------------------------------
void setup() {
  //Button setup
  for (int j = 0; j < COLS; j++) {
    pinMode(buttonPins[j], INPUT); //make inputs
    digitalWrite(buttonPins[j], HIGH); //activate pull up resistors
  }

  Serial.begin(115200);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleVelocityChange(OnVelocityChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleProgramChange(OnProgramChange);
  usbMIDI.setHandleAfterTouch(OnAfterTouch);
  usbMIDI.setHandlePitchChange(OnPitchChange);

  usbMIDI.setHandleRealTimeSystem(RealTimeSystem);  //midi clock stuff

  matrixPixels.begin(); // Initialize the matrix NeoPixels
  buttonPixels.begin(); //initialize the button neopixels

  SetColorsFromScale();
  for (int col=0;col<COLS;col++){
    bool inBounds = IsInBounds(GetAbsoluteScaleNote(Mirrored(col)));
    if (inBounds){
      SetButtonBackgroundColor(col); //update the buttons and take tonics into account
    }
    else{
      SetButtonLED(col,0,0,0); // value 0 if out of bounds
    }
    for (int row=0;row<ROWS;row++){
      matrixValues[row][col]=MATRIXAMBIENTVALUE; //fill the matrix with ambient values
      SetMatrixLED(row,Mirrored(col),GetHue(col),GetSaturation(col),GetMatrixValue(row,col)*inBounds); //Set the pixels
    }
  
  }
  matrixPixels.show(); //since this only updates when the midi clock is running otherwise
  buttonPixels.show(); // since this only runs if a trigger state has changed
  
}

//---------------------------------LOOP--------------------------------------------------
void loop() {
  usbMIDI.read(); // USB MIDI receive
  ButtonController();
}




//Control related functions ---------------------------------------------------------------------

void OnNoteOn(byte channel, byte note, byte velocity) {
  if (channel == operatingChannel) {
    for (int col = 0; col < COLS; col++) {
      if (note == GetAbsoluteScaleNote(Mirrored(col))) { //if the incoming midi note matches the set scales corresponding value
        if (clockIsTicking){ //add it to the matrix if the clock is ticking
          SetMatrixValue(0, Mirrored(col), velocity); //make sure it cant get darker than the background
        }
        SetTrigger(Mirrored(col),velocity);
        SetButtonLED(col, GetHue(col), GetSaturation(col), 127);
        buttonPixels.show(); //show instantly but could also put this outside loop for performance
      }
    }
  }
}

void OnNoteOff(byte channel, byte note, byte velocity) {
  if (channel == operatingChannel) {
    for (int col = 0; col < COLS; col++) {
      if (note == GetAbsoluteScaleNote(Mirrored(col))) { //reset the colors when the note is released
        SetTrigger(Mirrored(col),0); //dont mirror the model
        SetButtonBackgroundColor(col);
        buttonPixels.show();
      }
    }
  }
}

void OnVelocityChange(byte channel, byte note, byte velocity) {

}

void OnControlChange(byte channel, byte control, byte value) { //this is where settings are being changed
  if (channel == setupChannel) {
    if (control == tonicCC){
      SetTonic(value);
    }
    else if (control == scaleCC){
      SetScale(value);
    }
    else if (control == mirrorCC){
      SetMirror(value);
    }
     else if (control == transposeCC){
      SetscaleTranspose(value);
    }
    SetColorsFromScale();
    for (int col=0;col<COLS;col++){
      bool inBounds = IsInBounds(GetAbsoluteScaleNote(Mirrored(col)));// Is this out of bounds?
      SetTrigger(col,0); //update all the trigger states// since all are 0 mirror doesn't matter
      if (inBounds){
        SetButtonBackgroundColor(col);
      }
      else{
        SetButtonLED(col,0,0,0); //update the buttonpixels // value 0 if out of bounds
      }
      for (int row=0;row<ROWS;row++){ //update all the matrix pixels
        SetMatrixLED(row,col,GetHue(col),GetSaturation(col),GetMatrixValue(row,col) * inBounds); //should set value to 0 if out of bounds
      }
    }
    matrixPixels.show(); //show all updated pixels
    buttonPixels.show();
    usbMIDI.sendControlChange(123, 0, operatingChannel); //send all notes off on that channel to avoid hanging notes
  }
}

void OnProgramChange(byte channel, byte program) {

}

void OnAfterTouch(byte channel, byte pressure) {

}

void OnPitchChange(byte channel, int pitch) {

}


void RealTimeSystem(byte realtimebyte) {
  if (realtimebyte == CLOCK) {
    counter++;
    if (!clockIsTicking) // if clock already running at startup
      clockIsTicking = true;
    if (counter % 3 == 0) { // every 3 ticks means 1/32th note steps, every 6 means 1/16th note steps
      for (int col = 0; col < COLS; col++) {
        bool inBounds = IsInBounds(GetAbsoluteScaleNote(Mirrored(col)));
        for (int row=0;row<ROWS;row++){
          SetMatrixLED(row,col,GetHue(col),GetSaturation(col),GetMatrixValue(row,col)*inBounds); 
        }
        ShiftColumnValues(col); //Shift every row of the model down one step
      }
      matrixPixels.show(); //once all columns are updated view them at the same time
    }
    if (counter == 24) { //every 1/4th note
      counter = 0; //reset the tick counter
      for(int col=0;col<COLS;col++){
        //if you wanna do something on each beat    
      }
    }
  }

  if (realtimebyte == START || realtimebyte == CONTINUE) {
    counter = 0;
    clockIsTicking = true;
  }

  if (realtimebyte == STOP) {
    clockIsTicking = false;
  }
}






//View related functions ------------------------------------------------------------------------

void SetHue(byte col, byte hue){
  hues[col] = hue;
}

byte GetHue(byte col){
  return hues[Mirrored(col)];
}

byte GetSaturation(byte col){
  return saturations[Mirrored(col)];
}

void SetSaturation(byte col, byte saturation){
  saturations[col] = saturation;
}

// Change this function for different visualisations
void SetColorsFromScale(){ // this updates the model and should not be affected by mirroring // I tried mapping all saturation changes over all 128 notes, but then every 8-set of those looked to similar for being able to see in what direction the pitches were accending
  int tonicAdd = tonic%12; // how many semitones the tonic is over it's octave
  for(int col=0;col<COLS;col++){
    byte note = GetAbsoluteScaleNote(col);
    byte valueIndependantOfTonic = note - tonicAdd;
    hues[col] = constrain(map(map(scale, 0,numberOfScales+5,0,127) + map(valueIndependantOfTonic,24,48,0,10),0,127+32,0,127),0,127) ;
    if (valueIndependantOfTonic<24){ //will only distribute colors over 3 octaves, to get a good ramp
      saturations[col] = 127;
    }
    else if (valueIndependantOfTonic>48){
      saturations[col] = 70;
    }
    else{
      saturations[col] = map(valueIndependantOfTonic,24,48,127,70); //higher value more white
    }
  }
}

uint32_t ColorGenerator(byte hue, double sat, double val) { //modified colorwheel function to cover HSV
  val = val / 127;
  sat = (127 - sat)/127;
  hue = 255 - 2 * hue;
  uint8_t r;
  uint8_t b;
  uint8_t g;

  if(hue < 85) {
    r = ( (255-hue*3) + (255-(255-hue*3))*sat )*val;
    g = (255*sat)*val;
    b = ( (hue*3) + (255-(hue*3))*sat )*val;
  } else if(hue < 170) {
    hue -= 85;
    r = (255*sat)*val;
    g = ( (hue*3) + (255-(hue*3))*sat )*val;
    b = ( (255-hue*3) + (255-(255-hue*3))*sat )*val;
  } else {
   hue -= 170;
   r = ( (hue*3) + (255-(hue*3))*sat )*val;
   g = ( (255-hue*3) + (255-(255-hue*3))*sat )*val;
   b = (255*sat)*val;
  }
  return buttonPixels.Color(r,g,b);//currently produces a color for the buttonPixels object, but apparently the matrixPixel object can use it as well
}

void SetMatrixLED(int row, int col, byte hue, byte saturation, byte value) { //sets matrix leds and handles out of bounds
  if (value<MATRIXAMBIENTVALUE){
    value = map(value,0,127,MATRIXAMBIENTVALUE,127); // make sure it doesn't get darker than the background
  } 
  if (col == 0 || col % 2 == 0) { //if it is column 0,2,4,6 do it normal
    matrixPixels.setPixelColor(row + ROWS * col,ColorGenerator(hue,saturation,value));
  }
  else { //otherwise adress pixels in reverse due to hardware setup
    matrixPixels.setPixelColor(ROWS - 1 - row + ROWS * col,ColorGenerator(hue,saturation,value));
  }
}


void SetButtonLED(int buttonIndex, byte hue, byte saturation, byte value) { 
  value = map(value,0,127,BUTTONAMBIENTVALUE,127);
  buttonPixels.setPixelColor(buttonIndex, ColorGenerator(hue,saturation,value)); 
}

void SetButtonBackgroundColor(int col){
  if (IsButtonTonic(col) && !GetTrigger(col)) //verify if the button is tonic and not triggered through midi or button, in that case indicate that it is the tonic
    SetButtonLED(col, GetHue(col), GetSaturation(col)*0.5, BUTTONAMBIENTVALUE+3); // show tonic differently
  else
   SetButtonLED(col, GetHue(col), GetSaturation(col), BUTTONAMBIENTVALUE); //If the button is not pressed and not tonic, use regular ambient color
}


void ButtonController(){
  for ( int col = 0; col < COLS; col++ ) { //for all buttons
    if ( bouncers[col].update() ) { //if the button has changed

      byte note = GetAbsoluteScaleNote(Mirrored(col)); //the note corresponding to that button
      if (IsInBounds(note)){   //if it is in the legal range

        if ( bouncers[col].read() == LOW ) { //if the button is pressed
          if (bouncers[col].fallingEdge()) { //if it previously was unpressed
              usbMIDI.sendNoteOn(note, 127, operatingChannel);// do midi note stuff
              //for (int i=0;i<ROWS;i++){
              //  for (int j=0;j<COLS;j++){
              //    Serial.print(map(matrixValues[i][j],0,127,MATRIXAMBIENTVALUE,127));
               //   Serial.print(" ");
               // }
               // Serial.println("");
              //}
              //// Serial.print("scale index ");
              // Serial.println(GetScaleIndex(Mirrored(col)));
              if (clockIsTicking){  //only add it to the matrix if the clock is running
                SetMatrixValue(0, Mirrored(col), 127);
              }
              SetTrigger(Mirrored(col),127); //set the trigger and since we dont know if col is mirrored or not
              SetButtonLED(col, GetHue(col), GetSaturation(col), 127); // set the leds // moved this one step in
          }
        }

        else { //if the button is not pressed
          if (bouncers[col].risingEdge()) { //if it previously was pressed do note off stuff
            usbMIDI.sendNoteOff(note, 0, operatingChannel);
            SetTrigger(Mirrored(col),0); //update the button state
          }
          SetButtonBackgroundColor(col);
        }
      }
      else{ // if it is not in the legal range
        SetButtonLED(col, GetHue(col), GetSaturation(col), 0); //make it dark to show out of bounds
      }
      buttonPixels.show(); // only update if the button has changed
    }
  }
}




//Model related functions --------------------------------------------------------------------------------------


void ShiftColumnValues(int col) { 
  for (int i = ROWS - 1; i > 0 ; i--) {
    matrixValues[i][col] = matrixValues[i - 1][col]; //shift the values down in the models columns, starting at the bottom to not overwrite important data
  }
  if (GetTrigger(col) != 0){  //checks if this current col corresponds to a trigger or not
    matrixValues[0][col] = currentTriggerVelocities[col]*0.3; // if so, eadd a scaled value to the matrix // no mirroring needed since all between models
  }
  else{
    matrixValues[0][col] = MATRIXAMBIENTVALUE; //Set first position to ambient
  }
}

int GetOctaveOffset(int buttonIndex){ //is supposed to tell us in what octave the note corresponding to this button lies, relative to the octave the tonic is in
  int transposedButtonIndex = buttonIndex - scaleTranspose;
  if (transposedButtonIndex >= 0) {
    return transposedButtonIndex / scaleSizes[scale];
  }
  else {
    return ((transposedButtonIndex+1) / scaleSizes[scale]) - 1; //octave here is always -1 or less, never 0
  }
}

int GetScaleIndex(int buttonIndex){ // Given a buttonindex, what scaleindex does that correspond to in the current scale //is used to determin if a button is root or not
  int transposedButtonIndex = buttonIndex - scaleTranspose;
  int maxScaleIndex = scaleSizes[scale]-1;
  if (transposedButtonIndex >= 0) {
    return transposedButtonIndex % scaleSizes[scale];
  }
  else {
    transposedButtonIndex = abs(transposedButtonIndex)-1; // clearify calculations with "distance", we already know it's negative
    return maxScaleIndex - (transposedButtonIndex % scaleSizes[scale]); //find index relative to root in that new octave
  }
}

int GetRelativeScaleNote(int buttonIndex) {
  return GetOctaveOffset(buttonIndex) * 12 + scales[scale][GetScaleIndex(buttonIndex)]; //returns midi value for a buttonIndex, with respect to tonic, scale and octave offset
}

int GetAbsoluteScaleNote(int buttonIndex) {
  return tonic + GetRelativeScaleNote(buttonIndex);
}

void SetScale(int cc) {
  if (cc < numberOfScales) {
    scale = cc;
  }
  else {
    scale = numberOfScales - 1; //since there are fewer scales than cc values
  }
}

void SetTonic(int cc) {
  tonic = cc;
}

void SetscaleTranspose(int cc) {
  scaleTranspose = cc - 64; // remember that the offset zero is at cc 64 in any implementation
}

void SetMirror(int cc) { //incoming cc sets the mirror bool
  if (cc < 64) {
    mirror = false;
  }
  else {
    mirror=true;
  }
}

int Mirrored(int index){ // takes an index, if mirror is true, it returnes the mirrored index
  if (mirror)
    return map(index, 0, 7, 7, 0);
  //   return COLS-1-index;// -1 to get on index form
  // }
  else
    return index;
  // }
}


bool IsInBounds(byte value) {
  return (value <UPPERNOTELIMIT) && (value>=LOWERNOTELIMIT);
}

bool IsButtonTonic(byte buttonIndex){
  return (GetScaleIndex(Mirrored(buttonIndex)) == 0);
}

void SetTrigger(byte index, byte value){
  currentTriggerVelocities[index] = value;
}

byte GetTrigger(byte index){ //returns true if the current index is currently being triggered
  return currentTriggerVelocities[Mirrored(index)];
}


void SetMatrixValue(byte row, byte col, byte value){
  matrixValues[row][col] = value; //make sure it doesn't get darker than the background
}

byte GetMatrixValue(byte row, byte col){
  return matrixValues[row][Mirrored(col)];
}
