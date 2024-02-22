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

#include "gpio_tx_pins.h"
#include "common.h"

#include "hardware/gpio.h"

void __not_in_flash_func(init_gpio_tx_pins)() {
    // Tell the gpio pins that PWM is in charge of their values.
    gpio_set_function(SCORPIO_DEFAULT_WS2812_PIN, GPIO_FUNC_PWM);

    for (int gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        gpio_set_function(gpio_num, GPIO_FUNC_PWM);
    }
}