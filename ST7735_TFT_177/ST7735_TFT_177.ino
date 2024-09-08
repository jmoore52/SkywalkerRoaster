#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeSansBold9pt7b.h>
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

// Define the correct pins for NodeMCU
#define TFT_CS    15  // GPIO15 (D8) - Chip select
#define TFT_RST   4   // GPIO4 (D2) - Reset
#define TFT_DC    5   // GPIO5 (D1) - Data/Command
#define LED_PIN 0  // GPIO0 (D3) for backlight control

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup(void) {
  pinMode(LED_PIN, OUTPUT);
  // analogWriteFreq(500);  // Set PWM frequency to 500 Hz
  analogWrite(LED_PIN, 0);  // Set backlight 

  tft.initR(INITR_GREENTAB);  // Initialize display with the correct tab color
  tft.fillScreen(ST77XX_BLACK);  // Clear the screen to black
  delay(500);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.print("Test Screen");  // Print test message
}
void loop() {
  for(uint8_t i = 0; i < 100; i++) {
    changing_value(i);
    delay(1000);
} 

}


void show_page() {
  tft.setFont(&FreeSansBold9pt7b);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(14, 22);
  tft.print("AZ-Delivery");
  tft.drawFastHLine(0, 35, 128, ST77XX_GREEN);
  tft.drawTriangle(1, 45, 28, 70, 55, 45, ST77XX_WHITE);
  tft.fillTriangle(78, 70, 104, 45, 127, 70, 0xA3F6);
  tft.drawRect(1, 80, 50, 30, ST77XX_BLUE);
  tft.fillRoundRect(78, 80, 50, 30, 5, 0x2D4E);
  tft.fillCircle(25, 135, 15, 0x5BA9);
  tft.drawCircle(102, 135, 15, ST77XX_GREEN);
  tft.drawLine(45, 150, 80, 40, ST77XX_ORANGE);
}
void changing_value(uint8_t value) {
  if(value < 10) {
    tft.setCursor(15, 88);
    tft.print("0");
    tft.print(value);
}
else {
    tft.setCursor(15, 88);
    tft.print(value);
  }
}