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

#include "pwm_slices.h"

#include <hardware/pwm.h>
#include <hardware/clocks.h>


volatile uint32_t active_pwm_slice_num = 0u;

static pwm_config pwm_cfg = {0, 0, 0};

void init_pwm_config() {
    pwm_config_set_phase_correct(&pwm_cfg, false);
    pwm_config_set_output_polarity(&pwm_cfg, false, false);
    pwm_config_set_clkdiv_mode(&pwm_cfg, PWM_DIV_FREE_RUNNING);
    float div = (float)clock_get_hz(clk_sys) / (WS2812_PWM_BIT_PERIOD * WS2812_BITS_PER_SEC);
    pwm_config_set_clkdiv(&pwm_cfg, div);
    pwm_config_set_wrap(&pwm_cfg, WS2812_PWM_BIT_PERIOD - 1u);
}

void start_pwm_slice_for_gpio(uint8_t gpio_num) {
    //pwm_slice_wait_for_shutdown_blocking(active_pwm_slice_num);
    active_pwm_slice_num = pwm_gpio_to_slice_num(gpio_num);

    gpio_set_function_no_side_effects(gpio_num, GPIO_FUNC_PWM);

    pwm_init(active_pwm_slice_num, &pwm_cfg, true);
}
