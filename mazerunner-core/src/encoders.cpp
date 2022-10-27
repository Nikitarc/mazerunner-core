/******************************************************************************
 * Project: mazerunner-core                                                   * 
 * File:    encoders.cpp                                                      * 
 * File Created: Tuesday, 25th October 2022 10:58:35 am                       * 
 * Author: Peter Harrison                                                     * 
 * -----                                                                      * 
 * Last Modified: Thursday, 27th October 2022 10:43:06 pm                     * 
 * -----                                                                      * 
 * Copyright 2022 - 2022 Peter Harrison, Micromouseonline                     * 
 * -----                                                                      * 
 * Licence:                                                                   * 
 *     Use of this source code is governed by an MIT-style                    * 
 *     license that can be found in the LICENSE file or at                    * 
 *     https://opensource.org/licenses/MIT.                                   * 
 ******************************************************************************/


#include "encoders.h"

/**
 * Measurements indicate that even at 1500mm/s the total load due to
 * the encoder interrupts is less than 3% of the available bandwidth.
 */

#if defined(ARDUINO_ARCH_AVR)
// INT0 will respond to the XOR-ed pulse train from the left encoder
// runs in constant time of around 3us per interrupt.
// would be faster with direct port access
ISR(INT0_vect) {
  encoders.left_input_change();
}

// INT1 will respond to the XOR-ed pulse train from the right encoder
// runs in constant time of around 3us per interrupt.
// would be faster with direct port access
ISR(INT1_vect) {
  encoders.right_input_change();
}

#endif