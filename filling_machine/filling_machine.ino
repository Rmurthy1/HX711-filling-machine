#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, A4, A1);

const int relayPin = 7; // RELAY SWITCH
const int fillButtonPin = A5; // FILL BUTTON
const int tareButtonPin = 6; // TARE BUTTON
const int potPin = 7; // weight pot
const int stopButtonPin = A0; // emergency stop button

// encoder
const int CLK = 2;
const int DT = 3;

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir ="";

// color pins
const int redPin = 10;
const int greenPin = 9;
const int bluePin = 8;
// weight adjustment
int baseValue = 500;
int targetValue = baseValue; //modified in interrupts

const int _DoutPin = A3;
const int _SckPin = A2;
long _offset = 0; // tare value
int _scale = 500;


int period = 20;
unsigned long time_now = 0;

int oldPotValue = 0;
bool isFilling = false;

float readWeight() {
  long val = (getValue() - _offset);
  return (float) val / _scale;
}

long getValue() {
  uint8_t data[3];
    long ret;
    while (digitalRead(_DoutPin)); // wait until _dout is low
    for (uint8_t j = 0; j < 3; j++)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            digitalWrite(_SckPin, HIGH);
            bitWrite(data[2 - j], 7 - i, digitalRead(_DoutPin));
            digitalWrite(_SckPin, LOW);
        }
    }

    digitalWrite(_SckPin, HIGH);
    digitalWrite(_SckPin, LOW);
    ret = (((long) data[2] << 16) | ((long) data[1] << 8) | (long) data[0])^0x800000;
    return ret;
}

void prepareScale() {
    pinMode(_SckPin, OUTPUT);
    pinMode(_DoutPin, INPUT);

    digitalWrite(_SckPin, HIGH);
    delayMicroseconds(100);
    digitalWrite(_SckPin, LOW);
    _offset = getValue();
}

void setup() {
  Serial.begin(9600);
  prepareScale();
  lcd.begin(16, 2);
  lcd.print("curr |targ |T");
  lcd.setCursor(0, 1);
  pinMode(tareButtonPin, INPUT);
  pinMode(fillButtonPin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(stopButtonPin, INPUT);

  // Set encoder pins as inputs
  pinMode(CLK,INPUT);
  pinMode(DT,INPUT);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);
  
  // Call updateEncoder() when any high/low changed seen
  // on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(digitalPinToInterrupt(CLK), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT), updateEncoder, CHANGE);
}

void updateEncoder(){
  // Read the current state of CLK
  currentStateCLK = digitalRead(CLK);
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
    if (digitalRead(DT) != currentStateCLK) {
      counter --;
    } else {
      counter ++;
    }
  }
  lastStateCLK = currentStateCLK;
  targetValue = baseValue + (counter * 5);
}

int status = 0; // 0 = default, 1 = running, 2 = full, 3 = error
unsigned long currentTime = 0;
unsigned long startTime = 0;
void updateStatus(){
  switch (status) {
    case 0:
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
      digitalWrite(redPin, LOW);
      digitalWrite(relayPin, LOW);
      currentTime = 0;
      break;
    case 1:
      digitalWrite(bluePin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(redPin, LOW);
      digitalWrite(relayPin, HIGH);
      currentTime = millis() - startTime;
      Serial.println(currentTime);
      break;
    case 2:
      digitalWrite(redPin, HIGH);
      digitalWrite(bluePin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(relayPin, LOW);
      break;
  }
}


void loop() {


   long scaleValue;
   if (millis() > time_now + period) {
    time_now = millis();
    scaleValue = readWeight();

    
    clearSecondRowAndPositionCursor();
    lcd.print(scaleValue);
    lcd.print(" g");
    lcd.setCursor(5, 1);
    lcd.print("|");
    lcd.print(targetValue);
    lcd.print(" g|");
    lcd.print((currentTime / 1000));
   } 
  
  int tareButtonState = digitalRead(tareButtonPin);
  if (tareButtonState == HIGH) {
    _offset = getValue();
    Serial.print("tare pressed. offset: ");
    Serial.println(_offset);
    status = 0;
  }
  int fillButtonState = digitalRead(fillButtonPin);
  if (fillButtonState == HIGH && scaleValue < targetValue) {
   
    status = 1;
    Serial.println("fill started");
    startTime = millis();
  } else if (scaleValue >= targetValue) {
   
    status = 2;
  } 

  // update the status variable and then the led
  
  updateStatus();

  // check emergency stop
  int stopButtonState = digitalRead(stopButtonPin);
  if (stopButtonState == HIGH) {
    status = 0;
    digitalWrite(relayPin, LOW);
  }
}

void clearSecondRowAndPositionCursor() {
  lcd.setCursor(0,1);
  for (int i = 0; i < 16; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(0,1);
}
