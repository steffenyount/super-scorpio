/**
 * Copyright (C) 2024 Steffen Yount
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SUPER_SCORPIO_PWM_CC_FEED_H
#define SUPER_SCORPIO_PWM_CC_FEED_H

#include "tick_log/tick_log.h"
#include "pixel_tx.h"

#define TX_BYTE_ON  (0xffu)
#define TX_BYTE_OFF (0x00u)

extern volatile bool pwm_cc_feed_done;

void init_dma_pwm_cc_feed();


__force_inline static void wait_for_pwm_cc_feeds_done() {
    uint32_t pwm_cc_feed_count = 0u;
    while(!pwm_cc_feed_done) {
        if (pwm_cc_feed_count++ > 4096) break;
//        tight_loop_contents();
    }
    if (pwm_cc_feed_count > 1024) {
        log_tick("pwm_cc_feed_count > 1024");
    }
    pwm_cc_feed_done = false;
}


#endif //SUPER_SCORPIO_PWM_CC_FEED_H
