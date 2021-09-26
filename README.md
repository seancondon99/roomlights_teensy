# roomlights_teensy

This repo contains the arduino code to turn a Teensy and a strip of LEDs into a music reactive installation. There are four code files here which each do different music visualizations. For more info and for demos, please check out my personal website at https://seancondon99.github.io/freq_vis.html.

All the visualizations require reading in real-time audio information to the Teensy, and then the Teensy displays this audio information on a strip of LEDs.

## Description of files

#### fft.ino
Takes the fast fourier transform of audio signal and displays frequency power at different points of the LED strip. E.g. low frequencies are displayed in the middle of the strip, and high frequencies are displayed on the edges of the strip. On top of the frequency power, the lights also loop through a rainbow color scheme, and decay to black after being lit up by audio signal.

#### beat_detection_trailing_array.ino
This file detects beats in the bass range of the audio signal by keeping a trailing array of the past N bass frequency values, and detected significant deviations from the average of these trailing N values.

#### jump_game.ino
A single-player game in which a one-dimensional character needs to jump over one-dimensional spikes for a total of six levels. The player jumps when the Teensy detects a single clapping sound, so you need to time your claps to have the character jump over all of the spikes.

#### fullmaster.ino

This file contains the c++ code to handle five different light strips at once. There are four light strips that make up an audio EQ, where each light strip corresponds to a different frequency bin (bass, mid-low, mid-high, and high), and the final light strip plays an FFT of the audio.
