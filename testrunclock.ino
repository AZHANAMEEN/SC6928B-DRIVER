#define S PIN_PC0
#define C PIN_PC1
#define D PIN_PC2
#define IR PIN_PC3
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


uint8_t bffr[14] = {0};
static void sendbyte(uint8_t b) {
    pinMode(D, OUTPUT);
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(C, LOW);
        delayMicroseconds(2);
        digitalWrite(D, (b >> i) & 1);
        delayMicroseconds(2);
        digitalWrite(C, HIGH);
        delayMicroseconds(2);
    }
}


static void sendcmd(uint8_t cmd) {
    digitalWrite(S, LOW);
    sendbyte(cmd);
    digitalWrite(S, HIGH);
    delayMicroseconds(2);
}


void splitToDigits(uint32_t val, uint8_t *digits) {
  // Extract digits from right to left (ones up to millions)
  for (int8_t i = 6; i >= 0; i--) {
    digits[i] = val % 10;
    val /= 10;
  }
}


void recievebyte(uint8_t*rbf) {
  for (int i =0; i < 8; i++) {
    digitalWrite(C, LOW);
    delayMicroseconds(2);
    rbf[i] = digitalRead(D);
    delayMicroseconds(2);
    digitalWrite(C, HIGH);
    delayMicroseconds(2);
  }
}


static void writeram(const uint8_t *buff) {
    sendcmd(0x40);
    digitalWrite(S, LOW);
    sendbyte(0xC0);
    for (uint8_t i = 0; i < 14; i++) {
        sendbyte(buff[i]);
    }
    digitalWrite(S, HIGH);
    delayMicroseconds(2);

}


uint8_t rbt[40] = {0};
void getram() {
  digitalWrite(S, LOW);
  sendbyte(0x42);
  delayMicroseconds(2);
  pinMode(D, INPUT_PULLUP);
    for (uint8_t n = 0; n < 5; n++) {
    recievebyte(&rbt[n * 8]);          // fills rbt[n*8] .. rbt[n*8+7]
  }
  digitalWrite(S, HIGH);
}


void setDots(uint8_t *buf, bool dot1, bool dot2) {
  if (dot1) buf[2] |= (1 << 6); // Dot between digits 3-4 (Address 2, Bit 6)
  else      buf[2] &= ~(1 << 6);

  if (dot2) buf[4] |= (1 << 6); // Dot between digits 5-6 (Address 4, Bit 6)
  else      buf[4] &= ~(1 << 6);
}


void setdigit(uint8_t *buf, uint8_t grid, uint8_t digit) {
  if (grid > 6 || digit > 9) return;

  uint8_t mask = DIGIT_MASKS[digit];
  uint8_t bitPos = GRID_BITS[grid];

  // Bitwise OR sets segments ON, bitwise AND clears segments OFF
  for (uint8_t seg = 0; seg < 7; seg++) {
    if ((mask >> seg) & 1) {
      buf[SEG_ADDRS[seg]] |= (1 << bitPos);
    } else {
      buf[SEG_ADDRS[seg]] &= ~(1 << bitPos);
    }
  }
}


void displayFullNumber(uint8_t *buf, uint32_t number) {
  uint8_t digits[7];
  
  // 1. Break the number into an array of 7 digits
  splitToDigits(number, digits);

  // 2. Set each digit onto its respective grid (Grid 0 to 6)
  for (uint8_t grid = 0; grid < 7; grid++) {
    setdigit(buf, grid, digits[grid]);
  }
}

const uint8_t ADDR_CMD[14] = {
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6,
  0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD
};

uint8_t writeBit(uint8_t val, uint8_t bit, bool value) {
  if (value) val |=  (1u << bit);
  else       val &= ~(1u << bit);
  return val;
}

void sendbitat(uint8_t add, uint8_t bit, bool a) {
  if (add > 13 || bit > 7) return;

  bffr[add] = writeBit(bffr[add], bit, a);   // update the copy

  sendcmd(0x44);                             // data cmd: write, fixed address
  digitalWrite(S, LOW);
  sendbyte(ADDR_CMD[add]);                   // address cmd (no STB toggle)
  sendbyte(bffr[add]);                       // full byte with the new bit
  digitalWrite(S, HIGH);
  delayMicroseconds(2);
}


void setup() {
  uint8_t bffr[14] = {0};
  pinMode(S, OUTPUT);
  pinMode(C, OUTPUT);
  digitalWrite(S, HIGH);
  digitalWrite(C, HIGH);
  delay(100);
  Serial.begin(115200);
  uint8_t clearBuf[14] = {0};
  writeram(clearBuf);
  sendcmd(0x03);
  sendcmd(0x8C);
  sendbitat(6, 4, 1);
  sendbitat(2, 4, 1);
  sendbitat(4, 4, 1);
}

void loop() {

}