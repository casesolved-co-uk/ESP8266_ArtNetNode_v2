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

FastLED 14 channel pixel FX
*/


#ifndef WS_FX
#define WS_FX

#include <FastLED.h>
#include "debugLog.h"

#define MIN_FXINTERVAL 1 // milliseconds
#define MAX_FXINTERVAL 100
#define REFRESH_INTERVAL 5
#define DELAY_MULTIPLIER 10 // multiply up to 50ms
#define FADE_STEP 10 // decrement brightness step

class pixPatterns {
  // see FastLED colorutils.h
  enum FXPattern {
    STATIC,
    RAINBOW_CYCLE,
    THEATER_CHASE,
    TWINKLE,
    GRADIENT_STRIPE,
    KITT,
    NONE
  };

  enum FXFade {
    FADE_DISABLE=0,
    FADE_TOBLACK=1,
    FADE_TOCOL1=2
  };

  enum FXChannelMap {
    iIntensity=0,
    iEffect=1,
    iSpeed=2,
    iPosition=3,
    iSize1=4,
    iColour1=5,
    iSize2=8,
    iColour2=9,
    iFade=12,
    iDelay=13
  };

  public:
    FXPattern  ActivePattern;     // which pattern is running

    unsigned long Interval;       // milliseconds between updates
    unsigned long nextUpdate;     // next update of position
    unsigned long nextRefresh;    // next LED output
    unsigned long delayEnd;       // next FX start

    CRGB Colour1, Colour2;        // Colours pre-intensity
    CRGBPalette16 Palette;        // palette for suitable effects
    uint16_t TotalSteps;          // total number of steps in the pattern
    uint16_t Index;               // current step within the pattern
    uint8_t Speed;                // speed of effect (0 = stop, -ve is reverse, +ve is forwards)
    uint8_t Size1, Size, Pos;     // size, fading & position for static looks
    uint8_t Intensity;
    uint8_t FadeCount, FadeCounter, FadeStep, FadeType;
    uint16_t Delay;               // REFRESH_INTERVAL * DELAY_MULTIPLIER * Delay

    CLEDController* controller;  // FastLED controller

    // Methods
    pixPatterns(CLEDController* controller);
    void DMXSet(byte* buffer);
    bool Update(void);
    void Increment(void);
    void setFade(uint8_t f);
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
    void Kitt(void);
    void KittUpdate(void);
};

#endif
