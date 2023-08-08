#ifndef VOLTMETER_ARDUINO_H // this is a header guard
#define VOLTMETER_ARDUINO_H

#include "display.h"
#include "joystick.h"
#include "commonlib.h"

#define CYCLE_TIME 20
#define SLEEP_TIME 30000 //sleeps after inactive for 30 seconds
#define BUZZER_TIME 1 //buzzer sounds for at most 1 second
#define NOISE_THRESHOLD 0.2
#define MAX_VALUE 9999

#define BUZZER 9
#define VOLTAGE_PIN A2
#define VCC 5 //volts
#define ADC_MAX 1023 //it is a 10-bit ADC
#define AVERAGING_BUFFER_SIZE 20

// finite state machine state
typedef enum State {WAIT1, ALERT, SET1, MEASURE} State;

class Voltmeter {
  public:
    int setValue; //in hundreds of volts. E.g. setValue = 1 means 0.01 volts
    int cmd; //commands from the joystick
    FlashingState flashingDigitSelect; //which digit flashes/blinks
    Display myDisplay;
    Joystick joystick;

    Voltmeter();
    void print_info();
    void Voltmeter::finite_state_machine();
    String Voltmeter::state_string(State state);
    
  private:
    State state; //finite state machine state
    unsigned long enterStateTime; //when the last change of state happened

    int ADCVal;
    float volts;
    float averagedVolts;

    //states
    void Voltmeter::set1_state();
    void Voltmeter::measure_state();
    void Voltmeter::wait1_state();
    void Voltmeter::alert_state();

    float Voltmeter::adc_to_volts(int adc);
    float Voltmeter::moving_average(float newVal);
    String Voltmeter::state_string_helper(State state);
};

#endif
