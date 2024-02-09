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

#include "pwm_tx_slices.h"

#include "hardware/clocks.h"
#include "hardware/pwm.h"

void __not_in_flash_func(init_pwm_tx_slices)() {
    pwm_config pwm_cfg = {0, 0, 0};
    pwm_config_set_phase_correct(&pwm_cfg, false);
    pwm_config_set_output_polarity(&pwm_cfg, false, false);
    pwm_config_set_clkdiv_mode(&pwm_cfg, PWM_DIV_FREE_RUNNING);
    float div = (float)clock_get_hz(clk_sys) / (WS2812_PWM_BIT_PERIOD * WS2812_BITS_PER_SEC);
    pwm_config_set_clkdiv(&pwm_cfg, div);
    pwm_config_set_wrap(&pwm_cfg, WS2812_PWM_BIT_PERIOD - 1u);

    // Set WS2812 tx configurations for all 8 PWM slices
    for (int slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
        pwm_init(slice_num, &pwm_cfg, false);
    }
}
