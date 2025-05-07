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

#include <hardware/structs/xosc.h>
#include <hardware/structs/bus_ctrl.h>
#include "common.h"

#include "hardware/adc.h"
#include "hardware/structs/systick.h"
#include "hardware/sync.h"

rgbw_pixel_t rgbw_pixel_off = {
        .red = 0x00u,
        .green = 0x00u,
        .blue = 0x00u,
        .white = 0x00u,
};

uint8_t rx_gpio_nums[NUM_RX_PINS] = {5u, 6u, 24u, 25u};

static spin_lock_t * systick_sync_lock;

void init_systick() {
    // set initial SysTick value
    systick_hw->cvr = 0u;
    // set max SysTick value
    systick_hw->rvr = 0xffffffu;
    // enable systick
    systick_hw->csr = 0x5u;

    // init and acquire the systick_sync_lock on core0, to support later calls to sync_systicks()
    if (0u == sio_hw->cpuid) {
        systick_sync_lock = spin_lock_init(spin_lock_claim_unused(true));
        spin_lock_unsafe_blocking(systick_sync_lock);
    }
}

// this is not a perfect scheme, but it should get the cores' systick counters to within ~10 systicks of each other
void sync_systicks() {
    if (sio_hw->cpuid) {
        while(!*systick_sync_lock) tight_loop_contents(); // wait for lock
        while(xosc_hw->count) tight_loop_contents(); // wait for count down
        systick_hw->cvr = 0u; // reset systick

    } else {
        *systick_sync_lock = 0u; // unlock (was locked by calling init_systick() earlier)
        xosc_hw->count = 12u; // set count down (12 counts at 12Mhz = 1us)
        while(xosc_hw->count) tight_loop_contents(); // wait for count down
        systick_hw->cvr = 0u; // reset systick
    }
}

void init_dma_bus_priority() {
    // Grant high bus priority to the DMA channels & low bus priority to proc0 & proc1
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;
}

void init_gpio_pads_and_io() {
    // power monitor pin
    adc_gpio_init(ADC_GPIO_PIN);
    gpio_set_output_disabled(ADC_GPIO_PIN, true);

    // TX GPIO PINS
    gpio_set_dir_out_masked(GPIO_TX_PINS_MASK);
    gpio_clr_mask(GPIO_TX_PINS_MASK);

    for (uint8_t gpio_num = GPIO_TX_PINS_BEGIN; gpio_num < GPIO_TX_PINS_END; gpio_num++) {
        gpio_set_function(gpio_num, GPIO_FUNC_NULL);
        gpio_set_pulls(gpio_num, false, true);
        gpio_set_input_enabled(gpio_num, false);
        gpio_set_output_disabled(gpio_num, false);
    }

    // TX GPIO SCORPIO_DEFAULT_WS2812_PIN (for debug and testing)
    gpio_set_dir_out_masked(SCORPIO_DEFAULT_WS2812_PIN_MASK);
    gpio_clr_mask(SCORPIO_DEFAULT_WS2812_PIN_MASK);

    gpio_set_function(SCORPIO_DEFAULT_WS2812_PIN, GPIO_FUNC_NULL);
    gpio_set_pulls(SCORPIO_DEFAULT_WS2812_PIN, false, true);
    gpio_set_input_enabled(SCORPIO_DEFAULT_WS2812_PIN, false);
    gpio_set_output_disabled(SCORPIO_DEFAULT_WS2812_PIN, false);

    // RX GPIO PINS
    gpio_set_dir_in_masked(GPIO_RX_PINS_MASK);
    gpio_clr_mask(GPIO_RX_PINS_MASK);

    for (uint8_t gpio_num = GPIO_RX_PINS_LOW_BEGIN; gpio_num < GPIO_RX_PINS_HIGH_END; gpio_num++) {
        if (GPIO_RX_PINS_MASK & (1u << gpio_num)) {
            gpio_set_function(gpio_num, GPIO_FUNC_NULL);
            gpio_set_pulls(gpio_num, false, false);
            gpio_set_input_enabled(gpio_num, true);
            gpio_set_output_disabled(gpio_num, true);
        }
    }
}

void init_core0_irqs() {
    irq_set_enabled(DMA_IRQ_0, true);
    irq_set_enabled(PIO0_IRQ_0, true);
}
