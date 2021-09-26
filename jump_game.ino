#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
//#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
#include <math.h>

#define NUM_LEDS 300
#define BIN_WIDTH 1
const int bright = 100;
//I made the computer able to play too so you can see him
//absolutely kill it out here
bool computer_playing = false;

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


const int HALF_LEDS = floor(NUM_LEDS/2);

CRGB leds[NUM_LEDS];
CHSV hsv_leds[NUM_LEDS];


//PINS!
const int AUDIO_INPUT_PIN = A15;        // Input ADC pin for audio data.
const int POWER_LED_PIN = 13; 
//Adafruit Neopixel object
const int NEO_PIXEL_PIN = 5;           // Output pin for neo pixels.
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

const int positions_len = 10;
int player_position;
int positions[positions_len];
int color_array[6] = {128,96,64,20,0,224};

const int sensitivity = 30;

void setup() {
  // put your setup code here, to run once:
  AudioMemory(12);
  Serial.begin(9600);
  Serial.println("setup");
  


  FastLED.addLeds<NEOPIXEL, NEO_PIXEL_PIN>(leds, NUM_LEDS);
  FastLED.show();

  //set some obstacle positions
  //only set positions to greater than 36
  positions[0] = 15;
  positions[1] = 16;
  positions[2] = 37;
  positions[3] = 38;
  positions[4] = 39;
  positions[5] = 55;
  positions[6] = 70;
  positions[7] = 71;
  positions[8] = 90;

 

}

//lets represent obstacles as an array of positions
//when an obstacle has position > 150, reset it, with a
//chance to change its dimensions


int startTimer = 0;
int timer = 0;
int movetime = 100;
int gravhelper = 0;
int gravtimer = 0;

int levels_passed = 0;
int player_vertical_pos = 0;
int loop_counter = 0;

void loop() {

  
loop_counter += 1;
int jump;
bool jumped;

//set all pixels to 0
for (int j=0; j<HALF_LEDS *2; j++){
  set_an_led_a_color(j,20,20,0);
}

  //player position
  if (player_vertical_pos == 0){
    set_an_led_a_color(120,20,20,bright);
  }

  else{
    set_an_led_a_color(120,20,20,0);
  }
  

  //show obstacles
  for (int i=0; i<positions_len; i++){
    if (positions[i] != 0){
      set_an_led_a_color(positions[i], 30, 255, bright);
    }
  }

  //check for a jump
  jumped = jump_check();
  if (computer_playing){
    jumped = machine_learning_in_the_cloud_super_AI();
  }

  //if jumped, have a timer for 5 * velocity until player falls back down
  if (jumped and player_vertical_pos == 0){
    //change players vertical position
    player_vertical_pos = 1;
    //commence gravity!
    gravtimer = millis();
  }

//timer governing obstacle movement
  timer = millis();
  if(timer-startTimer > movetime){   //modifies color mapping
    startTimer = millis();
    //move obstacles!
    for (int i=0; i < positions_len; i++){

      if (positions[i] > 130){
        //obstacle is past player, update position and shape
        //loop to see if neighbors are close by
        positions[i] = 37;
      }

      else if (positions[i] == 120){
        if (player_vertical_pos == 1){
          //obstacle cleared
          //flash_green();
          ripple_animation();
        positions[i] += 1;
        }

        else{
          flash_red();
          movetime = 100;
          levels_passed = 0;
          loop_counter = 0;
          setup();
        }
      }
      
      else if (positions[i] != 0){
        positions[i] += 1;
      }
    }
 
  }
   FastLED.show();

  //timer governing gravity
  gravhelper = millis();
  if (gravhelper - gravtimer > movetime * 5){
    //player falls back down
    player_vertical_pos = 0;
    gravtimer = 0;
  }

//timer governing levels
  if(loop_counter > 1000){
    loop_counter = 0;
    movetime -= 15;
    levels_passed += 1;
    level_animation();
    setup();
    
  }

  //Win game code

  if (levels_passed > 5){
    win_animation();
    movetime = 100;
    //positions[i] += 1;
    levels_passed = 0;
    loop_counter = 0;
    setup();
  }
}



void set_an_led_a_color(int index, int color, int saturation, int level){
  leds[index] = CHSV(color,saturation,level);
  hsv_leds[index] = CHSV(color,saturation,level);
}

void flash_green(){
  for (int i=HALF_LEDS; i <NUM_LEDS; i++){
    set_an_led_a_color(i, 96,255,bright);
  }
}

void flash_green_full(){
  for (int i=HALF_LEDS; i <NUM_LEDS; i++){
    set_an_led_a_color(i, 96,255,bright);
    set_an_led_a_color(i-HALF_LEDS, 96,255,bright);
  }
}

void flash_green_indexed(int todex){
  for (int i=0; i<todex; i++){
    set_an_led_a_color(HALF_LEDS - i, 96,255,bright);
    set_an_led_a_color(i+HALF_LEDS, 96,255,bright);
  }
  FastLED.show();
  delay(10);
}

void flash_red(){
    for (int i=0; i <NUM_LEDS; i++){
    set_an_led_a_color(i, 0,255,bright);
  }
}

void flash_black(){
  for (int i=0; i <NUM_LEDS; i++){
    set_an_led_a_color(i, 0,255,0);
  }
  FastLED.show();
  //delay(50);
}

void level_animation(){
  int color;
  int lightsperlevel = 25;
  flash_black();
  for (int y=0; y < (levels_passed*lightsperlevel); y++){
    color = floor(y/lightsperlevel);
    set_an_led_a_color(HALF_LEDS - y, color_array[color], 255, bright);
    set_an_led_a_color(HALF_LEDS + y, color_array[color], 255, bright);
  }
  FastLED.show();
  delay(1000);
}

void ripple_animation(){
  for (int y=0; y < HALF_LEDS; y++){
    set_an_led_a_color(HALF_LEDS - y, color_array[100], 255, bright);
    set_an_led_a_color(HALF_LEDS + y, color_array[100], 255, bright);
    FastLED.show();
    flash_black();
  }
}

void win_animation(){
  //build up the levels passed totem with delays then flash green then
  //build out green from center then reset
  flash_black();
  levels_passed = 1;
  level_animation();
  levels_passed = 2;
  level_animation();
  levels_passed = 3;
  level_animation();
  levels_passed = 4;
  level_animation();
  levels_passed = 5;
  level_animation();
  levels_passed = 6;
  level_animation();

  for (int t=0; t<10; t++){
  flash_black();
  flash_green_full();
  FastLED.show();
  delay(100);
  }

  flash_black();
  FastLED.show();
  

  for (int n=0; n < HALF_LEDS; n++){
    flash_green_indexed(n);
  }

}

bool jump_check(){
  float new_value;
  new_value = fft.read(100,255) * 100;
  Serial.println(new_value);
  if (new_value > sensitivity){
    return true;
  }
  else{
    return false;
  }
}

bool machine_learning_in_the_cloud_super_AI(){
  //find nearest object in front of you
  int distance = 100;
  for (int i=0; i < positions_len; i++){
    if (positions[i] != 0 and positions[i]<121){
      int newdist = 120 - (positions[i]);

      if (newdist < distance){
        distance = newdist;
      }
    }
  }
  if (distance == 0){
    return true;
  }
  else{
    return false;
  }
}
