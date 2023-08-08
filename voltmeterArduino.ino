/*To understand the context and purpose of this code, visit:
 * https://www.instructables.com/How-to-Make-a-Battery-Tester-and-Voltmeter/
 * This code makes references to steps on this Instructables website
 * written by square1a on 8th August 2023
 * 
 * The code is free and open to be used and modified.
*/

#include "voltmeterArduino.h"

void setup() {
  // there is NO loop() function like in other Arduino code
  Serial.begin(BAUD_RATE); // activates the serial communication
  Serial.println("start of program42");
  Voltmeter meter;
   
  while(1) {
    char c = Serial.read();
    if (c == 'i') {
      meter.print_info();
      meter.joystick.print_info();
      meter.myDisplay.print_info();
    }
    
    meter.cmd = meter.joystick.get_command();
    if (meter.cmd != NO_ACTION) {
      PRINT_VAR("meter.cmd", meter.joystick.command_string(meter.cmd));
    }
    meter.finite_state_machine();
    
    delay(CYCLE_TIME);
  }
}

Voltmeter::Voltmeter() {
  pinMode(VOLTAGE_PIN, INPUT);
  this->state = MEASURE; //finite state machine state
  this->enterStateTime = millis(); //when the last change of state happened
  this->setValue = 110; //in seconds
  this->cmd = NO_ACTION; //commands from the joystick. NO_ACTION == CENTRE.
  this->flashingDigitSelect = HUNDREDS; //which digit flashes/blinks
  this->myDisplay.setDecimalPlace(TWO_DECIMAL_PLACE);
  this->myDisplay.flash_digit(ALL_ON);

  this->ADCVal = 0;
  this->volts = 0;
  this->averagedVolts = 0; 
  //from the moving average, which prevents the display from flickering
  //when voltage value fluctuates too quickly.
}

void Voltmeter::finite_state_machine() {
  State prev_state = this->state;
  switch (this->state) {
    case WAIT1:
    wait1_state();
    break;
    case ALERT:
    alert_state();
    break;
    case SET1:
    set1_state();
    break;
    case MEASURE:
    measure_state();
    break;
    default:
    print_var_full("state", this->state, "ERROR in Timer::finite_state_machine");
    break;
  }

  if (prev_state != this->state) {
    prev_state = this->state;
    print_var_full("state", this->state, "state changed in Timer::finite_state_machine");
    this->myDisplay.flash_digit(ALL_ON);
    this->enterStateTime = millis();
    noTone(BUZZER);
  }
}

void Voltmeter::set1_state() {
  const int digitValue[] = {1, 10, 100, 1000}; //ONES is 1, HUNDREDS is 100
  switch (this->cmd) {
    case UP + HOLDING:
    case UP:
    bounded_addition(&this->setValue, MAX_VALUE, digitValue[this->flashingDigitSelect], 0, true);
    break;
    case DOWN + HOLDING:
    case DOWN:
    bounded_addition(&this->setValue, MAX_VALUE, -digitValue[this->flashingDigitSelect], 0, true);
    break;
    case LEFT + HOLDING:
    case LEFT:
    CYCLICAL_NEXT(&this->flashingDigitSelect, MOST_SIGNIFICANT);
    break;
    case RIGHT + HOLDING:
    case RIGHT:
    CYCLICAL_PREV(&this->flashingDigitSelect, MOST_SIGNIFICANT);
    break;
  }

  //updates the Display class
  this->myDisplay.display_number(this->setValue);
  this->myDisplay.flash_digit(this->flashingDigitSelect);

  //state transitions
  if (this->cmd == BUTTON_CLICK) {
    this->state = MEASURE;
  } else if (this->cmd != NO_ACTION) {
    this->enterStateTime = millis(); //so it re-enters the SET1 state, to avoid WAIT1 from triggering
  } else if (millis() - this->enterStateTime > SLEEP_TIME) {
    this->state = WAIT1;
  }
}

void Voltmeter::wait1_state() {
  this->myDisplay.flash_digit(ALL_OFF);
  if (this->cmd != NO_ACTION) {
    this->state = MEASURE;
  }
}

void Voltmeter::measure_state() {
  static float prevVolts = this->volts;
  static int ifToAlert = 0;
  bool sleep = true;
  this->ADCVal = analogRead(VOLTAGE_PIN);
  this->volts = adc_to_volts(ADCVal);
  this->averagedVolts = moving_average(volts);
  this->myDisplay.display_number(round(this->averagedVolts * 100)); // * 100 since it diplays two decimal points

  if (abs(prevVolts - this->volts) > NOISE_THRESHOLD) { //a significant change in voltage is detected
    prevVolts = this->volts;
    sleep = false;
  }
  if (this->averagedVolts >= NOISE_THRESHOLD  && this->averagedVolts * 100 < this->setValue) {
    ifToAlert++;
  } else {
    ifToAlert = 0;
  }
  
  //state transitions
  if (this->cmd == BUTTON_CLICK) {
    this->state = SET1;
  } else if (ifToAlert > AVERAGING_BUFFER_SIZE) {
    this->state = ALERT;
  } else if (this->cmd != NO_ACTION || !sleep) {
    this->enterStateTime = millis(); //so it re-enters the SET1 state, to avoid WAIT1 from triggering
  } else if (millis() - this->enterStateTime > SLEEP_TIME) {
    this->state = WAIT1;
  }
}

void Voltmeter::alert_state() {
  tone(BUZZER, 4000);
  
  //state transitions
  if (this->cmd != NO_ACTION || millis() - this->enterStateTime > BUZZER_TIME) {
    this->state = MEASURE;
  }
}

float Voltmeter::adc_to_volts(int adc) {
  return adc * (float)VCC / (float)ADC_MAX;
}

float Voltmeter::moving_average(float newVal) {
  static float averaging_buffer[AVERAGING_BUFFER_SIZE] = {0};
  float sum = 0.0;
  for (int i = 0; i < AVERAGING_BUFFER_SIZE - 1; i++) {
    averaging_buffer[i] = averaging_buffer[i + 1];
    sum += averaging_buffer[i];
  }
  averaging_buffer[AVERAGING_BUFFER_SIZE - 1] = newVal;
  sum += averaging_buffer[AVERAGING_BUFFER_SIZE - 1];
  return sum / (float)AVERAGING_BUFFER_SIZE;
}

/////debugging
String Voltmeter::state_string(State state) {
  return state_string_helper(state) + " (" + String(state) + ")";
}
String Voltmeter::state_string_helper(State state) {
  switch (state) {
    case WAIT1:
    return "WAIT1";
    case ALERT:
    return "ALERT";
    case SET1:
    return "SET1";
    case MEASURE:
    return "MEASURE";
  }
  return "unknown";
}

void Voltmeter::print_info() {
  print_class_name("voltmeter");
  PRINT_VAR("state", state_string(this->state));
  PRINT_VAR("enterStateTime", this->enterStateTime);
  PRINT_VAR("setValue", this->setValue);
  PRINT_VAR("cmd", this->joystick.command_string(this->cmd));
  PRINT_VAR("flashingDigitSelect", this->flashingDigitSelect);

  PRINT_VAR("ADCVal", this->ADCVal);
  PRINT_VAR("volts", this->volts);
  PRINT_VAR("averagedVolts", this->averagedVolts);
}
