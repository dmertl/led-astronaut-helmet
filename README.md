# LED Astronaut Helmet

Arduino project for LED astronaut helmet.

## Hardware

Pro Micro 5v/16MHz: https://www.sparkfun.com/products/12640
Neopixel strip in helmet
Mokungit WS2812B LED Strip: https://www.amazon.com/gp/product/B01D1EDDR8
Button

### Configuration

Two separately controlled LED strips, one on the helmet, one on the face. Power
is chained from helmet to face strip.

Button: pin 8
Helmet LED strip pin: 2
Helmet LEDs: 26
Face LED strip pin: 10
Face LEDs: 46

Face LEDs are organized in a zig-zag pattern in 4 lines:

- 13 in forward direction
- 13 in reverse
- 12 in forward
- 8 in reverse

LED strips are running of off board voltage. Works fine as long as power draw is
low.

## Libraries

Adafruit Circuit Playground
Adafruit Neopixel
Bounce2
elapsedMillis
