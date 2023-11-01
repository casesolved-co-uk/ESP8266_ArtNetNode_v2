/*
ESP8266_ArtNetNode v2.1.0
Copyright (c) 2016, Matthew Tong
Copyright (c) 2023, Richard Case
https://github.com/casesolved-co-uk/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

FastLED 12 channel pixel FX:
Ch1: Intensity
Ch2: Effect
Ch3: Speed
Ch4: Pos
Ch5: Size
Ch6-8:  Colour1 RGB
Ch9-11: Colour2 RGB
Ch12: Size1
*/


#ifndef WS_FX
#define WS_FX

#include <FastLED.h>
#include "debugLog.h"

#define MIN_FXINTERVAL 1 // milliseconds
#define MAX_FXINTERVAL 100
#define REFRESH_INTERVAL 5

// see FastLED colorutils.h
enum  pattern { STATIC, RAINBOW_CYCLE, THEATER_CHASE, TWINKLE, GRADIENT_STRIPE, NONE };

class pixPatterns {
  public:
    pattern  ActivePattern;       // which pattern is running
    
    unsigned long Interval;       // milliseconds between updates
    unsigned long lastUpdate;     // last update of position
    unsigned long lastRefresh;    // last LED output
    
    CRGB Colour1, Colour2;        // Colours pre-intensity
    CRGBPalette16 Palette;        // palette for suitable effects
    uint16_t TotalSteps;          // total number of steps in the pattern
    uint16_t Index;               // current step within the pattern
    uint8_t Speed;                 // speed of effect (0 = stop, -ve is reverse, +ve is forwards)
    uint8_t Size1, Size, Fade, Pos; // size, fading & position for static looks
    uint8_t Intensity;

    CLEDController* controller;  // FastLED controller

    // Methods
    pixPatterns(CLEDController* controller);
    bool Update(void);
    void Increment(void);
    void setSpeed(uint8_t s);
    void setFX(uint8_t fx);
    void Static(void);
    void StaticUpdate(void);
    void RainbowCycle(void);
    void RainbowCycleUpdate(void);
    void TheaterChase(void);
    void TheaterChaseUpdate(void);
    void Twinkle(void);
    void TwinkleUpdate(void);
    void GradientStripe(void);
    void GradientStripeUpdate(void);
    uint32_t DimColour(uint32_t colour);
};

#endif
