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

#ifndef SUPER_SCORPIO_PWM_CC_FEED_TRIGGER_H
#define SUPER_SCORPIO_PWM_CC_FEED_TRIGGER_H

#include "tick_log/tick_log.h"
#include "pixel_tx.h"

#include "hardware/dma.h"

#define PWM_SENTINEL_SLICE (4u)
#define PWM_SENTINEL_SLICE_MASK (1u << PWM_SENTINEL_SLICE)

void init_dma_pwm_cc_feed_trigger();

//extern volatile bool pwm_cc_feed_trigger_busy;

static inline void trigger_pwm_cc_feeds() {
    log_tick("trigger_pwm_cc_feeds()");
//    pwm_cc_feed_trigger_busy = true;
    dma_hw->multi_channel_trigger = DMA_PWM_CC_FEED_TRIGGER_CHAN_MASK;
}

//static inline bool pwm_cc_feed_trigger_is_busy() {
//    return pwm_cc_feed_trigger_busy;
//    //    return 0 != (dma_hw->ch[DMA_PWM_CC_FEED_TRIGGER_CHAN].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS);
//}

#endif //SUPER_SCORPIO_PWM_CC_FEED_TRIGGER_H
