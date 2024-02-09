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

#include "on_off_feed.h"
#include "pixel_tx/pixel_tx.h"

bool __scratch_y("super_scorpio") on_off_feed_value = true;

uint32_t __scratch_x("super_scorpio") src_pixel = 0u;

static void __not_in_flash_func(on_off_feed__open_frame)(tx_feed_t * this, uint8_t gpio_num) {
}

static bool __not_in_flash_func(on_off_feed__advance_pixel)(tx_feed_t * this, uint8_t gpio_num) {
    if (curr_pixel < 1) {
        uint16_t pos = (curr_frame >> 1) + curr_pixel;
        uint16_t idx = pos & 3u;
        uint8_t bit = 1u << ((pos >> 2) & 7u);
        if (0u == idx) {
            gpio_tx_pixels[gpio_num] = (tx_pixel_t) {
                    .blue = on_off_feed_value ? 0x66 : 0x00,
                    .red = on_off_feed_value ? 0x66 : 0x00,
                    .green = on_off_feed_value ? 0x66 : 0x00,
            };
            set_tx_pixel_enabled(gpio_num);

            if (on_off_feed_value && 0 == curr_pixel) {
                src_pixel = ((uint32_t *) gpio_tx_pixels)[gpio_num];
            }

            return true;

        } else if (1u == idx) {
            gpio_tx_pixels[gpio_num] = (tx_pixel_t) {
                    .blue = 0x00,
                    .red = 0x00,
                    .green = on_off_feed_value ? bit : 0x00,
            };
            set_tx_pixel_enabled(gpio_num);

            if (on_off_feed_value && 0 == curr_pixel) {
                src_pixel = ((uint32_t *) gpio_tx_pixels)[gpio_num];
            }

            return true;

        } else if (2u == idx) {
            gpio_tx_pixels[gpio_num] = (tx_pixel_t) {
                    .blue = 0x00,
                    .red = on_off_feed_value ? bit : 0x00,
                    .green = 0x00,
            };
            set_tx_pixel_enabled(gpio_num);

            if (on_off_feed_value && 0 == curr_pixel) {
                src_pixel = ((uint32_t *) gpio_tx_pixels)[gpio_num];
            }

            return true;

        } else /* if (3u == idx) */ {
            gpio_tx_pixels[gpio_num]  = (tx_pixel_t) {
                    .blue = on_off_feed_value ? bit : 0x00,
                    .red = 0x00,
                    .green = 0x00,
            };
            set_tx_pixel_enabled(gpio_num);

            if (on_off_feed_value && 0 == curr_pixel) {
                src_pixel = ((uint32_t *) gpio_tx_pixels)[gpio_num];
            }

            return true;
        }
    } else {
        return false;
    }
}

static void __not_in_flash_func(on_off_feed__close_frame)(tx_feed_t * this, uint8_t gpio_num) {
    // no-op
}

tx_feed_t __scratch_y("super_scorpio") on_off_feed = {
        .open_frame = on_off_feed__open_frame,
        .advance_pixel = on_off_feed__advance_pixel,
        .close_frame = on_off_feed__close_frame
};
