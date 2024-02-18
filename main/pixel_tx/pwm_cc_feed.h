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

extern volatile bool pwm_cc_feeds_done;


void init_dma_pwm_cc_feed();

static inline void wait_for_pwm_cc_feeds_done() {
    uint32_t pwm_cc_feeds_count = 0u;
    while(!pwm_cc_feeds_done) {
        if (pwm_cc_feeds_count++ > 8192) break;
//        tight_loop_contents();
    }
    if (pwm_cc_feeds_count > 0u) {
        if (pwm_cc_feeds_count > 8192) {
            log_tick("pwm_cc_feeds_count > 8192");
        } else {

            log_tick("wait_for_pwm_cc_feeds done - after waiting");
        }

    } else {
        log_tick("wait_for_pwm_cc_feeds done");
    }

//    while(!pwm_cc_feeds_done) {
//        tight_loop_contents();
//    }
    pwm_cc_feeds_done = false;
}


#endif //SUPER_SCORPIO_PWM_CC_FEED_H
