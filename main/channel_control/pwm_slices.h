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

#ifndef SUPER_SCORPIO_PWM_SLICES_H
#define SUPER_SCORPIO_PWM_SLICES_H

#include "channel_discovery/channel_discovery.h"
#include "tick_log/tick_log.h"

#include <hardware/pwm.h>

#define WS2812_BITS_PER_SEC (800000u)
#define WS2812_PWM_BIT_PERIOD (10u)
#define WS2812_PWM_NO_BIT (0u)
#define WS2812_PWM_LOW_BIT (3u)
#define WS2812_PWM_HIGH_BIT (7u)

extern volatile uint32_t active_pwm_slice_num;

static inline void pwm_slice_wait_for_shutdown_blocking(uint8_t slice_num) {
    uint32_t slice_mask = 1u << slice_num;
    while(pwm_hw->en & slice_mask) {
        tight_loop_contents();
    }
}

static inline uint32_t * pwm_slice_csr_clear_alias_for_gpio(uint8_t gpio_num) {
    return hw_clear_alias_untyped(&pwm_hw->slice[pwm_gpio_to_slice_num(gpio_num)].csr);
}

void init_pwm_config();

void start_pwm_slice_for_gpio(uint8_t gpio_num);

static inline void stop_pwm_slice(uint8_t pwm_slice_num) {
    hw_clear_bits(&pwm_hw->slice[pwm_slice_num].csr, PWM_CH0_CSR_EN_BITS);
}


#endif //SUPER_SCORPIO_PWM_SLICES_H
