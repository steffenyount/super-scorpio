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

#include "power_monitor_adc.h"

#include "hardware/adc.h"

void start_power_monitor_adc() {
    adc_init();
    adc_set_clkdiv(0); // 0 -> full speed (500 kHz)
    adc_select_input(ADC_NUM);
    adc_set_temp_sensor_enabled(false);
    adc_irq_set_enabled(false);
    adc_fifo_setup(true, true, 1u, true, false);

    adc_run(true);
}

void stop_power_monitor_adc() {
    adc_run(false);

    // wait for done
    while (!(adc_hw->cs & ADC_CS_READY_BITS)) {
        tight_loop_contents();
    }

    // disable adc
    hw_clear_bits(&adc_hw->cs, ADC_CS_EN_BITS);
}