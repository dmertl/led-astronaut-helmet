#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

/*****************/
/* Configuration */
/*****************/

// Pin the mode button is wired to
const uint8_t button_pin = 8;
// Pin LED strip is wired to
const uint8_t strip_pin = 10;
// Number of LEDs in strip
const uint16_t num_leds = 26;

// Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
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
        setPixelColor(i, DimColor(Wheel(((i * 256 / numPixels()) + Index) & 255), Dim));
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
          setPixelColor(i, DimColor(getPixelColor(i)));
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
    uint32_t DimColor(uint32_t color, uint8_t shift = 1)
    {
      uint32_t dimColor = Color(Red(color) >> shift, Green(color) >> shift, Blue(color) >> shift);
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

/****************/
/* Global State */
/****************/

// Current drawing mode
uint8_t current_mode = 1;

NeoPatterns strip = NeoPatterns(num_leds, strip_pin, NEO_GRB + NEO_KHZ800);

Bounce mode_button = Bounce();

void setup() {
  // Init button
  pinMode(button_pin, INPUT_PULLUP);
  mode_button.attach(button_pin);
  mode_button.interval(10);

  // Init LED strip
  strip.begin();
  draw();
}

void loop() {

  // Button
  if (mode_button.update()) {
    if (mode_button.fell()) {
      current_mode++;
    }
    draw();
  }

  // LED Strip
  strip.Update();
}

void draw() {
  switch (current_mode) {
    // TODO: Better brightness control for rainbow, use a float
    case 1: strip.RainbowCycle(5, 3);
      break;
    case 2: strip.RainbowCycle(5, 2);
      break;
    case 3: strip.RainbowCycle(5, 1);
      break;
    case 4: strip.ColorWipe(strip.DimColor(strip.Color(255, 255, 255), 3), 10);
      break;
    case 5: strip.ColorWipe(strip.DimColor(strip.Color(255, 255, 255), 0), 10);
      break;
    case 6: strip.TheaterChase(strip.Color(255, 255, 255), strip.Color(0, 0, 0), 80);
      break;
    case 7: strip.TheaterChase(strip.Color(255, 255, 255), strip.Color(0, 0, 0), 40);
      break;
    case 8: strip.Scanner(strip.Color(200, 200, 200), 50);
      break;
    case 9: strip.Scanner(strip.Color(200, 200, 200), 100);
      break;
    case 10: strip.Scanner(strip.Color(200, 0, 0), 100);
      break;
    case 11: strip.Scanner(strip.Color(0, 0, 200), 100);
      break;
    default: strip.ColorWipe(strip.Color(0, 0, 0), 5);
      current_mode = 0;
      break;
  }
}
