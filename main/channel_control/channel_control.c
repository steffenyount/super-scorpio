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

#include "channel_control.h"
#include "pwm_slices.h"
#include "pwm_feeds.h"

void set_gpio_channel_pixels_on_for_byte_range(uint8_t gpio_num, uint32_t start, uint32_t end) {
    start_pwm_slice_for_gpio(gpio_num);
    feed_high_bits_to_pwm_slice_for_byte_range(gpio_num, start, end);
    pwm_slice_wait_for_shutdown_blocking(active_pwm_slice_num);
}

void init_pixel_control() {
    init_pwm_config();
}