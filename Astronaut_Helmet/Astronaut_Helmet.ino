#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

/*
 *  Two sets of strips
 *  Helmet strip is under the white helmet portion.
 *  Face strip is on the face plate.
 *  Modes:
 *  - 1 - Rainbow helmet
 *  - 2 - Rainbow all
 *  - 3 - All blinky
 *  - 4 - Scanner helmet
 *  - 5 - Scanner face
 *  - 6 - Vertical Scan & Flash
 *  - 7 - Night mode (red light)
 *  - 8 - Flashlight
 *  - 9 - Police mode
 *  - 10 - Off
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

// NeoPattern Class - derived from the Adafruit_NeoPixel class
/**
 * The pattern class is great, but we need patterns that we can coordinate
 * across multiple strips. Importantly we need a single timer to coordinate
 * pattern steps. Areas where it gets complex is patterns that rely on pixel
 * count. For example scanner needs to go end to end. Maybe Helmet can
 * differentiate between single and coordinated patterns.

 Init, set each strip vars
   Helmet can coordinate init between strips
 Update, call each strip update method
   Strip update can handle last update and interval
   Strip update will call pattern specific function
   Coordination between strips will still rely on using same vars
     For example, helmet flash does nothing until last index
     Has to be a separate function

 */
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
      ActivePattern = &ScannerUpdate;
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

    // Initialize for a FACE_SCANNNER
    void FaceScanner(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &FaceScannerUpdate;
      Interval = interval;
      // Hardcoded to length of longest strip
      TotalSteps = (13 - 1) * 2;
      Color1 = color1;
      Index = 0;
    }

    // Update the Face Scanner Pattern
    // Customized scanner to do entire face strip as a grid
    // 13 LEDs in longest 2 strips
    // 12 LEDs in middle, then 8 in top
    void FaceScannerUpdate()
    {
      // Lower strip goes backwards (13 LEDs, 13-25)
      int lower_i = 25;
      // i = bottom strip (13 LEDs, 0-12)
      for (int i = 0; i < 13; i++)
      {
        if (i == Index || i == TotalSteps - Index)
        {
          setPixelColor(i, Color1);
          setPixelColor(lower_i, Color1);
        }
        else  // fade to black
        {
          setPixelColor(i, DimColor(getPixelColor(i)));
          setPixelColor(lower_i, DimColor(getPixelColor(lower_i)));
        }
        lower_i--;
      }
      // Middle strip forwards (12 LEDs, 26-37)
      // Technically we miss one LED step at the end, but it looks fine
      for (int i = 0; i < 12; i++) {
        if (i == Index || i == TotalSteps - Index) {
          setPixelColor(i + 26, Color1);
        } else {
          setPixelColor(i + 26, DimColor(getPixelColor(i + 26)));
        }
      }
      // Top strip backward & offset (8 LEDs, 38-45)
      // Offset by 2 pixel on each side
      for (int i = 0; i < 8; i++) {
        if (i == Index - 2 || i == TotalSteps - Index - 2) {
          setPixelColor(45 - i, Color1);
        } else {
          setPixelColor(45 - i, DimColor(getPixelColor(45 - i)));
        }
      }
      show();
      Increment();
    }

    // Initialize for a VERTICAL_SCAN_FLASH_FACE
    void VerticalScanFlashFace(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &VerticalScanFlashFaceUpdate;
      Interval = interval;
      TotalSteps = 58;
      Color1 = color1;
      Index = 0;
    }

    // Scan vertically, then flash at the end
    // 0-12, 13-25, 26-37, 38-45
    void VerticalScanFlashFaceUpdate()
    {
      if (Index == 44) {
        // Flash
        for (int i = 0; i < 46; i++) {
          setPixelColor(i, Color1);
        }
      } else {
        // Vertical scan
        // Dim all pixels
        for (int i = 0; i < 46; i++) {
          setPixelColor(i, DimColor(getPixelColor(i)));
        }
        // Light current strip
        if (Index == 0) {
          // Bottom strip
          for (int i = 0; i < 13; i++) {
            setPixelColor(i, Color1);
          }
        } else if (Index == 8) {
          // Lower strip
          for (int i = 13; i < 26; i++) {
            setPixelColor(i, Color1);
          }
        } else if (Index == 16) {
          // Middle strip
          for (int i = 26; i < 38; i++) {
            setPixelColor(i, Color1);
          }
        } else if (Index == 24) {
          // Top strip
          for (int i = 38; i < 46; i++) {
            setPixelColor(i, Color1);
          }
        }
      }
      show();
      Increment();
    }

    // Initialize for a VERTICAL_SCAN_FLASH_HELMET
    void VerticalScanFlashHelmet(uint32_t color1, uint8_t interval)
    {
      ActivePattern = &VerticalScanFlashHelmetUpdate;
      Interval = interval;
      TotalSteps = 58;
      Color1 = color1;
      Index = 0;
    }

    // Helmet only does the flash
    void VerticalScanFlashHelmetUpdate()
    {
      if (Index == 44 || Index == 32) {
        // Flash
        for (int i = 0; i < numPixels(); i++) {
          setPixelColor(i, Color1);
        }
      } else {
        for (int i = 0; i < 46; i++) {
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

    void Off()
    {
      ActivePattern = NULL;
      for (int i = 0; i < numPixels(); i++) {
        setPixelColor(i, this->Color(0, 0, 0));
      }
      show();
    }
};

class Helmet
{
  public:
    NeoPatterns& helmet_strip;
    NeoPatterns& face_strip;
    Helmet(NeoPatterns& helmet, NeoPatterns& face);
    void begin();
    void Update();
    void rainbowHelmet();
    void rainbowAll();
    void blinky();
    void scannerHelmet();
    void scannerFace();
    void redFlashlight();
    void flashlight();
    void police();
    void off();
    void verticalScanFlash();
};

Helmet::Helmet(NeoPatterns& helmet, NeoPatterns& face):
  helmet_strip(helmet), face_strip(face) {};

void Helmet::begin() {
  helmet_strip.begin();
  face_strip.begin();
}

void Helmet::Update() {
  helmet_strip.Update();
  face_strip.Update();
}

void Helmet::rainbowHelmet() {
  helmet_strip.RainbowCycle(10, 3);
  face_strip.ColorWipe(face_strip.Color(0, 0, 0), 5);
}

void Helmet::rainbowAll() {
  helmet_strip.RainbowCycle(10, 3);
  face_strip.RainbowCycle(10, 3);
}

void Helmet::blinky() {
  helmet_strip.TheaterChase(helmet_strip.Color(128, 128, 128), helmet_strip.Color(0, 0, 0), 40);
  face_strip.TheaterChase(face_strip.Color(128, 128, 128), face_strip.Color(0, 0, 0), 40);
}

void Helmet::scannerHelmet() {
  helmet_strip.Scanner(helmet_strip.Color(200, 200, 200), 80);
  face_strip.Off();
}

void Helmet::scannerFace() {
  helmet_strip.Off();
  face_strip.FaceScanner(face_strip.Color(200, 200, 200), 80);
}

void Helmet::redFlashlight() {
  helmet_strip.ColorWipe(helmet_strip.Color(200, 0, 0), 10);
  face_strip.Off();
}

void Helmet::flashlight() {
  helmet_strip.ColorWipe(helmet_strip.Color(128, 128, 128), 10);
  face_strip.Off();
}

void Helmet::police() {
  helmet_strip.TheaterChase(helmet_strip.Color(255, 0, 0), helmet_strip.Color(0, 0, 255), 100);
  face_strip.TheaterChase(face_strip.Color(255, 0, 0), face_strip.Color(0, 0, 255), 100);
}

void Helmet::off() {
  helmet_strip.Off();
  face_strip.Off();
}

void Helmet::verticalScanFlash() {
  helmet_strip.VerticalScanFlashHelmet(helmet_strip.Color(255, 255, 255), 60);
  face_strip.VerticalScanFlashFace(face_strip.Color(255, 255, 255), 60);
}

/****************/
/* Global State */
/****************/

// Current drawing mode
uint8_t current_mode = 1;

NeoPatterns helmet_strip = NeoPatterns(helmet_num_leds, helmet_strip_pin, NEO_GRB + NEO_KHZ800);
NeoPatterns face_strip = NeoPatterns(face_num_leds, face_strip_pin, NEO_GRB + NEO_KHZ800);
Helmet helmet = Helmet(helmet_strip, face_strip);

Bounce mode_button = Bounce();

void setup() {
  // Init button
  pinMode(button_pin, INPUT_PULLUP);
  mode_button.attach(button_pin);
  mode_button.interval(10);

  // Init LED strips
  helmet.begin();
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
  // LEDs
  helmet.Update();
}

void draw() {
  switchMode(helmet, current_mode);
}

void switchMode(Helmet& helmet, uint8_t& mode) {
  switch (mode) {
    case 1: helmet.rainbowHelmet();
      break;
    case 2: helmet.rainbowAll();
      break;
    case 3: helmet.blinky();
      break;
    case 4: helmet.scannerHelmet();
      break;
    case 5: helmet.scannerFace();
      break;
    case 6: helmet.verticalScanFlash();
      break;
    case 7: helmet.redFlashlight();
      break;
    case 8: helmet.flashlight();
      break;
    case 9: helmet.police();
      break;
    default: helmet.off();
      mode = 0;
      break;
  }
}
