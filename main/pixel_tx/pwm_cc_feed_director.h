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

#ifndef SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H
#define SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H

#include "pixel_tx.h"

#include "hardware/dma.h"

extern volatile uint8_t ** next_srcs_for_tx_bytes_director;

extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__load_tx_bytes_stage_next_tx_bytes_and_feed_pwm_cc_bit_7[20];
extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__loop_back_tx_bytes_and_feed_pwm_cc_bit[34];
extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__discard_tx_bytes_and_feed_pwm_cc_bit_0[34];

extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__source_red_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8];
extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__source_blue_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8];
extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__source_green_tx_bytes_trigger_next_feed_and_feed_pwm_cc_bit_0[2][8];
extern uint32_t * trigger_termination_sequence_and_feed_pwm_cc_bit_0[2][8];
extern uint32_t * srcs_and_dests_for_pwm_cc_feed_director__trigger_next_feed_or_termination_sequence_and_feed_pwm_cc_bit_0[2][8];



void init_dma_pwm_cc_feed_director();

#endif //SUPER_SCORPIO_PWM_CC_FEED_DIRECTOR_H
