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

#include "power_limiter.h"
#include "channel_control/channel_control.h"
#include "pixel_channels/tx_channels.h"
#include "power_monitor/power_samples.h"

#define PIXEL_INCREMENT (16u)

// We're going to light up in increments, all the pixels of the tx_channels, chained in gpio_num order. When our
// 10A power limit threshold is exceeded, we then truncate all the remaining pixels from this chain by revising their
// registered tx_chan->pixel_count values down to zero.
void limit_tx_channel_power() {
    uint8_t gpio_num = GPIO_TX_PINS_BEGIN;
    uint16_t chain_offset = 0u;
    uint16_t chain_pixels_lit = PIXEL_INCREMENT;

    uint8_t last_good_gpio_num = gpio_num;
    uint16_t last_good_chain_offset = 0u;
    uint16_t last_good_chain_pixels_lit = 0u;

    set_all_gpio_channel_pixels_off();

    // start lighting up PIXEL_INCREMENT pixels at a time using the low precision sampling regime
    while (gpio_num < GPIO_TX_PINS_END) {
        uint16_t channel_pixels_lit = chain_pixels_lit - chain_offset;

        if (channel_pixels_lit <= gpio_tx_channels[gpio_num].pixel_count) {
            uint32_t channel_bytes_lit = pixel_count_to_byte_count(
                    gpio_tx_channels[gpio_num].pixel_type, channel_pixels_lit);
            set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, channel_bytes_lit);

            uint32_t test_sample_11_bit = get_precision_power_sample(11u);
            if (POWER_LIMIT_10A_THRESHOLD_11_BIT < test_sample_11_bit) {

                break;
            }
            last_good_gpio_num = gpio_num;
            last_good_chain_offset = chain_offset;
            last_good_chain_pixels_lit = chain_pixels_lit;
            chain_pixels_lit += PIXEL_INCREMENT;

        } else {
            if (gpio_tx_channels[gpio_num].pixel_count) {
                uint32_t channel_bytes_lit = pixel_count_to_byte_count(
                        gpio_tx_channels[gpio_num].pixel_type, gpio_tx_channels[gpio_num].pixel_count);
                set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, channel_bytes_lit);
                chain_offset += gpio_tx_channels[gpio_num].pixel_count;
            }
            if (gpio_num == (GPIO_TX_PINS_END - 1u)) { // just lit up the last channel, sample here
                uint32_t test_sample_11_bit = get_precision_power_sample(11u);
                if (POWER_LIMIT_10A_THRESHOLD_11_BIT < test_sample_11_bit) {

                    break;
                }
            }
            gpio_num++;
        }
    }

    // We've exceeded the limit, reset to the last_good position, and continue testing by lighting up 1 pixel
    // at a time and using the high precision sampling regime
    if (gpio_num < GPIO_TX_PINS_END) {
        // turn off pixels from channels that were beyond last_good_gpio_num
        for (uint8_t gpio_idx = last_good_gpio_num + 1; gpio_idx <= gpio_num; gpio_idx++) {
            set_gpio_channel_pixels_off(gpio_idx);
        }
        // reset counters to the last_good position + 1 pixel
        gpio_num = last_good_gpio_num;
        chain_offset = last_good_chain_offset;
        chain_pixels_lit = last_good_chain_pixels_lit + 1;

        while (gpio_num < GPIO_TX_PINS_END) {
            uint16_t channel_pixels_lit = chain_pixels_lit - chain_offset;

            if (channel_pixels_lit <= gpio_tx_channels[gpio_num].pixel_count) {
                uint32_t channel_bytes_lit =
                        pixel_count_to_byte_count(gpio_tx_channels[gpio_num].pixel_type, channel_pixels_lit);
                set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, channel_bytes_lit);

                uint32_t test_sample_13_bit = get_precision_power_sample(13u);
                if (POWER_LIMIT_10A_THRESHOLD_13_BIT < test_sample_13_bit) {
                    log_tick_with_value("WARNING: 10A power limit exceeded 7447 < %4d! remaining channel pixels will be disabled", test_sample_13_bit);

                    break;
                }
                last_good_gpio_num = gpio_num;
                last_good_chain_offset = chain_offset;
                last_good_chain_pixels_lit = chain_pixels_lit;
                chain_pixels_lit++;

            } else {
                if (gpio_tx_channels[gpio_num].pixel_count) {
                    uint32_t channel_bytes_lit =
                            pixel_count_to_byte_count(gpio_tx_channels[gpio_num].pixel_type,
                                                      gpio_tx_channels[gpio_num].pixel_count);
                    set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, channel_bytes_lit);
                    chain_offset += gpio_tx_channels[gpio_num].pixel_count;
                }
                gpio_num++;
            }
        }
    }

    uint32_t end_sample_13_bit = get_precision_power_sample(13u);
    log_tick_with_value("Max power = %5dmA", (end_sample_13_bit * 10000) / POWER_LIMIT_10A_THRESHOLD_13_BIT);


    set_all_gpio_channel_pixels_off();

    // We've exceeded the limit and found the last_good pixel below the limit, truncate the pixel_count value at
    // that last_good pixel and zero out the remaining channels' pixel_count values.
    if (gpio_num < GPIO_TX_PINS_END) {
        gpio_tx_channels[last_good_gpio_num].pixel_count = last_good_chain_pixels_lit - last_good_chain_offset;
        log_tick_with_value("limiting pixel_count for channel %2d", last_good_gpio_num);
        log_tick_with_value("new pixel_count = %d", gpio_tx_channels[last_good_gpio_num].pixel_count);

        for (uint8_t gpio_idx = last_good_gpio_num + 1; gpio_idx < GPIO_TX_PINS_END; gpio_idx++) {
            if (gpio_tx_channels[gpio_idx].pixel_count) {
                gpio_tx_channels[gpio_idx].pixel_count = 0u;
                log_tick_with_value("limiting pixel_count for channel %2d, new pixel_count = 0", gpio_idx);
            }
        }
    }
}
