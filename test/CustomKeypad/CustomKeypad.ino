#include "LiquidCrystal.h"
#include "Keypad.h"

byte touchSensorPin = 2;
byte resetButtonPin = 22;
byte initWheelSpeed = 50;
byte wheelPins[] = {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

// Adjust other classes as needed to remove blocking delays and use non-blocking timing

class LCDManager {
  private:
    const int rs = 12, en = 11, d4 = 53, d5 = 52, d6 = 51, d7 = 50;
    LiquidCrystal lcd;

  public:
    LCDManager() : lcd(rs, en, d4, d5, d6, d7) {
      lcd.begin(16, 2);
    }

    void displayMessage(const char* message, int col = 0, int row = 0) {
      lcd.setCursor(col, row);
      lcd.print(message);
    }

    void displayNumber(const int number, int col = 0, int row = 0) {
      lcd.setCursor(col, row);
      lcd.print(number);
    }

    void clear() {
      lcd.clear();
    }

    void resetGameScreen() {
      clear();
      delay(2000);
      displayMessage("Initializing", 0, 0);
      displayMessage("game", 0, 1);
      delay(1000);
      clear();
      delay(500);
      displayMessage("Welcome to", 0, 0);
      displayMessage("Miku Roulette", 0, 1);
      delay(3000);
      clear();
      delay(500);
      displayMessage("Current credits:", 0, 0);
      displayMessage("2000", 0, 1);
    }

    void loopIdleScreen(int number) {
      clear();
      displayCurrentCredit(number);
      delay(3000);
      clear();
      displayMessage("Press B", 0, 0);
      displayMessage("to bet", 0, 1);
      delay(3000);
    }

    void displayCurrentCredit(int number) {
      clear();
      displayMessage("Current credits:", 0, 0);
      displayNumber(number, 0, 1);
    }

    void displayWheelNumber(int number) {
      clear();
      displayMessage("Roulette lands", 0, 0);
      displayMessage("on", 0, 1);
      displayNumber(number, 3, 1);
    }

    void displayAfterMatch(int credits, int number) {
      clear();
      displayWheelNumber(number);
      delay(6000);
      clear();
      displayCurrentCredit(credits);
    }
};

class GameManager {
  private:
    int matchNumber;
    byte numZeroPin = wheelPins[0];
    byte numLastPin = wheelPins[sizeof(wheelPins) / sizeof(wheelPins[0]) - 1];
    CreditSetUp creditManager;
    LCDManager lcdManager;
    WheelManager wheelManager;

  public:
    GameManager() : wheelManager(numZeroPin, numLastPin, initWheelSpeed, lcdManager) {}

    void gameSetUp() {
      // Any setup you need for the game
    }

    void startWheelIfNeeded() {
      int dealerTouchInput = digitalRead(touchSensorPin);
      if (dealerTouchInput == HIGH && !wheelManager.isWheelRunning() && wheelManager.isWheelReady()) {
        wheelManager.startWheel();
      }
    }

    void finishOneMatch() {
      lcdManager.displayAfterMatch(creditManager.getCredits(), wheelManager.getStopNumber());
    }

    void runWheel() {
      wheelManager.runWheel();
    }

    void checkNextMatchCondition(int buttonPin) {
      if (digitalRead(buttonPin) == HIGH && !wheelManager.isWheelRunning()) {
        nextMatch();
      }
    }

    void nextMatch() {
      wheelManager.resetWheel();
      lcdManager.loopIdleScreen(creditManager.getCredits());
    }

    CreditSetUp& getCreditManager() {
      return creditManager;
    }

    LCDManager& getLCDManager() {
      return lcdManager;
    }

    WheelManager& getWheelManager() {
      return wheelManager;
    }
};

GameManager gameManager;
KeypadManager keypadManager;

void setup() {
  Serial.begin(9600);
  pinMode(touchSensorPin, INPUT_PULLUP);
  pinMode(resetButtonPin, INPUT_PULLUP);
  for (byte i = 0; i < (sizeof(wheelPins) / sizeof(wheelPins[0])); i++) {
    pinMode(wheelPins[i], OUTPUT);
  }
  randomSeed(analogRead(0));

  gameManager.gameSetUp();
  gameManager.getLCDManager().resetGameScreen();
}

void loop() {
  static unsigned long previousMillis = 0;
  static unsigned long idleScreenPreviousMillis = 0;
  unsigned long interval = 3000; // 3-second interval for the idle screen

  gameManager.startWheelIfNeeded();
  gameManager.runWheel();
  gameManager.checkNextMatchCondition(resetButtonPin);

  char key = keypadManager.getKey();
  if (key) { // Check if a key is pressed
    Serial.print("Key pressed: ");
    Serial.println(key);
  }

  if (!gameManager.getWheelManager().isWheelRunning()) {
    unsigned long currentMillis = millis();
    if (currentMillis - idleScreenPreviousMillis >= interval) {
      idleScreenPreviousMillis = currentMillis;
      gameManager.getLCDManager().loopIdleScreen(gameManager.getCreditManager().getCredits());
    }
  }
}
