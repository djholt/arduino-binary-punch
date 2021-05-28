#include <Adafruit_NeoMatrix.h>
#include <gamma.h>

#define PIN_KEY     13
#define PIN_BYTE    12
#define PIN_MATRIX  11
#define MATRIX_SIZE 8

byte byteRead = 0;

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_SIZE, MATRIX_SIZE, PIN_MATRIX,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

void setup() {
  pinMode(PIN_KEY, INPUT_PULLUP);
  pinMode(PIN_BYTE, INPUT_PULLUP);
  Serial.begin(9600);

  matrix.begin();
  matrix.setBrightness(100);
  matrix.setTextWrap(false);
}

int keyState = LOW;
int lastKeyState = LOW;
int readPos = 0;

long lastDebounceTime = 0;
long debounceDelay = 3;
long lastKeyTime = 0;

void fillScreen(uint16_t color) {
  matrix.fillScreen(color);
  matrix.show();
}

void fillColumn(int col, uint16_t color) {
  for (int row = 0; row < MATRIX_SIZE; row++) {
    matrix.drawPixel(col, row, color);
    matrix.show();
    delay(25);
  }
}

void drawByte(byte b, uint16_t color) {
  matrix.fillScreen(0);
  for (int col = 0; col < MATRIX_SIZE; col++) {
    if (b & (1 << MATRIX_SIZE-col-1)) {
      fillColumn(col, color);
    }
  }
}

void drawChar(char c, uint16_t color) {
  matrix.setTextColor(color);
  for (int x = matrix.width(); x >= 0; x--) {
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(c);
    matrix.show();
    delay(50);
  }
}

void loop() {
  // read the bottom row on punch card
  int keyRead = digitalRead(PIN_KEY);

  // reset if more than 1/4 second after reading starts
  if (millis() - lastKeyTime > 500 && readPos > 0) {
    readPos = 0;
    byteRead = 0;
    fillScreen(matrix.Color(128, 0, 0));
    Serial.println("---");
  }

  // if reading changed since last time, reset debounce timer
  if (keyRead != lastKeyState) {
    lastDebounceTime = millis();
  }

  // debounce: consider reading valid if not changed for debounceDelay milliseconds
  if (millis() - lastDebounceTime > debounceDelay) {
    if (keyRead != keyState) { // key is different than last time
      keyState = keyRead;
      if (keyState == LOW) { // key is now on: read bit immediately
        if (readPos == 0) {
          // clear screen when reading begins
          fillScreen(matrix.Color(0, 0, 0));
        }
        if (readPos < 8) { // still more bits to read
          readPos++;
          // record this bit
          byteRead = byteRead << 1;
          byteRead = byteRead | !digitalRead(PIN_BYTE);
          Serial.print(!digitalRead(PIN_BYTE));
        }
        if (readPos == 8) { // the last bit was just recorded
          // display the byte read
          Serial.println();
          Serial.write(byteRead);
          Serial.println();
          drawByte(byteRead, matrix.Color(230, 0, 220));
          delay(1000);
          drawChar(byteRead, matrix.Color(230, 120, 0));
          readPos = 0;
          byteRead = 0;
        }
      } else { // key is now off: continue
        lastKeyTime = millis();
      }
    }
  }

  // remeber last reading
  lastKeyState = keyRead;
}

