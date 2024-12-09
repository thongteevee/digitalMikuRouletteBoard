#include "LiquidCrystal.h"
#include "Keypad.h"

byte touchSensorPin = 2;
byte resetButtonPin = 22;
byte initWheelSpeed = 50;
byte wheelPins[] = {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

class KeypadManager {
  private:
    const byte ROWS = 4;
    const byte COLS = 4;
    char hexaKeys[4][4] = {
      {'1','2','3','A'},
      {'4','5','6','B'},
      {'7','8','9','C'},
      {'*','0','#','D'}
    };
    byte rowPins[4] = {10, 9, 8, 7};
    byte colPins[4] = {6, 5, 4, 3};

    char key;
    byte keyAsANumber;
    boolean entryComplete;
    unsigned long total = 0;
    byte digitCount = 0;

    Keypad customKeypad;

  public:
    KeypadManager() : customKeypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS) {}

    char getKey() {
      return customKeypad.getKey();
    }

    bool getInputComplete() {
      return entryComplete;
    }

    unsigned long getNumberInScratchPad() {
      return total;
    }
    
    void resetNumberInScratchPad() {
      total = 0;
      digitCount = 0;
      entryComplete = false;
    }

    void readKeypad(byte maxDigits, char enteredKey)
    {
      entryComplete = false;
      key = enteredKey;
      if (key == '*')
      {
        entryComplete = true;
        //digitCount = 0;
        return;
      }
      else if (key == 'D' && digitCount > 0)
      {
        total = total / 10;
        digitCount--;
      }
      if (digitCount == maxDigits)
      {
        return;
      }
      if (key >= '0' && key <= '9')
      {
        keyAsANumber = key - 48;
        total = total * 10;
        total = total + keyAsANumber;
        digitCount++;
      }
    }
};

KeypadManager keypadManager;

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

    void initGameScreen() {  //page index -1
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

    void displayCurrentCredit(int number) {   //page index 0
      clear();
      displayMessage("Current credits:", 0, 0);
      displayNumber(number, 0, 1);
    }

    void displayWheelNumber(int number) {   //page index 1
      clear();
      displayMessage("Roulette lands", 0, 0);
      displayMessage("on", 0, 1);
      displayNumber(number, 3, 1);
    }
    
    void displayBetPage1(){   //page index 2
      clear();
      displayMessage("Choose number", 0, 0);
      displayNumber(keypadManager.getNumberInScratchPad(), 0, 1);
    }

    void displayBetPage1Error(){    //page index 3
      clear();
      displayMessage("Number invalid", 0, 0);
    }

    void displayBetPage2(){   //page index4
      clear();
      displayMessage("Choose amount", 0, 0);
      displayNumber(keypadManager.getNumberInScratchPad(), 0, 1);
    }

    void displayBetPage2Error(){
      clear();
      displayMessage("Insufficient ", 0, 0);
      displayMessage("amount ", 0, 1);
    }

    void displayAddingBet(){
      clear();
      displayMessage("Adding bet", 0, 0);
      delay(1000);
      clear();
      displayMessage("Continue bet?", 0, 0);
      displayMessage("1-Yes, 2-No", 0, 1);
    }

    void displayBetFinished(){
      clear();
      displayMessage("Bet success!!", 0, 0);
      delay(1000);
      clear();
      displayMessage("Ready to roll!", 0, 0);
    }

    void displayMatchFinishedWin(int number){
      clear();
      displayMessage("You won!!", 0, 0);
      displayMessage(number, 0, 1);
      displayMessage("credits", 4, 1);
    }

    void displayMatchFinishedLost(int number){
      clear();
      displayMessage("You lost :(", 0, 0);
      //Serial.println(number);
      //displayMessage(number, 0, 1);
      //displayMessage("credits", 4, 1);
    }


};

class CreditSetUp {
  private:
    int credits;

  public:
    CreditSetUp() {
      credits = 2000;
    }

    int getCredits() {
      return credits;
    }

    void addCredits(int amount) {
      credits += amount;
    }

    void subtractCredits(int amount) {
      if (credits >= amount) {
        credits -= amount;
      } else {
        credits = 0;
      }
    }

    void resetCredits() {
      credits = 0;
    }
};

class WheelManager {
  private:
    unsigned long initWheelSpeedStore;
    int startPin, endPin;
    bool wheelRunning;
    bool wheelReadyForNextMatch;
    bool wheelFinished;
    unsigned long startTime;
    unsigned long slowdownStartTime;
    unsigned long stopTime;
    int lastPin;
    unsigned long currentDelay;
    bool slowingDown;
    LCDManager& lcdManager;

  public:
    WheelManager(int start, int end, unsigned long initWheelSpeed, LCDManager& lcd) 
      : startPin(start), endPin(end + 1), initWheelSpeedStore(initWheelSpeed), lcdManager(lcd) {
      wheelRunning = false;
      lastPin = start;
      currentDelay = initWheelSpeed;
      slowingDown = false;
      wheelReadyForNextMatch = true;
    }

    void stopWheel() {
      wheelRunning = false;
      wheelReadyForNextMatch = false;
      wheelFinished = false;
      digitalWrite(lastPin, HIGH);
      lcdManager.displayWheelNumber(getStopNumber());   //move to gameManager later
    }

    bool getWheelFinished () {
      return wheelFinished;
    }

    int getStopNumber() {
      if (lastPin == 39) {
        return 0; //Special case for pin 39
      } else {
        return (lastPin - 27);
      }
    }

    bool isWheelRunning() {
      return wheelRunning;
    }

    bool isWheelReady() {
      return wheelReadyForNextMatch;
    }

    void runWheel() {
      wheelRunning = true;
      wheelReadyForNextMatch = false;
      wheelFinished = false;
      startTime = millis();
      slowdownStartTime = startTime + random(7000, 12000);
      stopTime = startTime + random(22000, 38000);
      lastPin = random(startPin, endPin);
      if (wheelRunning) {
        for (int i = lastPin; i <= endPin; i++) {
          if (!slowingDown && millis() >= slowdownStartTime) {
            slowingDown = true;
          }

          if (slowingDown) {
            currentDelay += 5;
          }

          if (i == endPin) {
            i = startPin;
          }

          if (millis() >= stopTime) {
            stopWheel();
            wheelFinished = true;
            break;
          }

          digitalWrite(i, HIGH);
          delay(currentDelay);
          digitalWrite(i, LOW);
          delay(currentDelay);
          lastPin = i;
        }
      }
    }

    void resetWheel() {
      wheelRunning = false;
      wheelReadyForNextMatch = true;
      wheelFinished = false;
      currentDelay = initWheelSpeedStore;
      slowingDown = false;
      digitalWrite(lastPin, LOW);
    }
};

class GameManager {
  private:
    struct Bet {
        int number;
        int amount;
    };

    int pageIndex[10] = {0  //menu
                      , 1   //wheel result
                      , 2   //betpage1
                      , 3   //betpage1error
                      , 4   //betpage2
                      , 5   //betpage2error
                      , 6   //addingbet
                      , 7   //betFinished
                      , 8   //matchFinishedWin
                      , 9   //matchFinishedLost
                      };
    byte pageIndexActive;
    byte numZeroPin = wheelPins[0];
    byte numLastPin = wheelPins[sizeof(wheelPins) / sizeof(wheelPins[0]) - 1];
    bool betInitiated;
    bool betFinished;
    bool creditModified = false;
    byte tempBetNum;
    unsigned long tempBetAmount;
    Bet* bets;
    int betCount;
    CreditSetUp creditManager;
    LCDManager lcdManager;
    WheelManager wheelManager;

  public:
    GameManager() : wheelManager(numZeroPin, numLastPin, initWheelSpeed, lcdManager), bets(nullptr), betCount(0) {}

    void gameSetUp() {
      betInitiated = false;
      betFinished = false;
      if (bets != nullptr) {
        delete[] bets;
      }
      bets = new Bet[0];
      betCount = 0;
      pageManager(0);
    }

    void matchReset() {
      betInitiated = false;
      betFinished = false;
      creditModified = false;
    }

    ~GameManager() {
      delete[] bets;
    }

    void addBet(int number, int amount) {
      Bet* newBets = new Bet[betCount + 1];
      for (int i = 0; i < betCount; ++i) {
        newBets[i] = bets[i];
      }
      newBets[betCount].number = number;
      newBets[betCount].amount = amount;
      delete[] bets;
      bets = newBets;
      betCount++;
    }

    void refreshScreen() {
      switch (pageIndexActive) {
        case -1: 
          lcdManager.initGameScreen(); // Menu or credits screen
          break;
        case 0:
          lcdManager.displayCurrentCredit(creditManager.getCredits()); // Menu or credits screen
          break;
        case 1:
          lcdManager.displayWheelNumber(wheelManager.getStopNumber()); // Wheel screen
          break;
        case 2:
          lcdManager.displayBetPage1(); // Bet Page 1
          break;
        case 3:
          lcdManager.displayBetPage1Error();  //Bet page 1 error
          break;
        case 4:
          lcdManager.displayBetPage2();   // Bet page 2
          break;
        case 5:
          lcdManager.displayBetPage2Error();    //  Bet page 2 error
          break;
        case 6:
          lcdManager.displayAddingBet();
          break;
        case 7:
          lcdManager.displayBetFinished();
          break;
        case 8:
          lcdManager.displayMatchFinishedWin(bets[betCount].amount);
          break;
        case 9:
          lcdManager.displayMatchFinishedLost(bets[betCount].amount);
          break;
        default:
          break;
      }
    }

    void pageManager(byte newPageIndex) {
      if (newPageIndex < sizeof(pageIndex) / sizeof(pageIndex[0])) {
        pageIndexActive = newPageIndex;
        refreshScreen();
      }
    }

    void pageDirectorFromKeyPad(char key) {
      //Avoiders
      if (pageIndexActive == 2) {
        keypadManager.readKeypad(2, key);
      }
      if (pageIndexActive == 4) {
        keypadManager.readKeypad(4, key);
      }
      if (pageIndexActive == 2 && keypadManager.getInputComplete()) {
        if (0 <= keypadManager.getNumberInScratchPad() && keypadManager.getNumberInScratchPad() <= 11)
        {
          tempBetNum = keypadManager.getNumberInScratchPad();
          keypadManager.resetNumberInScratchPad();
          delay(200);
          pageManager(4);
        }
        else {
          pageManager(3);
        }
      }
      if (pageIndexActive == 3) {
        delay(3000);
        pageManager(2);
      }
      if (pageIndexActive == 4 && keypadManager.getInputComplete()) {
        if (0 <= keypadManager.getNumberInScratchPad() && keypadManager.getNumberInScratchPad() <= creditManager.getCredits())
        {
          tempBetAmount = keypadManager.getNumberInScratchPad();
          keypadManager.resetNumberInScratchPad();
          pageManager(6);
        }
        else {
          pageManager(5);
        }
      }
      if (pageIndexActive == 5) {
        delay(3000);
        pageManager(4);
      }
      if (pageIndexActive == 6 && key == '2') {
        pageManager(7);
        addBet(tempBetNum, tempBetAmount);
      }
      

      switch (key) {
        case 'B':
          initiateBet();
          pageManager(2);
          break;
        case 'C':
          pageManager(0);
          break;
        default:
          break;
      }
    }

    void checkIfBetFinished() {
      if (betCount > 0) {
        betFinished = true;
      }
    }

    void startWheelIfNeeded() {
      checkIfBetFinished();
      bool dealerTouchInput = digitalRead(touchSensorPin);
      //Serial.println("")
      if (dealerTouchInput == HIGH && !wheelManager.isWheelRunning() && wheelManager.isWheelReady() && betFinished) {
        wheelManager.runWheel();
      }
    }

    void runWheel() {
      startWheelIfNeeded();
      pageManager(0);
    }

    void initiateBet() {
      if (betInitiated) {
        return;
      }
      betInitiated = true;
    }

    void checkWinLost() {
      Serial.println(bets[0].amount);
      while (wheelManager.getWheelFinished()) {
        if (keypadManager.getKey() || digitalRead(resetButtonPin) == HIGH) {
          break;
        }
        else {
          //ADD A FOR LOOP TO LOOP THRU THE BETS AND SUBTRACT ADD ACCORINGLY
          if (bets[0].number == wheelManager.getStopNumber())
          {
            if (!creditModified) {
              creditManager.addCredits(bets[0].amount); 
              creditModified = true;
            }
            pageManager(1);
            delay(3000);
            pageManager(8);
            delay(3000);
            pageManager(0);
            delay(3000);
          }
          else {
            if (!creditModified) {
              creditManager.subtractCredits(bets[0].amount);
              creditModified = true;
            }
            pageManager(1);
            delay(3000);
            pageManager(9);
            delay(3000);
            pageManager(0);
            delay(3000);
          }    
        } 
      }
    }

    void checkNextMatchCondition(int buttonPin) {
      if (digitalRead(buttonPin) == HIGH && !wheelManager.isWheelRunning() && betFinished) {
        nextMatch();
      } else {
        if (wheelManager.getWheelFinished()){
          checkWinLost();          
        }
      }
    }

    void nextMatch() {
      wheelManager.resetWheel();
      betInitiated = false;
      betFinished = false;
      creditModified = false;
      delete[] bets;
      betCount = 0;
    }

    CreditSetUp& getCreditManager() {
      return creditManager;
    }

    LCDManager& getLCDManager() {
      return lcdManager;
    }
};

GameManager gameManager;

void setup() {
  Serial.begin(9600);
  pinMode(touchSensorPin, INPUT_PULLUP);
  pinMode(resetButtonPin, INPUT_PULLUP);
  for (byte i = 0; i < (sizeof(wheelPins) / sizeof(wheelPins[0])); i++) {
    pinMode(wheelPins[i], OUTPUT);
  }
  randomSeed(analogRead(0));

  gameManager.gameSetUp();
  gameManager.getLCDManager().initGameScreen();
}

void loop() {
  gameManager.startWheelIfNeeded();

  if (resetButtonPin){
    gameManager.checkNextMatchCondition(resetButtonPin);
  }

  char key = keypadManager.getKey();
  if (key){
    gameManager.pageDirectorFromKeyPad(key);
    gameManager.refreshScreen(); // Refresh screen based on current page
  }
}
