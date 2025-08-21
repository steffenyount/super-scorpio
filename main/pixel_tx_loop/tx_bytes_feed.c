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

#include "tx_bytes_feed.h"
#include "pixel_channels/tx_channels.h"
#include "tx_data_feeds.h"

#define GRB_3_BYTE_INDEX_RESET (2u)
#define RGBW_4_BYTE_INDEX_RESET (3u)


typedef struct stackable_tx_byte_feed {
    struct stackable_tx_byte_feed *volatile * (*const activate_when_ready)(struct stackable_tx_byte_feed *volatile *const this_feed_ref);
    struct stackable_tx_byte_feed *volatile * (*const advance_tx_pixel)(struct stackable_tx_byte_feed *volatile *const this_feed_ref);
    struct stackable_tx_byte_feed *volatile * (*const disable_tx_pixel)(struct stackable_tx_byte_feed *volatile *const this_feed_ref);
    struct stackable_tx_byte_feed *volatile * (*const advance_reset_count)(struct stackable_tx_byte_feed *volatile *const this_feed_ref);
    struct stackable_tx_byte_feed *volatile next;
} stackable_tx_byte_feed_t;

// usage: grb_3_tx_byte_feeds[tx_channel_num]
static stackable_tx_byte_feed_t grb_3_tx_byte_feeds[NUM_TX_PINS];

// usage: rgbw_4_tx_byte_feeds[tx_channel_num]
static stackable_tx_byte_feed_t rgbw_4_tx_byte_feeds[NUM_TX_PINS];

typedef struct {
    stackable_tx_byte_feed_t * volatile idle_feeds;
    stackable_tx_byte_feed_t * volatile active_feeds;
    stackable_tx_byte_feed_t * volatile terminal_feeds;
} tx_byte_feed_stacks_t;

static tx_byte_feed_stacks_t grb_3_pixel_channel_feeds = {
        .idle_feeds = NULL,
        .active_feeds = NULL,
        .terminal_feeds = NULL,
};

static tx_byte_feed_stacks_t rgbw_4_pixel_channel_feeds = {
        .idle_feeds = NULL,
        .active_feeds = NULL,
        .terminal_feeds = NULL,
};

stackable_tx_byte_feed_t * volatile resetting_feeds = NULL;

volatile uint32_t pixel_feeds_ready = 0u;

static uint32_t grb_3_pixel_channels = 0u;
static uint32_t rgbw_4_pixel_channels = 0u;

static uint8_t grb_3_channel_byte_index = 0u;
static uint8_t rgbw_4_channel_byte_index = 0u;

static int32_t tx_bytes_fed = 0;
static int32_t tx_bytes_fed_ready_target[NUM_TX_PINS] = {0};
static int32_t tx_bytes_fed_reset_target[NUM_TX_PINS] = {0};

__force_inline static void add_tx_bytes_feed(
        stackable_tx_byte_feed_t *const feed, stackable_tx_byte_feed_t *volatile *const target_stack) {

    feed->next = (*target_stack);
    (*target_stack) = feed;
}

__force_inline static void move_tx_bytes_feed(
        stackable_tx_byte_feed_t *volatile *const feed_ref, stackable_tx_byte_feed_t *volatile *const target_stack) {

    // remove feed from current stack
    stackable_tx_byte_feed_t *const feed = *feed_ref;
    (*feed_ref) = feed->next;

    // add feed to target_stack
    feed->next = (*target_stack);
    (*target_stack) = feed;
}

__force_inline static void begin_frame(const uint8_t tx_channel_num) {
//    core1_log_tick_with_value("begin_frame(%2d)", tx_channels[tx_channel_num].gpio_num);

    // reset the pixels_fed count before calling open_frame()
    tx_channels[tx_channel_num].tx_status.pixels_fed = 0u;
    // initialize the next tx_feed and update the active flag to reflect the status change
    tx_channels[tx_channel_num].root_feed.open_frame(&tx_channels[tx_channel_num].root_feed);
}

__force_inline static stackable_tx_byte_feed_t *volatile * activate_channel_when_ready(
        stackable_tx_byte_feed_t *volatile *const this_feed_ref,
        const uint8_t tx_channel_num,
        stackable_tx_byte_feed_t *volatile *const active_feeds_stack) {

    // is the next tx_feed ready to go?
    if (pixel_feeds_ready & GPIO_TX_CHANNEL_MASK(tx_channel_num)) {
        // clear the tx_feed ready flag
        clear_bits(&pixel_feeds_ready, GPIO_TX_CHANNEL_MASK(tx_channel_num));
        tx_bytes_fed_ready_target[tx_channel_num] = tx_bytes_fed + tx_channels[tx_channel_num].bytes_fed_ready_interval;

        begin_frame(tx_channel_num);
        // move tx_bytes_feed idle -> active
        move_tx_bytes_feed(this_feed_ref, active_feeds_stack);

        // this_feed_ref has been advanced to the next idle tx_bytes_feed
        return this_feed_ref;

    } else if ((tx_channels[tx_channel_num].bytes_fed_ready_interval) &&
            tx_bytes_fed - tx_bytes_fed_ready_target[tx_channel_num] >= 0) {

        // set the next ready_target value to a multiple of the ready_interval
        do {
            tx_bytes_fed_ready_target[tx_channel_num] += tx_channels[tx_channel_num].bytes_fed_ready_interval;
        } while (tx_bytes_fed - tx_bytes_fed_ready_target[tx_channel_num] >= 0);

        begin_frame(tx_channel_num);
        // move tx_feed idle -> active
        move_tx_bytes_feed(this_feed_ref, active_feeds_stack);

        // this_feed_ref has been advanced to the next idle tx_bytes_feed
        return this_feed_ref;
    }

//    core1_log_tick_with_value("activate_channel_when_ready(%2d)", tx_channels[tx_channel_num].gpio_num);

    // advance to the next idle feed
    return &(*this_feed_ref)->next;
}

__force_inline static void activate_grb_3_pixel_channels_when_ready() {
    for (stackable_tx_byte_feed_t *volatile * current_idle_feed_ref = &grb_3_pixel_channel_feeds.idle_feeds;
            *current_idle_feed_ref;
            current_idle_feed_ref = (*current_idle_feed_ref)->activate_when_ready(current_idle_feed_ref));
}

__force_inline static void activate_rgbw_4_pixel_channels_when_ready() {
    for (stackable_tx_byte_feed_t *volatile * current_idle_feed_ref = &rgbw_4_pixel_channel_feeds.idle_feeds;
            *current_idle_feed_ref;
            current_idle_feed_ref = (*current_idle_feed_ref)->activate_when_ready(current_idle_feed_ref));
}

__force_inline static void end_frame(const uint8_t tx_channel_num) {
//    core1_log_tick_with_value("end_frame(%2d)", tx_channels[tx_channel_num].gpio_num);

    // close feed
    tx_channels[tx_channel_num].root_feed.close_frame(&tx_channels[tx_channel_num].root_feed);
    tx_channels[tx_channel_num].tx_status.frames_fed++;
}

__force_inline static stackable_tx_byte_feed_t *volatile * advance_tx_pixel(
        stackable_tx_byte_feed_t *volatile *const this_feed_ref,
        const uint8_t tx_channel_num,
        const uint32_t dest_buffer_index,
        stackable_tx_byte_feed_t *volatile *const terminal_feeds_stack) {

    if (tx_channels[tx_channel_num].tx_status.pixels_fed < tx_channels[tx_channel_num].pixel_count) {
//        core1_log_tick_with_value("advance_tx_pixel(%2d)", tx_channels[tx_channel_num].gpio_num);

        // set the chain_index and feed the next tx_pixel
        tx_channels[tx_channel_num].layout.set_chain_index(tx_channel_num);
        tx_channels[tx_channel_num].root_feed.feed_pixel(&tx_channels[tx_channel_num].root_feed, &tx_pixels[dest_buffer_index][tx_channel_num]);

        tx_channels[tx_channel_num].tx_status.pixels_fed++;
        set_bits(&tx_pixels_enabled[dest_buffer_index], GPIO_TX_CHANNEL_MASK(tx_channel_num));

        // advance to the next active feed
        return &(*this_feed_ref)->next;

    } else {
        // done feeding, close the pixel_feed and begin this channel's reset period
        end_frame(tx_channel_num);
        tx_bytes_fed_reset_target[tx_channel_num] = tx_bytes_fed + NUM_WS2812_RESET_PERIOD_BYTES;

        // move tx_feed active -> terminal
        move_tx_bytes_feed(this_feed_ref, terminal_feeds_stack);

        // this_feed_ref has been advanced to the next active tx_bytes_feed
        return this_feed_ref;
    }
}

__force_inline static stackable_tx_byte_feed_t *volatile * disable_tx_pixel(
        stackable_tx_byte_feed_t *volatile *const this_feed_ref,
        const uint8_t tx_channel_num,
        const uint32_t dest_buffer_index,
        stackable_tx_byte_feed_t *volatile *const resetting_feeds_stack) {

//    core1_log_tick_with_value("disable_tx_pixel(%2d)", tx_channels[tx_channel_num].gpio_num);

    if (tx_pixels_enabled[dest_buffer_index] & GPIO_TX_CHANNEL_MASK(tx_channel_num)) {
        tx_pixels[dest_buffer_index][tx_channel_num].uint32 = rgbw_pixel_off.uint32;
        clear_bits(&tx_pixels_enabled[dest_buffer_index], GPIO_TX_CHANNEL_MASK(tx_channel_num));
        
        // advance to the next terminal feed
        return &(*this_feed_ref)->next;

    } else { // done disabling
        // move tx_feed terminal -> resetting
        move_tx_bytes_feed(this_feed_ref, resetting_feeds_stack);

        // this_feed_ref has been advanced to the next terminal tx_bytes_feed
        return this_feed_ref;
    }
}

__force_inline static void advance_grb_3_channel_tx_pixels() {
    for (stackable_tx_byte_feed_t *volatile * current_active_feed_ref = &grb_3_pixel_channel_feeds.active_feeds;
            *current_active_feed_ref;
            current_active_feed_ref = (*current_active_feed_ref)->advance_tx_pixel(current_active_feed_ref));

    // zero out/disable the tx_pixel values for inactive feeds
    for (stackable_tx_byte_feed_t *volatile * current_terminal_feed_ref = &grb_3_pixel_channel_feeds.terminal_feeds;
            *current_terminal_feed_ref;
            current_terminal_feed_ref = (*current_terminal_feed_ref)->disable_tx_pixel(current_terminal_feed_ref));
}

__force_inline static void advance_rgbw_4_channel_tx_pixels() {
    for (stackable_tx_byte_feed_t *volatile * current_active_feed_ref = &rgbw_4_pixel_channel_feeds.active_feeds;
            *current_active_feed_ref;
            current_active_feed_ref = (*current_active_feed_ref)->advance_tx_pixel(current_active_feed_ref));

    // zero out/disable the tx_pixel values for inactive feeds
    for (stackable_tx_byte_feed_t *volatile * current_terminal_feed_ref = &rgbw_4_pixel_channel_feeds.terminal_feeds;
            *current_terminal_feed_ref;
            current_terminal_feed_ref = (*current_terminal_feed_ref)->disable_tx_pixel(current_terminal_feed_ref));
}

__force_inline static stackable_tx_byte_feed_t *volatile * advance_reset_count(
        stackable_tx_byte_feed_t *volatile *const this_feed_ref,
        const uint8_t tx_channel_num,
        stackable_tx_byte_feed_t *volatile *const idle_feeds_stack) {

    if (tx_bytes_fed - tx_bytes_fed_reset_target[tx_channel_num] < 0) {
//        core1_log_tick_with_value("advance_reset_counts(%2d)", tx_channels[tx_channel_num].gpio_num);

        // advance to the next feed
        return &(*this_feed_ref)->next;

    } else {
        // move tx_bytes_feed resetting -> idle
        move_tx_bytes_feed(this_feed_ref, idle_feeds_stack);

//        core1_log_tick_with_value("advance_reset_counts(%2d) - end", tx_channels[tx_channel_num].gpio_num);

        // this_feed_ref has been advanced to the next resetting tx_bytes_feed
        return this_feed_ref;
    }
}

__force_inline static void advance_reset_counts() {
    for (stackable_tx_byte_feed_t *volatile * current_resetting_feed_ref = &grb_3_pixel_channel_feeds.terminal_feeds;
            *current_resetting_feed_ref;
            current_resetting_feed_ref = (*current_resetting_feed_ref)->advance_reset_count(current_resetting_feed_ref));

    for (stackable_tx_byte_feed_t *volatile * current_resetting_feed_ref = &rgbw_4_pixel_channel_feeds.terminal_feeds;
            *current_resetting_feed_ref;
            current_resetting_feed_ref = (*current_resetting_feed_ref)->advance_reset_count(current_resetting_feed_ref));

    for (stackable_tx_byte_feed_t *volatile * current_resetting_feed_ref = &resetting_feeds;
            *current_resetting_feed_ref;
            current_resetting_feed_ref = (*current_resetting_feed_ref)->advance_reset_count(current_resetting_feed_ref));
}

static stackable_tx_byte_feed_t *volatile * ch0_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 0u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 1u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 2u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 3u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 4u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 5u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 6u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 7u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 8u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 9u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 10u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 11u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 12u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 13u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 14u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_activate_grb_3_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 15u, &grb_3_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 0u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 1u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 2u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 3u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 4u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 5u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 6u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 7u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 8u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 9u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 10u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 11u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 12u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 13u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 14u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_advance_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 15u, grb_3_channel_dest_buffer_index, &grb_3_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 0u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 1u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 2u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 3u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 4u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 5u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 6u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 7u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 8u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 9u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 10u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 11u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 12u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 13u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 14u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_disable_grb_3_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 15u, grb_3_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 0u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 1u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 2u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 3u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 4u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 5u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 6u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 7u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 8u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 9u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 10u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 11u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 12u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 13u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 14u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_advance_grb_3_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 15u, &grb_3_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t grb_3_tx_byte_feeds[NUM_TX_PINS] = {
        {
                .activate_when_ready = ch0_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch0_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch0_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch0_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch1_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch1_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch1_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch1_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch2_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch2_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch2_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch2_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch3_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch3_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch3_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch3_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch4_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch4_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch4_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch4_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch5_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch5_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch5_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch5_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch6_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch6_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch6_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch6_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch7_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch7_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch7_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch7_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch8_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch8_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch8_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch8_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch9_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch9_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch9_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch9_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch10_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch10_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch10_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch10_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch11_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch11_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch11_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch11_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch12_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch12_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch12_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch12_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch13_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch13_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch13_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch13_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch14_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch14_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch14_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch14_advance_grb_3_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch15_activate_grb_3_channel_when_ready,
                .advance_tx_pixel = ch15_advance_grb_3_channel_tx_pixel,
                .disable_tx_pixel = ch15_disable_grb_3_channel_tx_pixel,
                .advance_reset_count = ch15_advance_grb_3_channel_reset_count,
                .next = NULL,
        },
};


static stackable_tx_byte_feed_t *volatile * ch0_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 0u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 1u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 2u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 3u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 4u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 5u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 6u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 7u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 8u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 9u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 10u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 11u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 12u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 13u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 14u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_activate_rgbw_4_channel_when_ready(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return activate_channel_when_ready(this_feed_ref, 15u, &rgbw_4_pixel_channel_feeds.active_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 0u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 1u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 2u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 3u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 4u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 5u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 6u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 7u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 8u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 9u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 10u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 11u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 12u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 13u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 14u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_advance_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_tx_pixel(this_feed_ref, 15u, rgbw_4_channel_dest_buffer_index, &rgbw_4_pixel_channel_feeds.terminal_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 0u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 1u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 2u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 3u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 4u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 5u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 6u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 7u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 8u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 9u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 10u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 11u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 12u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 13u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 14u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_disable_rgbw_4_channel_tx_pixel(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return disable_tx_pixel(this_feed_ref, 15u, rgbw_4_channel_dest_buffer_index, &resetting_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch0_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 0u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch1_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 1u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch2_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 2u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch3_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 3u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch4_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 4u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch5_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 5u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch6_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 6u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch7_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 7u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch8_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 8u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch9_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 9u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch10_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 10u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch11_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 11u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch12_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 12u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch13_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 13u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch14_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 14u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t *volatile * ch15_advance_rgbw_4_channel_reset_count(stackable_tx_byte_feed_t *volatile *const this_feed_ref) {
    return advance_reset_count(this_feed_ref, 15u, &rgbw_4_pixel_channel_feeds.idle_feeds);
}

static stackable_tx_byte_feed_t rgbw_4_tx_byte_feeds[NUM_TX_PINS] = {
        {
                .activate_when_ready = ch0_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch0_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch0_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch0_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch1_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch1_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch1_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch1_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch2_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch2_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch2_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch2_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch3_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch3_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch3_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch3_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch4_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch4_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch4_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch4_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch5_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch5_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch5_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch5_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch6_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch6_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch6_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch6_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch7_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch7_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch7_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch7_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch8_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch8_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch8_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch8_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch9_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch9_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch9_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch9_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch10_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch10_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch10_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch10_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch11_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch11_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch11_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch11_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch12_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch12_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch12_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch12_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch13_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch13_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch13_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch13_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch14_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch14_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch14_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch14_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        }, {
                .activate_when_ready = ch15_activate_rgbw_4_channel_when_ready,
                .advance_tx_pixel = ch15_advance_rgbw_4_channel_tx_pixel,
                .disable_tx_pixel = ch15_disable_rgbw_4_channel_tx_pixel,
                .advance_reset_count = ch15_advance_rgbw_4_channel_reset_count,
                .next = NULL,
        },
};

__force_inline static void advance_tx_bytes_for_grb_3_channels() {
    if (grb_3_channel_byte_index) {
        grb_3_channel_byte_index--;

    } else { // previous grb_3_byte_index == 0
        // reset pixel_byte_index for start of the new pixel
        grb_3_channel_byte_index = GRB_3_BYTE_INDEX_RESET;

        activate_grb_3_pixel_channels_when_ready();
        advance_grb_3_channel_tx_pixels();
        advance_grb_3_channel_dest_buffer_index();
    }
}

__force_inline static void advance_tx_bytes_for_rgbw_4_channels() {
    if (rgbw_4_channel_byte_index) {
        rgbw_4_channel_byte_index--;

    } else { // previous rgbw_4_byte_index == 0
        // reset pixel_byte_index for start of the new pixel
        rgbw_4_channel_byte_index = RGBW_4_BYTE_INDEX_RESET;

        activate_rgbw_4_pixel_channels_when_ready();
        advance_rgbw_4_channel_tx_pixels();
        advance_rgbw_4_channel_dest_buffer_index();
    }
}

void advance_tx_bytes() {
    advance_tx_bytes_for_grb_3_channels();
    advance_tx_bytes_for_rgbw_4_channels();
    advance_reset_counts();
    tx_bytes_fed++;
}

void init_tx_byte_feeds() {
    pixel_feeds_ready = 0u;
    tx_bytes_fed = 0;

    for (uint8_t tx_channel_num = 0u; tx_channel_num < NUM_TX_PINS; tx_channel_num++) {
        switch (tx_channels[tx_channel_num].pixel_type) {
            case GRB_3_BYTE:
                add_tx_bytes_feed(&grb_3_tx_byte_feeds[tx_channel_num], &grb_3_pixel_channel_feeds.idle_feeds);
                set_bits(&grb_3_pixel_channels, GPIO_TX_CHANNEL_MASK(tx_channel_num));
                clear_bits(&rgbw_4_pixel_channels, GPIO_TX_CHANNEL_MASK(tx_channel_num));
                break;
            case RGBW_4_BYTE:
                add_tx_bytes_feed(&rgbw_4_tx_byte_feeds[tx_channel_num], &rgbw_4_pixel_channel_feeds.idle_feeds);
                set_bits(&rgbw_4_pixel_channels, GPIO_TX_CHANNEL_MASK(tx_channel_num));
                clear_bits(&grb_3_pixel_channels, GPIO_TX_CHANNEL_MASK(tx_channel_num));
                break;
        }
        tx_channels[tx_channel_num].tx_status = (tx_status_t) {
                .pixels_fed = 0u,
                .reserved = 0u,
                .frames_fed = 0u,
        };
        tx_bytes_fed_ready_target[tx_channel_num] = 0;
        tx_bytes_fed_reset_target[tx_channel_num] = 0;
    }
}

uint32_t get_tx_pixels_enabled() {
    return (tx_pixels_enabled[grb_3_channel_src_buffer_index] & grb_3_pixel_channels) |
           (tx_pixels_enabled[rgbw_4_channel_src_buffer_index] & rgbw_4_pixel_channels);
}