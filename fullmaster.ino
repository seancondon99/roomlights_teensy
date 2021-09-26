
//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include "FastLED.h"
#include <math.h>

#define NUM_LEDS 300
#define volNUM_LEDS 72
#define BIN_WIDTH 1

// MATRIX VARIABLES FROM BEFORE
const int maxBrightness = 40;
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



float decay = 0.93;
const int colorRange = 80;
const int startColor = 0;
const int volcolorRange = 75;
const int volstartColor = 120;
const int HALF_LEDS = floor(NUM_LEDS/2);
const int NUM_FLEDS = ceil((255./colorRange) * NUM_LEDS);
const int volHALF_LEDS = floor(volNUM_LEDS/2);
const int volNUM_FLEDS = ceil((255./volcolorRange) * volNUM_LEDS);
CRGB leds[NUM_LEDS];
CHSV hsv_leds[NUM_LEDS];
CHSV fleds[NUM_FLEDS];
CRGB hatsleds[volNUM_LEDS];
CHSV hatshsv_leds[volNUM_LEDS];
CRGB midsleds[NUM_LEDS];
CHSV midshsv_leds[NUM_LEDS];
CRGB lowsleds[volNUM_LEDS];
CHSV lowshsv_leds[volNUM_LEDS];
CRGB bassleds[NUM_LEDS];
CHSV basshsv_leds[NUM_LEDS];
CRGB volleds[volNUM_LEDS];
CHSV volhsv_leds[volNUM_LEDS];
CHSV volfleds[volNUM_FLEDS];


//PINS!
const int AUDIO_INPUT_PIN = A15;        // Input ADC pin for audio data.
const int POWER_LED_PIN = 13; 
//Adafruit Neopixel object
const int HATS_PIN = 2;
const int BASS_PIN = 3; 
const int FFT_PIN = 5;
const int LOWS_PIN = 6;
const int MIDS_PIN = 5;
const int NUM_BINS = floor(NUM_LEDS/(BIN_WIDTH)); //Get the number of bins based on NUM_LEDS and BIN_WIDTH
const int HALF_NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap
const int volNUM_BINS = floor(volNUM_LEDS/(BIN_WIDTH)); //Get the number of bins based on NUM_LEDS and BIN_WIDTH
const int volHALF_NUM_BINS = floor(volNUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap

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


int startTimer = 0;
int timer = 0;
int counter = 0;

int hats_startTimer = 0;
int hats_levelcalc = 0;
int hats_timer = 0;
int hats_leveltime = 0;
int hats_counter = 0;

int lows_startTimer = 0;
int lows_levelcalc = 0;
int lows_timer = 0;
int lows_leveltime = 0;
int lows_counter = 0;

int mids_startTimer = 0;
int mids_levelcalc = 0;
int mids_timer = 0;
int mids_leveltime = 0;
int mids_counter = 0;

int bass_startTimer = 0;
int bass_levelcalc = 0;
int bass_timer = 0;
int bass_leveltime = 0;
int bass_counter = 0;
String config;
//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------


// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.println("setup");
  writeFrequencyBinsHorizontal();
  //creates spectrum array (fleds) for color reference later. First value is range (0-255) of spectrum to use, second is starting value. Negate range to flip order.
  //Note: (-255,0 will be solid since the starting value input only loops for positive values, all negative values are equiv to 0 so you would want -255,255 for a reverse spectrum)
  
  color_spectrum_half_wrap_setup();
  volume_meter_half_wrap_setup();
  //color_spectrum_setup(255,0);

//initialize strip object
  FastLED.addLeds<NEOPIXEL, FFT_PIN>(leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, HATS_PIN>(hatsleds, volNUM_LEDS);
  FastLED.addLeds<NEOPIXEL, LOWS_PIN>(lowsleds, volNUM_LEDS);
  FastLED.addLeds<NEOPIXEL, MIDS_PIN>(midsleds, volNUM_LEDS);
  FastLED.addLeds<NEOPIXEL, BASS_PIN>(bassleds, volNUM_LEDS);
  FastLED.show();

  //set config to on at start
  delay(100);
  config = "on";
  Serial1.print("AT+RESET");

  //compute the vertical thresholds before starting. These are used for intensity graphing of frequencies
  //computeVerticalLevels();
}


int hats_maxlevel = 0;
int hats_mastermax = 0;
int lows_maxlevel = 0;
int lows_mastermax = 0;
int mids_maxlevel = 0;
int mids_mastermax = 0;
int bass_maxlevel = 0;
int bass_mastermax = 0;
String message = "";
void loop() {

  //check for serial1 incoming data
  int incomingByte;
  if (Serial1.available()){
    incomingByte = Serial1.read();
    message += char(incomingByte);
    }else{
      if (message == ""){
        //do nothing
      }else{
        Serial.println(message);
        config = message;
        message = "";
      }
    }

    Serial.println(config);

    if (config=="on"){

   //check if Audio processing for that sample frame is compelte
   if (fft.available()) {

    //HATS VOLUME
    unsigned int hats_locallevel;
      hats_locallevel = hats_color_spectrum_half_wrap(true, hats_maxlevel, hats_mastermax);
      if (hats_locallevel > hats_maxlevel){
        hats_maxlevel = hats_locallevel;
        hats_mastermax = hats_locallevel;
        hats_startTimer = millis();
      }
      if (hats_maxlevel < 0){
        hats_maxlevel=0;
      }

      //LOWS VOLUME
      unsigned int lows_locallevel;
      lows_locallevel = lows_color_spectrum_half_wrap(true, lows_maxlevel, lows_mastermax);
      if (lows_locallevel > lows_maxlevel){
        lows_maxlevel = lows_locallevel;
        lows_mastermax = lows_locallevel;
        lows_startTimer = millis();
      }
      if (lows_maxlevel < 0){
        lows_maxlevel=0;
      }

      //MIDS VOLUME
      unsigned int mids_locallevel;
      mids_locallevel = mids_color_spectrum_half_wrap(true, mids_maxlevel, mids_mastermax);
      if (mids_locallevel > mids_maxlevel){
        mids_maxlevel = mids_locallevel;
        mids_mastermax = mids_locallevel;
        mids_startTimer = millis();
      }
      if (mids_maxlevel < 0){
        mids_maxlevel=0;
      }

      //BASS VOLUME
       unsigned int bass_locallevel;
      bass_locallevel = bass_color_spectrum_half_wrap(true, bass_maxlevel, bass_mastermax);
      
      if (bass_locallevel > bass_maxlevel){
        bass_maxlevel = bass_locallevel;
        bass_mastermax = bass_locallevel;
        bass_startTimer = millis();
      }
      if (bass_maxlevel < 0){
        bass_maxlevel=0;
      }
    
      // uncomment spectrum visual type
      color_spectrum_half_wrap(true);
      //color_spectrum(255,0);
      FastLED.show();
   }
      //Serial.println(millis());
  }else if (config=="off"){
    turnoff();
    FastLED.show();
  }

  //choose any time based modifiers
  timer = millis();
  if(timer-startTimer > 100){
    moving_color_spectrum_half_wrap();    //modifies color mapping
    startTimer = millis();
    //FastLED.show();
  }

  hats_timer = millis();
  hats_leveltime = millis();
  //Serial.println(timer-startTimer);
  if(hats_timer-hats_startTimer > 750){
   // moving_color_spectrum_half_wrap();
   //start moving the maxlevel
    if(hats_leveltime - hats_levelcalc > 50){
    hats_maxlevel = hats_maxlevel - 1;
    hats_levelcalc = millis();
  }
    
  }
    lows_timer = millis();
  lows_leveltime = millis();
  //Serial.println(timer-startTimer);
  if(lows_timer-lows_startTimer > 750){
   // moving_color_spectrum_half_wrap();
   //start moving the maxlevel
    if(lows_leveltime - lows_levelcalc > 50){
    lows_maxlevel = lows_maxlevel - 1;
    lows_levelcalc = millis();
  }
    
  }

    mids_timer = millis();
  mids_leveltime = millis();
  //Serial.println(timer-startTimer);
  if(mids_timer-mids_startTimer > 750){
   // moving_color_spectrum_half_wrap();
   //start moving the maxlevel
    if(mids_leveltime - mids_levelcalc > 50){
    mids_maxlevel = mids_maxlevel - 1;
    mids_levelcalc = millis();
  }
    
  }
    bass_timer = millis();
  bass_leveltime = millis();
  //Serial.println(timer-startTimer);
  if(bass_timer-bass_startTimer > 750){
   // moving_color_spectrum_half_wrap();
   //start moving the maxlevel
    if(bass_leveltime - bass_levelcalc > 50){
    bass_maxlevel = bass_maxlevel - 1;
    bass_levelcalc = millis();
  }
    
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
void volume_meter_half_wrap_setup() {
  for (int i = 0;i < volNUM_FLEDS/2; i++){
    float number = i * 255;
    float number1 = number / (volNUM_FLEDS/2);
    float number2 = floor(number1);

    
    volfleds[i] = CHSV((volstartColor - number2),255,0);
    volfleds[volNUM_FLEDS - i - 1] = CHSV((volstartColor-number2),255,0);
  
  }
  for (int i = 0; i< volHALF_LEDS; i++){
    volleds[volHALF_LEDS - i - 1] = volfleds[i];
    volhsv_leds[volHALF_LEDS - i - 1] = volfleds[i];
    volleds[volHALF_LEDS + i] = volfleds[i];
    volhsv_leds[volHALF_LEDS + i] = volfleds[i];
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

unsigned int hats_color_spectrum_half_wrap(bool useEq, unsigned int local_max, unsigned int mastermax){
  unsigned int x, freqBin;
  float level;
  float normvol;
  int currval;
  int j;
  int k;
  int distance;
  int right;
  int left;
  float dist_mult;
  freqBin = 0;
  //calculate volume level here
  level = fft.read(100, 255);
  float max_level;
  max_level = 1.2;
  //Serial.println(local_max);
  normvol = level*volNUM_BINS/max_level;
  if (normvol >= volNUM_BINS){
    normvol = volNUM_BINS - 1;
  }
  
  //level varies from 0 to maybe like 4
  //we want to decay relative to a local max sampled every x seconds
  for (x=0; x < volNUM_BINS; x++) {

      if (x<normvol){
        CHSV fled = volfleds[x];
        hatsleds[x] = CHSV(fled.hue,255,maxBrightness);
        hatshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else if (x==local_max){

          CHSV fled = volfleds[mastermax]; 
          hatsleds[x] = CHSV(fled.hue,255,maxBrightness);
          hatshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else {
        distance = x-normvol;
        dist_mult = sqrt(distance)/100;

        //decay value at that pixel
        currval = hatshsv_leds[x].val;

        if (currval<15){
          CHSV fled = volfleds[mastermax]; 
           hatsleds[x] = CHSV(fled.hue,255,0);
          hatshsv_leds[x] = CHSV(fled.hue,255,0);   
        }
        else if (x>local_max){
          CHSV fled = volfleds[mastermax]; 
          hatsleds[x] = CHSV(fled.hue,255,0);
          hatshsv_leds[x] = CHSV(fled.hue,255,0);
        }
        else{
          CHSV fled = volfleds[x]; 
          hatsleds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
          hatshsv_leds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
        }
      }
        
    }
  return normvol;
}

unsigned int lows_color_spectrum_half_wrap(bool useEq, unsigned int local_max, unsigned int mastermax){
  unsigned int x, freqBin;
  float level;
  float normvol;
  int currval;
  int j;
  int k;
  int distance;
  int right;
  int left;
  float dist_mult;
  freqBin = 0;
  //calculate volume level here
  level = fft.read(4, 30);
  float max_level;
  max_level = 2;
  normvol = level*volNUM_BINS/max_level;
  if (normvol >= volNUM_BINS){
    normvol = volNUM_BINS - 1;
  }
  
  //level varies from 0 to maybe like 4
  //we want to decay relative to a local max sampled every x seconds
  for (x=0; x < volNUM_BINS; x++) {

      if (x<normvol){
        CHSV fled = volfleds[x];
        lowsleds[x] = CHSV(fled.hue,255,maxBrightness);
        lowshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else if (x==local_max){

          CHSV fled = volfleds[mastermax]; 
          lowsleds[x] = CHSV(fled.hue,255,maxBrightness);
          lowshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else {
        distance = x-normvol;
        dist_mult = sqrt(distance)/100;

        //decay value at that pixel
        currval = lowshsv_leds[x].val;




        if (currval<15){
          CHSV fled = volfleds[mastermax]; 
           lowsleds[x] = CHSV(fled.hue,255,0);
          lowshsv_leds[x] = CHSV(fled.hue,255,0);   
        }
        else if (x>local_max){
          CHSV fled = volfleds[mastermax]; 
          lowsleds[x] = CHSV(fled.hue,255,0);
          lowshsv_leds[x] = CHSV(fled.hue,255,0);
        }
        else{
          CHSV fled = volfleds[x]; 
          lowsleds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
          lowshsv_leds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
        }
      }
        
    }
       if (normvol > 69){
    normvol = 69;
   }
  return normvol;
}
unsigned int mids_color_spectrum_half_wrap(bool useEq, unsigned int local_max, unsigned int mastermax){
  unsigned int x, freqBin;
  float level;
  float normvol;
  int currval;
  int j;
  int k;
  int distance;
  int right;
  int left;
  float dist_mult;
  freqBin = 0;
  //calculate volume level here
  level = fft.read(20, 60);
  float max_level;
  max_level = 2.5;
  normvol = level*volNUM_BINS/max_level;
  if (normvol >= volNUM_BINS){
    normvol = volNUM_BINS - 1;
  }
  
  //level varies from 0 to maybe like 4
  //we want to decay relative to a local max sampled every x seconds
  for (x=0; x < volNUM_BINS; x++) {

      if (x<normvol){
        CHSV fled = volfleds[x];
        midsleds[x] = CHSV(fled.hue,255,maxBrightness);
        midshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else if (x==local_max){

          CHSV fled = volfleds[mastermax]; 
          midsleds[x] = CHSV(fled.hue,255,maxBrightness);
          midshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else {
        distance = x-normvol;
        dist_mult = sqrt(distance)/100;

        //decay value at that pixel
        currval = midshsv_leds[x].val;




        if (currval<15){
          CHSV fled = volfleds[mastermax]; 
           midsleds[x] = CHSV(fled.hue,255,0);
          midshsv_leds[x] = CHSV(fled.hue,255,0);   
        }
        else if (x>local_max){
          CHSV fled = volfleds[mastermax]; 
          midsleds[x] = CHSV(fled.hue,255,0);
          midshsv_leds[x] = CHSV(fled.hue,255,0);
        }
        else{
          CHSV fled = volfleds[x]; 
          midsleds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
          midshsv_leds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
        }
      }
        
    }
  if (normvol > 69){
    normvol = 69;
  }
  return normvol;
}

unsigned int bass_color_spectrum_half_wrap(bool useEq, unsigned int local_max, unsigned int mastermax){
  unsigned int x, freqBin;
  float level;
  float normvol;
  int currval;
  int j;
  int k;
  int distance;
  int right;
  int left;
  float dist_mult;
  freqBin = 0;
  //calculate volume level here
  level = fft.read(0, 4);
  float max_level;
  
  
  max_level = 1.5;
  //Serial.println(local_max);
  normvol = level*volNUM_BINS/max_level;
  if (normvol >= volNUM_BINS){
    normvol = volNUM_BINS - 1;
  }

  for (x=0; x < volNUM_BINS; x++) {

      if (x<normvol){
        CHSV fled = volfleds[x];
        bassleds[x] = CHSV(fled.hue,255,maxBrightness);
        basshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else if (x==local_max){

          CHSV fled = volfleds[mastermax]; 
          bassleds[x] = CHSV(fled.hue,255,maxBrightness);
          basshsv_leds[x] = CHSV(fled.hue,255,maxBrightness);
      }

      else {
        distance = x-normvol;
        dist_mult = sqrt(distance)/100;

        //decay value at that pixel
        currval = basshsv_leds[x].val;




        if (currval<15){
          CHSV fled = volfleds[mastermax]; 
           bassleds[x] = CHSV(fled.hue,255,0);
          basshsv_leds[x] = CHSV(fled.hue,255,0);   
        }
        else if (x>local_max){
          CHSV fled = volfleds[mastermax]; 
          bassleds[x] = CHSV(fled.hue,255,0);
          basshsv_leds[x] = CHSV(fled.hue,255,0);
        }
        else{
          CHSV fled = volfleds[x]; 
          bassleds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
          basshsv_leds[x] = CHSV(fled.hue,255,currval*(decay-dist_mult));
        }
      }
        
    }
  return normvol;
}

void turnoff(){
  //

  for (int x=0; x < volNUM_BINS; x++) {
    midsleds[x] = CHSV(0,255,0);
    midshsv_leds[x] = CHSV(0,255,0);
    hatsleds[x] = CHSV(0,255,0);
    hatshsv_leds[x] = CHSV(0,255,0);
    bassleds[x] = CHSV(0,255,0);
    basshsv_leds[x] = CHSV(0,255,0);
    lowsleds[x] = CHSV(0,255,0);
    lowshsv_leds[x] = CHSV(0,255,0);
  }
}
