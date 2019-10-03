# LED Fur Coat

- Pro Micro 5V: https://www.sparkfun.com/products/12640
- 30/m WS2812 LED strip: https://www.amazon.com/gp/product/B00ZHB9M6A
- 1000 µF capacitor: https://www.amazon.com/gp/product/B06WGPNM19
- 500 ohm resistor: https://www.amazon.com/Projects-100EP514510R-510-Resistors-Pack/dp/B0185FGTMY/
- Momentary switch: https://www.amazon.com/gp/product/B004RXKWI6
- Snappable perfboard
- Pin headers: https://www.amazon.com/gp/product/B074HVBTZ4
- 3-pin JST connectors: https://www.amazon.com/gp/product/B00NBSH4CA

JST connectors are directly soldered to the board and hot glued. Might be
better to use wire terminals.

Pattern ideas:

- https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/

## Wiring

GND -> 1000 µF Cap -
VCC -> 1000 µF Cap +

pin 2 -> R500 -> strip 1 data
VCC -> strip 1 power
GND -> strip 1 ground

pin 8 -> R500 -> strip 2 data
VCC -> strip 2 power
GND -> strip 2 ground

pin 0 -> button
GND -> button
