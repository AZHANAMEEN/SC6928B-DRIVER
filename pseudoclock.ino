#define PIN_STB PIN_PC0
#define PIN_CLK PIN_PC1
#define PIN_DIO PIN_PC2

// Bit positions corresponding to Grid 0 through Grid 6
const uint8_t GRID_BITS[7] = {0, 1, 7, 2, 3, 4, 5};

// Address offsets corresponding to Segments A through G
const uint8_t SEG_ADDRS[7] = {0, 2, 4, 6, 8, 10, 12};

// Standard 7-segment bitmasks for digits 0-9 (Bit 0 = Seg A ... Bit 6 = Seg G)
const uint8_t DIGIT_MASKS[10] = {
  0b00111111, // 0: A B C D E F
  0b00000110, // 1:   B C
  0b01011011, // 2: A B   D E   G
  0b01001111, // 3: A B C D     G
  0b01100110, // 4:   B C     F G
  0b01101101, // 5: A   C D   F G
  0b01111101, // 6: A   C D E F G
  0b00000111, // 7: A B C
  0b01111111, // 8: A B C D E F G
  0b01101111  // 9: A B C D   F G
};

static void sendByte(uint8_t b) {
  pinMode(PIN_DIO, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(PIN_CLK, LOW);
    digitalWrite(PIN_DIO, (b >> i) & 1);
    delayMicroseconds(1);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(1);
  }
}

static void cmd(uint8_t c) {
  digitalWrite(PIN_STB, LOW);
  sendByte(c);
  digitalWrite(PIN_STB, HIGH);
  delayMicroseconds(2);
}

void writeRam(const uint8_t *buf) {
  cmd(0x40);
  digitalWrite(PIN_STB, LOW);
  sendByte(0xC0);
  for (uint8_t i = 0; i < 14; i++) sendByte(buf[i]);
  digitalWrite(PIN_STB, HIGH);
}

void displayInit(uint8_t brightness) {
  pinMode(PIN_STB, OUTPUT); 
  pinMode(PIN_CLK, OUTPUT);
  digitalWrite(PIN_STB, HIGH); 
  digitalWrite(PIN_CLK, HIGH);
  delay(200);
  
  uint8_t zero[14] = {0};
  writeRam(zero);
  cmd(0x03);                    // 7 grids, 11 segments
  cmd(0x88 | (brightness & 7)); // display on
}

// Sets a single digit (0-9) at a specific grid position (0-6)
void setDigit(uint8_t *buf, uint8_t grid, uint8_t digit) {
  if (grid > 6 || digit > 9) return;
  
  uint8_t mask = DIGIT_MASKS[digit];
  uint8_t bitPos = GRID_BITS[grid];
  
  for (uint8_t seg = 0; seg < 7; seg++) {
    if ((mask >> seg) & 1) {
      buf[SEG_ADDRS[seg]] |= (1 << bitPos);
    }
  }
}

// Sets or clears the colon dots between digits
void setDots(uint8_t *buf, bool dot1, bool dot2) {
  if (dot1) buf[2] |= (1 << 6); // Dot between digits 3-4 (Address 2, Bit 6)
  else      buf[2] &= ~(1 << 6);

  if (dot2) buf[4] |= (1 << 6); // Dot between digits 5-6 (Address 4, Bit 6)
  else      buf[4] &= ~(1 << 6);
}

// Displays HH:MM:SS or HH MM SS across 7 grids
// Layout: [H1] [H2] : [M1] [M2] : [S1] [S2] (Grids 1 through 6)
// Grid 0 is left blank or used as a leading space.
void displayTime(uint8_t hours, uint8_t mins, uint8_t secs, bool showDots) {
  uint8_t buf[14] = {0};

  // Extract tens and ones
  uint8_t h1 = hours / 10;
  uint8_t h2 = hours % 10;
  uint8_t m1 = mins / 10;
  uint8_t m2 = mins % 10;
  uint8_t s1 = secs / 10;
  uint8_t s2 = secs % 10;

  // Grid mapping for 7-digit layout (Grid 1 to Grid 6 used)
  setDigit(buf, 1, h1); // Hours Tens
  setDigit(buf, 2, h2); // Hours Ones
  setDigit(buf, 3, m1); // Mins Tens
  setDigit(buf, 4, m2); // Mins Ones
  setDigit(buf, 5, s1); // Secs Tens
  setDigit(buf, 6, s2); // Secs Ones

  // Set dots state
  setDots(buf, showDots, showDots);

  writeRam(buf);
}

// Simulated clock variables
uint8_t hours = 7;
uint8_t minutes = 39;
uint8_t seconds = 40;
unsigned long lastTick = 0;
bool blinkState = true;

void setup() {
  Serial.begin(115200);
  displayInit(4);
  Serial.println("Displaying Live Time (HH:MM:SS)...");
}

void loop() {
  // Simple non-blocking 1-second clock tick
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    blinkState = !blinkState; // Toggle dots every second

    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) hours = 0;
      }
    }

    // Refresh display
    displayTime(hours, minutes, seconds, blinkState);
  }
}