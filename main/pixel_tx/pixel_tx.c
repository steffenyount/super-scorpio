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

#include "pixel_tx.h"
#include "common.h"
#include "tick_log/tick_log.h"
#include "tx_pixels_buffer.h"
#include "tx_bytes_buffer.h"
#include "tx_bytes_feed.h"
#include "tx_bytes_feed_director.h"
#include "tx_bytes_to_pwm_cc.pio.h"
#include "pwm_cc_feed.h"
#include "pwm_cc_feed_director.h"
#include "pwm_cc_feed_trigger.h"
#include "pwm_cc_feed_discard.h"
#include "pixel_feeds/pixel_feeds.h"
#include "pixel_feeds/on_off_feed.h"
#include "gpio_tx_pins.h"
#include "pwm_tx_slices.h"
#include "irq_handling.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"

#define printbits_n(x,n) for (uint j=x, i=n;i;i--,putchar('0'|((j>>i)&1)))
#define printbits_32(x) printbits_n(x,32)
#define printbits_16(x) printbits_n(x,16)


uint32_t __scratch_y("super_scorpio") tx_pixels_enabled = 0u;

static inline bool advance_tx_pixels() {
    log_tick("advance_tx_pixels()");
    static uint32_t __scratch_y("super_scorpio") tx_feeds_status = 0u;

    uint32_t new_tx_feeds_status = tx_feeds_status;

    if (GPIO_TX_PINS_MASK != (new_tx_feeds_status & GPIO_TX_PINS_MASK)) { // if not all done
        for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
            const uint32_t gpio_num_mask = 1u << gpio_num;
            if (!(new_tx_feeds_status & gpio_num_mask)) { // if feed not marked as done already
                if (!gpio_tx_feeds[gpio_num]->advance_pixel(gpio_tx_feeds[gpio_num], gpio_num)) { // if feed done now
                    // set done bit
                    new_tx_feeds_status |= gpio_num_mask;

                    // zero out/disable the tx_pixel values
                    next_gpio_tx_pixels[gpio_num] = tx_pixel_off;
                    set_tx_pixel_disabled(gpio_num);

                    // close frame
                    gpio_tx_feeds[gpio_num]->close_frame(gpio_tx_feeds[gpio_num], gpio_num);
                }
            }
        }
    }

    if (GPIO_TX_PINS_MASK != (new_tx_feeds_status & GPIO_TX_PINS_MASK)) {
        tx_feeds_status = new_tx_feeds_status;
        curr_pixel++;

        log_tick("advance_tx_pixels() - done (true)");
        return true;

    } else { // if all done
        // clear the feeds status
        tx_feeds_status = 0u;
        curr_pixel = 0u;

        log_tick("advance_tx_pixels() - done (false)");
        return false;
    }
}


static inline uint8_t get_curr_byte_green(uint gpio_num) {
    return curr_gpio_tx_pixels[gpio_num].green;
}

static inline uint8_t get_curr_byte_red(uint gpio_num) {
    return curr_gpio_tx_pixels[gpio_num].red;
}

static inline uint8_t get_curr_byte_blue(uint gpio_num) {
    return curr_gpio_tx_pixels[gpio_num].blue;
}

static inline tx_byte_t tx_byte_on(uint8_t curr_byte) {
    const tx_byte_t tx_byte = {
            .tx_enabled = TX_ENABLED_ON,
            .curr_byte = curr_byte,
    };

    return tx_byte;
}

static inline tx_byte_t get_tx_byte(bool tx_pixel_enabled, uint8_t curr_byte) {
    if (tx_pixel_enabled) {
        return tx_byte_on(curr_byte);

    } else {
        return tx_byte_off;
    };
}

//bool tx_pixel_enabled, uint8_t curr_byte
static inline pwm_slice_tx_bytes_t get_pwm_slice_tx_bytes(
        bool chan_a_tx_pixel_enabled, uint8_t chan_a_curr_byte,
        bool chan_b_tx_pixel_enabled, uint8_t chan_b_curr_byte) {

    const pwm_slice_tx_bytes_t pwm_slice_tx_bytes = {
            .channel_a = get_tx_byte(chan_a_tx_pixel_enabled, chan_a_curr_byte),
            .channel_b = get_tx_byte(chan_b_tx_pixel_enabled, chan_b_curr_byte),
    };

    return pwm_slice_tx_bytes;
}

static inline void stage_tx_bytes_green() {
//    log_tick("stage_tx_bytes_green()");

    const uint32_t tx_pixels_en = tx_pixels_enabled;

//    for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
//        const uint gpio_num_channel_a = GPIO_TX_PINS_BEGIN + (slice_num << 1);
//        const uint gpio_num_channel_b = gpio_num_channel_a + 1;
//
//        pwm_slice_tx_bytes[slice_num] =
//                get_pwm_slice_tx_bytes(
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_a),
//                        get_curr_byte_green(gpio_num_channel_a),
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_b),
//                        get_curr_byte_green(gpio_num_channel_b));
//
//    }

    pwm_slice_tx_bytes[0] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 8u),
                    get_curr_byte_green(8u),
                    get_tx_pixel_enabled(tx_pixels_en, 9u),
                    get_curr_byte_green(9u));
    pwm_slice_tx_bytes[1] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 10u),
                    get_curr_byte_green(10u),
                    get_tx_pixel_enabled(tx_pixels_en, 11u),
                    get_curr_byte_green(11u));
    pwm_slice_tx_bytes[2] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 12u),
                    get_curr_byte_green(12u),
                    get_tx_pixel_enabled(tx_pixels_en, 13u),
                    get_curr_byte_green(13u));
    pwm_slice_tx_bytes[3] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 14u),
                    get_curr_byte_green(14u),
                    get_tx_pixel_enabled(tx_pixels_en, 15u),
                    get_curr_byte_green(15u));
    pwm_slice_tx_bytes[4] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 16u),
                    get_curr_byte_green(16u),
                    get_tx_pixel_enabled(tx_pixels_en, 17u),
                    get_curr_byte_green(17u));
    pwm_slice_tx_bytes[5] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 18u),
                    get_curr_byte_green(18u),
                    get_tx_pixel_enabled(tx_pixels_en, 19u),
                    get_curr_byte_green(19u));
    pwm_slice_tx_bytes[6] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 20u),
                    get_curr_byte_green(20u),
                    get_tx_pixel_enabled(tx_pixels_en, 21u),
                    get_curr_byte_green(21u));
    pwm_slice_tx_bytes[7] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 22u),
                    get_curr_byte_green(22u),
                    get_tx_pixel_enabled(tx_pixels_en, 23u),
                    get_curr_byte_green(23u));

//    log_tick("stage_tx_bytes_green() - done");
}

static inline void stage_tx_bytes_red() {
//    log_tick("stage_tx_bytes_red()");

    const uint32_t tx_pixels_en = tx_pixels_enabled;

//    for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
//        const uint gpio_num_channel_a = GPIO_TX_PINS_BEGIN + (slice_num << 1);
//        const uint gpio_num_channel_b = gpio_num_channel_a + 1;
//
//        pwm_slice_tx_bytes[slice_num] =
//                get_pwm_slice_tx_bytes(
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_a),
//                        get_curr_byte_red(gpio_num_channel_a),
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_b),
//                        get_curr_byte_red(gpio_num_channel_b));
//    }

    pwm_slice_tx_bytes[0] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 8u),
                    get_curr_byte_red(8u),
                    get_tx_pixel_enabled(tx_pixels_en, 9u),
                    get_curr_byte_red(9u));
    pwm_slice_tx_bytes[1] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 10u),
                    get_curr_byte_red(10u),
                    get_tx_pixel_enabled(tx_pixels_en, 11u),
                    get_curr_byte_red(11u));
    pwm_slice_tx_bytes[2] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 12u),
                    get_curr_byte_red(12u),
                    get_tx_pixel_enabled(tx_pixels_en, 13u),
                    get_curr_byte_red(13u));
    pwm_slice_tx_bytes[3] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 14u),
                    get_curr_byte_red(14u),
                    get_tx_pixel_enabled(tx_pixels_en, 15u),
                    get_curr_byte_red(15u));
    pwm_slice_tx_bytes[4] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 16u),
                    get_curr_byte_red(16u),
                    get_tx_pixel_enabled(tx_pixels_en, 17u),
                    get_curr_byte_red(17u));
    pwm_slice_tx_bytes[5] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 18u),
                    get_curr_byte_red(18u),
                    get_tx_pixel_enabled(tx_pixels_en, 19u),
                    get_curr_byte_red(19u));
    pwm_slice_tx_bytes[6] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 20u),
                    get_curr_byte_red(20u),
                    get_tx_pixel_enabled(tx_pixels_en, 21u),
                    get_curr_byte_red(21u));
    pwm_slice_tx_bytes[7] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 22u),
                    get_curr_byte_red(22u),
                    get_tx_pixel_enabled(tx_pixels_en, 23u),
                    get_curr_byte_red(23u));

//    log_tick("stage_tx_bytes_red() - done");
}

static inline void stage_tx_bytes_blue() {
//    log_tick("stage_tx_bytes_blue()");

    const uint32_t tx_pixels_en = tx_pixels_enabled;

//    for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
//        const uint gpio_num_channel_a = GPIO_TX_PINS_BEGIN + (slice_num << 1);
//        const uint gpio_num_channel_b = gpio_num_channel_a + 1;
//
//        pwm_slice_tx_bytes[slice_num] =
//                get_pwm_slice_tx_bytes(
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_a),
//                        get_curr_byte_blue(gpio_num_channel_a),
//                        get_tx_pixel_enabled(tx_pixels_en, gpio_num_channel_b),
//                        get_curr_byte_blue(gpio_num_channel_b));
//    }

    pwm_slice_tx_bytes[0] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 8u),
                    get_curr_byte_blue(8u),
                    get_tx_pixel_enabled(tx_pixels_en, 9u),
                    get_curr_byte_blue(9u));
    pwm_slice_tx_bytes[1] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 10u),
                    get_curr_byte_blue(10u),
                    get_tx_pixel_enabled(tx_pixels_en, 11u),
                    get_curr_byte_blue(11u));
    pwm_slice_tx_bytes[2] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 12u),
                    get_curr_byte_blue(12u),
                    get_tx_pixel_enabled(tx_pixels_en, 13u),
                    get_curr_byte_blue(13u));
    pwm_slice_tx_bytes[3] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 14u),
                    get_curr_byte_blue(14u),
                    get_tx_pixel_enabled(tx_pixels_en, 15u),
                    get_curr_byte_blue(15u));
    pwm_slice_tx_bytes[4] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 16u),
                    get_curr_byte_blue(16u),
                    get_tx_pixel_enabled(tx_pixels_en, 17u),
                    get_curr_byte_blue(17u));
    pwm_slice_tx_bytes[5] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 18u),
                    get_curr_byte_blue(18u),
                    get_tx_pixel_enabled(tx_pixels_en, 19u),
                    get_curr_byte_blue(19u));
    pwm_slice_tx_bytes[6] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 20u),
                    get_curr_byte_blue(20u),
                    get_tx_pixel_enabled(tx_pixels_en, 21u),
                    get_curr_byte_blue(21u));
    pwm_slice_tx_bytes[7] =
            get_pwm_slice_tx_bytes(
                    get_tx_pixel_enabled(tx_pixels_en, 22u),
                    get_curr_byte_blue(22u),
                    get_tx_pixel_enabled(tx_pixels_en, 23u),
                    get_curr_byte_blue(23u));

//    log_tick("stage_tx_bytes_blue() - done");
}

static inline void stage_tx_bytes_off() {
//    log_tick("stage_tx_bytes_off()");

//    for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
//        pwm_slice_tx_bytes[slice_num] = pwm_slice_tx_bytes_off;
//    }

    pwm_slice_tx_bytes[0] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[1] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[2] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[3] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[4] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[5] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[6] = pwm_slice_tx_bytes_off;
    pwm_slice_tx_bytes[7] = pwm_slice_tx_bytes_off;

//    log_tick("stage_tx_bytes_off() - done");
}

static inline bool advance_tx_bytes() {
    log_tick("advance_tx_bytes()");

    static uint __scratch_y("super_scorpio") tx_byte_index = 0;

    if (tx_byte_index) {
        tx_byte_index--;

        if (tx_byte_index == 1u) {
            wait_for_pwm_cc_feeds_done();
            stage_tx_bytes_red();

            log_tick("advance_tx_bytes() - done (true : red)");
            return true;

        } else { // tx_byte_index == 0u
            wait_for_pwm_cc_feeds_done();
            stage_tx_bytes_blue();

            log_tick("advance_tx_bytes() - done (true : blue)");
            return true;
        }
    } else { // tx_byte_index == 0
        if (advance_tx_pixels()) {
            tx_byte_index = 2u;

            // tx_byte_index == 2u
            wait_for_pwm_cc_feeds_done();
            tx_bytes_pending = true;
            stage_tx_bytes_green();

            log_tick("advance_tx_bytes() - done (true : green)");
            return true;

        } else {
            wait_for_pwm_cc_feeds_done();
            tx_bytes_pending = false;
            stage_tx_bytes_off();

            log_tick("advance_tx_bytes() - done (false : off)");
            return false;
        }
    }
}

static inline void init_dma_channels_for_tx_byte_send() {
    init_dma_gpio_tx_bytes_feed();
    init_dma_gpio_tx_bytes_feed_director();
    init_dma_pwm_cc_feed();
    init_dma_pwm_cc_feed_director();
    init_dma_pwm_cc_feed_trigger();
    init_dma_pwm_cc_feed_discard();
}

//static inline void dump_gpio_tx_pixels_and_gpio_tx_bytes() {
//    printf("gpio_tx_pixels:\n");
//    for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
//        printf("%2d: ", gpio_num);
//        printbits_32(((uint32_t *)gpio_tx_pixels)[gpio_num]);
//        printf(" -> ");
//        printbits_16(((uint16_t *)gpio_tx_bytes)[gpio_num]);
//        printf("\n");
//    }
//}
//
//static inline void dump_pwm_slice_tx_bytes() {
//    printf("pwm_slice_tx_bytes:\n");
//    for (uint slice_num = 0; slice_num < NUM_PWM_SLICES; slice_num++) {
//        printf("%2d: ", slice_num);
//        printbits_32(((uint32_t *)pwm_slice_tx_bytes)[slice_num]);
//        printf("\n");
//    }
//}

static inline void discard_pwm_cc_data() {

}

void __not_in_flash_func(send_frame)() {
    // lock the feeds until done with the frame
    spin_lock_unsafe_blocking(tx_feeds_lock);

    // open frame
    for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        gpio_tx_feeds[gpio_num]->open_frame(gpio_tx_feeds[gpio_num], gpio_num);
    }

    start_tick_logs();

    // don't block stage_tx_bytes step in next pass for advance_tx_bytes()
    pwm_cc_feeds_done = true;

    // stage the initial tx_bytes, shutdown if none available
    if (!advance_tx_bytes()) { // frame already done, no tx_bytes to feed, close frame
        for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
            gpio_tx_feeds[gpio_num]->close_frame(gpio_tx_feeds[gpio_num], gpio_num);
        }

        curr_frame++;
        // we're done with the frame unlock feeds
        spin_unlock_unsafe(tx_feeds_lock);

        return;
    }

    // let pwm trigger CC data collection
    pwm_count = 0u;
    pwm_clear_irq(PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE);
    pwm_set_irq_enabled(PWM_SCORPIO_DEFAULT_WS2812_PIN_SLICE, true);

    // send the initial tx_bytes
//    trigger_pwm_slice_tx_bytes_feed();
    wait_for_rxf_pwm_cc_data_ready();

    // reset the DREQ counter for the DMA_PWM_CC_FEED_TRIGGER_CHAN
    dma_debug_hw->ch[DMA_PWM_CC_FEED_TRIGGER_CHAN].ctrdeq = 0u;

    trigger_pwm_cc_feeds();

    // start all the PWMs
    pwm_set_mask_enabled(NUM_PWM_SLICES_MASK);
    log_tick("PWMs started");

    // don't block stage_tx_bytes step in next pass for advance_tx_bytes()
    pwm_cc_feeds_done = true;

    // send the next tx_bytes
    while (advance_tx_bytes()) {
        tight_loop_contents();
    }

    // frame data is done!

    // wait for PWMs shutdown
    while(pwm_hw->en) {
        tight_loop_contents();
    }
}

void init_core0_pixel_tx() {
    init_core0_irq_handling();

//    systick_hw->csr = 0x5u;
//    while (!systick_start) {
//        tight_loop_contents();
//    }
//    systick_hw->cvr = systick_start;
}

void core1_pixel_tx() {
    printf("Starting core1_pixel_tx()\n");


//    // Figure out which slice/channel we just connected to the LED pin
//    uint slice_num = pwm_gpio_to_slice_num(SCORPIO_DEFAULT_WS2812_PIN);
//    uint channel_num = pwm_gpio_to_channel(SCORPIO_DEFAULT_WS2812_PIN);

//    systick_hw->csr = 0u;
//    systick_hw->rvr = 0x00ffffffu;
    systick_hw->csr = 0x5u;

    init_gpio_tx_pins();
    init_pwm_tx_slices();
    init_pio_sms_for_tx_bytes_to_pwm_cc_program();
    init_dma_channels_for_tx_byte_send();
    init_irq_handling();
    init_tx_feeds();

    for (int ii = 0; ii < 2 /*32*/; ii++) {
        printf("##%d##\n", ii);

        on_off_feed_value = true;
        send_frame();

        printf("\nOn\n");

        sleep_ms(1000);

        printf("frame: %2d\n", curr_frame - 1);
        printf("src_pixel: ");
        printbits_32(src_pixel);
        printf("\n");

        uint32_t flevel = pio1->flevel;
        printf("rx_fifo_levels = %d, %d, %d, %d\n",
               ((flevel & PIO_FLEVEL_RX0_BITS) >> PIO_FLEVEL_RX0_LSB),
               ((flevel & PIO_FLEVEL_RX1_BITS) >> PIO_FLEVEL_RX1_LSB),
               ((flevel & PIO_FLEVEL_RX2_BITS) >> PIO_FLEVEL_RX2_LSB),
               ((flevel & PIO_FLEVEL_RX3_BITS) >> PIO_FLEVEL_RX3_LSB));
        print_tick_logs();

        printf("\npwm_count: %d\n 0: ", pwm_count);
        for (int ii = 0; ii < pwm_count; ii++) {
            if (!((ii - 2) & 7u)) {
                printf("%2d: ", ii);
            }
            printbits_16(pwm_slice2_lower_cc_vals[ii]);
            if ((ii-1) & 7u) {
                printf(" ");
            } else {
                printf("\n");
            }
        }
        printf("\n");

        on_off_feed_value = false;
        send_frame();

        printf("\nOff\n");

        sleep_ms(1000);
    }

    printf("\nDone.\n");

    while (true) {
        tight_loop_contents();
    }
}
