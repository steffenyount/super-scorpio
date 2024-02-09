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

#include "empty_feed.h"


static void __not_in_flash_func(empty_feed__open_frame)(tx_feed_t * this, uint8_t gpio_num) {
}

static bool __not_in_flash_func(empty_feed__advance_pixel)(tx_feed_t * this, uint8_t gpio_num) {
    return false;
}

static void __not_in_flash_func(empty_feed__close_frame)(tx_feed_t * this, uint8_t gpio_num) {
    // no-op
}

tx_feed_t __scratch_y("super_scorpio") empty_feed = {
        .open_frame = empty_feed__open_frame,
        .advance_pixel = empty_feed__advance_pixel,
        .close_frame = empty_feed__close_frame
};
