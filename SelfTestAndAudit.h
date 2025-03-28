/**************************************************************************
 *     This file is part of the Bally/Stern OS for Arduino Project.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 6/1/2020

    BallySternOS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BallySternOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.


    Version 2025.01 by Dave's Think Tank

    - Reduced to self-tests only

 */

// Self-Test Machine States (See also FGyyyypmm.h)
#define MACHINE_STATE_TEST_LAMPS           -1
#define MACHINE_STATE_TEST_DISPLAYS        -2
#define MACHINE_STATE_TEST_SOLENOIDS       -3
#define MACHINE_STATE_TEST_STUCK_SWITCHES  -4
#define MACHINE_STATE_TEST_SWITCH_BOUNCE   -5
#define MACHINE_STATE_TEST_SOUNDS          -6
#define MACHINE_STATE_TEST_DIP_SWITCHES    -7


#define MACHINE_STATE_TEST_DONE            -7

unsigned long GetLastSelfTestChangedTime();
void SetLastSelfTestChangedTime(unsigned long setSelfTestChange);
int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch, byte otherSwitch, byte endSwitch);
