#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

/*
 *  Two sets of strips
 *  Helmet strip is under the white helmet portion.
 *  Face strip is on the face plate.
 *  
 *  Modes:
 *  
 *  - Rainbow helmet
 *  - Rainbow all
 *  - Twinkle
 *  - Scanner helmet
 *  - Scanner face
 *  - Vertical Strobe
 *  - Night mode (red light)
 *  - Flashlight
 *  - Fire helmet
 *  - Rainbow fire face
 *  - Blinky
 *  - Police mode
  */

/*****************/
/* Configuration */
/*****************/

// Pin the mode button is wired to
const uint8_t button_pin = 8;
// Pin helmet LED strip is wired to
const uint8_t helmet_strip_pin = 2;
// Number of LEDs in helmet strip
const uint16_t helmet_num_leds = 26;
// Pin face shield LED strip is wired to
const uint8_t face_strip_pin = 10;
// Number of LEDs in face shield strip
// 13 + 13 + 12 + 8
const uint16_t face_num_leds = 46;

// Patern directions supported:
enum  direction { FORWARD, REVERSE };

class PixelGrid
{
  public:
    uint8_t static const NA = 255;
    uint8_t width = NULL;
    uint8_t height = NULL;
    uint8_t *grid;
    uint8_t y_offset = NULL;

    PixelGrid(uint8_t width, uint8_t height, uint8_t *grid)
    :width(width), height(height), grid(grid) {
        this->y_offset = this->width * (this->height - 1);
    };

    uint8_t get(uint8_t x, uint8_t y)
    {
      uint8_t pos = this->y_offset - (this->width * y) + x;
      return this->grid[pos];
    }

    uint8_t numPixels()
    {
      return this->width * this->height;
    }
};

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
  public:
    // Pointer to active pattern update method
    void (NeoPatterns::*ActivePattern)() = NULL;
    // Direction to run the pattern
    direction Direction;
    // ms between updates
    unsigned long Interval;
    // Last update of position
    unsigned long lastUpdate;
    // Colors in pattern
    uint32_t Color1, Color2;
    // Total number of steps in the pattern
    uint16_t TotalSteps;
    // Current step within the pattern
    uint16_t Index;
    // Amount to dim brightness (bit shift)
    uint8_t Dim;
    // Heat index for fire
    uint8_t *heat;
    // Pixel grid
    PixelGrid *Grid = NULL;
    // On complete callback
    void (*OnComplete)();

    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type)
      : Adafruit_NeoPixel(pixels, pin, type)
    {
      OnComplete = NULL;
    }

    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
      : Adafruit_NeoPixel(pixels, pin, type)
    {
      OnComplete = callback;
    }

    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, PixelGrid &grid)
      : NeoPatterns(pixels, pin, type)
    {
      this->Grid = &grid;
    }

    // Update the pattern
    void Update()
    {
      if ((millis() - lastUpdate) > Interval) // time to update
      {
        lastUpdate = millis();
        if (ActivePattern != NULL) {
          (*this.*ActivePattern)();
        }
      }
    }

    // Increment the Index and reset at the end
    void Increment()
    {
      if (Direction == FORWARD)
      {
        Index++;
        if (Index >= TotalSteps)
        {
          Index = 0;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback
          }
        }
      }
      else // Direction == REVERSE
      {
        --Index;
        if (Index <= 0)
        {
          Index = TotalSteps - 1;
          if (OnComplete != NULL)
          {
            OnComplete(); // call the comlpetion callback
          }
        }
      }
    }

    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, uint8_t dim = 0, direction dir = FORWARD)
    {
      ActivePattern = &RainbowCycleUpdate;
      Interval = interval;
      TotalSteps = 255;
      Index = 0;
      Direction = dir;
      Dim = dim;
    }

    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
      for (uint8_t i = 0; i < numPixels(); i++)
      {
        setPixelColor(i, DimColorShift(Wheel(((i * 256 / numPixels()) + Index) & 255), Dim));
      }
      show();
      Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = &ColorWipeUpdate;
      Interval = interval;
      TotalSteps = numPixels();
      Color1 = color;
      Index = 0;
      Direction = dir;
    }

    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
      setPixelColor(Index, Color1);
      show();
      Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = &TheaterChaseUpdate;
      Interval = interval;
      TotalSteps = numPixels();
      Color1 = color1;
      Color2 = color2;
      Index = 0;
      Direction = dir;
    }

    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
      for (uint8_t i = 0; i < numPixels(); i++)
      {
        if ((i + Index) % 3 == 0)
        {
          setPixelColor(i, Color1);
        }
        else
        {
          setPixelColor(i, Color2);
        }
      }
      show();
      Increment();
    }

    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &ScannerUpdate;
      Interval = interval;
      TotalSteps = (numPixels() - 1) * 2;
      Color1 = color1;
      Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    {
      for (uint8_t i = 0; i < numPixels(); i++)
      {
        if (i == Index) // first half of the scan
        {
          setPixelColor(i, Color1);
        }
        else if (i == TotalSteps - Index) // The return trip.
        {
          setPixelColor(i, Color1);
        }
        else  // fade to black
        {
          setPixelColor(i, DimColor(getPixelColor(i)));
        }
      }
      show();
      Increment();
    }

    // Face scanner init
    void FaceScanner(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &FaceScannerUpdate;
      Interval = interval;
      TotalSteps = this->Grid->width * 2;
      Color1 = color1;
      Index = 0;
    }

    // Face scanner update
    void FaceScannerUpdate()
    {
      for (uint8_t x = 0; x < this->Grid->width; x++) {
        for (uint8_t y = 0; y < this->Grid->height; y++) {
          uint8_t pixel = this->Grid->get(x, y);
          if (pixel != PixelGrid::NA) {
            // Light up curent column
            if (x == Index || x == TotalSteps - Index) {
              setPixelColor(pixel, Color1);
            } else {
              setPixelColor(pixel, DimColor(getPixelColor(pixel)));
            }
          }
        }
      }
      show();
      Increment();
    }

    // Initialize for a strobe on face
    void StrobeFace(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &StrobeFaceUpdate;
      Interval = interval;
      // 8 for each row (including helmet)
      // 4 for extra pause before flash
      // 14 for extra pause before reset
      TotalSteps = 58;
      Color1 = color1;
      Index = 0;
    }

    // Scan vertically, then flash at the end
    void StrobeFaceUpdate()
    {
      if (Index == 44) {
        // Flash
        for (uint8_t i = 0; i < this->numPixels(); i++) {
          setPixelColor(i, Color1);
        }
      } else {
        // Vertical scan
        // Dim all pixels
        for (uint8_t i = 0; i < 46; i++) {
          setPixelColor(i, DimColor(getPixelColor(i)));
        }
        // Light a row every 8 steps
        if (Index % 8 == 0) {
          // Current row
          uint8_t row = Index / 8;
          // Only first 4 are on the face
          if (row < 4) {
            // Light every pixel in the current row
            for (uint8_t i = 0; i < this->Grid->width; i++) {
              setPixelColor(this->Grid->get(i, row), Color1);
            }
          }
        }
      }
      show();
      Increment();
    }

    // Initialize for a strobe on helmet
    void StrobeHelmet(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &StrobeHelmetUpdate;
      Interval = interval;
      TotalSteps = 58;
      Color1 = color1;
      Index = 0;
    }

    // Strobe pattern component for the helmet
    void StrobeHelmetUpdate()
    {
      if (Index == 44 || Index == 32) {
        // Flash
        for (uint8_t i = 0; i < numPixels(); i++) {
          setPixelColor(i, Color1);
        }
      } else {
        for (uint8_t i = 0; i < numPixels(); i++) {
          setPixelColor(i, DimColor(getPixelColor(i)));
        }
      }
      show();
      Increment();
    }

    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = &FadeUpdate;
      Interval = interval;
      TotalSteps = steps;
      Color1 = color1;
      Color2 = color2;
      Index = 0;
      Direction = dir;
    }

    // Update the Fade Pattern
    void FadeUpdate()
    {
      uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
      uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
      uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
      ColorSet(Color(red, green, blue));
      show();
      Increment();
    }

    // Twinkle pattern
    void Twinkle(uint32_t color, uint8_t interval)
    {
      ActivePattern = &TwinkleUpdate;
      Interval = interval;
      Color1 = color;
    }

    // Twinkle pattern update
    void TwinkleUpdate()
    {
      // Randomly choose if we are going to light a pixel
      uint8_t no_twinkle = random(0, 3);
      uint8_t chosen = NULL;
      if (no_twinkle == 0) {
        // Randomly choose pixel to twinkle
        chosen = random(0, numPixels());
      }
      
      for (uint8_t i = 0; i < numPixels(); i++) {
        if (chosen != NULL && i == chosen) {
          // Light chosen pixel
          setPixelColor(i, Color1);
        } else {
          // Fade all other pixels
          setPixelColor(i, DimColor(getPixelColor(i), 0.75));
        }
      }
      show();
    }

    void Fire(uint8_t interval) {
      ActivePattern = &FireUpdate;
      Interval = interval;
      heat = new uint8_t[this->numPixels()];
    }

    void FireUpdate() {
      uint8_t cooldown;

      // Cooldown pixels
      for (uint8_t i = 0; i < this->numPixels(); i++) {
        cooldown = random(0, ((55 * 10) / this->numPixels()) + 2);
        heat[i] = max(0, heat[i] - cooldown);
      }

      // Heat drifts up and diffuses
      for (uint8_t y = this->numPixels() - 1; y >= 2; y--) {
        heat[y] = (heat[y - 1] + heat[y - 2] + heat[y - 2]) / 3;
      }

      // Randomly ignite sparks near bottom
      if (random(255) < 120) {
        uint8_t y = random(7);
        heat[y] = heat[y] + random(160, 255);
      }
      
      // Set pixel colors
      for (uint8_t i = 0; i < this->numPixels(); i++) {
        setPixelHeatColor(i, heat[i]);
      }

      this->show();
    }

    void setPixelHeatColor (int pixel, uint8_t temperature) {
      // Scale 'heat' down from 0-255 to 0-191
      uint8_t t192 = round((temperature/255.0)*191);
     
      // calculate ramp up from
      uint8_t heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252
     
      // figure out which third of the spectrum we're in:
      if( t192 > 0x80) {                     // hottest
        this->setPixelColor(pixel, this->DimColor(this->Color(255, 255, heatramp), 0.75));
      } else if( t192 > 0x40 ) {             // middle
        this->setPixelColor(pixel, this->DimColor(this->Color(255, heatramp, 0), 0.75));
      } else {                               // coolest
        this->setPixelColor(pixel, this->DimColor(this->Color(heatramp, 0, 0), 0.75));
      }
    }

    void RainbowFire(uint8_t interval) {
      ActivePattern = &RainbowFireUpdate;
      Interval = interval;
      TotalSteps = 255;
      Index = 0;
      heat = new uint8_t[this->numPixels()];
    }

    void RainbowFireUpdate() {
      uint8_t cooldown;

      // Rotate color
      Color1 = Wheel(Index);

      // Cooldown pixels
      for (uint8_t i = 0; i < this->numPixels(); i++) {
        cooldown = random(0, ((55 * 10) / this->numPixels()) + 2);
        heat[i] = max(0, heat[i] - cooldown);
      }

      // Heat drifts up and diffuses
      for (uint8_t y = this->numPixels() - 1; y >= 2; y--) {
        heat[y] = (heat[y - 1] + heat[y - 2] + heat[y - 2]) / 3;
      }

      // Randomly ignite sparks near bottom
      if (random(255) < 120) {
        uint8_t y = random(7);
        heat[y] = heat[y] + random(160, 255);
      }
      
      // Set pixel colors
      for (uint8_t i = 0; i < this->numPixels(); i++) {
        setPixelRainbowHeatColor(i, heat[i], Color1);
      }

      this->show();
      Increment();
    }

    void setPixelRainbowHeatColor (int pixel, uint8_t temperature, uint32_t color) {
      // Scale 'heat' down from 0-255 to 0-191
      uint8_t t192 = round((temperature/255.0)*191);
     
      // calculate ramp up from
      uint8_t heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252
      uint16_t heatscale;
     
      // Keep the same scale as fire, but split it between R G B based on current color
      if( t192 > 0x80) {                     // hottest
        heatscale = heatramp + 255 + 255;
      } else if( t192 > 0x40 ) {             // middle
        heatscale = heatramp + 255;
      } else {                               // coolest
        heatscale = heatramp;
      }
      this->setPixelColor(pixel, this->DimColor(this->Color(Red(color) / 255.0 * heatscale, Green(color) / 255.0 * heatscale, Blue(color) / 255.0 * heatscale), 0.5));
    }

    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
      return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
      return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
      return color & 0xFF;
    }

    // Return color, dimmed by 75% (used by scanner)
    uint32_t DimColorShift(uint32_t color, uint8_t shift = 1)
    {
      uint32_t dimColor = Color(Red(color) >> shift, Green(color) >> shift, Blue(color) >> shift);
      return dimColor;
    }

    // Return color, dimmed to a percentage, default 25% of original
    uint32_t DimColor(uint32_t color, float fade = 0.25)
    {
      uint32_t dimColor = Color(Red(color) * fade, Green(color) * fade, Blue(color) * fade);
      return dimColor;
    }

    // Return color, brightened by a %
    uint32_t BrightenColor(uint32_t color, float brighten = 1.25)
    {
      uint32_t brightColor = Color(min(255, Red(color) * brighten), min(255, Green(color) * brighten), min(255, Blue(color) * brighten));
      return brightColor;
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
      WheelPos = 255 - WheelPos;
      if (WheelPos < 85)
      {
        return Color(255 - WheelPos * 3, 0, WheelPos * 3);
      }
      else if (WheelPos < 170)
      {
        WheelPos -= 85;
        return Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      else
      {
        WheelPos -= 170;
        return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
      }
    }

    // Reverse direction of the pattern
    void Reverse()
    {
      if (Direction == FORWARD)
      {
        Direction = REVERSE;
        Index = TotalSteps - 1;
      }
      else
      {
        Direction = FORWARD;
        Index = 0;
      }
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
      for (uint8_t i = 0; i < numPixels(); i++)
      {
        setPixelColor(i, color);
      }
      show();
    }

    void Off()
    {
      ActivePattern = NULL;
      for (uint8_t i = 0; i < numPixels(); i++) {
        setPixelColor(i, this->Color(0, 0, 0));
      }
      show();
    }
};

/*
 * Runs entire astronaut helmet.
 */
class Helmet
{
  public:
    // Current display mode (pattern switching)
    uint8_t current_mode = 0;
    // List of display mode method pointers
    void (Helmet::*modes[13])() = {
      &off,
      &rainbowHelmet,
      &rainbowAll,
      &twinkle,
      &scannerHelmet,
      &scannerFace,
      &strobe,
      &redFlashlight,
      &flashlight,
      &fire,
      &rainbowFire,
      &blinky,
      &police
    };
    // LED strip in helmet
    NeoPatterns& helmet_strip;
    // LED strip on face shield
    NeoPatterns& face_strip;
    
    // Constructor
    Helmet(NeoPatterns& helmet, NeoPatterns& face):
      helmet_strip(helmet), face_strip(face) {};

    // Begin
    void begin() {
      helmet_strip.begin();
      face_strip.begin();
      this->next();
    }

    // Update
    void Update() {
      helmet_strip.Update();
      face_strip.Update();
    }
    
    // Switch to next mode
    void next() {
      this->current_mode++;
      // Loop at max mode
      if (this->current_mode > 12) {
        this->current_mode = 0;
      }
      // Call current mode method
      (*this.*modes[this->current_mode])();
    }

    // Helmet only rainbow pattern
    void rainbowHelmet() {
      helmet_strip.RainbowCycle(10, 3);
      face_strip.Off();
    }

    // Helmet and face rainbow pattern
    void rainbowAll() {
      helmet_strip.RainbowCycle(10, 3);
      face_strip.RainbowCycle(10, 3); 
    }

    // Rapidly flashing all the lights in white
    void blinky() {
      helmet_strip.TheaterChase(helmet_strip.Color(128, 128, 128), helmet_strip.Color(0, 0, 0), 40);
      face_strip.TheaterChase(face_strip.Color(128, 128, 128), face_strip.Color(0, 0, 0), 40);
    }

    // Scan white back and forth horizontally on the helmet
    void scannerHelmet() {
      helmet_strip.Scanner(helmet_strip.Color(200, 200, 200), 80);
      face_strip.Off();
    }

    // Scan white back and forth horizontally on the face shield
    void scannerFace() {
      helmet_strip.Off();
      face_strip.FaceScanner(face_strip.Color(200, 200, 200), 80);
    }

    // Red flashlight to help preserve your night vision
    void redFlashlight() {
      helmet_strip.ColorWipe(helmet_strip.Color(200, 0, 0), 10);
      face_strip.Off();
    }

    // White flashlight
    void flashlight() {
      helmet_strip.ColorWipe(helmet_strip.Color(128, 128, 128), 10);
      face_strip.Off();
    }

    // Red and blue blinky on all the lights
    void police() {
      helmet_strip.TheaterChase(helmet_strip.Color(255, 0, 0), helmet_strip.Color(0, 0, 255), 100);
      face_strip.TheaterChase(face_strip.Color(255, 0, 0), face_strip.Color(0, 0, 255), 100);
    }

    // Flash each horizontal strip from bottom to top, then flash all LEDs, all in white
    void strobe() {
      helmet_strip.StrobeHelmet(helmet_strip.Color(255, 255, 255), 60);
      face_strip.StrobeFace(face_strip.Color(255, 255, 255), 60);
    }

    // Twinkle random LEDs
    void twinkle() {
      helmet_strip.Twinkle(face_strip.Color(255, 255, 255), 40);
      face_strip.Twinkle(face_strip.Color(255, 255, 255), 40);
    }

    // Fire
    void fire() {
      helmet_strip.Fire(40);
      face_strip.Off();
    }

    // Rainbow fire
    void rainbowFire() {
      helmet_strip.Off();
      face_strip.RainbowFire(40);
    }

    // Turn off all the LEDs
    void off() {
      helmet_strip.Off();
      face_strip.Off();
    }
};

/****************/
/* Global State */
/****************/

// Helmet is a single LED strip
NeoPatterns helmet_strip = NeoPatterns(helmet_num_leds, helmet_strip_pin, NEO_GRB + NEO_KHZ800);
// Face LEDs
uint8_t const NA = PixelGrid::NA;
// Pixel grid pattern for face LED strip
// Strip is 13, 13, 12, 8 from bottom to top in zig zag pattern
uint8_t grid_pixels[] = {
  NA, NA, 45, 44, 43, 42, 41, 40, 39, 38, NA, NA, NA,
  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, NA,
  25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13,
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12
};
PixelGrid grid = PixelGrid(13, 4, grid_pixels);
NeoPatterns face_strip = NeoPatterns(face_num_leds, face_strip_pin, NEO_GRB + NEO_KHZ800, grid);
// Helmet
Helmet helmet = Helmet(helmet_strip, face_strip);

Bounce mode_button = Bounce();

void setup() {
  // Init button
  pinMode(button_pin, INPUT_PULLUP);
  mode_button.attach(button_pin);
  mode_button.interval(10);

  // Init helmet
  helmet.begin();
}

void loop() {
  // Button
  if (mode_button.update()) {
    if (mode_button.fell()) {
      helmet.next();
    }
  }
  // LEDs
  helmet.Update();
}
