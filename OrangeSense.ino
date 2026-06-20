#include <Wire.h>
#include <SparkFun_AS726X.h>
#include <Arduino_GFX_Library.h>
#include <math.h>

// ================= COLOR =================
#define BLACK       0x0000
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define DARK_ORANGE 0xFBE0
#define GREEN       0x07E0
#define YELLOW      0xFFE0
#define BLUE        0x001F
#define DARKGREY    0x7BEF

// ================= LCD PIN: PCBfun ESP32-S3 1.47B =================
#define TFT_BL 46

Arduino_DataBus *bus = new Arduino_ESP32SPI(
  41,  // DC
  42,  // CS
  40,  // SCLK
  45,  // MOSI
  -1
);

Arduino_GFX *gfx = new Arduino_ST7789(
  bus,
  39,     // RST
  1,      // Landscape
  true,   // IPS
  172,
  320,
  34, 0,
  34, 0
);

// ================= AS7263 =================
#define SDA_PIN 8
#define SCL_PIN 9

AS726X sensor;

// ================= BUTTON =================
#define BUTTON_PIN 2   // GPIO2 ---- Button ---- GND

// ================= SETTINGS =================
const int NUM_BANDS = 6;
const int NUM_REF_READINGS = 10;
const int NUM_SAMPLE_READINGS = 10;

float refVal[NUM_BANDS];
float sampleVal[NUM_BANDS];
float R[NUM_BANDS];
float A[NUM_BANDS];

// =====================================================
// SIMPLE UI HELPERS
// =====================================================
void clearScreen() {
  gfx->fillScreen(BLACK);
}

void drawOrangeIcon(int x, int y, int r) {
  gfx->fillCircle(x, y, r, ORANGE);
  gfx->drawCircle(x, y, r, YELLOW);

  // leaf
  gfx->fillTriangle(x + r - 5, y - r + 4,
                    x + r + 25, y - r - 15,
                    x + r + 8, y - r + 12,
                    GREEN);

  // shine
  gfx->fillCircle(x - r / 3, y - r / 3, r / 5, YELLOW);
}

void drawHeader(String title) {
  gfx->fillRect(0, 0, 320, 34, ORANGE);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(12, 9);
  gfx->println(title);
}

void drawButtonHint(String text) {
  gfx->fillRoundRect(45, 190, 230, 38, 10, DARKGREY);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(72, 202);
  gfx->println(text);
}

void showStartup() {
  clearScreen();

  drawOrangeIcon(160, 78, 38);

  gfx->setTextColor(ORANGE);
  gfx->setTextSize(3);
  gfx->setCursor(58, 130);
  gfx->println("OrangeSense");

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(62, 172);
  gfx->println("Non-destructive");

  gfx->setCursor(84, 198);
  gfx->println("Brix Meter");

  delay(1800);
}

void showReady() {
  clearScreen();
  drawHeader("OrangeSense");

  drawOrangeIcon(55, 85, 28);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(105, 68);
  gfx->println("Ready");

  gfx->setCursor(105, 100);
  gfx->println("Place white");

  gfx->setCursor(105, 126);
  gfx->println("reference");

  drawButtonHint("PRESS START");
}

void showInstruction(String title, String line1, String line2 = "") {
  clearScreen();
  drawHeader(title);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);

  gfx->setCursor(20, 70);
  gfx->println(line1);

  gfx->setCursor(20, 105);
  gfx->println(line2);

  drawButtonHint("PRESS START");
}

void showReading(String title, int current, int total) {
  clearScreen();
  drawHeader(title);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(35, 70);
  gfx->println("Reading spectrum");

  gfx->setTextColor(YELLOW);
  gfx->setTextSize(3);
  gfx->setCursor(115, 112);
  gfx->print(current);
  gfx->print("/");
  gfx->print(total);

  int barX = 40;
  int barY = 170;
  int barW = 240;
  int barH = 18;
  int fillW = map(current, 0, total, 0, barW);

  gfx->drawRoundRect(barX, barY, barW, barH, 6, WHITE);
  gfx->fillRoundRect(barX, barY, fillW, barH, 6, ORANGE);
}

void showBrix(float brix) {
  clearScreen();
  drawHeader("Result");

  drawOrangeIcon(55, 82, 28);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(110, 58);
  gfx->println("Predicted");

  gfx->setCursor(110, 84);
  gfx->println("Sweetness");

  gfx->setTextColor(GREEN);
  gfx->setTextSize(5);
  gfx->setCursor(60, 128);
  gfx->print(brix, 1);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(210, 150);
  gfx->println("Brix");

  String level = "";
  uint16_t levelColor = WHITE;

  if (brix < 10.0) {
    level = "Low";
    levelColor = YELLOW;
  } else if (brix < 13.0) {
    level = "Medium";
    levelColor = ORANGE;
  } else {
    level = "Sweet";
    levelColor = GREEN;
  }

  gfx->fillRoundRect(85, 205, 150, 34, 10, levelColor);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(120, 214);
  gfx->println(level);
}

// =====================================================
// BUTTON
// =====================================================
void waitButton() {
  while (digitalRead(BUTTON_PIN) == HIGH) {
    delay(20);
  }

  delay(250);

  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(20);
  }

  delay(250);
}

// =====================================================
// SENSOR
// =====================================================
void readAS7263(float values[]) {
  sensor.takeMeasurementsWithBulb();

  values[0] = sensor.getCalibratedR(); // 610 nm
  values[1] = sensor.getCalibratedS(); // 680 nm
  values[2] = sensor.getCalibratedT(); // 730 nm
  values[3] = sensor.getCalibratedU(); // 760 nm
  values[4] = sensor.getCalibratedV(); // 810 nm
  values[5] = sensor.getCalibratedW(); // 860 nm
}

void averageReadings(float avg[], int nReadings, String title) {
  float temp[NUM_BANDS];

  for (int b = 0; b < NUM_BANDS; b++) {
    avg[b] = 0;
  }

  for (int i = 0; i < nReadings; i++) {
    readAS7263(temp);

    for (int b = 0; b < NUM_BANDS; b++) {
      avg[b] += temp[b];
    }

    showReading(title, i + 1, nReadings);
    delay(200);
  }

  for (int b = 0; b < NUM_BANDS; b++) {
    avg[b] = avg[b] / nReadings;
  }
}

void calculateReflectanceAbsorbance() {
  for (int b = 0; b < NUM_BANDS; b++) {
    float ref = refVal[b];
    float sam = sampleVal[b];

    if (ref <= 0) ref = 0.0001;
    if (sam <= 0) sam = 0.0001;

    R[b] = sam / ref;
    A[b] = log10(ref / sam);
  }
}

float calculateBrix() {
  float R610 = R[0];
  float R680 = R[1];
  float R760 = R[3];

  float A610 = A[0];
  float A730 = A[2];
  float A810 = A[4];

  float brix =
    -39.102377
    - 38.803934 * R610
    + 20.47284  * R680
    + 74.262429 * R760
    - 46.117035 * A610
    + 102.133231 * A730
    + 47.008893 * A810;

  return brix;
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  showStartup();

  Wire.begin(SDA_PIN, SCL_PIN);

  if (sensor.begin() == false) {
    clearScreen();
    drawHeader("Sensor Error");
    gfx->setTextColor(RED);
    gfx->setTextSize(2);
    gfx->setCursor(25, 80);
    gfx->println("AS7263 not found");
    gfx->setTextColor(WHITE);
    gfx->setCursor(25, 120);
    gfx->println("Check wiring");
    while (1);
  }

  sensor.enableBulb();
  sensor.setGain(3);
  sensor.setIntegrationTime(50);

  showReady();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  waitButton();

  averageReadings(refVal, NUM_REF_READINGS, "White Ref");

  showInstruction("Orange", "Place orange", "on sensor");
  waitButton();

  averageReadings(sampleVal, NUM_SAMPLE_READINGS, "Orange");

  calculateReflectanceAbsorbance();

  float brix = calculateBrix();

  showBrix(brix);

  delay(6000);

  showReady();
}
