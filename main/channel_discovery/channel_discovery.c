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

#include "channel_discovery.h"
#include "power_monitor/power_samples.h"
#include "pixel_channels/tx_channels.h"
#include "channel_control/channel_control.h"

#include <stdio.h>
#include "hardware/structs/systick.h"

#define printbits_n(x,n) for (uint j=x, i=n;i;i--,putchar('0'|((j>>i)&1)))
#define printbits_16(x) printbits_n(x,16)
#define printbits_32(x) printbits_n(x,32)


#define NUM_SAMPLES_FOR_DUMP (256u)


static uint32_t off_threshold_12_bit = 0u;
static uint32_t off_threshold_14_bit = 0u;

static inline void set_off_ref_thresholds() {
    uint32_t off_ref_sample_16_bit = get_precision_power_sample(16u);
    // adding half the new LSB for rounding, then shift the new LSB to position 0
    uint32_t off_ref_sample_14_bit = (off_ref_sample_16_bit + 2u) >> 2;
    uint32_t off_ref_sample_12_bit = (off_ref_sample_16_bit + 8u) >> 4;

    printf("16_bit %4d ", off_ref_sample_16_bit);
    printbits_16(off_ref_sample_16_bit);
    printf(" 14_bit %4d ", off_ref_sample_14_bit);
    printbits_16(off_ref_sample_14_bit);
    printf(" 12_bit %4d ", off_ref_sample_12_bit);
    printbits_16(off_ref_sample_12_bit);
    printf("\n");

    off_threshold_14_bit = off_ref_sample_14_bit + 3u;
    off_threshold_12_bit = off_ref_sample_12_bit + 2u;
}

static inline void detect_gpio_channel_pixels_for_gpio(uint8_t gpio_num) {
    printf("detect_gpio_channel_pixels_for_gpio(%d)\n", gpio_num);
    tx_channel_t * const tx_chan = &gpio_tx_channels[gpio_num];

    // If there's no pixel at position 0, we assume there's no pixels on this whole channel and move on
    set_gpio_channel_pixels_on_for_byte_range(gpio_num, 0u, 3u);
    if (get_precision_power_sample(12u) <= off_threshold_12_bit) {
        // it's OFF
        tx_chan->pixel_type = GRB_3_BYTE;
        tx_chan->pixel_count = 0u;
        printf("gpio channel (%d) pixels detected: type = %s count = %d\n\n", gpio_num,
               (GRB_3_BYTE == tx_chan->pixel_type) ? "GRB_3_BYTE" : "RGBW_4_BYTE", tx_chan->pixel_count);

        // We're done testing, turn off the lights
        set_gpio_channel_pixels_off(gpio_num);

        printf("detect_gpio_channel_pixels_for_gpio(%d) - done ZERO\n", gpio_num);
        return;
    }

    // bisect to find last pixel byte on channel, assuming all pixels/sub-pixels will light
    uint16_t last_possible_on = 3u; // pixel at 0 was ON
    uint16_t first_known_off = MAX_BYTES_PER_PIXEL_CHANNEL; /* (512 * 3) */
    uint16_t test_target = MAX_BYTES_PER_PIXEL_CHANNEL >> 1;
    uint16_t target_increment = MAX_BYTES_PER_PIXEL_CHANNEL >> 2;

    // start with 3-bytes ON using low precision LED detection regime
    printf("Starting in 3-Byte regime\n");
    do {
        printf("target_increment = %4d, last_possible_on = %4d, first_known_off = %4d, test_target = %4d\n",
               target_increment, last_possible_on, first_known_off, test_target);

        set_gpio_channel_pixels_on_for_byte_range(gpio_num, test_target, test_target + 3u);

        // sensitive enough to notice one ON pixel of a ~5mA per pixel unit like the WS2812C-2020
        uint32_t test_sample_12_bit = get_precision_power_sample(12u);
        if (test_sample_12_bit <= off_threshold_12_bit) {
            // it's OFF
            printf("OFF with %4d <= %4d\n", test_sample_12_bit, off_threshold_12_bit);
            first_known_off = test_target;
            test_target -= target_increment;

        } else {
            // it's ON
            printf("ON  with %4d  > %4d\n", test_sample_12_bit, off_threshold_12_bit);
            last_possible_on = test_target + 3u;
            test_target += target_increment;
        }
        target_increment >>= 1u;
    } while (target_increment >= 6u);


    // continue with 1-byte ON using high precision LED detection regime
    printf("Entering 1-Byte regime\n");
    uint16_t twelve_byte_base_idx = first_known_off; // here first_known_off will be at a (0 mod 12) byte boundary
    last_possible_on -= 2; // was set after a 3-byte block was determined to be "ON", don't assume all 3 bytes were lit

    do {
        test_target = (last_possible_on + first_known_off) >> 1;

        printf("target_increment = %4d, last_possible_on = %4d, first_known_off = %4d, test_target = %4d\n",
               ((first_known_off - last_possible_on) >> 1), last_possible_on, first_known_off, test_target);

        set_gpio_channel_pixels_on_for_byte_range(gpio_num, test_target, test_target + 1u);

        // sensitive enough to notice one ON sub-pixel (1/3) of a ~5mA per pixel unit like the WS2812C-2020
        uint32_t test_sample_14_bit = get_precision_power_sample(14u);
        if (test_sample_14_bit <= off_threshold_14_bit) {
            // it's OFF
            printf("OFF with %4d <= %4d\n", test_sample_14_bit, off_threshold_14_bit);
            first_known_off = test_target;

        } else {
            // it's ON
            printf("ON  with %4d  > %4d\n", test_sample_14_bit, off_threshold_14_bit);
            last_possible_on = test_target + 1u;
        }
    } while (last_possible_on < first_known_off);

    // We're done testing, turn off the lights
    set_gpio_channel_pixels_off(gpio_num);

    printf("channel bytes detected = %4d mod 3 = %4d mod 4 = %4d\n", first_known_off, first_known_off % 3, first_known_off % 4);

    uint16_t pixel_byte_count_mod_12 = twelve_byte_base_idx - first_known_off;
    switch (pixel_byte_count_mod_12) {
        case 0: // at 0, pixels could be 3 or 4 bytes wide, we'll assume they're 3 bytes wide
        case 3:
        case 6:
        case 9:
            tx_chan->pixel_type = GRB_3_BYTE;
            tx_chan->pixel_count = fast_16_divide_by_3(first_known_off);
            break;
        case 4:
        case 8:
            tx_chan->pixel_type = RGBW_4_BYTE;
            tx_chan->pixel_count = first_known_off >> 2;
            break;
        default: // at 1, 2, 5, 7, 10, 11 we've got values that don't divide evenly by 3 or 4. Assume 3 bytes wide.
            tx_chan->pixel_type = GRB_3_BYTE;
            tx_chan->pixel_count = fast_16_divide_by_3(first_known_off + 2u);
            printf("WARNING: detect_gpio_channel_pixels_for_gpio(%d) unexpected byte count %d = %d mod 12! Assuming %d 3-byte pixels.\n",
                   gpio_num, first_known_off, pixel_byte_count_mod_12, tx_chan->pixel_count);
            break;
    }

    printf("gpio channel (%d) pixels detected: type = %s count = %d\n\n", gpio_num,
           (GRB_3_BYTE == tx_chan->pixel_type) ? "GRB_3_BYTE" : "RGBW_4_BYTE", tx_chan->pixel_count);

    printf("detect_gpio_channel_pixels_for_gpio(%d) - done\n", gpio_num);
}

static inline void detect_all_gpio_channel_pixels() {
    for (uint8_t gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        detect_gpio_channel_pixels_for_gpio(gpio_num);
    }
//    detect_gpio_channel_pixels_for_gpio(16u);
}

void reset_collected_samples() {
    median_samples_count = 0u;
    prev_precision_power_sample = precision_power_sample;
    precision_power_sample = 0u;
}

void dump_collected_samples() {
    int32_t threshold_sqd = 2;
    threshold_sqd *= threshold_sqd;
    int32_t pps_diff_sqd = ((int32_t)precision_power_sample - (int32_t)prev_precision_power_sample);
    pps_diff_sqd *= pps_diff_sqd;
    if (pps_diff_sqd < threshold_sqd) {
        uint32_t median_min = 0u - 1u;
        uint32_t median_max = 0u;
        for (uint32_t mm = 0u; mm < median_samples_count; mm++) {
            if (median_samples[mm] < median_min) {
                median_min = median_samples[mm];
            }
            if (median_samples[mm] > median_max) {
                median_max = median_samples[mm];
            }
            if (precision_power_sample > 0u) {
                for (uint32_t rr = 0u; rr < POWER_SAMPLES_SIZE - 1u; rr += 16u) {
                    if (rr < (POWER_SAMPLES_SIZE - 16u)) {
                        printf("raw_sample_count[%4d][%4d]: %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d\n",
                               mm, rr,
                               raw_samples[mm][rr], raw_samples[mm][rr + 1], raw_samples[mm][rr + 2],
                               raw_samples[mm][rr + 3],
                               raw_samples[mm][rr + 4], raw_samples[mm][rr + 5], raw_samples[mm][rr + 6],
                               raw_samples[mm][rr + 7],
                               raw_samples[mm][rr + 8], raw_samples[mm][rr + 9], raw_samples[mm][rr + 10],
                               raw_samples[mm][rr + 11],
                               raw_samples[mm][rr + 12], raw_samples[mm][rr + 13], raw_samples[mm][rr + 14],
                               raw_samples[mm][rr + 15]);
                    } else {
                        printf("raw_sample_count[%4d][%4d]: %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d %4d      =>  %4d\n",
                               mm, rr,
                               raw_samples[mm][rr], raw_samples[mm][rr + 1], raw_samples[mm][rr + 2],
                               raw_samples[mm][rr + 3],
                               raw_samples[mm][rr + 4], raw_samples[mm][rr + 5], raw_samples[mm][rr + 6],
                               raw_samples[mm][rr + 7],
                               raw_samples[mm][rr + 8], raw_samples[mm][rr + 9], raw_samples[mm][rr + 10],
                               raw_samples[mm][rr + 11],
                               raw_samples[mm][rr + 12], raw_samples[mm][rr + 13], raw_samples[mm][rr + 14],
                               median_samples[mm]);
                    }
                }
            }
        }
        uint32_t median_avg = (sample_aggregate + (median_samples_count >> 1)) / median_samples_count;
        printf("min/max avg range %4d/%4d %4d %4d saps %4d vs prev = %4d                         precision_power_sample => %5d\n",
               median_min, median_max, median_avg, (median_max - median_min), median_samples_count,
               (precision_power_sample - prev_precision_power_sample), precision_power_sample);
//    printbits_32(sample_aggregate);
//    printf("\n                ");
//    printbits_16(precision_power_sample);
//    printf("\n\n");
    }
}

// 12 for 3 bytes, 14 for 1 byte where (x-0)^2 < 4.
#define PPS_BITS (16u)
void discover_tx_channel_pixels() {
    set_all_gpio_channel_pixels_off();
    for (uint32_t ii = 0; ii < 50; ii++) {
        reset_collected_samples();
        uint32_t off_ref_sample_x_bit = get_precision_power_sample(14u);
//        dump_collected_samples();

        printf("14_bit %4d ", off_ref_sample_x_bit);
        printbits_16(off_ref_sample_x_bit);
        printf("\n");
//        sleep_ms(1000u);
    }
    reset_collected_samples();
    set_off_ref_thresholds();
    dump_collected_samples();

    detect_all_gpio_channel_pixels();

////    printf("get_median_of_power_samples() = %d\n", get_median_of_power_samples());
//    reset_collected_samples();
//    get_precision_power_sample(PPS_BITS);
//    dump_collected_samples();
//for (uint32_t ii = 0; ii < 10u; ii++) {
//
////    for (uint32_t byte_count = 0; byte_count < 60; byte_count++) {
//////        printf("sleep(1)\n");
//////        sleep_ms(1000u);
////        printf("1 pix_count = %3d precision = %3d\n", byte_count, PPS_BITS);
////        set_gpio_channel_pixels_on_for_byte_range(16u, 0u, byte_count);
////        //printf("gpio_channel_pixels set\n");
////        reset_collected_samples();
////        get_precision_power_sample(PPS_BITS);
////        dump_collected_samples();
////    }
////    for (uint32_t jj = 0; jj < 4; jj++) {
////        set_gpio_channel_pixels_off(16u);
////        //printf("gpio_channel_pixels set\n");
////        reset_collected_samples();
////        get_precision_power_sample(PPS_BITS);
////        dump_collected_samples();
////    }
//
//    for (uint32_t pix_count = 0; pix_count < 20; pix_count++) {
////        printf("sleep(1)\n");
////        sleep_ms(1000u);
//        printf("3 pix_count = %3d precision = %3d\n", pix_count*3u, PPS_BITS);
//        set_gpio_channel_pixels_on_for_byte_range(16u, 0u, pix_count * 3u);
//        //printf("gpio_channel_pixels set\n");
//        reset_collected_samples();
//        get_precision_power_sample(PPS_BITS);
//        dump_collected_samples();
//    }
////    for (uint32_t pix_count = 0; pix_count < 16; pix_count++) {
////        printf("sleep(1)\n");
//////        sleep_ms(1000u);
////        printf("4 pix_count = %3d\n", pix_count);
////        set_gpio_channel_pixels_on_for_byte_range(16u, 0u, pix_count * 4u);
////        //printf("gpio_channel_pixels set\n");
////        reset_collected_samples();
////        get_precision_power_sample(PPS_BITS);
////        dump_collected_samples();
////    }
//    for (uint32_t jj = 0; jj < 4; jj++) {
//        set_gpio_channel_pixels_off(16u);
//        //printf("gpio_channel_pixels set\n");
//        reset_collected_samples();
//        get_precision_power_sample(PPS_BITS);
//        dump_collected_samples();
//    }
//    printf("\n");
//}
//    set_all_gpio_channel_pixels_off();
//    reset_collected_samples();
//    get_precision_power_sample(PPS_BITS);
//    dump_collected_samples();
}

