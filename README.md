# SC6928B Driver

This sketch drives an SC6928B 7-segment display driver over a simple serial-like interface.
It stores a display buffer, writes digits to the driver RAM, and can configure decimal points and custom bit-level updates.

## Hardware pins

The sketch uses these Arduino pins:

- `S` = `PIN_PC0`
- `C` = `PIN_PC1`
- `D` = `PIN_PC2`
- `IR` = `PIN_PC3`

`S` is the chip select / latch line, `C` is the clock, and `D` is the data pin.

## Display buffer layout

The driver uses a 14-byte buffer (`bffr`) that maps to the 7 segment grids and their control bits.

- `GRID_BITS[7]` maps each grid position to a bit position in the driver RAM.
- `SEG_ADDRS[7]` maps segment A..G to the corresponding byte addresses.
- `DIGIT_MASKS[10]` stores the on/off pattern for digits 0..9.

## Functions and what they do

### `sendbyte(uint8_t b)`
Sends one byte serially to the SC6928B using the `C` clock and `D` data line.

Usage:

```cpp
sendbyte(0x40);
```

This is a low-level helper used by other functions such as `sendcmd()` and `writeram()`.

### `sendcmd(uint8_t cmd)`
Sends a command byte to the display controller.

Usage:

```cpp
sendcmd(0x03);
```

It pulls `S` low, sends the command, then brings `S` high again.

### `splitToDigits(uint32_t val, uint8_t *digits)`
Converts a numeric value into 7 decimal digits, stored from left to right in the `digits` array.

Usage:

```cpp
uint8_t digits[7];
splitToDigits(1234567, digits);
```

The array is filled like this:

- `digits[0]` = most significant digit
- `digits[6]` = least significant digit

### `recievebyte(uint8_t *rbf)`
Reads one byte from the display data line into an 8-byte array.

This is mainly used by `getram()` to read memory back from the driver.

### `writeram(const uint8_t *buff)`
Writes a 14-byte buffer to the display RAM.

Usage:

```cpp
uint8_t buf[14] = {0};
writeram(buf);
```

This sends the command `0x40`, then writes the 14 RAM bytes.

### `getram()`
Reads the controller RAM into the global `rbt` buffer.

Usage:

```cpp
getram();
```

This is useful for debugging or verifying the current display memory state.

### `setDots(uint8_t *buf, bool dot1, bool dot2)`
Turns the decimal points on or off for the two display sections.

Usage:

```cpp
setDots(buf, true, false);
```

This modifies the relevant bits in the display buffer.

### `setdigit(uint8_t *buf, uint8_t grid, uint8_t digit)`
Writes one digit to a specific grid position.

Usage:

```cpp
setdigit(buf, 0, 5);  // grid 0 = left-most digit, value 5
```

- `grid` must be between 0 and 6
- `digit` must be between 0 and 9

### `displayFullNumber(uint8_t *buf, uint32_t number)`
Breaks a number into seven digits and writes them to the display buffer.

Usage:

```cpp
uint8_t buf[14] = {0};
displayFullNumber(buf, 1234567);
writeram(buf);
```

This is the easiest way to update the display with a numeric value.

### `writeBit(uint8_t val, uint8_t bit, bool value)`
A helper to set or clear a specific bit in a byte.

Usage:

```cpp
uint8_t x = writeBit(x, 3, true);
```

### `sendbitat(uint8_t add, uint8_t bit, bool a)`
Updates one bit in the internal buffer at a specific RAM address.

Usage:

```cpp
sendbitat(2, 6, true);  // set a bit at address 2, bit 6
```

This writes to the display memory in a more targeted way than updating the whole buffer.

## Typical usage

A common pattern is:

```cpp
void setup() {
  pinMode(S, OUTPUT);
  pinMode(C, OUTPUT);
  digitalWrite(S, HIGH);
  digitalWrite(C, HIGH);

  uint8_t buf[14] = {0};
  writeram(buf);
  sendcmd(0x03);
  sendcmd(0x8C);
}

void loop() {
  uint8_t buf[14] = {0};
  displayFullNumber(buf, 123456);
  setDots(buf, true, false);
  writeram(buf);
  delay(1000);
}
```

This clears the driver, configures the display, fills a buffer with a number, adds a decimal point, and writes it to the hardware.

## Notes

- The sketch keeps a global `bffr` buffer that is used by some low-level write functions.
- The `IR` pin is defined but not actively used in this version.
- The driver is configured at setup time and then updated by writing new buffers from the main loop.

