#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

/*****************/
/* Configuration */
/*****************/

// Pin the mode button is wired to
const uint8_t button_pin = 0;
// Pin LED strip is wired to
const uint8_t strip_pin = 2;
const uint8_t strip_2_pin = 8;
// Number of LEDs in strip
const uint16_t num_leds = 44;

// Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE, TWINKLE, FIRE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
  public:

    // Member Variables:
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern

    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position

    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    uint8_t Dim; // Amount to dim brightness (bit shift)
    // Heat index for fire
    uint8_t *heat;

    void (*OnComplete)();  // Callback on completion of pattern

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

    // Update the pattern
    void Update()
    {
      if ((millis() - lastUpdate) > Interval) // time to update
      {
        lastUpdate = millis();
        switch (ActivePattern)
        {
          case RAINBOW_CYCLE:
            RainbowCycleUpdate();
            break;
          case THEATER_CHASE:
            TheaterChaseUpdate();
            break;
          case COLOR_WIPE:
            ColorWipeUpdate();
            break;
          case SCANNER:
            ScannerUpdate();
            break;
          case FADE:
            FadeUpdate();
            break;
          case TWINKLE:
            TwinkleUpdate();
            break;
          case FIRE:
            FireUpdate();
            break;
            break;
          default:
            break;
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
      ActivePattern = RAINBOW_CYCLE;
      Interval = interval;
      TotalSteps = 255;
      Index = 0;
      Direction = dir;
      Dim = dim;
    }

    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
      for (int i = 0; i < numPixels(); i++)
      {
        setPixelColor(i, DimColorShift(Wheel(((i * 256 / numPixels()) + Index) & 255), Dim));
      }
      show();
      Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = COLOR_WIPE;
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
      ActivePattern = THEATER_CHASE;
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
      for (int i = 0; i < numPixels(); i++)
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
      ActivePattern = SCANNER;
      Interval = interval;
      TotalSteps = (numPixels() - 1) * 2;
      Color1 = color1;
      Index = 0;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    {
      for (int i = 0; i < numPixels(); i++)
      {
        if (i == Index) // first half of the scan
        {
          Serial.print(i);
          setPixelColor(i, Color1);
        }
        else if (i == TotalSteps - Index) // The return trip.
        {
          Serial.print(i);
          setPixelColor(i, Color1);
        }
        else  // fade to black
        {
          setPixelColor(i, DimColorShift(getPixelColor(i)));
        }
      }
      show();
      Increment();
    }

    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
      ActivePattern = FADE;
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

    void Twinkle(uint32_t color1, uint8_t interval)
    {
      ActivePattern = TWINKLE;
      Interval = interval;
      Color1 = color1;
    }

    void TwinkleUpdate()
    {
      // Fade all pixels
      for (uint8_t i = 0; i < numPixels(); i++) {
        setPixelColor(i, DimColor(getPixelColor(i), 0.8));
      }
      // Randomly choose if we are going to light a pixel
      uint8_t no_twinkle = random(0, 5);
      if (no_twinkle == 0) {
        // Light a random pixel
        setPixelColor(random(0, numPixels()), Color1);
      }
      show();
    }

    /**
     * Fire is hardcoded custom implementation for coat with dual strips. For now.
     */
    void Fire(uint8_t interval) {
      ActivePattern = FIRE;
      Interval = interval;
      heat = new uint8_t[44];
    }

    void FireUpdate() {
      uint8_t cooldown;

      // Cooldown pixels
      for (uint8_t i = 0; i < numPixels(); i++) {
        cooldown = random(0, ((55 * 10) / 22) + 2);
        heat[i] = max(0, heat[i] - cooldown);
      }

      // First strip reverse direction

      // Heat drifts up (down) and diffuses
      for (uint8_t y = 0; y < 24; y++) {
        heat[y] = (heat[y + 1] + heat[y + 2] + heat[y + 2]) / 3;
      }

      // Randomly ignite sparks near bottom (top)
      if (random(255) < 120) {
        uint8_t y = random(15, 22);
        heat[y] = heat[y] + random(160, 255);
      }

      // Second strip normal direction, but offset by 22

      // Heat drifts up and diffuses
      for (uint8_t y = 44 - 1; y >= 24; y--) {
        heat[y] = (heat[y - 1] + heat[y - 2] + heat[y - 2]) / 3;
      }

      // Randomly ignite sparks near bottom
      if (random(255) < 120) {
        uint8_t y = random(22, 29);
        heat[y] = heat[y] + random(160, 255);
      }
      
      // Set pixel colors (both strips)
      for (uint8_t i = 0; i < numPixels(); i++) {
        setPixelHeatColor(i, heat[i]);
      }

      show();
    }

    void setPixelHeatColor (int pixel, uint8_t temperature) {
      // Scale 'heat' down from 0-255 to 0-191
      uint8_t t192 = round((temperature/255.0)*191);
     
      // calculate ramp up from
      uint8_t heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252
     
      // figure out which third of the spectrum we're in:
      if( t192 > 0x80) {                     // hottest
        this->setPixelColor(pixel, this->DimColor(this->Color(255, 255, heatramp), 0.6));
      } else if( t192 > 0x40 ) {             // middle
        this->setPixelColor(pixel, this->DimColor(this->Color(255, heatramp, 0), 0.6));
      } else {                               // coolest
        this->setPixelColor(pixel, this->DimColor(this->Color(heatramp, 0, 0), 0.6));
      }
    }
    
// TODO: Meteor rain idea
//    void Meteor(uint32_t color1, uint8_t interval) {
//      ActivePattern = METEOR;
//      Interval = interval;
//      Color1 = color1;
//    }
//
//    void MeteorUpdate() {
//
//      // Fade all pixels
//      for (uint8_t i = 0; i < numPixels(); i++) {
//        if (random(10) > 5) {
//          setPixelColor(i, DimColor(getPixelColor(i), 0.75));
//        }
//      }
//
//      if (drawMeteor) {
//        
//        // Draw meteor (size 10)
//        for (uint8_t i = 0; i < 10; i++) {
//          if (meteorPosition - i >= 0) {
//            setPixelColor(meteorPosition - i, Color1);
//          }
//        }
//        
//        // Move meteor along
//        meteorPosition++;
//  
//        // If meteor is now off the strip, stop drawing it
//        if (meteorPosition > numPixels()) {
//          drawMeteor = false;
//        }
//      } else {
//        // Choose if we should draw meteor
//        if (random(100) > 99) {
//          drawMeteor = true;
//        }
//      }
//      
//      show();
//    }

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
      for (int i = 0; i < numPixels(); i++)
      {
        setPixelColor(i, color);
      }
      show();
    }
};

/*
 * Runs entire astronaut helmet.
 */
class Coat
{
  public:
    // Current display mode (pattern switching)
    uint8_t current_mode = 0;
    // List of display mode method pointers
    void (Coat::*modes[6])() = {
      &off,
      &twinkle,
      &rainbow,
      &theaterChase,
      &knightRider,
      &fire,
    };
    NeoPatterns& strip_left;
    NeoPatterns& strip_right;
    
    // Constructor
    Coat(NeoPatterns& left, NeoPatterns& right):
      strip_left(left), strip_right(right) {};

    void begin() {
      strip_left.begin();
      strip_right.begin();
      this->next();
    }

    void update() {
      strip_left.Update();
      strip_right.Update();
    }
    
    // Switch to next mode
    void next() {
      this->current_mode++;
      // Loop at max mode
      if (this->current_mode >= 6) {
        this->current_mode = 0;
      }
      // Call current mode method
      (*this.*modes[this->current_mode])();
    }

    /*********/
    /* Modes */
    /*********/

    /**
     * Turn everything off.
     */
    void off() {
      strip_left.ColorWipe(strip_left.Color(0, 0, 0), 5);
      strip_right.ColorWipe(strip_right.Color(0, 0, 0), 5);
    }

    /**
     * Rainbow cycle.
     */
    void rainbow() {
      strip_left.RainbowCycle(5, 4);
      strip_right.RainbowCycle(5, 4);
    }

    /**
     * White theater chase.
     */
    void theaterChase() {
      strip_left.TheaterChase(strip_left.Color(150, 150, 150), strip_left.Color(0, 0, 0), 80);
      strip_right.TheaterChase(strip_right.Color(150, 150, 150), strip_right.Color(0, 0, 0), 80);
    }
    /**
     * Red scanner, like Kitt in Knight Rider.
     */
    void knightRider() {
      strip_left.Scanner(strip_left.Color(255, 255, 255), 50);
      strip_right.Scanner(strip_right.Color(255, 255, 255), 50);
    }
    /**
     * Twinkle random lights.
     */
    void twinkle() {
      strip_left.Twinkle(strip_left.Color(255, 255, 255), 40);
      strip_right.Twinkle(strip_right.Color(255, 255, 255), 40);
    }

    /**
     * Fire.
     */
    void fire() {
      strip_left.Fire(60);
      // TODO: Figure out what the conflict is between running 2 strips on Fire. Shared heat var?
//      strip_right.Fire2(60);
      strip_right.ColorWipe(strip_right.Color(0, 0, 0), 5);
    }
};

/****************/
/* Global State */
/****************/

// Current drawing mode
uint8_t current_mode = 1;

NeoPatterns strip = NeoPatterns(num_leds, strip_pin, NEO_GRB + NEO_KHZ800);
NeoPatterns strip_2 = NeoPatterns(num_leds, strip_2_pin, NEO_GRB + NEO_KHZ800);
Coat coat = Coat(strip, strip_2);
Bounce mode_button = Bounce();

void setup() {
  // Init button
  pinMode(button_pin, INPUT_PULLUP);
  mode_button.attach(button_pin);
  mode_button.interval(10);

  // Init coat
  coat.begin();
}

void loop() {
  // Button
  if (mode_button.update()) {
    if (mode_button.fell()) {
      coat.next();
    }
  }

  // Coat
  coat.update();
}
//
// void draw() {
//   switch (current_mode) {
//     // TODO: Make a Coat class with 2 LED strips as inputs
//     // TODO: Move modes into Coat class, have it control LED strips with patterns
//     // TODO: Rainbow with less brightness might be better, show less of the LED spots
//     /**
//      * Pattern ideas
//      *
//      * left to right
//      * top to bottom
//      * Theater chase in opposite directions as stripes
//      * Could try fire pattern on each strip
//      * Breathe pattern, fade patches in and out of brightness
//      *
//      * LED Layout
//      * back_l  back_r
//      * 1   44  1   44
//      * 2   43  2   43
//      * 3   42  3   42
//      * 4   41  4   41
//      * 5   40  5   40
//      * 6   39  6   39
//      * 7   38  7   38
//      * 8   37  8   37
//      * 9   36  9   36
//      * 10  35  10  35
//      * 11  34  11  34
//      * 12  33  12  33
//      * 13  32  13  32
//      * 14  31  14  31
//      * 15  30  15  30
//      * 16  29  16  29
//      * 17  28  17  28
//      * 18  27  18  27
//      * 19  26  19  26
//      * 20  25  20  25
//      * 21  24  21  24
//      * 22  23  22  23
//      */
//     case 1: Modes::rainbow(strip);
//       Modes::rainbow(strip_2);
//       break;
//     case 2: Modes::twinkle(strip);
//       Modes::twinkle(strip_2);
//       break;
//     case 3: Modes::knightRider(strip);
//       Modes::knightRider(strip_2);
//       break;
//     case 4: Modes::theaterChase(strip);
//       Modes::theaterChase(strip_2);
//       break;
//     case 5: Modes::flashlight(strip);
//       Modes::flashlight(strip_2);
//       break;
//     default: Modes::off(strip);
//       Modes::off(strip_2);
//       current_mode = 0;
//       break;
//   }
// }
