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

#define __alignment(size_bits) __attribute__((aligned(1u << size_bits)))

#define SCORPIO_DEFAULT_WS2812_PIN (4u)
#define NUM_PWM_SLICES_MASK ((1u << NUM_PWM_SLICES) - 1u)

#endif //SUPER_SCORPIO_COMMON_H
