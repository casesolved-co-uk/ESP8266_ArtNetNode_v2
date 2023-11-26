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


#include "wsFX.h"

pixPatterns::pixPatterns(CLEDController* c) {
  controller = c;
  nextUpdate = 0;
  nextRefresh = 0;
  delayEnd = 0;
  Intensity = 0;
  Speed = 0;
  TotalSteps = 100;
  ActivePattern = NONE;
}

void pixPatterns::DMXSet(byte* buffer) {
  Intensity = buffer[iIntensity];
  setSpeed(buffer[iSpeed]);
  Pos = buffer[iPosition];
  Size = buffer[iSize1];
  Colour1 = CRGB(buffer[iColour1], buffer[iColour1+1], buffer[iColour1+2]); // background
  Size1 = buffer[iSize2];
  Colour2 = CRGB(buffer[iColour2], buffer[iColour2+1], buffer[iColour2+2]); // active
  setFade(buffer[iFade]);
  Delay = REFRESH_INTERVAL * DELAY_MULTIPLIER * buffer[iDelay]; // milliseconds

  // depends on other params
  setFX(buffer[iEffect]);
}

// when calling do setup last, after setting other params
void pixPatterns::setFX(uint8_t fx) {
  FXPattern before = ActivePattern;

  if (fx < 25)
    Static();
  else if (fx < 50)
    RainbowCycle();
  else if (fx < 75)
    TheaterChase();
  else if (fx < 100)
    Twinkle();
  else if (fx < 125)
    GradientStripe();
  else if (fx < 150)
    Kitt();

  // resets
  if (before != ActivePattern) {
    Index = 0;
    FadeCounter = FadeCount;
    controller->clearLedData();
  }
}

// Called in main loop
bool pixPatterns::Update(void) {
  bool ret = false;
  unsigned long now = millis();

  if (now >= nextUpdate && now >= delayEnd) { // time to update
    nextUpdate = now + Interval;
    ret = true;

    switch(ActivePattern) {
      case STATIC:
        StaticUpdate();
        break;
      case RAINBOW_CYCLE:
        RainbowCycleUpdate();
        break;
      case THEATER_CHASE:
        TheaterChaseUpdate();
        break;
      case TWINKLE:
        TwinkleUpdate();
        break;
      case GRADIENT_STRIPE:
        GradientStripeUpdate();
        break;
      case KITT:
        KittUpdate();
        break;
      default: // NONE - have not been initialised
        return false;
    }
  }

  now = millis();
  // update more frequently for dithering at low brightness
  if (now >= nextRefresh) {
    if (ret)
      yield(); // we have spent time updating so yield first
    nextRefresh = now + REFRESH_INTERVAL;

  #ifdef TRIGGER_LEDS
    digitalWrite(TRIGGER, HIGH);
  #endif

    if (FadeStep && FadeType) {
      if (!FadeCounter--) { // we reached zero
        FadeCounter = FadeCount; // can be zero for every loop
        if (FadeType == FADE_TOBLACK) {
          fadeToBlackBy(controller->leds(), controller->size(), FadeStep);
        } else if (FadeType == FADE_TOCOL1) {
          fadeUsingColor(controller->leds(), controller->size(), Colour1);
        }
      }
    }
    controller->showLeds(Intensity);

  #ifdef TRIGGER_LEDS
    digitalWrite(TRIGGER, LOW);
  #endif
  }

  return ret;
}
  
// Increment the Index and reset at the end
// dead patch between 123 and 131
void pixPatterns::Increment(void) {
  if (Speed > 131) {
    Index++;
    
    if (Index >= TotalSteps) {
      Index = 0;
      delayEnd = millis() + Delay;
    }
  
  } else if (Speed < 123) {
    if (Index == 0 || Index > TotalSteps) {
      Index = TotalSteps - 1;
      delayEnd = millis() + Delay;
    } else {
      Index--;
    }
  }
}

// count of REFRESH_INTERVAL loops then decrement by FADE_STEP
void pixPatterns::setFade(uint8_t f) {
  FadeStep = f==255 ? 0 : 1;
  FadeCount = f;
}

void pixPatterns::setSpeed(uint8_t s) {
  // Index reset mode
  if (s < 20) {
    if (Speed != s)
      Index = 0;
    Speed = s;
    Interval = MIN_FXINTERVAL;

  // Indexed non-reset mode
  } else if (s > 235) {
    Interval = MIN_FXINTERVAL;
    Speed = s;

  // Chase mode
  } else {
    Speed = s;
    if (Speed > 127)
      Interval = map(Speed, 132, 235, MAX_FXINTERVAL, MIN_FXINTERVAL);
    else
      Interval = map(Speed, 20, 122, MIN_FXINTERVAL, MAX_FXINTERVAL);
  }
}


// ################################# EFFECTS ###################################


// Initialize static looks
void pixPatterns::Static(void) {
  if (ActivePattern == STATIC)
    return;
  
  ActivePattern = STATIC;
  TotalSteps = controller->size();
  FadeType = FADE_DISABLE;
}
// Update the static look
void pixPatterns::StaticUpdate(void) {

  // Calculate the values to use mapped to the number of pixels we have
  uint16_t mSize = map(Size, 0, 255, 2, TotalSteps);           // Overall size
  uint16_t mSize1 = map(Size1, 0, 255, 0, mSize);                               // Colour1 size
//  uint16_t mFade = map(Fade, 0, 255, 0, (mSize/2));                             // Colour fade size

  // Calculate the position offset - the shapes are centered using Pos
  uint16_t midPoint = map(Pos, 0, 255, 0, TotalSteps);
  uint16_t mPos = mSize - (midPoint - (mSize/2) - (uint16_t)((midPoint - (mSize/2)) / mSize) * mSize) + Index;
  DEBUG_MSG("%u", mPos);
  
  for(uint16_t p = 0; p < TotalSteps; p++) {
    uint16_t i = (p + mPos) % mSize;
    if (i < mSize1)
      controller->leds()[p] = Colour1;
    else
      controller->leds()[p] = Colour2;
  }
  Increment();
}


// Initialize for a RainbowCycle
void pixPatterns::RainbowCycle(void) {
  if (ActivePattern == RAINBOW_CYCLE)
    return;
  
  ActivePattern = RAINBOW_CYCLE;
  TotalSteps = 255;
  FadeType = FADE_DISABLE;
}
// Update the Rainbow Cycle Pattern
void pixPatterns::RainbowCycleUpdate(void) {
  uint16_t hueStep = 256 / (Size + 2); // 0 -> 128, 255 -> 0

  fill_rainbow(controller->leds(), controller->size(), (Index + Pos) & 0xFF, hueStep);
  Increment();
}


// Initialize for a Theater Chase
void pixPatterns::TheaterChase(void) {
  if (ActivePattern == THEATER_CHASE)
    return;
  
  ActivePattern = THEATER_CHASE;
  TotalSteps = controller->size();
  FadeType = FADE_TOCOL1;
}
// Update the Theater Chase Pattern
void pixPatterns::TheaterChaseUpdate(void) {
  uint8_t mSize = map(Size, 0, 255, 3, 50);
  uint8_t a = (Index / map(mSize, 3, 50, 8, 2)) + map(Pos, 0, 255, mSize, 0);
  
  for(int i = 0; i < TotalSteps; i++) {
    if ((i + a) % mSize == 0)
      controller->leds()[i] = Colour1;
    else
      controller->leds()[i] = Colour2;
  }
  Increment();
}


// Initialize for Twinkle
void pixPatterns::Twinkle(void) {
  if (ActivePattern == TWINKLE)
    return;
  
  ActivePattern = TWINKLE;
  TotalSteps = 3;
  FadeType = FADE_TOCOL1;

  randomSeed(analogRead(0));
}
// Update the Twinkle Pattern
void pixPatterns::TwinkleUpdate(void) {
  // Fade resets the twinkles
  // Set strip to Colour 1
//  if (Index % 3 == 0 || Speed < 20 || Speed > 235) {
//    fill_solid(controller->leds(), controller->size(), Colour1);
//  }

  // Make twinkles
  if (Index % 3 == 0 && Speed >= 20 && Speed <= 235) {
    uint16_t numTwinks = map(Size, 0, 255, 1, (controller->size() / 10));
    for (uint8_t n = 0; n < numTwinks; n++)
      controller->leds()[random(0, controller->size())] = Colour2;
  }
  
  Increment();
}


//TODO: fix
void pixPatterns::GradientStripe() {
  if (ActivePattern != GRADIENT_STRIPE) {
    ActivePattern = GRADIENT_STRIPE;
    TotalSteps = controller->size();
    FadeType = FADE_TOBLACK;
  }

  Palette = CRGBPalette16(Colour1, Colour2);
}
void pixPatterns::GradientStripeUpdate() {
  uint16_t mStart = map(Pos + Index, 0, 255, 0, TotalSteps-1);
  uint16_t mSize = map(Size, 0, 255, 1, TotalSteps);
  uint16_t remaining = TotalSteps - mStart;
  if (remaining < mSize)
    mSize = remaining;

  //L pointer to the LED array to fill, N number of LEDs, startIndex in the palette, incIndex how much to increment the palette color index per LED
  fill_palette(&(controller->leds()[mStart]), mSize, 0, 256 / mSize, Palette, 255, LINEARBLEND);
  //fill_palette_circular(controller->leds(), mSize, mPos, palette);
  Increment();
}


void pixPatterns::Kitt() {
  if (ActivePattern != KITT) {
    ActivePattern = KITT;
    TotalSteps = controller->size() * 2;
    FadeType = FADE_TOCOL1;
  }
}
void pixPatterns::KittUpdate() {
  uint16_t halfway = controller->size();
  uint16_t start = map(Pos, 0, 255, 0, halfway);
  uint16_t idx;

  if (Index < halfway) {
    idx = Index;
  } else {
    idx = halfway - (Index % halfway) - 1;
  }

  controller->leds()[(start + idx) % halfway] = Colour2;
  Increment();
}
