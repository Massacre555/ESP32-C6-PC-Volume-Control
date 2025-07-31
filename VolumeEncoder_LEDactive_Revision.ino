#include <Adafruit_NeoPixel.h>
#include <KY040.h>

// NeoPixel setup
#define LED_PIN        8
#define NUMPIXELS      1
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Rotary encoder setup
#define CLK_PIN 4
#define DT_PIN  5
#define SW_PIN  3  // Button Pin for the rotary encoder
KY040 g_rotaryEncoder(CLK_PIN, DT_PIN);

volatile bool rotaryChanged = false;
bool buttonPressed = false;
bool redMode = false;  // Toggle state for red LED

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// LED color cycling setup
unsigned long previousMillis = 0;
const long interval = 20;  // Smaller interval for smoother cycling
uint16_t hue = 0;          // 0 - 65535 for full color wheel

// Interrupt handler for encoder
void IRAM_ATTR ISR_rotaryEncoder() {
  rotaryChanged = true;
}

void checkButtonPress() {
  static bool lastButtonState = HIGH;
  static bool debouncedState = HIGH;

  bool currentButtonState = digitalRead(SW_PIN);

  if (currentButtonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentButtonState != debouncedState) {
      debouncedState = currentButtonState;

      if (debouncedState == LOW) {
        buttonPressed = true;
        Serial.println("BUTTON_PRESS");
      } else {
        buttonPressed = false;
      }
    }
  }

  lastButtonState = currentButtonState;
}

void setup() {
  pixels.begin();
  pixels.setBrightness(100);
  Serial.begin(9600);

  pinMode(SW_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(CLK_PIN), ISR_rotaryEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT_PIN), ISR_rotaryEncoder, CHANGE);
}

void loop() {
  static int lastValue = 0;
  static int v_value = 0;

  if (rotaryChanged) {
    rotaryChanged = false;

    switch (g_rotaryEncoder.getRotation()) {
      case KY040::CLOCKWISE:
        v_value++;
        Serial.println("VOLUME_UP");

        if (redMode) {
          redMode = false;
          Serial.println("LED_CYCLE_RESUMED");
        }
        break;

      case KY040::COUNTERCLOCKWISE:
        v_value--;
        Serial.println("VOLUME_DOWN");
        break;
    }

    lastValue = v_value;
  }

  checkButtonPress();

  if (buttonPressed) {
    buttonPressed = false;

    redMode = !redMode;

    if (redMode) {
      Serial.println("LED_RED_ON");
      pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // Red LED
      pixels.show();
    } else {
      Serial.println("LED_CYCLE_RESUMED");
    }
  }

  if (!redMode) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      uint32_t color = pixels.ColorHSV(hue, 255, 255);  // Full saturation and brightness
      pixels.setPixelColor(0, color);
      pixels.show();

      hue += 156;
      if (hue >= 65536) hue = 0;
    }
  }
}
