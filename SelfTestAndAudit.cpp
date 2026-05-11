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


  Version 2024.04 by Dave's Think Tank

  - Features were added to the Self-Test function:
  - Sound test, changed to allow cycling through all sounds. Features using game button allow skipping sounds, repeating sounds, fast forward through sounds.
  - Added options in self test for all accounting values to be increased, as well as zeroed.

  Version 2024.07 by Dave's Think Tank

  - Switch Test: Added count of switches set to "On", in credit display.
  - Switch Test: Double-click on credit button resets all drop targets. FLASH GORDON SPECIFIC CODE!
  - These changes to switch test allow a user to impliment a switch-matrix test, by setting multiple switches and looking for incorrect totals.

  Version 2024.08 by Dave's Think Tank

  - Removed slamSwitch from inputs to RunBaseSelfTest. Replaced with otherSwitch, a second input switch to be used in tests.
  - DIP Switch Test: Added a test for DIP switches. Dip switch banks are displayed in binary (7 digits per display, plus one in ball-in-play or credit window.) 
  - DIP switches can be temporarily changed in game, using otherSwitch (described above).

  Version 2024.09 by Dave's Think Tank

  - Added "Reset Hold" feature to tests for lights, displays, and DIP switches. Reset Hold scrolls quickly through the display / review options.
  - Reset Hold option on audit settings sped up considerably.
  - Solenoids can be made to stop firing during solenoid test by pressing otherSwitch (Coin slot 3 switch).
  - endSwitch fixed so that it will register as a switch in switch test, and NOT end self-test mode.

  Version 2024.11 by Dave's Think Tank

  - Added monitoring of switches to solenoid test, to warn if vibration from a solenoid is setting off a switch

Version 2024.12 by Dave's Think Tank

  - When the solenoid test identifies a switch set off by vibration, it will also note the time in milliseconds between the solenoid firing and the switch activating
  - Cleaned up code by removing old, unused CPC (coins per credit) code.

  Version 2025.01 by Dave's Think Tank

  - Reduced to self-tests only for use in PinballTestUnit
  - New double-hit switch bounce test added, in addition to the stuck switch / switch matrix test.
  - Converted from BSOS (Bally/Stern Operating System) to RPU (Retro Pin Upgrade). RPU is an extension of BSOS. BSOS is no longer maintained.
  - Removed references to RPU_OS_USE_GEETEOH (BSOS_OS_USE_GEETEOH). RPU_OS_USE_GEETEOH is now defined in FGyyyypmm.ino as a user definition, where it belongs.

  Version 2026.05 by Dave's Think Tank

  - Updated to include improvements made to Star Trek and Flash Gordon, 2026.04
  - Replaced ResetHold with ResetBeingHeld in all tests.
  - Modified code to fully distinguish between single click, double click, and long press of reset button. For example, previously a double click registered as
    a single click followed by a double click.
  - Modified light test to allow testing of brightness level. Test covers off to full brightness, and flashing.
  - Replaced use of other button with double-click of reset button. Now only reset button required for all tests, with click, double-click, or hold.
  - Extended display test to allow cycling displays with value 8 only.
- Increase time to stop sound from 1/2 second to one second.

 */

#include <Arduino.h>
#include "SelfTestAndAudit.h"
#include "RPU_Config.h"
#include "RPU.h"

#define MACHINE_STATE_ATTRACT         0
//#define USE_SB100

byte dipBankVal[4];
unsigned long DisplayDIP[6];

int l, m, n, count; // xxx

// xxx EEPROM Variables
byte LselectedGame; // L for local
byte lightLevel;
byte LnumDisplays;
byte LnumDigits;
byte LnumCredBIPDigits;
byte LnumDisplay6Digits;
byte LnumLamps;
byte LnumSolenoids;
byte LsolenoidRelay;
byte LnumSwitches;
byte LnumSounds;
unsigned int LminSound;
byte LsoundBoard;
byte LdropTargetID[6];
byte LmaxDropTargets = 6;

unsigned long LastSolTestTime = 0; 
unsigned long LastSelfTestChange = 0;
unsigned long SavedValue = 0;
unsigned long SolSwitchTimer = 0;
unsigned long ResetHold = 0;
unsigned long otherHold = 0;
unsigned long NextSpeedyValueChange = 0;
unsigned long NumSpeedyChanges = 0;
unsigned long LastResetPress = 0;
unsigned long LastOtherPress = 0;
unsigned long LastAnyOtherPress = 0;

unsigned long SwitchTimer = 0;
byte HoldSwitch = SW_SELF_TEST_SWITCH;

byte CurValue = 0;
byte CurDisplay = 0;
byte CurDigit = 0;
byte xDigit = 0;
byte xDisplay = 0;
byte holdDisplay = 0;
boolean SoundPlayed = false;
byte SoundPlaying = 0;
byte SoundToPlay = 0;
boolean SolenoidCycle = true;
boolean SolenoidOn = true;

boolean coinLockoutOn;
boolean flippersOn;
boolean display8s;

byte curSwitch;
boolean resetDoubleClick = false;
boolean otherDoubleClick = false;
boolean anyOtherClick = false;
boolean anyOtherDoubleClick = false;

int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch, byte otherSwitch, byte endSwitch) {
  // Set resetSwitch to the game / credit button on the front of your pinball.
  // Set otherSwitch to any other switch easily accessible from the door of your pinball. This is used in some tests, where more than one switch is required to perform all necessary functions.
  //   I like to use SW_COIN_3, as I have it wired to a handy button for free game purposes!
  // Set endSwitch to the slam switch. This is used to end self test and return to attract mode.

  int returnState = curState;

  curSwitch = RPU_PullFirstFromSwitchStack();
  
  resetDoubleClick = false;
  otherDoubleClick = false;
  anyOtherClick = false;
  anyOtherDoubleClick = false;

  if (curSwitch == resetSwitch) {
    curSwitch = SWITCH_STACK_EMPTY;
    ResetHold = CurrentTime;
    if ((CurrentTime - LastResetPress) < 400) {
      resetDoubleClick = true;
      LastResetPress = 0;
    }
    else {
      LastResetPress = CurrentTime;
    }
  }

  if (!RPU_ReadSingleSwitchState(resetSwitch) && CurrentTime - LastResetPress > 400 && LastResetPress != 0) {
    curSwitch = resetSwitch;
    LastResetPress = 0;
  }

  if (curSwitch == otherSwitch) {
    otherHold = CurrentTime;
    if ((CurrentTime - LastOtherPress) < 400) {
      otherDoubleClick = true;
      curSwitch = SWITCH_STACK_EMPTY;
    }
    LastOtherPress = CurrentTime;
  }

  if (curSwitch != resetSwitch && curSwitch != otherSwitch && curSwitch != endSwitch && curSwitch != SW_SELF_TEST_SWITCH && curSwitch != SWITCH_STACK_EMPTY) {
    anyOtherClick = true;
    if ((CurrentTime - LastAnyOtherPress) < 400) {
      anyOtherDoubleClick = true;
      anyOtherClick = false;
    }
    LastAnyOtherPress = CurrentTime;
  }

  if (ResetHold != 0 && !RPU_ReadSingleSwitchState(resetSwitch)) {
    ResetHold = 0;
    NextSpeedyValueChange = 0;
  }

  boolean resetBeingHeld = false;
  if (ResetHold != 0 && (CurrentTime - ResetHold) > 1000) {
    resetBeingHeld = true;
    LastResetPress = 0;
    if (NextSpeedyValueChange == 0) {
      NextSpeedyValueChange = CurrentTime;
      NumSpeedyChanges = 0;
    }
  }

  if ((curSwitch == endSwitch) && (curState != MACHINE_STATE_TEST_STUCK_SWITCHES)) {
    return MACHINE_STATE_ATTRACT;
  }
  
  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>250) {
    returnState -= 1;
    LastSelfTestChange = CurrentTime;
  }

  if (curStateChanged) {
    if (curState == MACHINE_STATE_TEST_LAMPS) { // xxx Read in limits, first time through loop
      LselectedGame          = RPU_ReadByteFromEEProm(RPU_EEPROM_SELECTED_GAME);
      LnumDisplays           = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAYS);
      LnumDigits             = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DIGITS);
      LnumCredBIPDigits      = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_CREDIT_BIP_DIGITS);
      LnumDisplay6Digits     = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAY_6_DIGITS);
      LnumLamps              = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_LAMPS);
      LnumSolenoids          = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOLENOIDS);
      LsolenoidRelay         = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOLENOID_RELAY);
      LnumSwitches           = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SWITCHES);
      LnumSounds             = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOUNDS);
      LsoundBoard            = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOUND_BOARD);
      LminSound              = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_MIN_SOUND) * 256
                              + RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_MIN_SOUND + 1); 
      
      for (count = 0; count < LmaxDropTargets; ++count) 
        LdropTargetID[count] = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (LselectedGame * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_DROP_TARGET_ID + count);
    }
    
    for (count=0; count < LnumDisplays - 1; count++) {
      RPU_SetDisplay(count, 0);
      RPU_SetDisplayBlank(count, 0x00);
    }

    // if (curState<=MACHINE_STATE_TEST_DIP_SWITCHES) {
    //   RPU_SetDisplayCredits(MACHINE_STATE_TEST_DIP_SWITCHES-curState);
    //   RPU_SetDisplayBallInPlay(0, false);
    // }
  }

  if (curState==MACHINE_STATE_TEST_LAMPS) { //                                                            *** Test Lamps ***
    if (curStateChanged) {
      RPU_DisableSolenoidStack();        
      RPU_SetDisableFlippers(true);
      RPU_SetDisplayCredits(0, true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplayBallInPlay(1, true, true, LnumCredBIPDigits == 6);
      RPU_TurnOffAllLamps();
      lightLevel = 5; // Flashing
      for (count = 0; count  <=  LnumLamps; count++) {
        RPU_SetLampState(count, 1, 0, 500);
      }
      CurValue = 99;
      RPU_SetDisplay(0, CurValue, true);
      RPU_SetDisplay(1, lightLevel, true);
      LastSolTestTime = CurrentTime;
    }

    if (resetDoubleClick || curSwitch == otherSwitch) {
      lightLevel += 1;
      if (lightLevel > 5) lightLevel = 0;
      if (CurValue == 99) {
        for (count = 0; count  <=  LnumLamps; count++) {
          RPU_SetLampState(count, lightLevel != 0, lightLevel == 5 ? 0: lightLevel - 1, lightLevel == 5 ? 500 : 0);
        }
      } else {
        RPU_TurnOffAllLamps();
        RPU_SetLampState(CurValue, lightLevel != 0, lightLevel == 5 ? 0: lightLevel - 1, lightLevel == 5 ? 500 : 0);
      }
      RPU_SetDisplay(1, lightLevel, true);   
    }

    if (curSwitch==resetSwitch || (resetBeingHeld && CurrentTime > LastSolTestTime + 250)) {
      LastSolTestTime = CurrentTime;
      CurValue += 1;
      if (CurValue>99) {
        CurValue = 0;
        lightLevel = 1;
        RPU_SetDisplay(1, lightLevel, true);
      }
      if (CurValue > LnumLamps) {
        CurValue = 99;
        lightLevel = 5; // Flashing
        RPU_SetDisplay(1, lightLevel, true);
        for (count = 0; count  <=  LnumLamps; count++) {
          RPU_SetLampState(count,  lightLevel != 0, lightLevel == 5 ? 0: lightLevel - 1, lightLevel == 5 ? 500 : 0);
        }
      } else {
        RPU_TurnOffAllLamps();
        RPU_SetLampState(CurValue,  lightLevel != 0, lightLevel == 5 ? 0: lightLevel - 1, lightLevel == 5 ? 500 : 0);
      }      
      RPU_SetDisplay(0, CurValue, true);
    }    
  } else if (curState==MACHINE_STATE_TEST_DISPLAYS) { //                                                  *** Test Displays ***
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      RPU_SetDisplayCredits(0, true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplayBallInPlay(2, true, true, LnumCredBIPDigits == 6);
      for (count=0; count < LnumDisplays - 1; count++) {
        RPU_SetDisplayBlank(count, 0x3F);        
      }
      CurValue = 0;
      LastSolTestTime = CurrentTime;
      display8s = 0;
    }
    if (curSwitch==resetSwitch || (resetBeingHeld && CurrentTime > LastSolTestTime + 250)) {
      CurValue += 1;
      LastSolTestTime = CurrentTime;
      if (LnumDigits == 7) {
        if (CurValue>(LnumCredBIPDigits == 6 ? 34 : 35)) CurValue = 0;
      }
      else {
        if (CurValue>(LnumCredBIPDigits == 6 ? 30 : 31)) CurValue = 0;
      }
    }
    if (resetDoubleClick || curSwitch == otherSwitch) display8s = !display8s;
    RPU_CycleAllDisplays(CurrentTime, CurValue, LnumDigits == 6, display8s);

  } else if (curState==MACHINE_STATE_TEST_SOLENOIDS) { //                                                 *** Test Solenoids ***
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      LastSolTestTime = CurrentTime;
      SolSwitchTimer = CurrentTime;
      RPU_EnableSolenoidStack(); 
      RPU_SetDisableFlippers(flippersOn = true);
      RPU_SetCoinLockout(coinLockoutOn = true);
      RPU_SetDisplayBlank(4, 0);
      RPU_SetDisplayBallInPlay(3, true, true, LnumCredBIPDigits == 6);
      SolenoidCycle = true;
      SolenoidOn = true;
      SavedValue = 0;
      RPU_PushToSolenoidStack(SavedValue, 5, false, LsolenoidRelay);
    } 
    if (curSwitch==resetSwitch) SolenoidCycle = !SolenoidCycle;
    if (resetDoubleClick || curSwitch == otherSwitch) SolenoidOn = !SolenoidOn;
    if (curSwitch!=resetSwitch && curSwitch != otherSwitch && curSwitch != endSwitch && curSwitch != SWITCH_STACK_EMPTY && curSwitch != SW_SELF_TEST_SWITCH) {
      RPU_SetDisplayCredits(curSwitch, true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplay(3, CurrentTime - SolSwitchTimer, true, 3);
    }
    if (!SolenoidOn) {
      RPU_SetDisplayCredits(99, false); // Blank display when solenoids turned off
      RPU_SetDisplayBlank(3, 0);
    }

    if ((CurrentTime-LastSolTestTime)>1000) {
      if (SolenoidCycle) {
        SavedValue += 1;
        if (SavedValue > LnumSolenoids + 2) SavedValue = 0;
      }
      if (SolenoidOn) {
        SolSwitchTimer = CurrentTime;

        if (SavedValue == LnumSolenoids + 1)  // Test coin lockout
          RPU_SetCoinLockout(coinLockoutOn = !coinLockoutOn);
        else if (SavedValue == LnumSolenoids + 2)  // Test flipper enable
          RPU_SetDisableFlippers(flippersOn = !flippersOn);
        else
          RPU_PushToSolenoidStack(SavedValue, 5);
      }
      RPU_SetDisplay(0, SavedValue, true);
      LastSolTestTime = CurrentTime;
    }
    
   } else if (curState==MACHINE_STATE_TEST_STUCK_SWITCHES) { //                                                  *** Test Stuck Switches ***
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      RPU_DisableSolenoidStack(); // switches will not activate solenoids!
      RPU_SetDisableFlippers(true);
      RPU_SetDisplayCredits(0, true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplayBallInPlay(4, true, true, LnumCredBIPDigits == 6);
    }

    byte displayOutput = 0;
    for (byte switchCount=0; switchCount<=LnumSwitches; switchCount++) {
      if (RPU_ReadSingleSwitchState(switchCount)) {
        if (displayOutput < 4) RPU_SetDisplay(displayOutput, switchCount, true);
        displayOutput += 1;
      }
    }

    if (displayOutput<4) {
      for (count=displayOutput; count < LnumDisplays - 1; count++) {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }
    RPU_SetDisplayCredits(displayOutput, true, true, LnumCredBIPDigits == 6); // Let user know how many switches are on, since max four displayed
    
    if (resetDoubleClick) { // reset designated solenoids
      n = 0;
      for (m = 0; m < LmaxDropTargets; ++m) {
        if (LdropTargetID[m] != 255) {
          RPU_PushToTimedSolenoidStack(LdropTargetID[m], 15, CurrentTime + 250 * (unsigned long) n, true);
          n += 1;
        }
      }
    }
} else if (curState==MACHINE_STATE_TEST_SWITCH_BOUNCE) { //                                                  *** Test for Switch Bounce ***
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      RPU_DisableSolenoidStack(); // switches will not activate solenoids!
      RPU_SetDisableFlippers(true);
      RPU_SetDisplayCredits(0, false);
      RPU_SetDisplayBallInPlay(5, true, true, LnumCredBIPDigits == 6);

      for (count=0; count < 4; count++)
          RPU_SetDisplayBlank(count, 0x00);

      SwitchTimer = 0;
      HoldSwitch = SW_SELF_TEST_SWITCH;
    }

    
    if (curSwitch == HoldSwitch && curSwitch != SWITCH_STACK_EMPTY && curSwitch != SW_SELF_TEST_SWITCH && CurrentTime - SwitchTimer < 500) { // double-hit detected on a single switch
      RPU_SetDisplay(0, curSwitch, true);
      RPU_SetDisplay(1, CurrentTime - SwitchTimer, true);
      SwitchTimer = CurrentTime;
    }
    else {
      if (curSwitch != SWITCH_STACK_EMPTY && curSwitch != SW_SELF_TEST_SWITCH) { // single switch hit once
        RPU_SetDisplay(0, curSwitch, true);
        RPU_SetDisplayBlank(1, 0x00);
        HoldSwitch = curSwitch;
        SwitchTimer = CurrentTime;
      }
    }
    if (resetDoubleClick) { // reset designated solenoids
          n = 0;
          for (m = 0; m < LmaxDropTargets; ++m) {
            if (LdropTargetID[m] != 255) {
              RPU_PushToTimedSolenoidStack(LdropTargetID[m], 15, CurrentTime + 250 * (unsigned long) n, true);
              n += 1;
            }
          }
        }

  } else if (curState==MACHINE_STATE_TEST_SOUNDS) { //                                                    *** Test Sounds ***
    if (curStateChanged) {
      // RPU_TurnOffAllLamps();
      RPU_SetDisplayCredits(0, true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplayBallInPlay(6, true, true, LnumCredBIPDigits == 6);
      SolenoidCycle = true;
      SoundToPlay = LnumSounds; 
      // RPU_PlaySoundSquawkAndTalk(SoundToPlay);
      SoundPlaying = SoundToPlay;
      SoundPlayed = true;
      // RPU_SetDisplay(0, (unsigned long)SoundToPlay, true);
      LastSolTestTime = CurrentTime - 5000; // Time the sound started to play (5 seconds ago)
    } 

    if (resetBeingHeld && (CurrentTime - LastSolTestTime > 250)) {
      SoundToPlay += 1;
      if (SoundToPlay > LnumSounds) SoundToPlay = 0;
      SoundPlayed = false;
      RPU_SetDisplay(0,(unsigned long) LminSound + SoundToPlay, true);
      LastSolTestTime = CurrentTime;
      SolenoidCycle = true;
    }
    else {
      if (curSwitch==resetSwitch || resetDoubleClick) {
        if (CurrentTime - LastSolTestTime <= 1000) { // Allow 1 second to click and move forward without playing sound
          SoundToPlay +=1;
          if (SoundToPlay > LnumSounds) SoundToPlay = 0;
          RPU_SetDisplay(0, (unsigned long) LminSound + SoundToPlay, true);
          LastSolTestTime = CurrentTime - 500;
          }
        else {
          SolenoidCycle = !SolenoidCycle;
          }
        }
      if ((CurrentTime - LastSolTestTime) >= 1000 && !SoundPlayed) {
        if (LsoundBoard == 0)                         // S&T or Geeteoh
          RPU_PlaySoundSAndT(SoundToPlay);
        else if (LsoundBoard == 1) {                  // Wave Trigger
          returnState = 10000 + LminSound + SoundToPlay;          // Main program has all the info to play sounds using WAV Trigger!
          }
        
        SoundPlaying = SoundToPlay;
        SoundPlayed = true;
        }
      if ((CurrentTime - LastSolTestTime) >= 5000) {
        if (SolenoidCycle) {
          SoundToPlay += 1;
          if (SoundToPlay > LnumSounds) SoundToPlay = 0;
          }
        LastSolTestTime = CurrentTime;
        SoundPlayed = false;
        RPU_SetDisplay(0, (unsigned long) LminSound + SoundToPlay, true);
      }
    }
  } else if (curState==MACHINE_STATE_TEST_DIP_SWITCHES && LnumDigits == 7) {  //                                              *** Test DIP Switches, 32 digital displays ***
    
    if (curStateChanged) {
      RPU_TurnOffAllLamps();

      for (int i=0; i<4; i++) { // Get four DIP banks from memory, convert to binary display
        dipBankVal[i] = RPU_ReadByteFromEEProm(RPU_EEPROM_DIP_BANK + i);
        
        DisplayDIP[i] = 0;
        int k = 64;
        for (int j=0; j<7; ++j) {
          DisplayDIP[i] = 10 * DisplayDIP[i] + ((dipBankVal[i] & k) != 0);
          k = k >> 1;
        }
        RPU_SetDisplayBlank(i, 127);
        RPU_SetDisplay(i, DisplayDIP[i], false);
      }
      DisplayDIP[4] = 10 * (dipBankVal[1] >= 128) + (dipBankVal[0] >= 128);
      DisplayDIP[5] = 10 * (dipBankVal[3] >= 128) + (dipBankVal[2] >= 128);
      RPU_SetDisplayBallInPlay(DisplayDIP[4], true, true, LnumCredBIPDigits == 6);
      RPU_SetDisplayCredits(DisplayDIP[5], true, true, LnumCredBIPDigits == 6);

      CurValue = 0;
      xDisplay = CurDisplay = 0;
      LastSolTestTime = CurrentTime;
    }
   
    if (curSwitch==resetSwitch || (resetBeingHeld && CurrentTime > LastSolTestTime + 250)) {
      if (xDisplay < 4) RPU_SetDisplayBlank(CurDisplay, 127); // Reset previous digit to not flash
      else RPU_SetDisplayBlank(4, 108);
      
      CurValue += 1;
      if (CurValue>=32) CurValue = 0;
      LastSolTestTime = CurrentTime;
    }    
    xDisplay = CurDisplay = CurValue / 8;
    xDigit = CurDigit = CurValue % 8;

    if (CurDigit == 7) { // Final digit must be displayed in ball-in-play or credit window
      xDigit = CurDisplay & 1; // Digit 0 or 1, depending on which display being completed
      xDisplay = 4 + CurValue / 16; // Ball in play or credit window
    }

    if (resetDoubleClick || curSwitch == otherSwitch) { // Flip current digit in current display
      dipBankVal[CurDisplay] = dipBankVal[CurDisplay] ^ (1 << CurDigit); // exclusive or function, reverses current digit
      RPU_WriteByteToEEProm(RPU_EEPROM_DIP_BANK + CurDisplay, dipBankVal[CurDisplay]);

      if (xDisplay < 4) { // display value as binary
        DisplayDIP[CurDisplay] = 0;
        int k = 64;
        for (int j=0; j<7; ++j) {
          DisplayDIP[CurDisplay] = 10 * DisplayDIP[CurDisplay] + ((dipBankVal[CurDisplay] & k) != 0);
          k = k >> 1;
        }
        RPU_SetDisplay(CurDisplay, DisplayDIP[CurDisplay], false);
      }
      else if (xDisplay == 4) {
        DisplayDIP[4] = 10 * (dipBankVal[1] >= 128) + (dipBankVal[0] >= 128);
        RPU_SetDisplayBallInPlay(DisplayDIP[4], true, true, LnumCredBIPDigits == 6);
      }
      else {
        DisplayDIP[5] = 10 * (dipBankVal[3] >= 128) + (dipBankVal[2] >= 128);
        RPU_SetDisplayCredits(DisplayDIP[5], true, true, LnumCredBIPDigits == 6);
      }
    }

    if (xDisplay < 4) // set mask for flashing digit
      RPU_SetDigitFlash(CurDisplay, CurDigit, DisplayDIP[CurDisplay], CurrentTime, 250);
    else if (xDisplay == 4) 
      RPU_SetDigitFlashBallInPlay(xDigit, CurrentTime, 250);
    else
      RPU_SetDigitFlashCredits(xDigit, CurrentTime, 250);

  }  else if (curState==MACHINE_STATE_TEST_DIP_SWITCHES && LnumDigits == 6) {  //                                              *** Test DIP Switches, only 28 digital displays ***
    
    if (curStateChanged) {
      RPU_TurnOffAllLamps();
      RPU_SetDisplayBallInPlay(7, true, true, LnumCredBIPDigits == 6);

      for (int i=0; i<4; i++) { // Get four DIP banks from memory, convert to binary display
        dipBankVal[i] = RPU_ReadByteFromEEProm(RPU_EEPROM_DIP_BANK + i);

        DisplayDIP[i] = 0;
        int k = 32;
        for (int j=0; j<6; ++j) {
          DisplayDIP[i] = 10 * DisplayDIP[i] + ((dipBankVal[i] & k) != 0);
          k = k >> 1;
        }
        RPU_SetDisplayBlank(i, 127);
        RPU_SetDisplay(i, DisplayDIP[i], false);
      }
      DisplayDIP[4] = 10 * (0 != (dipBankVal[0] & 128)) + (0 != (dipBankVal[0] & 64));
      RPU_SetDisplayCredits(DisplayDIP[4], true, true, LnumCredBIPDigits == 6);

      CurValue = 0;
      holdDisplay = xDisplay = CurDisplay = 0;
      LastSolTestTime = CurrentTime;
    }
   
    if (curSwitch==resetSwitch || (resetBeingHeld && CurrentTime > LastSolTestTime + 250)) {
      if (xDisplay < 4) RPU_SetDisplayBlank(CurDisplay, 127); // Reset previous digit to not flash
      else RPU_SetDisplayBlank(4, 108);
      
      CurValue += 1;
      if (CurValue>=32) CurValue = 0;
      LastSolTestTime = CurrentTime;
    }    
    xDisplay = CurDisplay = CurValue / 8; // CurDisplay and CurValue set as if there are 8 digits available
    xDigit = CurDigit = CurValue % 8;     // xDisplay and xDigit will be adjusted to use credit window for last two digits

    if (CurDigit >= 6) { // Final two digits must be displayed in credit window
      xDigit = CurDigit - 6; // Digit 0 or 1 of credit window
      xDisplay = 4; // Credit window
    }
    
    if (CurDisplay != holdDisplay) { // Credit window reset to match last two digits of current display
      DisplayDIP[4] = 10 * (0 != (dipBankVal[CurDisplay] & 128)) + (0 != (dipBankVal[CurDisplay] & 64));
      RPU_SetDisplayCredits(DisplayDIP[4], true, true, LnumCredBIPDigits == 6);
      holdDisplay = CurDisplay;
    }

    if (resetDoubleClick || curSwitch == otherSwitch) { // Flip current digit in current display
      dipBankVal[CurDisplay] = dipBankVal[CurDisplay] ^ (1 << CurDigit); // exclusive or function, reverses current digit
      RPU_WriteByteToEEProm(RPU_EEPROM_DIP_BANK + CurDisplay, dipBankVal[CurDisplay]);

      if (xDisplay < 4) { // display value as binary
        DisplayDIP[CurDisplay] = 0;
        int k = 32;
        for (int j=0; j<6; ++j) {
          DisplayDIP[CurDisplay] = 10 * DisplayDIP[CurDisplay] + ((dipBankVal[CurDisplay] & k) != 0);
          k = k >> 1;
        }
        RPU_SetDisplay(CurDisplay, DisplayDIP[CurDisplay], false);
      }
      else {
      DisplayDIP[4] = 10 * (0 != (dipBankVal[CurDisplay] & 128)) + (0 != (dipBankVal[CurDisplay] & 64));
      RPU_SetDisplayCredits(DisplayDIP[4], true, true, LnumCredBIPDigits == 6);
      }
    }

    if (xDisplay < 4) // set mask for flashing digit
      RPU_SetDigitFlash(CurDisplay, CurDigit, DisplayDIP[CurDisplay], CurrentTime, 250);
    else
      RPU_SetDigitFlashCredits(xDigit, CurrentTime, 250, LnumCredBIPDigits == 6);
  } 
  return returnState;
}


unsigned long GetLastSelfTestChangedTime() {
  return LastSelfTestChange;
}

void SetLastSelfTestChangedTime(unsigned long setSelfTestChange) {
  LastSelfTestChange = setSelfTestChange;
}
