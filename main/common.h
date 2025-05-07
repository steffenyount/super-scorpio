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

#ifndef SUPER_SCORPIO_COMMON_H
#define SUPER_SCORPIO_COMMON_H

#include "pico/platform.h"
#include "hardware/gpio.h"

#define __alignment(size_bits) __attribute__((aligned(1u << size_bits)))
#define SWAP(a, b) do { typeof(a) tmp = a; a = b; b = tmp; } while (false)

#define ADC_NUM (0u)
#define ADC_GPIO_PIN (26u + ADC_NUM)

#define GPIO_RX_PINS_LOW_BEGIN (5u)
#define GPIO_RX_PINS_LOW_END (7u)
#define GPIO_RX_PINS_HIGH_BEGIN (24u)
#define GPIO_RX_PINS_HIGH_END (26u)
#define GPIO_RX_PINS_MASK (0x3000060u)
#define NUM_RX_PINS (NUM_PIO_STATE_MACHINES)

#define SCORPIO_DEFAULT_WS2812_PIN (4u)
#define SCORPIO_DEFAULT_WS2812_PIN_MASK (1u << SCORPIO_DEFAULT_WS2812_PIN)
#define GPIO_TX_PINS_BEGIN (8u)
#define GPIO_TX_PINS_END (24u)
#define GPIO_TX_PINS_MASK (0x00ffff00u)
#define GPIO_TX_CHANNEL_0 (8u)
#define GPIO_TX_CHANNEL_0_MASK (1u << GPIO_TX_CHANNEL_0)
#define GPIO_TX_CHANNEL_1 (9u)
#define GPIO_TX_CHANNEL_1_MASK (1u << GPIO_TX_CHANNEL_1)
#define GPIO_TX_CHANNEL_2 (10u)
#define GPIO_TX_CHANNEL_2_MASK (1u << GPIO_TX_CHANNEL_2)
#define GPIO_TX_CHANNEL_3 (11u)
#define GPIO_TX_CHANNEL_3_MASK (1u << GPIO_TX_CHANNEL_3)
#define GPIO_TX_CHANNEL_4 (12u)
#define GPIO_TX_CHANNEL_4_MASK (1u << GPIO_TX_CHANNEL_4)
#define GPIO_TX_CHANNEL_5 (13u)
#define GPIO_TX_CHANNEL_5_MASK (1u << GPIO_TX_CHANNEL_5)
#define GPIO_TX_CHANNEL_6 (14u)
#define GPIO_TX_CHANNEL_6_MASK (1u << GPIO_TX_CHANNEL_6)
#define GPIO_TX_CHANNEL_7 (15u)
#define GPIO_TX_CHANNEL_7_MASK (1u << GPIO_TX_CHANNEL_7)
#define GPIO_TX_CHANNEL_8 (16u)
#define GPIO_TX_CHANNEL_8_MASK (1u << GPIO_TX_CHANNEL_8)
#define GPIO_TX_CHANNEL_9 (17u)
#define GPIO_TX_CHANNEL_9_MASK (1u << GPIO_TX_CHANNEL_9)
#define GPIO_TX_CHANNEL_10 (18u)
#define GPIO_TX_CHANNEL_10_MASK (1u << GPIO_TX_CHANNEL_10)
#define GPIO_TX_CHANNEL_11 (19u)
#define GPIO_TX_CHANNEL_11_MASK (1u << GPIO_TX_CHANNEL_11)
#define GPIO_TX_CHANNEL_12 (20u)
#define GPIO_TX_CHANNEL_12_MASK (1u << GPIO_TX_CHANNEL_12)
#define GPIO_TX_CHANNEL_13 (21u)
#define GPIO_TX_CHANNEL_13_MASK (1u << GPIO_TX_CHANNEL_13)
#define GPIO_TX_CHANNEL_14 (22u)
#define GPIO_TX_CHANNEL_14_MASK (1u << GPIO_TX_CHANNEL_14)
#define GPIO_TX_CHANNEL_15 (23u)
#define GPIO_TX_CHANNEL_15_MASK (1u << GPIO_TX_CHANNEL_15)
#define GPIO_TX_CHANNEL(c) (GPIO_TX_PINS_BEGIN + c)
#define GPIO_TX_CHANNEL_MASK(c) (1u << GPIO_TX_CHANNEL(c))
#define NUM_TX_PINS (16u)

#define SCORPIO_DEFAULT_WS2812_PIN_PWM_SLICE (2u)
#define SCORPIO_DEFAULT_WS2812_PIN_PWM_SLICE_MASK (1u << SCORPIO_DEFAULT_WS2812_PIN_PWM_SLICE)
#define NUM_PWM_SLICES_MASK ((1u << NUM_PWM_SLICES) - 1u)

// power monitoring
#define DMA_POWER_MONITOR_FEED_CHAN (0u)
#define DMA_POWER_MONITOR_FEED_CHAN_MASK (1u << DMA_POWER_MONITOR_FEED_CHAN)
#define DMA_POWER_MONITOR_FEED_TRIGGER_CHAN (1u)
#define DMA_POWER_MONITOR_FEED_TRIGGER_CHAN_MASK (1u << DMA_POWER_MONITOR_FEED_TRIGGER_CHAN)
#define DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN (2u)
#define DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN_MASK (1u << DMA_POWER_SAMPLES_SNAPSHOTTER_CHAN)
// channel discovery
#define DMA_PWM_CC_DATA_FEED_CHAN (3u)
#define DMA_PWM_CC_DATA_FEED_CHAN_MASK (1u << DMA_PWM_CC_DATA_FEED_CHAN)
#define DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN (4u)
#define DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN_MASK (1u << DMA_PWM_CC_DATA_FEED_DIRECTOR_CHAN)
// pixel bytes receive
#define DMA_SM0_RX_BYTES_FEED_CHAN (5u)
#define DMA_SM0_RX_BYTES_FEED_CHAN_MASK (1u << DMA_SM0_RX_BYTES_FEED_CHAN)
#define DMA_SM1_RX_BYTES_FEED_CHAN (6u)
#define DMA_SM1_RX_BYTES_FEED_CHAN_MASK (1u << DMA_SM1_RX_BYTES_FEED_CHAN)
#define DMA_SM2_RX_BYTES_FEED_CHAN (7u)
#define DMA_SM2_RX_BYTES_FEED_CHAN_MASK (1u << DMA_SM2_RX_BYTES_FEED_CHAN)
#define DMA_SM3_RX_BYTES_FEED_CHAN (8u)
#define DMA_SM3_RX_BYTES_FEED_CHAN_MASK (1u << DMA_SM3_RX_BYTES_FEED_CHAN)
// pixel bytes transmit
#define DMA_TX_BYTES_FEED_CHAN (9u)
#define DMA_TX_BYTES_FEED_CHAN_MASK (1u << DMA_TX_BYTES_FEED_CHAN)
#define DMA_TX_DATA_FEED_CHAN (10u)
#define DMA_TX_DATA_FEED_CHAN_MASK (1u << DMA_TX_DATA_FEED_CHAN)
#define DMA_TX_DATA_FEED_DIRECTOR_CHAN (11u)
#define DMA_TX_DATA_FEED_DIRECTOR_CHAN_MASK (1u << DMA_TX_DATA_FEED_DIRECTOR_CHAN)


#define NUM_PIO_STATE_MACHINE_TX_PINS (4u)
#define PIO_STATE_MACHINE_TX_PINS_MASK ((1u << NUM_PIO_STATE_MACHINE_TX_PINS) - 1u)


typedef struct __attribute__((packed)) {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} grb_pixel_t;

typedef union {
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t white;
    };
    uint8_t bytes[4];
    uint32_t uint32;
} rgbw_pixel_t;

typedef enum : uint8_t {
    GRB_3_BYTE = 3u,
    RGBW_4_BYTE = 4u,
} pixel_type_t;

extern rgbw_pixel_t rgbw_pixel_off;

static inline uint16_t pixel_count_to_byte_count(pixel_type_t pixel_type, uint16_t pixel_count) {
    return (GRB_3_BYTE == pixel_type) ? pixel_count * 3u : pixel_count * 4u;
}

static inline uint16_t fast_16_divide_by_3(uint16_t dividend) {
    // effectively doing a multiply by 1/3 ~ 21845/65536
    return (uint16_t)(((((uint32_t)dividend) + 1u) * 21845u) >> 16u);
}

static inline void gpio_set_function_no_side_effects(uint gpio, enum gpio_function fn) {
    // Zero all fields apart from fsel; we want this IO to do what the peripheral tells it.
    // This doesn't affect e.g. pullup/pulldown, as these are in pad controls.
    iobank0_hw->io[gpio].ctrl = fn << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
}

static inline void gpio_set_output_disabled(uint gpio, bool disabled) {
    if (disabled)
        hw_set_bits(&padsbank0_hw->io[gpio], PADS_BANK0_GPIO0_OD_BITS);
    else
        hw_clear_bits(&padsbank0_hw->io[gpio], PADS_BANK0_GPIO0_OD_BITS);
}

__force_inline static void set_bits(volatile uint32_t * const bits, uint32_t mask) {
    *bits |= mask;
}

__force_inline static void clear_bits(volatile uint32_t * const bits, uint32_t mask) {
    *bits &= ~mask;
}

// usage: gpio_num = rx_gpio_nums[rx_channel_num]
extern uint8_t rx_gpio_nums[NUM_RX_PINS];

static inline uint8_t rx_channel_num(uint8_t gpio_num) {
    switch (gpio_num) {
        case 5u:
            return 0;
        case 6u:
            return 1;
        case 24u:
            return 2;
        case 25u:
            return 3;

        default:
            return 0;
    }
}

void init_systick();

void sync_systicks();

void init_dma_bus_priority();

void init_gpio_pads_and_io();

void init_core0_irqs();

#endif //SUPER_SCORPIO_COMMON_H
