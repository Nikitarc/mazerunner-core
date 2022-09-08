/*
 * File: sensors.h
 * Project: mazerunner
 * File Created: Monday, 29th March 2021 11:05:58 pm
 * Author: Peter Harrison
 * -----
 * Last Modified: Friday, 9th April 2021 11:45:23 am
 * Modified By: Peter Harrison
 * -----
 * MIT License
 *
 * Copyright (c) 2021 Peter Harrison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "../config.h"
#include "digitalWriteFast.h"
#include <Arduino.h>
#include <util/atomic.h>
#include <wiring_private.h>

// struct Sensor {
//   int raw;       // whatever the ADC gives us
//   int value;     // normalised to 100 at reference position
//   bool has_wall; // true if a wall is present
// };

enum {
  STEER_NORMAL,
  STEER_LEFT_WALL,
  STEER_RIGHT_WALL,
  STEERING_OFF,
};

// used in the wait_for_user_start_function to indicate whih sensor was occluded
const uint8_t NO_START = 0;
const uint8_t LEFT_START = 1;
const uint8_t RIGHT_START = 2;

//***************************************************************************//
extern volatile float g_battery_voltage;
extern volatile float g_battery_scale; // adjusts PWM for voltage changes
//***************************************************************************//



/*** wall sensor variables ***/
extern volatile int g_lfs;
extern volatile int g_lss;
extern volatile int g_rss;
extern volatile int g_rfs;

extern volatile int g_front_sum;

/*** These are the values before normalisation */
extern volatile int g_lfs_raw;
extern volatile int g_lss_raw;
extern volatile int g_rss_raw;
extern volatile int g_rfs_raw;

// true if a wall is present
extern volatile bool g_front_has_wall;

extern volatile bool g_lfs_has_wall;
extern volatile bool g_lss_has_wall;
extern volatile bool g_rss_has_wall;
extern volatile bool g_rfs_has_wall;

/*** steering variables ***/
extern uint8_t g_steering_mode;
extern bool g_steering_enabled;
extern volatile float g_cross_track_error;
extern volatile float g_steering_adjustment;

//***************************************************************************//
class Sensors;
extern Sensors sensors;
class Sensors {
  private:
  float last_steering_error = 0;
  volatile bool s_sensors_enabled = false;
  volatile int adc[6];
  volatile int battery_adc_reading;
  volatile int switches_adc_reading;

  public:
  
volatile float g_battery_voltage;
volatile float g_battery_scale;

volatile int g_ws_lfs;
volatile int g_ws_lss;
volatile int g_ws_rss;
volatile int g_ws_rfs;

/*** wall sensor variables ***/
volatile int g_lfs;
volatile int g_lss;
volatile int g_rss;
volatile int g_rfs;

volatile int g_front_sum;

volatile int g_lfs_raw;
volatile int g_lss_raw;
volatile int g_rss_raw;
volatile int g_rfs_raw;

/*** true if a wall is present ***/
volatile bool g_lss_has_wall;
volatile bool g_front_has_wall;
volatile bool g_rss_has_wall;

/*** steering variables ***/
uint8_t g_steering_mode = STEER_NORMAL;
bool g_steering_enabled;
volatile float g_cross_track_error;
volatile float g_steering_adjustment;

  /**
   *  The default for the Arduino is to give a slow ADC clock for maximum
   *  SNR in the results. That typically means a prescale value of 128
   *  for the 16MHz ATMEGA328P running at 16MHz. Conversions then take more
   *  than 100us to complete. In this application, we want to be able to
   *  perform about 16 conversions in around 500us. To do that the prescaler
   *  is reduced to a value of 32. This gives an ADC clock speed of
   *  500kHz and a single conversion in around 26us. SNR is still pretty good
   *  at these speeds:
   *  http://www.openmusiclabs.com/learning/digital/atmega-adc/
   *
   * @brief change the ADC prescaler to give a suitable conversion rate.
   */
  void setup_adc() {
    // Change the clock prescaler from 128 to 32 for a 500kHz clock
    bitSet(ADCSRA, ADPS2);
    bitClear(ADCSRA, ADPS1);
    bitSet(ADCSRA, ADPS0);
  }

  /**
   * The adc_thresholds may beed adjusting for non-standard resistors.
   *
   * @brief  Convert the switch ADC reading into a switch reading.
   * @return integer in range 0..16 or -1 if there is an error
   */
  int get_switches() {
    const int adc_thesholds[] = {660, 647, 630, 614, 590, 570, 545, 522, 461, 429, 385, 343, 271, 212, 128, 44, 0};

    if (switches_adc_reading > 800) {
      return 16;
    }
    for (int i = 0; i < 16; i++) {
      if (switches_adc_reading > (adc_thesholds[i] + adc_thesholds[i + 1]) / 2) {
        return i;
      }
    }
    return -1;
  }

  //***************************************************************************//

  /**
   * The steering adjustment is an angular error that is added to the
   * current encoder angle so that the robot can be kept central in
   * a maze cell.
   *
   * A PD controller is used to generate the adjustment and the two constants
   * will need to be adjusted for the best response. You may find that only
   * the P term is needed
   *
   * The steering adjustment is limited to prevent over-correction. You should
   * experiment with that as well.
   *
   * @brief Calculate the steering adjustment from the cross-track error.
   * @param error calculated from wall sensors, Negative if too far right
   * @return steering adjustment in degrees
   */
  float calculate_steering_adjustment(float error) {
    // always calculate the adjustment for testing. It may not get used.
    float pTerm = STEERING_KP * error;
    float dTerm = STEERING_KD * (error - last_steering_error);
    float adjustment = (pTerm + dTerm) * LOOP_INTERVAL;
    // TODO: are these limits appropriate, or even needed?
    adjustment = constrain(adjustment, -STEERING_ADJUST_LIMIT, STEERING_ADJUST_LIMIT);
    last_steering_error = error;
    return adjustment;
  }

  void set_steering_mode(uint8_t mode) {
    last_steering_error = g_cross_track_error;
    g_steering_adjustment = 0;
    g_steering_mode = mode;
  }

  //***************************************************************************//

  void enable_sensors() {
    s_sensors_enabled = true;
  }

  void disable_sensors() {
    s_sensors_enabled = false;
  }

  //***************************************************************************//

  void update_battery_voltage() {
    g_battery_voltage = BATTERY_MULTIPLIER * battery_adc_reading;
    g_battery_scale = 255.0 / g_battery_voltage;
  }
  /*********************************** Wall tracking **************************/

  /*********************************** Wall tracking **************************/
  // calculate the alignment errors - too far left is negative

  /***
   * Note: Runs in the systick interrupt. DO NOT call this directly.
   * @brief update the global wall sensor values.
   * @return robot cross-track-error. Too far left is negative.
   */
  float update_wall_sensors() {
    if (not s_sensors_enabled) {
      return 0;
    }

    // they should never be negative
    adc[0] = max(0, adc[0]);
    adc[1] = max(0, adc[1]);
    adc[2] = max(0, adc[2]);
    adc[3] = max(0, adc[3]);

    // this should be the only place that the aactual ADC channels are referenced
    // if there is only a single front sensor (Basic sensor board) then the value is
    // just used twice
    // keep these values for calibration assistance
    g_rfs_raw = adc[RFS_CHANNEL];
    g_rss_raw = adc[RSS_CHANNEL];
    g_lss_raw = adc[LSS_CHANNEL];
    g_lfs_raw = adc[LFS_CHANNEL];

    // normalise to a nominal value of 100
    g_rfs = (int)(g_rfs_raw * FRONT_RIGHT_SCALE);
    g_rss = (int)(g_rss_raw * RIGHT_SCALE);
    g_lss = (int)(g_lss_raw * LEFT_SCALE);
    g_lfs = (int)(g_lfs_raw * FRONT_LEFT_SCALE);

    // set the wall detection flags
    g_lss_has_wall = g_lss > LEFT_THRESHOLD;
    g_rss_has_wall = g_rss > RIGHT_THRESHOLD;
    g_front_sum = g_lfs + g_rfs;
    g_front_has_wall = g_front_sum > FRONT_THRESHOLD;

    // calculate the alignment errors - too far left is negative
    int error = 0;
    int right_error = SIDE_NOMINAL - g_rss;
    int left_error = SIDE_NOMINAL - g_lss;
    if (g_steering_mode == STEER_NORMAL) {
      if (g_lss_has_wall && g_rss_has_wall) {
        error = left_error - right_error;
      } else if (g_lss_has_wall) {
        error = 2 * left_error;
      } else if (g_rss_has_wall) {
        error = -2 * right_error;
      }
    } else if (g_steering_mode == STEER_LEFT_WALL) {
      error = 2 * left_error;
    } else if (g_steering_mode == STEER_RIGHT_WALL) {
      error = -2 * right_error;
    }

    // the side sensors are not reliable close to a wall ahead.
    // TODO: The magic number 100 may need adjusting
    if (g_front_sum > 100) {
      error = 0;
    }
    return error;
  }

  //***************************************************************************//

  /***
   * NOTE: Manual analogue conversions
   * All eight available ADC channels are automatically converted
   * by the sensor interrupt. Attempting to performa a manual ADC
   * conversion with the Arduino AnalogueIn() function will disrupt
   * that process so avoid doing that.
   */

   const uint8_t ADC_REF = DEFAULT;

   void start_adc(uint8_t pin) {
    if (pin >= 14)
      pin -= 14; // allow for channel or pin numbers
                 // set the analog reference (high two bits of ADMUX) and select the
                 // channel (low 4 bits).  Result is right-adjusted
    ADMUX = (ADC_REF << 6) | (pin & 0x07);
    // start the conversion
    sbi(ADCSRA, ADSC);
  }

   int get_adc_result() {
    // ADSC is cleared when the conversion finishes
    // while (bit_is_set(ADCSRA, ADSC));

    // we have to read ADCL first; doing so locks both ADCL
    // and ADCH until ADCH is read.  reading ADCL second would
    // cause the results of each conversion to be discarded,
    // as ADCL and ADCH would be locked when it completed.
    uint8_t low = ADCL;
    uint8_t high = ADCH;

    // combine the two bytes
    return (high << 8) | low;
  }

  uint8_t sensor_phase = 0;
  inline bool button_pressed() {
  return get_switches() == 16;
}

 void wait_for_button_press() {
  while (not(button_pressed())) {
    delay(10);
  };
}

 void wait_for_button_release() {
  while (button_pressed()) {
    delay(10);
  };
}

 void wait_for_button_click() {
  wait_for_button_press();
  wait_for_button_release();
  delay(250);
}

 bool occluded_left() {
  return g_lfs_raw > 100 && g_rfs_raw < 100;
}

 bool occluded_right() {
  return g_lfs_raw < 100 && g_rfs_raw > 100;
}

 uint8_t wait_for_user_start() {
  enable_sensors();
  int count = 0;
  uint8_t choice = NO_START;
  while (choice == NO_START) {
    count = 0;
    while (occluded_left()) {
      count++;
      delay(20);
    }
    if (count > 5) {
      choice = LEFT_START;
      break;
    }
    count = 0;
    while (occluded_right()) {
      count++;
      delay(20);
    }
    if (count > 5) {
      choice = RIGHT_START;
      break;
    }
  }
  disable_sensors();
  delay(250);
  return choice;
}

  void start_sensor_cycle() {
    sensor_phase = 0;     // sync up the start of the sensor sequence
    bitSet(ADCSRA, ADIE); // enable the ADC interrupt
    start_adc(0);         // begin a conversion to get things started
  }

  /** @brief Sample all the sensor channels with and without the emitter on
   *
   * At the end of the 500Hz systick interrupt, the ADC interrupt is enabled
   * and a conversion started. After each ADC conversion the interrupt gets
   * generated and this ISR is called. The eight channels are read in turn with
   * the sensor emitter(s) off.
   * At the end of that sequence, the emiter(s) get turned on and a dummy ADC
   * conversion is started to provide a delay while the sensors respond.
   * After that, all channels are read again to get the lit values.
   * After all the channels have been read twice, the ADC interrupt is disabbled
   * and the sensors are idle until triggered again.
   *
   * The ADC service runs all th etime even with the sensors 'disabled'. In this
   * software, 'enabled' only means that the emitters are turned on in the second
   * phase. Without that, you might expect the sensor readings to be zero.
   *
   * Timing tests indicate that the sensor ISR consumes no more that 5% of the
   * available system bandwidth.
   *
   * There are actually 16 available channels and channel 8 is the internal
   * temperature sensor. Channel 15 is Gnd. If appropriate, a read of channel
   * 15 can be used to zero the ADC sample and hold capacitor.
   *
   * NOTE: All the channels are read even though only 5 are used for the maze
   * robot. This gives worst-case timing so there are no surprises if more
   * sensors are added.
   * If different types of sensor are used or the I2C is needed, there
   * will need to be changes here.
   */
  void update() {
    // digitalWriteFast(13, 1);
    switch (sensor_phase) {
      case 0:
        // always start conversions as soon as  possible so they get a
        // full 50us to convert
        start_adc(BATTERY_VOLTS);
        break;
      case 1:
        battery_adc_reading = get_adc_result();
        start_adc(FUNCTION_PIN);
        break;
      case 2:
        switches_adc_reading = get_adc_result();
        start_adc(A0);
        break;
      case 3:
        adc[0] = get_adc_result();
        start_adc(A1);
        break;
      case 4:
        adc[1] = get_adc_result();
        start_adc(A2);
        break;
      case 5:
        adc[2] = get_adc_result();
        start_adc(A3);
        break;
      case 6:
        adc[3] = get_adc_result();
        start_adc(A4);
        break;
      case 7:
        adc[4] = get_adc_result();
        start_adc(A5);
        break;
      case 8:
        adc[5] = get_adc_result();
        if (s_sensors_enabled) {
          // got all the dark ones so light them up
          digitalWriteFast(EMITTER_A, 1);
          digitalWriteFast(EMITTER_B, 1);
        }
        start_adc(A7); // dummy read of the battery to provide delay
        // wait at least one cycle for the detectors to respond
        break;
      case 9:
        start_adc(A0);
        break;
      case 10:
        adc[0] = get_adc_result() - adc[0];
        start_adc(A1);
        break;
      case 11:
        adc[1] = get_adc_result() - adc[1];
        start_adc(A2);
        break;
      case 12:
        adc[2] = get_adc_result() - adc[2];
        start_adc(A3);
        break;
      case 13:
        adc[3] = get_adc_result() - adc[3];
        start_adc(A4);
        break;
      case 14:
        adc[4] = get_adc_result() - adc[4];
        start_adc(A5);
        break;
      case 15:
        adc[5] = get_adc_result() - adc[5];
        digitalWriteFast(EMITTER_A, 0);
        digitalWriteFast(EMITTER_B, 0);
        bitClear(ADCSRA, ADIE); // turn off the interrupt
        break;
      default:
        break;
    }
    sensor_phase++;
    // digitalWriteFast(13, 0);
  }
};

#endif