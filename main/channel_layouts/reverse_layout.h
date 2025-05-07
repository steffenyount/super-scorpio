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

#ifndef SUPER_SCORPIO_REVERSE_LAYOUT_H
#define SUPER_SCORPIO_REVERSE_LAYOUT_H

#include "channel_layouts.h"

extern channel_layout_t reverse_layout;

void set_reverse_layout(uint8_t tx_gpio_num);


#endif //SUPER_SCORPIO_REVERSE_LAYOUT_H
