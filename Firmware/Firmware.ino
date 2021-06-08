#include <Wire.h>
#include <avr/sleep.h>

// ------------------------------------------
// Configuration
// ------------------------------------------
#define I2C_ADDRESS 0x18
#define SWITCH_DEBOUNCE_TIME 50 // ms

//Analog
#define ANALOG_PIN_X 7
#define ANALOG_PIN_Y 6
#define VOLTAGE_PIN 2
#define AMPERAGE_PIN 3

//Digital
#define LOWBATT_PIN 12
#define MUTE_PIN 13
const int BTN_DIS = 9;
const int BTN_MUTE = 11;
int PWMarray[] = {55,63,71,79,87,95,103,111,255};
int PWMposition = 1;
int BTN_DIS_STATUS = 0;
int BTN_MUTE_STATUS = 0;

//PWM
#define PWM_PIN 5

// switch type definition
#define BTN_A           0x00
#define BTN_B           0x01
#define BTN_X           0x02
#define BTN_Y           0x03
#define BTN_LT          0x04
#define BTN_RT          0x05
#define BTN_SL          0x06
#define BTN_ST          0x07
#define BTN_U           0x08
#define BTN_D           0x09
#define BTN_L           0x0A
#define BTN_R           0x0B
#define BTN_H           0x0C
//#define BTN_DIS          0x0D
//#define BTN_14          0x0E
//#define BTN_15          0x0F

struct InputSwitch {
  unsigned char pin;
  unsigned char state;
  unsigned long time;
  unsigned char code;
};

// the (up to 16) switches (arduino pin #, state, time, code)
InputSwitch switches[] = {
  {4, HIGH, 0, BTN_A},    // A
  {6, HIGH, 0, BTN_B},    // B
  {20, HIGH, 0, BTN_X},   // X
  {21, HIGH, 0, BTN_Y},   // Y
  {14, HIGH, 0, BTN_LT},  // LTRIGGER
  {3, HIGH, 0, BTN_RT},   // RTRIGGER
  {8, HIGH, 0, BTN_SL},   // SELECT
  {7, HIGH, 0, BTN_ST},   // START
  {0, HIGH, 0, BTN_U},    // UP
  {1, HIGH, 0, BTN_D},    // DOWN
  {15, HIGH, 0, BTN_L},   // LEFT
  {2, HIGH, 0, BTN_R},    // RIGHT
  {10, HIGH, 0, BTN_H},   // HOME
//  {9, HIGH, 0, BTN_DIS},    // DISPLAY
};


// return true if switch state has changed!
bool updateSwitch(struct InputSwitch *sw) {
  int newState = digitalRead(sw->pin);

  if(newState != sw->state && millis() - sw->time > SWITCH_DEBOUNCE_TIME) {
    // change state!
    sw->state = newState;

    // record last update
    sw->time = millis();
    
    return true;
  }

  // else 
  return false;
}

// I2C data definition
struct I2CJoystickStatus {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
  uint8_t voltage; //
  uint8_t amperage; //
};

I2CJoystickStatus joystickStatus;

//void (*stateFunction)(void);
//unsigned long wakeUpTime = 0;

//void Scan() {
  // update switch etc
//  scanAnalog();
//  scanInput();
//}

void setup()
{
  Wire.begin(I2C_ADDRESS);      // join i2c bus 
  Wire.onRequest(requestEvent); // register event
  //change pwm frequency
//  TCCR0B = TCCR0B & B11111000 | B00000001;
  //Start PWM
  pinMode(LOWBATT_PIN, OUTPUT);
  digitalWrite(LOWBATT_PIN, 0);
  delay(1000);
  digitalWrite(LOWBATT_PIN, 1);
  delay(1000);
  digitalWrite(LOWBATT_PIN, 0);
  
  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, 0);
  delay(1000);
  digitalWrite(MUTE_PIN, 1);
  delay(1000);
  digitalWrite(MUTE_PIN, 0);


  pinMode(PWM_PIN, OUTPUT);
  analogWrite(PWM_PIN, PWMarray[PWMposition]);
  pinMode(BTN_DIS, INPUT_PULLUP);
  pinMode(BTN_MUTE, INPUT_PULLUP);

  // default status
  joystickStatus.buttons = 0;
  joystickStatus.axis0 = 127;
  joystickStatus.axis1 = 127;
  
  // pin configuration
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    pinMode(switches[i].pin, INPUT_PULLUP);
  }
//  stateFunction = Scan;
}

void scanInput() {
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    if(updateSwitch(&switches[i])) {
      if(switches[i].state == HIGH) // button released
        joystickStatus.buttons &= ~(1 << switches[i].code);
      else // button pressed
        joystickStatus.buttons |= (1 << switches[i].code);
    }
  }

  static uint16_t oldButtons = 0;
  if(joystickStatus.buttons != oldButtons) {
    oldButtons = joystickStatus.buttons;
  }
}

void scanAnalog() {
  // read analog stick values, bitshift to uint8_t because it doesnt need more accuracy
  uint8_t x = analogRead(ANALOG_PIN_X) / 4;
  uint8_t y = analogRead(ANALOG_PIN_Y) / 4;


   //temporary troubleshooting 
//  x = 127;
//  y = 127;
  // store them in the I2C data
  joystickStatus.axis0 = x;
  joystickStatus.axis1 = y;
  // read raw power status. calculations might eventually be done on atmega and sent to Pi
  // this is sent over 8 bits because the maximum value is 116
  joystickStatus.voltage = analogRead(VOLTAGE_PIN);
  joystickStatus.amperage = analogRead(AMPERAGE_PIN);
}

void loop() {
  // execute state function
//  Scan();
  scanAnalog();
  scanInput();
    if(digitalRead(BTN_DIS) == 0 and BTN_DIS_STATUS == 0){         // If button is pressed
    BTN_DIS_STATUS = 1;
    PWMposition++;
    if(PWMposition > 8) {PWMposition = 0;}
    analogWrite(PWM_PIN, PWMarray[PWMposition]);
    }
    if(digitalRead(BTN_DIS) == 1 and BTN_DIS_STATUS == 1){         // If button is not pressed
    BTN_DIS_STATUS = 0;
    }
    
    if(digitalRead(BTN_MUTE) == 0 and BTN_MUTE_STATUS == 0){         // If button is pressed
    digitalWrite(MUTE_PIN, (! digitalRead(MUTE_PIN)));
    BTN_MUTE_STATUS = 1;
    }
    if(digitalRead(BTN_MUTE) == 1 and BTN_MUTE_STATUS == 1){         // If button is not pressed
    BTN_MUTE_STATUS = 0;
    }

  //was set to 10, im testing
  delay(10);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write((char *)&joystickStatus, sizeof(I2CJoystickStatus)); 
}
