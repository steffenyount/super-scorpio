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

#ifndef SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H
#define SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H

#include "pixel_tx.h"

#include "hardware/dma.h"

extern uint32_t * rxf_dests_for_dma_pwm_cc_feed[17];
extern uint32_t * rxf_dests_for_dma_pwm_cc_feed_last[17];


void init_dma_pwm_cc_feed_director();

#endif //SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H
