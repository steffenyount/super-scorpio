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

#ifndef SUPER_SCORPIO_IRQ_HANDLING_H
#define SUPER_SCORPIO_IRQ_HANDLING_H

#include "pixel_tx.h"

#define PWM_SLICE_LOWER_CC_SAMPLES (1000u)

extern uint32_t pwm_count;
extern uint16_t pwm_slice2_lower_cc_vals[PWM_SLICE_LOWER_CC_SAMPLES];

void init_irq_handling();

void init_core0_irq_handling();


#endif //SUPER_SCORPIO_IRQ_HANDLING_H
