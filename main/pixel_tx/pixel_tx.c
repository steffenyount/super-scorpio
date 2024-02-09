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
#include "tx_bytes_feed.h"
#include "tx_bytes_to_pwm_cc.pio.h"
#include "pwm_cc_feed.h"
#include "pwm_cc_feed_director.h"
#include "pwm_cc_feed_trigger.h"
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

// 16 GPIO_TX_PINS x 4 bytes (curr & enable) = 64
#define TX_PIXELS_ALIGN_SIZE_BITS (6u)
#define PIXELS_PER_FRAME (512u)

volatile bool __scratch_y("super_scorpio") tx_bytes_pending = false;

static tx_pixel_t __scratch_y("super_scorpio") __alignment(TX_PIXELS_ALIGN_SIZE_BITS) tx_pixels[NUM_GPIO_TX_PINS];
tx_pixel_t __scratch_y("super_scorpio") * gpio_tx_pixels = ((tx_pixel_t *) &tx_pixels[0]) - GPIO_TX_PINS_BEGIN;

uint32_t __scratch_y("super_scorpio") tx_pixels_enabled = 0u;


//static uint32_t __scratch_y("super_scorpio") __alignment(5) tx_bytesdump[NUM_PWM_SLICES];


static inline bool advance_tx_pixels() {
    log_tick("advance_tx_pixels()");
    static uint32_t tx_feeds_status = 0u;

    uint32_t new_tx_feeds_status = tx_feeds_status;

    if (GPIO_TX_PINS_MASK != (new_tx_feeds_status & GPIO_TX_PINS_MASK)) { // if not all done
        for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
            const uint32_t gpio_num_mask = 1u << gpio_num;
            if (!(new_tx_feeds_status & gpio_num_mask)) { // if feed not marked as done already
                if (!gpio_tx_feeds[gpio_num]->advance_pixel(gpio_tx_feeds[gpio_num], gpio_num)) { // if feed done now
                    // set done bit
                    new_tx_feeds_status |= gpio_num_mask;

                    // zero out/disable the tx_pixel values
                    gpio_tx_pixels[gpio_num] = (tx_pixel_t) {
                            .blue = 0u,
                            .red = 0u,
                            .green = 0u,
                    };
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

static uint __scratch_y("super_scorpio") tx_byte_index = 0;

static inline bool advance_tx_bytes() {
    log_tick("advance_tx_bytes()");

    if (tx_byte_index) {
        tx_byte_index--;

        log_tick("advance_tx_bytes() - done (true)");
        return true;

    } else {
        if (advance_tx_pixels()) {
            tx_byte_index = 2;

            log_tick("advance_tx_bytes() - done (true)");
            return true;

        } else {
            log_tick("advance_tx_bytes() - done (false)");
            return false;
        }
    }
}

static __force_inline uint8_t get_curr_byte(uint tx_byte_idx, uint gpio_num) {
    switch (tx_byte_idx) {
        case 2:
            return gpio_tx_pixels[gpio_num].green;
        case 1:
            return gpio_tx_pixels[gpio_num].red;
        case 0:
            return gpio_tx_pixels[gpio_num].blue;
        default:
//            printf("Unrecognized tx_byte_index = %d in get_curr_byte(%d)\n", tx_byte_index, gpio_num);
            return TX_BYTE_OFF;
    }
}

static __force_inline tx_byte_t get_tx_byte(uint32_t tx_pixels_en, uint tx_byte_idx, uint gpio_num) {
    if (get_tx_pixel_enabled(tx_pixels_en, gpio_num)) {
        return (tx_byte_t) {
                .tx_enabled = TX_BYTE_ON,
                .curr_byte = get_curr_byte(tx_byte_idx, gpio_num),
        };
    } else {
        return (tx_byte_t) {
                .tx_enabled = TX_BYTE_OFF,
                .curr_byte = TX_BYTE_OFF,
        };
    };
}

static __force_inline pwm_slice_tx_bytes_t get_pwm_slice_tx_bytes(uint32_t tx_pixels_en, uint tx_byte_idx, uint gpio_num_channel_a, uint gpio_num_channel_b) {
    return (pwm_slice_tx_bytes_t) {
            .channel_a = get_tx_byte(tx_pixels_en, tx_byte_idx, gpio_num_channel_a),
            .channel_b = get_tx_byte(tx_pixels_en, tx_byte_idx, gpio_num_channel_b),
    };
}

static __force_inline void feed_pwm_slice_tx_bytes(volatile void * write_addr,
        uint32_t tx_pixels_en, uint tx_byte_idx, uint gpio_num_channel_a, uint gpio_num_channel_b) {

    *((pwm_slice_tx_bytes_t *) write_addr) =
            get_pwm_slice_tx_bytes(tx_pixels_en, tx_byte_idx, gpio_num_channel_a, gpio_num_channel_b);
}

static void __not_in_flash_func(stage_tx_bytes)() {
    log_tick("stage_tx_bytes()");

    const uint32_t tx_pixels_en = tx_pixels_enabled;
    const uint tx_byte_idx = tx_byte_index;

//    feed_pwm_slice_tx_bytes(&pio1->txf[0], tx_pixels_en, tx_byte_idx, 8u, 9u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[1], tx_pixels_en, tx_byte_idx, 10u, 11u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[2], tx_pixels_en, tx_byte_idx, 12u, 13u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[3], tx_pixels_en, tx_byte_idx, 14u, 15u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[0], tx_pixels_en, tx_byte_idx, 16u, 17u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[1], tx_pixels_en, tx_byte_idx, 18u, 19u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[2], tx_pixels_en, tx_byte_idx, 20u, 21u);
//    feed_pwm_slice_tx_bytes(&pio1->txf[3], tx_pixels_en, tx_byte_idx, 22u, 23u);

//    for (int gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
//        gpio_tx_bytes[gpio_num] = get_tx_byte(tx_pixels_en, tx_byte_idx, gpio_num);
//    }
    gpio_tx_bytes[8u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 8u);
    gpio_tx_bytes[9u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 9u);
    gpio_tx_bytes[10u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 10u);
    gpio_tx_bytes[11u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 11u);
    gpio_tx_bytes[12u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 12u);
    gpio_tx_bytes[13u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 13u);
    gpio_tx_bytes[14u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 14u);
    gpio_tx_bytes[15u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 15u);
    gpio_tx_bytes[16u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 16u);
    gpio_tx_bytes[17u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 17u);
    gpio_tx_bytes[18u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 18u);
    gpio_tx_bytes[19u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 19u);
    gpio_tx_bytes[20u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 20u);
    gpio_tx_bytes[21u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 21u);
    gpio_tx_bytes[22u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 22u);
    gpio_tx_bytes[23u] = get_tx_byte(tx_pixels_en, tx_byte_idx, 23u);

    //gpio_tx_bytes
    log_tick("stage_tx_bytes - done");
}




static inline void init_dma_channels_for_tx_byte_send() {
    init_dma_tx_bytes_feed();
    init_dma_pwm_cc_feed();
    init_dma_pwm_cc_feed_director();
    init_dma_pwm_cc_feed_trigger();
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

void __not_in_flash_func(send_frame)() {
    // lock the feeds until done with the frame
    spin_lock_unsafe_blocking(tx_feeds_lock);

    // open frame
    for (uint gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        gpio_tx_feeds[gpio_num]->open_frame(gpio_tx_feeds[gpio_num], gpio_num);
    }

    start_tick_logs();

    // fetch the initial tx_bytes, shutdown if none available
    if (!(tx_bytes_pending = advance_tx_bytes())) { // frame already done, no tx_bytes to feed, close frame
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

    // stage the initial tx_bytes & trigger the initial tx_bytes_feed
    stage_tx_bytes();
    trigger_tx_bytes_feed();

    // fetch the next tx_bytes
    while ((tx_bytes_pending = advance_tx_bytes())) {
        wait_for_tx_bytes_feed_done();
        stage_tx_bytes();
        wait_for_pwm_cc_feeds_done();
        trigger_tx_bytes_feed();
    }

    // frame data is done!

    // wait for PWMs shutdown
//    while(pwm_hw->en) {
//        tight_loop_contents();
//    }
}

void init_core0_pixel_tx() {
//    // init on_pwm_slice_irq irq handler
//    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_slice_irq);
//    irq_set_enabled(PWM_IRQ_WRAP, true);
//    irq_set_priority(PWM_IRQ_WRAP, 0x40);
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

    for (int ii = 0; ii < 4 /*32*/; ii++) {
        printf("##%d##\n", ii);

        on_off_feed_value = true;
        send_frame();

        printf("\nOn\n");

        print_tick_logs();

        sleep_ms(1000);

        printf("frame: %2d\n", curr_frame - 1);
        printf("src_pixel: ");
        printbits_32(src_pixel);
        printf("\n");

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
