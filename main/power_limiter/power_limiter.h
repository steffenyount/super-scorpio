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

#ifndef SUPER_SCORPIO_POWER_LIMITER_H
#define SUPER_SCORPIO_POWER_LIMITER_H

#include "common.h"

#define POWER_LIMIT_10A_THRESHOLD_11_BIT (1862u)
#define POWER_LIMIT_10A_THRESHOLD_13_BIT (7447u)

void limit_tx_channel_power();


#endif //SUPER_SCORPIO_POWER_LIMITER_H
