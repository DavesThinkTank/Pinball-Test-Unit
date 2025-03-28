/**************************************************************************
  This file is part of Flash Gordon 2021 - Mega

  Version: 1.0.0

  I, Tim Murren, the author of this program disclaim all copyright
  in order to make this program freely available in perpetuity to
  anyone who would like to use it. Tim Murren, 2/5/2021

  Flash Gordon 2021 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Flash Gordon 2021 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  See <https://www.gnu.org/licenses/>.

*/

#define SW_GAME_BUTTON              5 // CREDIT BUTTON
#define SW_TILT                     6 // TILT (3)
#define SW_COIN_3                   8 // COIN III (RIGHT)
#define SW_SLAM                     15 // SLAM (2)

#define MACHINE_STATE_SELECT_GAME           0
#define MACHINE_STATE_SET_TEST_BUTTONS      1
#define MACHINE_STATE_DISPLAY_DATA          2
#define MACHINE_STATE_LAMP_DATA             3
#define MACHINE_STATE_SOLENOID_DATA         4
#define MACHINE_STATE_SWITCH_DATA           5
#define MACHINE_STATE_SOUND_DATA            6
#define MACHINE_STATE_SOUND_BOARD           7
#define MACHINE_STATE_IDENTIFY_DROP_TARGETS 8

// SWITCHES_WITH_TRIGGERS are for switches that will automatically
// activate a solenoid (like in the case of a chime that rings on a rollover)
// but SWITCHES_WITH_TRIGGERS are fully debounced before being activated
#define NUM_SWITCHES_WITH_TRIGGERS 0

// PRIORITY_SWITCHES_WITH_TRIGGERS are switches that trigger immediately
// (like for pop bumpers or slings) - they are not debounced completely
#define NUM_PRIORITY_SWITCHES_WITH_TRIGGERS 0
