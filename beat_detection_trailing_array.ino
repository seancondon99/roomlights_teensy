#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
//#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
#include <math.h>

#define NUM_LEDS 300
#define BIN_WIDTH 1

// MATRIX VARIABLES FROM BEFORE
const unsigned int matrix_width = 60;
const unsigned int matrix_height = 32;
const unsigned int max_height = 255;
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"
const int standardBrightness = 60;

const float dynamicRange = 60.0; // total range to display, in decibels
const float linearBlend = 0.4;   // useful range is 0 to 0.7
// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above. 
float thresholdVertical[max_height];



float decay = 0.985;
const int maxBrightness = 100;  //from 0 to 255
const int colorRange = 1;
int startColor = 60;
const int beat_array_len = 10;
const int beatThreshold = 40;
const float deviationRequirement = 0.0;
const int HALF_LEDS = floor(NUM_LEDS/2);
const int NUM_FLEDS = ceil((255./colorRange) * NUM_LEDS);
CRGB leds[NUM_LEDS];
CHSV hsv_leds[NUM_LEDS];
CHSV fleds[NUM_FLEDS];

//PINS!
const int AUDIO_INPUT_PIN = A15;        // Input ADC pin for audio data.
const int POWER_LED_PIN = 13; 
//Adafruit Neopixel object
const int NEO_PIXEL_PIN = 6;           // Output pin for neo pixels.
const int NUM_BINS = floor(NUM_LEDS/(BIN_WIDTH)); //Get the number of bins based on NUM_LEDS and BIN_WIDTH
const int HALF_NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap

//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);


// Audio library objects
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       //xy=99,55
AudioAnalyzeFFT1024      fft;            //xy=265,75
AudioConnection          patchCord1(adc1, fft);


//empty array for generating 
int genFrequencyBinsHorizontal[NUM_BINS];
float genFrequencyLabelsHorizontal[NUM_BINS];

int genFrequencyHalfBinsHorizontal[HALF_NUM_BINS];
float genFrequencyHalfLabelsHorizontal[HALF_NUM_BINS];
float logLevelsEq[HALF_NUM_BINS];

//Array with some clipping to highest frequencies
int frequencyBinsHorizontal[60] = {
   1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,
   2,  2,  2,  2,  2,
   2,  2,  2,  2,  3,
   3,  3,  3,  3,  4,
   4,  4,  4,  4,  5,
   5,  5,  6,  6,  6,
   7,  7,  7,  8,  8,
   9,  9, 10, 10, 11,
   12, 12, 13, 14, 15,
   15, 15, 15, 15, 16,
   16, 17, 18, 19, 21
};

int startTimer = 0;
int levelcalc = 0;
int timer = 0;
int leveltime = 0;
int counter = 0;
//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

int trailingArray[beat_array_len];
// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);
  Serial.begin(9600);
  Serial.println("setup");
  writeFrequencyBinsHorizontal();
  //creates spectrum array (fleds) for color reference later. First value is range (0-255) of spectrum to use, second is starting value. Negate range to flip order.
  //Note: (-255,0 will be solid since the starting value input only loops for positive values, all negative values are equiv to 0 so you would want -255,255 for a reverse spectrum)
  
  color_spectrum_half_wrap_setup();
  //color_spectrum_setup(255,0);

//initialize strip object
  FastLED.addLeds<NEOPIXEL, NEO_PIXEL_PIN>(leds, NUM_LEDS);
  FastLED.show();

  //compute the vertical thresholds before starting. These are used for intensity graphing of frequencies
  //computeVerticalLevels();

  //instantiate trailing beat detection array
  for (int i=0; i<beat_array_len; i++){
    trailingArray[i] = 0;
  }

  
}



int maxlevel = 0;
float prev_val = 0.0;
float new_val = 0.0;

void loop() {

   //check if Audio processing for that sample frame is compelte
   if (fft.available()) {
    
      // uncomment spectrum visual type
      unsigned int locallevel;
      new_val = beat_detection(trailingArray);
      prev_val = new_val;
      //with each new val, pop first trailingArray value at 0, append new_val to 9
      //loop update trailing array
      for (int i=0; i<beat_array_len; i++){
        if (i==beat_array_len - 1){
         trailingArray[i] = new_val;
        }
        else{
         trailingArray[i] = trailingArray[i+1];
        }
      }

    timer = millis();
    if(timer-startTimer > 300){
    moving_color_spectrum_half_wrap();    //modifies color mapping
    startTimer = millis();
    //FastLED.show();
    }
   
      


      FastLED.show();
  }




  
}

//-----------------------------------------------------------------------
//----------------------------SETUP FUNCTIONS----------------------------
//-----------------------------------------------------------------------
// Run once from setup, the compute the vertical levels
// We don't actually use this right now. why not?
void computeVerticalLevels() {
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y=0; y < max_height; y++) {
    n = (float)y / (float)(max_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}

//Dynamically create frequency bin volume array for NUM_BINS
void writeFrequencyBinsHorizontal(){
  int sum = 0;
  int binFreq = 44;
  for (int i=0; i < NUM_BINS; i++){
    genFrequencyBinsHorizontal[i] = ceil(60./NUM_BINS*0.7964*pow(M_E,0.0583*(i + 1)*(60./NUM_BINS)));
    
  }
  for (int i=0; i < HALF_NUM_BINS; i++){
    genFrequencyHalfBinsHorizontal[i] = ceil(60./HALF_NUM_BINS*0.7964*pow(M_E,0.0583*(i + 1)*(60./HALF_NUM_BINS)));
  }
  
  for (int i=0; i<NUM_BINS;i++){
    genFrequencyLabelsHorizontal[i] = genFrequencyBinsHorizontal[i]*binFreq + sum;
    sum = genFrequencyLabelsHorizontal[i];
  }
  sum = 0;
  for (int i=0; i<HALF_NUM_BINS;i++){
    genFrequencyHalfLabelsHorizontal[i] = genFrequencyHalfBinsHorizontal[i]*binFreq + sum;
    //logLevelsEq[i] = (log10(genFrequencyHalfLabelsHorizontal[i])*0.00875+65.)/60.;
    if(genFrequencyHalfLabelsHorizontal[i]<4500){
      //logLevelsEq[i] = float(genFrequencyHalfLabelsHorizontal[i]*0.0875 + 45)/65;
      logLevelsEq[i] = (float(log10(genFrequencyHalfLabelsHorizontal[i])*0.008+65.))/65.;

    }
    else{
      logLevelsEq[i] = (float(-log10(genFrequencyHalfLabelsHorizontal[i])*0.007 + 160.))/65.;
    }

    sum = genFrequencyHalfLabelsHorizontal[i];
  }
  
}

//----------------------------------------------------------------------
//---------------------------RAINBOW VISUALS----------------------------
//----------------------------------------------------------------------

//NON REACTIVE----SPECTRUM BUILDERS

//creates full spectrum from red -> magenta that maps from 0->NUM_LEDS
void color_spectrum_setup(int colorRange, int startColor) {
  for (int i = 0;i < NUM_LEDS; i++){
    float number = i * colorRange;
    float number1 = number / NUM_LEDS;
    float number2 = floor(number1);
    leds[i] = CHSV((number2 + startColor),255,255);
    hsv_leds[i] = CHSV((number2 + startColor),255,255);
  }
}

//creates full spectrum from center out, input variables determine amount of full spectrum to use (i.e can limit to smaller color ranges)
void color_spectrum_half_wrap_setup() {
  for (int i = 0;i < NUM_FLEDS/2; i++){
    float number = i * 255;
    float number1 = number / (NUM_FLEDS/2);
    float number2 = floor(number1);

    
    fleds[i] = CHSV((startColor - number2),255,0);
    fleds[NUM_FLEDS - i - 1] = CHSV((startColor-number2),255,0);
   // fleds[i] = CHSV((startColor + number2),255,0);
    //fleds[NUM_FLEDS - i - 1] = CHSV((startColor+number2),255,0);
  
  }
  for (int i = 0; i< HALF_LEDS; i++){
    leds[HALF_LEDS - i - 1] = fleds[i];
    hsv_leds[HALF_LEDS - i - 1] = fleds[i];
    leds[HALF_LEDS + i] = fleds[i];
    hsv_leds[HALF_LEDS + i] = fleds[i];
  }
}

unsigned int beat_detection(int trailArray[beat_array_len]){
  unsigned int x;
  float new_value;
  float norm_value;
  int max_level;
  int thresh = 50;
  int j;
  int k;
  int right;
  int left;
  int currval;
  float mean;
  float var;
  float stddev;
  float beatPower;
  float setLevel;
  float multFactor = 1.5;
  max_level = 1;
  new_value = fft.read(0,3);
  new_value = pow(new_value,2);
  norm_value = new_value*HALF_NUM_BINS/max_level;
 //calculate mean of trailArray
 for (int i=0; i<beat_array_len; i++){
  mean += trailArray[i];
 }
 mean = mean/beat_array_len;

 //calculate variance of trailArray
 for(int j=0; j<beat_array_len; j++){
  var += pow(trailArray[j] - mean,2);
 }
 var = var / beat_array_len;
 stddev = pow(var, 0.5);

 //check for beat
 beatPower = norm_value;
 Serial.println(beatPower);
 if (beatPower > beatThreshold){
 for (x=0; x < HALF_NUM_BINS; x++) {
      right = (HALF_NUM_BINS - x);
      left = (HALF_NUM_BINS + x);
      j = right - 1;
      k = left;
      if ((beatPower+standardBrightness)*multFactor > maxBrightness){
        setLevel = maxBrightness;
      }else{
        setLevel = (beatPower+standardBrightness)*multFactor;
      }
      color_spectrum_half_wrap_update(j, setLevel);
      color_spectrum_half_wrap_update(k, setLevel);
    }
    return norm_value;
 }else{
  for (x=0; x < HALF_NUM_BINS; x++) {
      right = (HALF_NUM_BINS - x);
      left = (HALF_NUM_BINS + x);
      j = right - 1;
      k = left;
      currval = hsv_leds[j].val;
      
      if (currval>standardBrightness){
          color_spectrum_half_wrap_update(j,currval*(decay));
          color_spectrum_half_wrap_update(k,currval*(decay));
          
        }
        else{
          
          color_spectrum_half_wrap_update(j, standardBrightness);
          color_spectrum_half_wrap_update(k, standardBrightness);
        }
      }
      return norm_value;
  
  
 }
  
}



void color_spectrum_half_wrap_update(int index, float level) {
  if(index >= NUM_LEDS || index < 0){
    ;
  }else if(index >= HALF_LEDS){
    int f_index = index - HALF_LEDS;
    CHSV fled = fleds[f_index];
    leds[index] = CHSV(fled.hue,255,level);
    hsv_leds[index] = CHSV(fled.hue,255,level);

  }else{
    int f_index = abs(index - HALF_LEDS +1);
    CHSV fled = fleds[f_index];
    leds[index] = CHSV(fled.hue,255,level);
    hsv_leds[index] = CHSV(fled.hue,255,level);
  }
}

void moving_color_spectrum_half_wrap(){
  CHSV fled = fleds[0];
  for(int i=0;i<(NUM_FLEDS-1);i++){
    fleds[i].hue = fleds[i].hue+1;
  }
  fleds[NUM_FLEDS-1] = fled;
}
