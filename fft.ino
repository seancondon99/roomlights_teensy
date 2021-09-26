//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
//#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
#include <math.h>

#define NUM_LEDS 71
#define BIN_WIDTH 1

// MATRIX VARIABLES FROM BEFORE
const unsigned int matrix_width = 60;
const unsigned int matrix_height = 32;
const unsigned int max_height = 255;
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"

const float dynamicRange = 60.0; // total range to display, in decibels
const float linearBlend = 0.4;   // useful range is 0 to 0.7
// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above. 
float thresholdVertical[max_height];



float decay = 0.95;
const int colorRange = 80;
const int startColor = 0;
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


// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.

//This is the original array, curve = 0.7964*e^(0.0583x)
//int frequencyBinsHorizontal[60] = {
//   1,  1,  1,  1,  1,
//   1,  1,  1,  1,  1,
//   2,  2,  2,  2,  2,
//   2,  2,  2,  2,  3,
//   3,  3,  3,  3,  4,
//   4,  4,  4,  4,  5,
//   5,  5,  6,  6,  6,
//   7,  7,  7,  8,  8,
//   9,  9, 10, 10, 11,
//   12, 12, 13, 14, 15,
//   15, 16, 17, 18, 19,
//   20, 22, 23, 24, 25
//};

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
int timer = 0;
int counter = 0;
//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------


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
}



void loop() {

   //check if Audio processing for that sample frame is compelte
   if (fft.available()) {
    Serial.println(NUM_FLEDS);
      // uncomment spectrum visual type
      color_spectrum_half_wrap(true);
      //color_spectrum(255,0);
      FastLED.show();
      //Serial.println(millis());
  }

  //choose any time based modifiers
  timer = millis();
  if(timer-startTimer > 100){
    moving_color_spectrum_half_wrap();    //modifies color mapping
    startTimer = millis();
    //FastLED.show();
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
    fleds[i] = CHSV((number2 + startColor),255,0);
    fleds[NUM_FLEDS - i - 1] = CHSV((number2 + startColor),255,0);
  }
  for (int i = 0; i< HALF_LEDS; i++){
    leds[HALF_LEDS - i - 1] = fleds[i];
    hsv_leds[HALF_LEDS - i - 1] = fleds[i];
    leds[HALF_LEDS + i] = fleds[i];
    hsv_leds[HALF_LEDS + i] = fleds[i];
  }
}





void color_spectrum_half_wrap(bool useEq){
  unsigned int x, freqBin;
  float level;
  int j_val;
  //int k_val;
  int j;
  int k;
  int right;
  int left;
  freqBin = 0;
  for (x=0; x < HALF_NUM_BINS; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + genFrequencyHalfBinsHorizontal[x] - 1);
       //using equal volume contours to create a liner approximation (lazy fit) and normalizing. took curve for 60Db. labels geerates freq in hz for bin
      //gradient value (0.00875) was calculated but using rlly aggressive 0.06 to account for bassy speaker, mic,  and room IR.Numbers seem way off though...
      if(useEq==true){
        level = level*logLevelsEq[x]*255*5; 
      }
      
      right = (HALF_NUM_BINS - x)*BIN_WIDTH;
      left = (HALF_NUM_BINS + x)*BIN_WIDTH;
      // uncomment to see the spectrum in Arduino's Serial Monitor
      //Serial.println(level);
      //uncomment for full spec
      //level = 255;
      if (level>50) {
          for(int i=0;i<BIN_WIDTH;i++){
            j = right - i - 1;
            k = left + i;
            color_spectrum_half_wrap_update(j,level);
            color_spectrum_half_wrap_update(k,level);
          }

          
        } else {
          for(int i=0;i<BIN_WIDTH;i++){
            j = right - i - 1;
            k = left + i;
            j_val = hsv_leds[j].val;
            if(j_val < 15){
              color_spectrum_half_wrap_update(j,0);
              color_spectrum_half_wrap_update(k,0);
            }else{
              color_spectrum_half_wrap_update(j,j_val * decay);
              color_spectrum_half_wrap_update(k,j_val * decay);
            }
          }
          
         
        }
        
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + genFrequencyHalfBinsHorizontal[x];
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
    fleds[i] = fleds[i+1];
  }
  fleds[NUM_FLEDS-1] = fled;
}
