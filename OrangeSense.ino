/*
  ================================================================
  OrangeSense: เครื่องประเมินความหวานส้มแบบไม่ทำลายผล
  ================================================================

  แนวคิดของโปรแกรม
  1) วัดแสงจากแผ่นอ้างอิงสีขาว เพื่อใช้เป็นค่าฐาน
  2) วัดแสงจากผลส้ม
  3) เปรียบเทียบค่าทั้งสองเพื่อหา Reflectance และ Absorbance
  4) นำค่าที่ได้เข้าสมการเพื่อประมาณค่า Brix
  5) แสดงผลค้างไว้จนผู้ใช้กดปุ่ม จึงเริ่มเตรียมรอบใหม่

  ฮาร์ดแวร์หลัก
  - บอร์ด PCBfun ESP32-S3 1.47B พร้อมจอ ST7789
  - เซนเซอร์ AS7263 เชื่อมต่อด้วย I2C
  - ปุ่มกดต่อระหว่าง GPIO 2 กับ GND

  หมายเหตุสำหรับผู้เริ่มต้น:
  ข้อความที่ขึ้นต้นด้วยเครื่องหมายทับคู่ หรืออยู่ในบล็อกคำอธิบาย คือ comment
  ไมโครคอนโทรลเลอร์จะไม่ประมวลผลข้อความเหล่านี้
*/

// Wire ใช้สื่อสารกับอุปกรณ์แบบ I2C เช่น AS7263
#include <Wire.h>
// AS726X เป็นไลบรารีสำหรับควบคุมเซนเซอร์ตระกูล AS7262/AS7263
#include <AS726X.h>
// Arduino_GFX_Library ใช้วาดข้อความ รูปทรง และสีบนจอ TFT
#include <Arduino_GFX_Library.h>
// math.h ให้ฟังก์ชันคณิตศาสตร์ เช่น log10()
#include <math.h>

// ================= สีที่ใช้บนหน้าจอ =================
// จอใช้สีแบบ RGB565 ซึ่งเก็บสีหนึ่งสีด้วยเลข 16 บิต
// #define ทำหน้าที่ตั้งชื่อแทนค่าคงที่ เพื่อให้โค้ดอ่านเข้าใจง่าย
#define BLACK       0x0000
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define DARK_ORANGE 0xFBE0
#define GREEN       0x07E0
#define YELLOW      0xFFE0
#define BLUE        0x001F
#define DARKGREY    0x7BEF
#define RED         0xF800

// ================= ขาเชื่อมต่อจอ LCD =================
// TFT_BL คือขาควบคุมไฟส่องหลังจอ หากเป็น HIGH จอจะมีแสงสว่าง
#define TFT_BL 46

// สร้างวัตถุ bus สำหรับการสื่อสารแบบ SPI ระหว่าง ESP32-S3 กับจอ
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

// ================= เซนเซอร์ AS7263 =================
// I2C ใช้สายสัญญาณ 2 เส้น คือ SDA สำหรับข้อมูล และ SCL สำหรับสัญญาณนาฬิกา
#define SDA_PIN 8
#define SCL_PIN 9

// สร้างวัตถุชื่อ sensor เพื่อเรียกใช้คำสั่งของ AS7263
AS726X sensor;

// ================= ปุ่มกด =================
// ปุ่มต่อระหว่าง GPIO 2 กับ GND
// เมื่อไม่กดจะอ่านได้ HIGH และเมื่อกดจะอ่านได้ LOW เพราะใช้ INPUT_PULLUP
#define BUTTON_PIN 2   // GPIO2 ---- Button ---- GND

// ================= ค่าตั้งต้นของการวัด =================
// AS7263 มีช่องวัดทั้งหมด 6 ช่วงคลื่น
const int NUM_BANDS = 6;
// อ่านซ้ำแล้วเฉลี่ย 10 ครั้ง ช่วยลดผลของสัญญาณรบกวน
const int NUM_REF_READINGS = 10;
const int NUM_SAMPLE_READINGS = 10;

// refVal เก็บค่าเฉลี่ยจากแผ่นอ้างอิงสีขาว
float refVal[NUM_BANDS];
// sampleVal เก็บค่าเฉลี่ยจากผลส้ม
float sampleVal[NUM_BANDS];
// R เก็บ Reflectance และ A เก็บ Absorbance ของแต่ละช่วงคลื่น
float R[NUM_BANDS];
float A[NUM_BANDS];

// =====================================================
// ฟังก์ชันช่วยสร้างส่วนติดต่อผู้ใช้ (UI)
// =====================================================
// ล้างภาพเดิมทั้งหมดด้วยพื้นสีดำ
void clearScreen() {
  gfx->fillScreen(BLACK);
}

// วาดไอคอนผลส้ม โดย x,y คือจุดศูนย์กลาง และ r คือรัศมี
void drawOrangeIcon(int x, int y, int r) {
  // วาดตัวผลส้มเป็นวงกลมสีส้มและใส่เส้นขอบสีเหลือง
  gfx->fillCircle(x, y, r, ORANGE);
  gfx->drawCircle(x, y, r, YELLOW);

  // วาดใบด้วยรูปสามเหลี่ยมสีเขียว
  gfx->fillTriangle(x + r - 5, y - r + 4,
                    x + r + 25, y - r - 15,
                    x + r + 8, y - r + 12,
                    GREEN);

  // วาดจุดสะท้อนแสงเพื่อให้ไอคอนดูมีมิติ
  gfx->fillCircle(x - r / 3, y - r / 3, r / 5, YELLOW);
}

// วาดแถบหัวเรื่องสีส้มด้านบนของทุกหน้า
void drawHeader(String title) {
  gfx->fillRect(0, 0, 320, 34, ORANGE);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(12, 9);
  gfx->println(title);
}

// วาดกล่องคำแนะนำให้ผู้ใช้กดปุ่ม
void drawButtonHint(String text) {
  gfx->fillRoundRect(45, 190, 230, 38, 10, DARKGREY);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(72, 202);
  gfx->println(text);
}

// แสดงหน้าเริ่มต้นเมื่อเปิดเครื่อง และค้างไว้ 1.8 วินาที
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

// แสดงหน้าเตรียมวัดแผ่นอ้างอิงสีขาว
// ฟังก์ชันนี้เพียงแสดงภาพ ส่วนการรอปุ่มทำใน loop()
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

// แสดงคำสั่งให้ผู้ใช้เตรียมวัตถุ
// line2 มีค่าเริ่มต้นเป็นข้อความว่าง จึงส่งมาแค่บรรทัดเดียวก็ได้
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

// แสดงความคืบหน้าระหว่างอ่านเซนเซอร์
// current คือครั้งปัจจุบัน และ total คือจำนวนครั้งทั้งหมด
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
  // map() แปลงจำนวนครั้งที่อ่านแล้วให้เป็นความกว้างของแถบสีส้ม
  int fillW = map(current, 0, total, 0, barW);

  gfx->drawRoundRect(barX, barY, barW, barH, 6, WHITE);
  gfx->fillRoundRect(barX, barY, fillW, barH, 6, ORANGE);
}

// แสดงค่า Brix และแปลผลเป็น Low, Medium หรือ Sweet
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
  // เลข 1 ใน print(brix, 1) หมายถึงให้แสดงทศนิยม 1 ตำแหน่ง
  gfx->print(brix, 1);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(210, 150);
  gfx->println("Brix");

  // เตรียมตัวแปรสำหรับข้อความระดับความหวานและสีของกล่อง
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
// การอ่านปุ่มกด
// =====================================================
/*
  รอการกดปุ่มหนึ่งครั้งอย่างสมบูรณ์

  ลำดับคือ:
  1) รอจนสัญญาณเปลี่ยนจาก HIGH เป็น LOW แปลว่าผู้ใช้กดปุ่ม
  2) หน่วงเล็กน้อยเพื่อลดปัญหาหน้าสัมผัสปุ่มเด้ง (debounce)
  3) รอจนกลับเป็น HIGH แปลว่าผู้ใช้ปล่อยปุ่มแล้ว
  4) debounce อีกครั้ง

  การรอให้ปล่อยปุ่มสำคัญ เพราะช่วยป้องกันการกดครั้งเดียว
  แล้วโปรแกรมนำการกดเดียวกันไปใช้กับขั้นตอนถัดไปด้วย
*/
void waitButton() {
  // INPUT_PULLUP ทำให้ค่าปกติเป็น HIGH จึงวนรอจนกว่าจะกด
  while (digitalRead(BUTTON_PIN) == HIGH) {
    delay(20);
  }

  // รอให้การสั่นของหน้าสัมผัสปุ่มสงบ
  delay(250);

  // ขณะยังกดอยู่ ค่าจะเป็น LOW จึงรอจนผู้ใช้ปล่อยปุ่ม
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(20);
  }

  delay(250);
}

// =====================================================
// การอ่านและประมวลผลข้อมูลจากเซนเซอร์
// =====================================================
// อ่านค่าที่ผ่านการปรับเทียบแล้วทั้ง 6 ช่วงคลื่นเก็บลงในอาร์เรย์ values
void readAS7263(float values[]) {
  // เปิดหลอดไฟของเซนเซอร์ อ่านค่าครบทุกช่อง แล้วจัดเก็บผลในตัวเซนเซอร์
  sensor.takeMeasurementsWithBulb();

  // ตำแหน่ง 0-5 ต้องคงลำดับนี้ เพราะใช้จับคู่กับช่วงคลื่นในสมการ
  values[0] = sensor.getCalibratedR(); // 610 nm
  values[1] = sensor.getCalibratedS(); // 680 nm
  values[2] = sensor.getCalibratedT(); // 730 nm
  values[3] = sensor.getCalibratedU(); // 760 nm
  values[4] = sensor.getCalibratedV(); // 810 nm
  values[5] = sensor.getCalibratedW(); // 860 nm
}

/*
  อ่านเซนเซอร์หลายครั้งแล้วหาค่าเฉลี่ย

  avg[]       อาร์เรย์ปลายทางที่จะรับค่าเฉลี่ย
  nReadings   จำนวนครั้งที่ต้องอ่าน
  title       ชื่อที่แสดงบนหัวหน้าจอ เช่น White Ref หรือ Orange
*/
void averageReadings(float avg[], int nReadings, String title) {
  // temp เก็บผลการอ่านเพียงครั้งเดียว ก่อนนำไปบวกสะสม
  float temp[NUM_BANDS];

  // ตั้งผลรวมของทุกช่วงคลื่นให้เป็นศูนย์ก่อนเริ่มรอบใหม่
  for (int b = 0; b < NUM_BANDS; b++) {
    avg[b] = 0;
  }

  // วนอ่านตามจำนวนครั้งที่กำหนด
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

/*
  คำนวณค่าทางสเปกตรัมของทั้ง 6 ช่วงคลื่น

  Reflectance (R) = แสงจากตัวอย่าง / แสงจากแผ่นอ้างอิง
  Absorbance  (A) = log10(แสงอ้างอิง / แสงจากตัวอย่าง)
*/
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

/*
  นำ Reflectance และ Absorbance บางช่วงคลื่นเข้าสมการถดถอย
  เพื่อประมาณค่าความหวานในหน่วย Brix

  R[0], R[1], R[2], R[3] ตรงกับ 610, 680, 730, 760 nm
  A[0], A[3], A[4]       ตรงกับ 610, 760, 810 nm
*/
float calculateBrix() {
  // ตั้งชื่อตัวแปรตามช่วงคลื่น ทำให้ตรวจสมการได้ง่ายกว่าใช้ R[0] โดยตรง
  float R610 = R[0];
  float R680 = R[1];
  float R730 = R[2];
  float R760 = R[3];

  float A610 = A[0];
  float A760 = A[3];
  float A810 = A[4];
  // สมการนี้ได้จากแบบจำลองที่สร้างจากข้อมูลทดลอง
  // เครื่องหมายและสัมประสิทธิ์ทุกตัวมีผลต่อคำตอบ จึงไม่ควรแก้โดยพลการ
  float brix =
    -32.421822
    - 33.970504 * R610
    + 20.545394 * R680
    - 69.613837 * R730
    + 131.275340 * R760
    - 47.053119 * A610
    + 81.624047 * A760
    + 60.949098 * A810;
  // ส่งค่าที่คำนวณได้กลับไปยังผู้เรียกฟังก์ชัน
  return brix;
}

// =====================================================
// setup(): ทำงานเพียงครั้งเดียวเมื่อเปิดเครื่องหรือกด Reset
// =====================================================
void setup() {
  // เปิด Serial สำหรับตรวจสอบการทำงานผ่านคอมพิวเตอร์ที่ 115200 baud
  Serial.begin(115200);
  delay(1000);

  // เปิดตัวต้านทาน pull-up ภายใน ทำให้ไม่ต้องต่อตัวต้านทานภายนอกกับปุ่ม
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // เปิดไฟส่องหลังของจอ
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
    // while (1) คือการวนซ้ำตลอดไป เพื่อหยุดโปรแกรมไว้ที่หน้าข้อผิดพลาด
    while (1);
  }

  // เปิดหลอดไฟ ตั้ง gain และเวลารับแสงของเซนเซอร์
  sensor.enableBulb();
  sensor.setGain(3);
  sensor.setIntegrationTime(50);

  // เมื่อทุกอย่างพร้อม ให้แสดงหน้ารอวัดแผ่นอ้างอิง
  showReady();
}

// =====================================================
// loop(): ทำซ้ำตั้งแต่ต้นถึงท้ายตลอดเวลาที่เครื่องเปิดอยู่
// =====================================================
void loop() {
  // ขั้นที่ 1: หน้า Ready แสดงอยู่แล้ว จึงรอให้ผู้ใช้กดเริ่ม
  waitButton();

  // ขั้นที่ 2: อ่านแผ่นอ้างอิงสีขาว 10 ครั้งและเก็บค่าเฉลี่ยใน refVal
  averageReadings(refVal, NUM_REF_READINGS, "White Ref");

  // ขั้นที่ 3: ขอให้เปลี่ยนจากแผ่นอ้างอิงเป็นผลส้ม แล้วรอการกดปุ่ม
  showInstruction("Orange", "Place orange", "on sensor");
  waitButton();

  // ขั้นที่ 4: อ่านผลส้ม 10 ครั้งและเก็บค่าเฉลี่ยใน sampleVal
  averageReadings(sampleVal, NUM_SAMPLE_READINGS, "Orange");

  // ขั้นที่ 5: แปลงค่าดิบเป็น Reflectance และ Absorbance
  calculateReflectanceAbsorbance();

  // ขั้นที่ 6: คำนวณค่า Brix จากสมการ และเก็บคำตอบไว้ในตัวแปร brix
  float brix = calculateBrix();

  // ขั้นที่ 7: แสดงผลบนจอ
  showBrix(brix);

  // หน้าผลจะค้างอยู่ตรงนี้จนผู้ใช้กดและปล่อยปุ่ม
  waitButton();

  // กลับหน้า Ready เมื่อกดปุ่ม แล้ว loop() จะเริ่มรอบใหม่
  showReady();
}
