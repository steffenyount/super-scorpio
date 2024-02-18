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

#ifndef SUPER_SCORPIO_PWM_TX_SLICES_H
#define SUPER_SCORPIO_PWM_TX_SLICES_H

//#define WS2812_BITS_PER_SEC (800000u)
#define WS2812_BITS_PER_SEC (8000u)
#define WS2812_PWM_BIT_PERIOD (10u)
// WS2812 reset: gpio pin low for 280us
#define WS2812_PWM_RESET_PERIOD (2240u)
#define WS2812_PWM_OFF_BIT (0u)
#define WS2812_PWM_LOW_BIT (3u)
#define WS2812_PWM_HIGH_BIT (7u)

void init_pwm_tx_slices();

#endif //SUPER_SCORPIO_PWM_TX_SLICES_H
