/**************************************************************************

Version 2001.01 by Dave's Think Tank

Additions and changes in this version:

- Initial version created from Flash Gordon v2024.12
- Reduced to self-tests only for use in PinballTestUnit
- New double-hit switch bounce test added, in addition to the stuck switch / switch matrix test.
- Converted from BSOS (Bally/Stern Operating System) to RPU (Retro Pin Upgrade). RPU is an extension of BSOS. BSOS is no longer maintained.
- WAV Trigger audio functions adapted from https://github.com/RetroPinUpgrade/SBM23 (Silverball Mania)

*/

//======================================= OPERATOR GAME ADJUSTMENTS =======================================
#define VERSION_NUMBER  2001.01   // Version number appears in Display #1 / Credit display at start of game
//=========================================================================================================

#include "RPU_Config.h"
#include "RPU.h"
#include "PinballTestUnit.h"
#include "SelfTestAndAudit.h"

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
#include "SendOnlyWavTrigger.h"
SendOnlyWavTrigger wTrig;             // Our WAV Trigger object
#endif

#define SOUND_EFFECT_NONE                             0
#define NUM_VOICE_NOTIFICATIONS                      13
#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START   100
byte MusicLevel =        4;
byte MusicVolume =      10;
int  SongNormalVolume = -4;

int i, j, k;

int MachineState = 0;
boolean MachineStateChanged = true;
unsigned long CurrentTime = 0;

// EEPROM Variables
byte dipBank[4];
byte selectedGame;
byte primarySwitch;
byte secondarySwitch;
byte endSwitch;
byte numDisplays;
byte numDigits;
byte numCredBIPDigits;
byte numDisplay6Digits;
byte numLamps;
byte numSolenoids;
byte solenoidRelay;
byte numSwitches;
byte numSounds;
byte soundBoard;
byte dropTargetID[6];

byte maxSelectedGame = 99;
byte maxDisplays = 5;
byte maxDigits = 7;
byte maxLamp = 87;
byte maxSolenoid = 29;
byte maxNonRelay = 15;
byte maxSwitch = 63;
byte maxSound = 255;
byte maxSoundBoard = 1;
byte maxDropTargets = 6;

boolean switchesVerified = false;
boolean validGame = false;
boolean relayStage = false;

byte upswitch;
byte downswitch;
boolean upHold, downHold;
unsigned long upHoldTime, downHoldTime;
boolean doneNothing = true;
byte curdisp;

byte dropTarget;
unsigned long solTimer;
boolean isDropTarget;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
int SoundEffectsNormalVolume = -4;
int SongDuckedVolume = -20;

void PlaySoundEffect(byte soundEffectNum, int gain = 100);
#endif

void test(byte val) { // display a test value
  j = 0;
  for (i = 0; i < 6; ++i)
    j += (dropTargetID[i] != 255);
  RPU_SetDisplay(2, j, true, 2);
    RPU_SetDisplay(3, val, true, 2); 
    delay(4000);
}



// ################## Set Selected Game Defaults ############
void SetSelectedGameDefaults() {
  primarySwitch = SW_GAME_BUTTON; // Default values for data table
  secondarySwitch = SW_COIN_3;
  endSwitch = SW_SLAM;
  numDisplays = 5;
  numDigits = 6;
  numCredBIPDigits = numDigits;
  numDisplay6Digits = numDigits;
  numLamps = 59;
  numSolenoids = maxNonRelay - 1;
  solenoidRelay = 99;
  numSwitches = 39;
  numSounds = 255;
  soundBoard = 0;
  for (i = 0; i < maxDropTargets; ++i)
    dropTargetID[i] = 0xFF;
}


// #################### VALID GAME DATA ##################
boolean ValidGameData() {
  if (primarySwitch > maxSwitch) return false;
  if (secondarySwitch > maxSwitch) return false;
  if (endSwitch > maxSwitch) return false;
  if (primarySwitch == secondarySwitch || primarySwitch == endSwitch || secondarySwitch == endSwitch) return false;
  if (numDisplays < 5 || numDisplays > maxDisplays) return false; // number of displays is currently limited to 5
  if (numDigits < 6 || numDigits > maxDigits) return false;
  if (numCredBIPDigits < 6 || numCredBIPDigits > maxDigits) return false;
  if (numDisplay6Digits < 6 || numDisplay6Digits > maxDigits) return false;
  if (numLamps == 0 || numLamps > maxLamp) return false;
  if (numSolenoids == 0 || numSolenoids > maxSolenoid) return false;
  if (numSolenoids >= maxNonRelay && solenoidRelay == 99) return false;
  if (solenoidRelay > numLamps && solenoidRelay != 99) return false;
  if (numSwitches == 0 || numSwitches > maxSwitch) return false;
  if (numSounds == 0 || numSounds > maxSound) return false;
  if (soundBoard > maxSoundBoard) return false;
  for (i = 0; i < maxDropTargets; ++i)
    if (dropTargetID[i] > maxSolenoid && dropTargetID[i] != 0xFF) return false;
  return true;
}



// ################### SET VALID DROP TARGET DATA ############

void SetValidDTData() {
  for (i = 0; i < maxDropTargets; ++i) // all values < maxDropTargets, or else set to 0xFF
    if (dropTargetID[i] > maxSolenoid) 
      dropTargetID[i] = 0xFF;

  for (i = 0; i < maxDropTargets - 1; ++i) // Remove duplicates
    if (dropTargetID[i] != 0xFF)
      for (j = i + 1; j < maxDropTargets; ++j)
        if (dropTargetID[j] == dropTargetID[i]) 
          dropTargetID[j] = 0xFF;
}


// ################### READ SELECTED GAME ################
void ReadSelectedGame(unsigned short game) {
  primarySwitch     = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_PRIMARY_SWITCH);
  secondarySwitch   = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SECONDARY_SWITCH);
  endSwitch         = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_END_SWITCH);
  numDisplays       = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAYS);
  numDigits         = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DIGITS);
  numCredBIPDigits  = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_CREDIT_BIP_DIGITS);
  numDisplay6Digits = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAY_6_DIGITS);
  numLamps          = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_LAMPS);
  numSolenoids      = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOLENOIDS);
  solenoidRelay     = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOLENOID_RELAY);
  numSwitches       = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SWITCHES);
  numSounds         = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOUNDS);
  soundBoard        = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOUND_BOARD);
  
  for (i = 0; i < 6; ++i) 
    dropTargetID[i] = RPU_ReadByteFromEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_DROP_TARGET_ID + i);

  validGame = ValidGameData();

  if (!validGame) { // Replace all invalid data
    if (primarySwitch > maxSwitch) primarySwitch = maxSwitch;
    if (secondarySwitch > maxSwitch) secondarySwitch = maxSwitch;
    if (endSwitch > maxSwitch) endSwitch = maxSwitch;
    if (endSwitch == primarySwitch || endSwitch == secondarySwitch) endSwitch = SW_SLAM;
    if (primarySwitch == secondarySwitch || primarySwitch == endSwitch) primarySwitch = SW_GAME_BUTTON;
    if (secondarySwitch == primarySwitch || secondarySwitch == endSwitch) secondarySwitch = SW_COIN_3;
    if (numDisplays < 5 || numDisplays > maxDisplays) numDisplays = maxDisplays; // number of displays is currently limited to 5
    if (numDigits < 6 || numDigits > maxDigits) numDigits = maxDigits;
    if (numCredBIPDigits < 6 || numCredBIPDigits > maxDigits) numCredBIPDigits = maxDigits;
    if (numDisplay6Digits < 6 || numDisplay6Digits > maxDigits) numDisplay6Digits = maxDigits;
    if (numLamps == 0 || numLamps > maxLamp) numLamps = maxLamp;
    if (numSolenoids == 0 || numSolenoids > maxSolenoid) numSolenoids = maxSolenoid;
    if (numSolenoids < maxNonRelay && solenoidRelay != 99) solenoidRelay = 99;
    if (numSolenoids >= maxNonRelay && solenoidRelay == 99) solenoidRelay = numLamps;
    if (numSwitches == 0 || numSwitches > maxSwitch) numSwitches = maxSwitch;
    if (numSounds == 0 || numSounds > maxSound) numSounds = maxSound;
    if (soundBoard > maxSoundBoard) soundBoard = maxSoundBoard;
    SetValidDTData();
  }
}


// #################### WRITE SELECTED GAME ##############
boolean WriteSelectedGame(unsigned short game) {
  validGame = ValidGameData();
  if ((!validGame) || game > maxSelectedGame) // do not write until entry is validated!
    return false;
  else {
    RPU_WriteByteToEEProm(RPU_EEPROM_SELECTED_GAME, game);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_PRIMARY_SWITCH, primarySwitch);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SECONDARY_SWITCH, secondarySwitch);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_END_SWITCH, endSwitch);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAYS, numDisplays);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DIGITS, numDigits);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_CREDIT_BIP_DIGITS, numCredBIPDigits);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUMBER_OF_DISPLAY_6_DIGITS, numDisplay6Digits);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_LAMPS, numLamps);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOLENOIDS, numSolenoids);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOLENOID_RELAY, solenoidRelay);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SWITCHES, numSwitches);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_NUM_SOUNDS, numSounds);
    RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_SOUND_BOARD, soundBoard);

    for (i = 0; i < 6; ++i) 
      RPU_WriteByteToEEProm(RPU_EEPROM_START_TABLE_DATA + (game * RPU_EEPROM_TABLE_ROW_SIZE) + RPU_EEPROM_DROP_TARGET_ID + i, dropTargetID[i]);
    }
  return true;
}



// #################### RUN SELF TEST ####################
int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;

  if (curState >= MACHINE_STATE_TEST_DONE) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, primarySwitch, secondarySwitch, endSwitch); 
    if (returnState >= 10000) { // Fudged to play sounds in the main program!
      StopAudio();
      PlaySoundEffect(returnState - 10000);
      returnState = MACHINE_STATE_TEST_SOUNDS;
    }
  }
  else {
    returnState = MACHINE_STATE_SELECT_GAME;
  }
  return returnState;
}



// #################### Select Game ####################
int SelectGame(int curState, boolean curStateChanged) {
  
  int  returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_SetDisplayCredits(0, false);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_TurnOffAllLamps();

    RPU_SetDisplay(0, selectedGame, true, 2);
    RPU_SetDisplay(1, validGame, true, 2);  
    RPU_SetDisplayBlank(2, 0);  
    RPU_SetDisplayBlank(3, 0);

    ReadSelectedGame(selectedGame);

    upswitch    = SWITCH_STACK_EMPTY;
    downswitch  = SWITCH_STACK_EMPTY;
    doneNothing = true;
  }

  if (switchesVerified) {
    upswitch = primarySwitch;
    downswitch = secondarySwitch;
  }

  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    if (doneNothing && validGame)       
      return returnState = -1; // Start tests
    else 
      return returnState += 1; // Enter/verify data
  }

  if (curSwitch == SW_SELF_TEST_SWITCH) curSwitch = SWITCH_STACK_EMPTY;

  upHold = false;
  if (upswitch != SWITCH_STACK_EMPTY) { // determine if up switch being held
    if (curSwitch == upswitch) // set time up switch hit if up switch hit
      upHoldTime = CurrentTime;
    
    if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(upswitch)) // zero out time hit if up switch released
      upHoldTime = 0;

    if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
      upHold = true;
      upHoldTime = CurrentTime;
    }
  }

  downHold = false;
  if (downswitch != SWITCH_STACK_EMPTY) { // determine if down switch being held
    if (curSwitch == downswitch) // same for down switch
      downHoldTime = CurrentTime;
    
    if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(downswitch))
      downHoldTime = 0;

    if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
      downHold = true;
      downHoldTime = CurrentTime;
    }
  }

  if (curSwitch != SWITCH_STACK_EMPTY || upHold || downHold) {
    doneNothing = false;
    if (upswitch == SWITCH_STACK_EMPTY) {
      upswitch = curSwitch;
      upHoldTime = CurrentTime;
    }
    else if (downswitch == SWITCH_STACK_EMPTY && curSwitch != upswitch) {
      downswitch = curSwitch;
      downHoldTime = CurrentTime;
    }

    if (curSwitch == upswitch || upHold) {
      if (selectedGame < maxSelectedGame)
        selectedGame += 1;
      else
        selectedGame = 0;
      ReadSelectedGame(selectedGame);
    }
    else if (curSwitch == downswitch || downHold) {
    if (selectedGame > 0)
      selectedGame -= 1;
    else
      selectedGame = maxSelectedGame;
    ReadSelectedGame(selectedGame);
    }
    if (!validGame) SetSelectedGameDefaults();
  }

  RPU_SetDisplayFlash(0, selectedGame, CurrentTime, 250, 2);
  RPU_SetDisplay(1, validGame, true, 2);  
  return returnState;
}



// #################### Set Test Buttons ####################
int SetTestButtons(int curState, boolean curStateChanged) {
  byte holdswitch;
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_SetDisplayCredits(0, false);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_TurnOffAllLamps();

    if (endSwitch == upswitch || endSwitch == downswitch)
      if (secondarySwitch != upswitch && secondarySwitch != downswitch)
        endSwitch = secondarySwitch;
      else
        endSwitch = primarySwitch;
    if (upswitch   != SWITCH_STACK_EMPTY) primarySwitch   = upswitch;
    if (downswitch != SWITCH_STACK_EMPTY) secondarySwitch = downswitch;

    RPU_SetDisplayFlash(0, (unsigned long) primarySwitch, CurrentTime, (int)250, 2);
    RPU_SetDisplay(1, secondarySwitch, true, 2);  
    RPU_SetDisplay(2, endSwitch, true, 2);
    RPU_SetDisplayBlank(3, 0); 

    curdisp = 0;
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    switchesVerified = true;
    return returnState += 1; 
  }

  if (curSwitch != SWITCH_STACK_EMPTY && curSwitch != SW_SELF_TEST_SWITCH) {
    if (curdisp == 0) {
      holdswitch = primarySwitch;
      primarySwitch = curSwitch;
      if (primarySwitch == secondarySwitch) secondarySwitch = holdswitch;
      if (primarySwitch == endSwitch) endSwitch = holdswitch;
      RPU_SetDisplay(0, primarySwitch, true, 2);
      RPU_SetDisplayFlash(1, (unsigned long) secondarySwitch, CurrentTime, (int)250, 2);
      RPU_SetDisplay(2, endSwitch, true, 2);
      curdisp = 1;
    } else if (curdisp == 1) {
      holdswitch = secondarySwitch;
      secondarySwitch = curSwitch;
      if (secondarySwitch == primarySwitch) primarySwitch = holdswitch;
      if (secondarySwitch == endSwitch) endSwitch = holdswitch;
      RPU_SetDisplay(0, primarySwitch, true, 2);
      RPU_SetDisplay(1, secondarySwitch, true, 2);
      RPU_SetDisplayFlash(2, (unsigned long) endSwitch, CurrentTime, (int)250, 2);
      curdisp = 2;
    } else {
      holdswitch = endSwitch;
      endSwitch = curSwitch;
      if (endSwitch == primarySwitch) primarySwitch = holdswitch;
      if (endSwitch == secondarySwitch) secondarySwitch = holdswitch;
      RPU_SetDisplayFlash(0, (unsigned long) primarySwitch, CurrentTime, (int)250, 2);
      RPU_SetDisplay(1, secondarySwitch, true, 2);
      RPU_SetDisplay(2, endSwitch, true, 2);
      curdisp = 0;
    }
  }
  if (curdisp == 0) RPU_SetDisplayFlash(0, (unsigned long) primarySwitch, CurrentTime, (int)250, 2);
  if (curdisp == 1) RPU_SetDisplayFlash(1, (unsigned long) secondarySwitch, CurrentTime, (int)250, 2);
  if (curdisp == 2) RPU_SetDisplayFlash(2, (unsigned long) endSwitch, CurrentTime, (int)250, 2);
  return returnState;
}



// #################### Display Data ####################
int DisplayData(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, numDigits, true, 2);
    RPU_SetDisplay(1, numDigits, true, 2);  
    RPU_SetDisplay(2, numDigits, true, 2);
    RPU_SetDisplay(3, numDigits, true, 2); 
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(numCredBIPDigits, true, true, numCredBIPDigits == 6);

    curdisp = 0;

    numDisplay6Digits = 6; // Cannot currently work with 6th display
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    return returnState += 1; 
  }

  if (curSwitch == primarySwitch) {
    if (curdisp == 0) {
      numDigits += 1;
      if (numDigits > maxDigits) numDigits = 6;
      RPU_SetDisplay(0, numDigits, true, 2);
      RPU_SetDisplay(1, numDigits, true, 2);
      RPU_SetDisplay(2, numDigits, true, 2);
      RPU_SetDisplay(3, numDigits, true, 2);
    } else {
      numCredBIPDigits += 1;
      if (numCredBIPDigits > maxDigits) numCredBIPDigits = 6;
      RPU_SetDisplayCredits(numCredBIPDigits, true, true, numCredBIPDigits == 6);
      RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6);
    }
  } else if (curSwitch == secondarySwitch) {
    if (curdisp == 0) {
      curdisp = 5;
      RPU_SetDisplay(0, numDigits, true, 2);
    } else {
      curdisp = 0;
      RPU_SetDisplayCredits(numCredBIPDigits, true, true, numCredBIPDigits == 6);
    }
  }
  
  if (curdisp == 0) RPU_SetDisplayFlash(0, numDigits, CurrentTime, 250, 2);
  if (curdisp == 5) RPU_SetDisplayCredits(numCredBIPDigits, (CurrentTime/250)%2, true, numCredBIPDigits == 6);
  return returnState;
}



// #################### Lamp Data ####################
int LampData(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, numLamps, true, 2);
    RPU_SetDisplayBlank(1, 0x00);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    return returnState += 1; 
  }

  upHold = false;
  if (curSwitch == primarySwitch) // set time up switch hit if up switch hit
    upHoldTime = CurrentTime;
  if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(primarySwitch)) // zero out time hit if up switch released
    upHoldTime = 0;
  if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
    upHold = true;
    upHoldTime = CurrentTime;
  }

  downHold = false;
  if (curSwitch == secondarySwitch) // same for down switch
    downHoldTime = CurrentTime;
  if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(secondarySwitch))
    downHoldTime = 0;
  if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
    downHold = true;
    downHoldTime = CurrentTime;
  }

  if (curSwitch == primarySwitch || upHold) {
    numLamps += 1;
    if (numLamps > maxLamp) numLamps = 1;
  } else if (curSwitch == secondarySwitch || downHold) {
    numLamps -= 1;
    if (numLamps < 1) numLamps = maxLamp;
  }
  RPU_SetDisplayFlash(0, numLamps, CurrentTime, 250, 2);
  return returnState;
}



// #################### Solenoid Data ####################
int SolenoidData(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, numSolenoids, true, 2);
    RPU_SetDisplayBlank(1, 0x00);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);

    relayStage = false; // First stage, the number of solenoids is entered. Second stage, the solenoid relay is set, if required.
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    if (relayStage) return returnState += 1; 
    relayStage = true;
    if (numSolenoids < maxNonRelay) {
      solenoidRelay = 99;
      return returnState += 1;
    }
    RPU_SetDisplay(0, numSolenoids, true, 2);
    RPU_SetDisplay(1, solenoidRelay, true, 2);
  }

  upHold = false;
  if (curSwitch == primarySwitch) // set time up switch hit if up switch hit
    upHoldTime = CurrentTime;
  if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(primarySwitch)) // zero out time hit if up switch released
    upHoldTime = 0;
  if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
    upHold = true;
    upHoldTime = CurrentTime;
  }

  downHold = false;
  if (curSwitch == secondarySwitch) // same for down switch
    downHoldTime = CurrentTime;
  if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(secondarySwitch))
    downHoldTime = 0;
  if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
    downHold = true;
    downHoldTime = CurrentTime;
  }
  if (relayStage) {
     if (curSwitch == primarySwitch || upHold) {
      solenoidRelay += 1;
      if (solenoidRelay > numSwitches) solenoidRelay = 0;
    } else if (curSwitch == secondarySwitch || downHold) {
      solenoidRelay -= 1;
      if (solenoidRelay == 255) solenoidRelay = maxSwitch;
    }
  } else {
    if (curSwitch == primarySwitch || upHold) {
      numSolenoids += 1;
      if (numSolenoids > maxSolenoid) numSolenoids = 1;
    } else if (curSwitch == secondarySwitch || downHold) {
      numSolenoids -= 1;
      if (numSolenoids < 1) numSolenoids = maxSolenoid;
    }
  }

  if (relayStage)
    RPU_SetDisplayFlash(1, solenoidRelay, CurrentTime, 250, 2);
  else
    RPU_SetDisplayFlash(0, numSolenoids, CurrentTime, 250, 2);
  return returnState;
}



// #################### Switch Data ####################
int SwitchData(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, numSwitches, true, 2);
    RPU_SetDisplayBlank(1, 0x00);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    return returnState += 1; 
  }

  upHold = false;
  if (curSwitch == primarySwitch) // set time up switch hit if up switch hit
    upHoldTime = CurrentTime;
  if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(primarySwitch)) // zero out time hit if up switch released
    upHoldTime = 0;
  if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
    upHold = true;
    upHoldTime = CurrentTime;
  }

  downHold = false;
  if (curSwitch == secondarySwitch) // same for down switch
    downHoldTime = CurrentTime;
  if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(secondarySwitch))
    downHoldTime = 0;
  if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
    downHold = true;
    downHoldTime = CurrentTime;
  }

  if (curSwitch == primarySwitch || upHold) {
    numSwitches += 1;
    if (numSwitches > maxSwitch) numSwitches = 1;
  } else if (curSwitch == secondarySwitch || downHold) {
    numSwitches -= 1;
    if (numSwitches < 1) numSwitches = maxSwitch;
  }
  RPU_SetDisplayFlash(0, numSwitches, CurrentTime, 250, 2);
  return returnState;
}



// #################### Sound Data ####################
int SoundData(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, numSounds, true, 2);
    RPU_SetDisplayBlank(1, 0x00);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    return returnState += 1; 
  }

  upHold = false;
  if (curSwitch == primarySwitch) // set time up switch hit if up switch hit
    upHoldTime = CurrentTime;
  if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(primarySwitch)) // zero out time hit if up switch released
    upHoldTime = 0;
  if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
    upHold = true;
    upHoldTime = CurrentTime;
  }

  downHold = false;
  if (curSwitch == secondarySwitch) // same for down switch
    downHoldTime = CurrentTime;
  if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(secondarySwitch))
    downHoldTime = 0;
  if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
    downHold = true;
    downHoldTime = CurrentTime;
  }


  if (curSwitch == primarySwitch || upHold) {
    numSounds += 1;
    if (numSounds > maxSound || numSounds == 0) numSounds = 1;
  } else if (curSwitch == secondarySwitch || downHold) {
    numSounds -= 1;
    if (numSounds < 1) numSounds = maxSound;
  }

  RPU_SetDisplayFlash(0, numSounds, CurrentTime, 250, 2);
  return returnState;
}



// #################### Sound Board ####################
int SoundBoard(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_DisableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();
    
    RPU_SetDisplay(0, soundBoard, true, 2);
    RPU_SetDisplayBlank(1, 0x00);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    return returnState += 1; 
  }

  upHold = false;
  if (curSwitch == primarySwitch) // set time up switch hit if up switch hit
    upHoldTime = CurrentTime;
  if (upHoldTime != 0 && !RPU_ReadSingleSwitchState(primarySwitch)) // zero out time hit if up switch released
    upHoldTime = 0;
  if (upHoldTime != 0 && (CurrentTime - upHoldTime) > 333) { // recognize when up switch held for 333 ms
    upHold = true;
    upHoldTime = CurrentTime;
  }

  downHold = false;
  if (curSwitch == secondarySwitch) // same for down switch
    downHoldTime = CurrentTime;
  if (downHoldTime != 0 && !RPU_ReadSingleSwitchState(secondarySwitch))
    downHoldTime = 0;
  if (downHoldTime != 0 && (CurrentTime - downHoldTime) > 333) {
    downHold = true;
    downHoldTime = CurrentTime;
  }

  if (curSwitch == primarySwitch || upHold) {
    soundBoard += 1;
    if (soundBoard > maxSoundBoard) soundBoard = 0;
  } else if (curSwitch == secondarySwitch || downHold) {
    soundBoard -= 1;
    if (soundBoard == 255) soundBoard = maxSoundBoard;
  }

  RPU_SetDisplayFlash(0, soundBoard, CurrentTime, 250, 2);
  return returnState;
}



// #################### Identify Drop Targets ####################
int IdentifyDropTargets(int curState, boolean curStateChanged) {
  int returnState = curState;
  byte curSwitch = RPU_PullFirstFromSwitchStack();
  
  if (curStateChanged) {
    RPU_EnableSolenoidStack();        
    RPU_SetDisableFlippers(true);
    RPU_TurnOffAllLamps();

    dropTarget = 99;
    solTimer = CurrentTime - 4000;
    isDropTarget = false;
    SetValidDTData();
    
    RPU_SetDisplay(0, dropTarget, true, 2);
    RPU_SetDisplay(1, isDropTarget, true, 2);
    RPU_SetDisplayBlank(2, 0x00);
    RPU_SetDisplayBlank(3, 0x00);
    RPU_SetDisplayBallInPlay(curState + 1, true, true, numCredBIPDigits == 6); // Ball in play displays current test step
    RPU_SetDisplayCredits(0, false, true, numCredBIPDigits == 6);
  }

  if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
    SetLastSelfTestChangedTime(CurrentTime);
    SetValidDTData();
    RPU_SetLampState(solenoidRelay, 0, 0, 0);
    return returnState += 1; 
  }

  if (dropTarget != 99) { // one second pause before first solenoid fires
    if (curSwitch == primarySwitch) { // Drop target identified: Add to list (only once!)
      
      isDropTarget = false;
      for (i = 0; i < maxDropTargets; ++i) { // Delete identified drop target from list (to ensure added once only)
        if (dropTargetID[i] == dropTarget)
          dropTargetID[i] = 0xFF;
      }
      for (i = 0; i < maxDropTargets; ++i) { // Add identified drop target to first available spot in list
        if (dropTargetID[i] == 0xFF) {
          dropTargetID[i] = dropTarget;
          isDropTarget = true;
          i = maxDropTargets; // end search
        }
      }
      RPU_SetDisplay(1, isDropTarget, true, 2);
      solTimer = CurrentTime - 4500;
    }
    if (curSwitch == secondarySwitch) { // Current solenoid identified as NOT a drop target
      isDropTarget = false;
      for (i = 0; i < maxDropTargets; ++i) { // Delete identified solenoid from list
        if (dropTargetID[i] == dropTarget) {
          dropTargetID[i] = 0xFF;
        }
      }
      RPU_SetDisplay(1, isDropTarget, true, 2);
      solTimer = CurrentTime - 4500;
    }
  }

  if (CurrentTime - solTimer > 5000) {
    solTimer = CurrentTime;
    dropTarget += 1;
    if (dropTarget > numSolenoids) dropTarget = 0;

    isDropTarget = false;
    for (i = 0; i < maxDropTargets; ++i)
      if (dropTargetID[i] == dropTarget) isDropTarget = true;

    RPU_SetDisplay(0, dropTarget, true, 2);
    RPU_SetDisplay(1, isDropTarget, true, 2);

    RPU_PushToSolenoidStack(dropTarget, 5, false, solenoidRelay);
  }
  RPU_SetDisplay(0, dropTarget, true, 2);
  RPU_SetDisplayFlash(1, isDropTarget, CurrentTime, 250, 2);
  return returnState;
}



// #################### LOOP ####################
void loop() {
  // This line has to be in the main loop
  RPU_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState<0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SELECT_GAME) {
    newMachineState = SelectGame(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SET_TEST_BUTTONS) {
    newMachineState = SetTestButtons(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_DISPLAY_DATA) {
    newMachineState = DisplayData(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_LAMP_DATA) {
    newMachineState = LampData(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SOLENOID_DATA) {
    newMachineState = SolenoidData(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SWITCH_DATA) {
    newMachineState = SwitchData(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SOUND_DATA) {
    newMachineState = SoundData(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_SOUND_BOARD) {
    newMachineState = SoundBoard(MachineState, MachineStateChanged);
  } else if (MachineState==MACHINE_STATE_IDENTIFY_DROP_TARGETS) {
    newMachineState = IdentifyDropTargets(MachineState, MachineStateChanged);
  } else {
    validGame = WriteSelectedGame(selectedGame);
    newMachineState=MACHINE_STATE_SELECT_GAME;
  }

  if (newMachineState!=MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_ApplyFlashToLamps(CurrentTime);
  RPU_UpdateTimedSolenoidStack(CurrentTime, solenoidRelay);
}

// #################### SETUP ####################
void setup() {
  
  CurrentTime = millis();
  // Set up the chips and interrupts
  RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED | RPU_CMD_PERFORM_MPU_TEST, SW_GAME_BUTTON);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Get all dip variables
  for (i = 0; i < 4; ++i) {
    dipBank[i] = RPU_GetDipSwitches(i);
    RPU_WriteByteToEEProm(RPU_EEPROM_DIP_BANK + i, dipBank[i]);
  }
  MachineState = 0;

  selectedGame = RPU_ReadByteFromEEProm(RPU_EEPROM_SELECTED_GAME); // Start by reading data for most recent selected game
  if (selectedGame > 99) selectedGame = 0;
  ReadSelectedGame(selectedGame);
  if (!validGame) SetSelectedGameDefaults();

  RPU_SetDisplay(0, floor(VERSION_NUMBER), true, 2);
  RPU_SetDisplayCredits(floor(100 * (VERSION_NUMBER + 0.005 - floor(VERSION_NUMBER))), true, true, numCredBIPDigits == 6);
  RPU_SetDisplayBlank(1, 0x00);
  RPU_SetDisplayBlank(2, 0x00);
  RPU_SetDisplayBlank(3, 0x00);

  delay(4000);

  RPU_SetDisplayBlank(0, 0);
  SetLastSelfTestChangedTime(CurrentTime);

  #if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);
  #endif

  StopAudio();
  // PlaySoundEffect(123);
}


////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
byte CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void StopAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif
}

void ResumeBackgroundSong() {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  byte curSong = CurrentBackgroundSong;
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
  PlayBackgroundSong(curSong);
#endif
}

void PlayBackgroundSong(byte songNum) {

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  if (MusicLevel > 3) {
    if (CurrentBackgroundSong != songNum) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);
      if (songNum != SOUND_EFFECT_NONE) {
#ifdef RPU_OS_USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else
        wTrig.trackPlayPoly(songNum);
#endif
        wTrig.trackLoop(songNum, true);
        wTrig.trackGain(songNum, -4);
      }
      CurrentBackgroundSong = songNum;
    }
  }
#else
  byte test = songNum;
  songNum = test;
#endif

}



#ifdef RPU_OS_USE_DASH51


// These SoundEFfectEntry & Queue functions parcel out FX to the
// built-in sound card because it can only handle one sound
// at a time.
struct SoundEffectEntry {
  byte soundEffectNum;
  unsigned long requestedPlayTime;
  byte playUntil;
  byte priority; // 0 is least important, 100 is most
};

#define SOUND_EFFECT_QUEUE_SIZE 20
SoundEffectEntry CurrentSoundPlaying;
SoundEffectEntry SoundEffectQueue[SOUND_EFFECT_QUEUE_SIZE];

void InitSoundEffectQueue() {
  CurrentSoundPlaying.soundEffectNum = 0;
  CurrentSoundPlaying.requestedPlayTime = 0;
  CurrentSoundPlaying.playUntil = 0;
  CurrentSoundPlaying.priority = 0;

  for (byte count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    SoundEffectQueue[count].soundEffectNum = 0;
    SoundEffectQueue[count].requestedPlayTime = 0;
    SoundEffectQueue[count].playUntil = 0;
    SoundEffectQueue[count].priority = 0;
  }
}

boolean PlaySoundEffectWhenPossible(byte soundEffectNum, unsigned long requestedPlayTime = 0, unsigned short playUntil = 50, byte priority = 10) {
  byte count = 0;
  for (count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    if ((SoundEffectQueue[count].priority & 0x80) == 0) break;
  }
  if (playUntil > 2550) playUntil = 2550;
  if (priority > 100) priority = 100;
  if (count == SOUND_EFFECT_QUEUE_SIZE) return false;
  SoundEffectQueue[count].soundEffectNum = soundEffectNum;
  SoundEffectQueue[count].requestedPlayTime = requestedPlayTime + CurrentTime;
  SoundEffectQueue[count].playUntil = playUntil / 10;
  SoundEffectQueue[count].priority = priority | 0x80;

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Sound 0x%04X slotted at %d\n\r", soundEffectNum, count);
    Serial.write(buf);
  }
  return true;
}

void UpdateSoundQueue() {
  byte highestPrioritySound = 0xFF;
  byte queuePriority = 0;

  for (byte count = 0; count < SOUND_EFFECT_QUEUE_SIZE; count++) {
    // Skip sounds that aren't in use
    if ((SoundEffectQueue[count].priority & 0x80) == 0) continue;

    // If a sound has expired, flush it
    if (CurrentTime > (((unsigned long)SoundEffectQueue[count].playUntil) * 10 + SoundEffectQueue[count].requestedPlayTime)) {
      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Expiring sound in slot %d (CurrentTime=%lu > PlayUntil=%d)\n\r", count, CurrentTime, SoundEffectQueue[count].playUntil * 10);
        Serial.write(buf);
      }
      SoundEffectQueue[count].priority &= ~0x80;
    } else if (CurrentTime > SoundEffectQueue[count].requestedPlayTime) {
      // If this sound is ready to be played, figure out its priority
      if (SoundEffectQueue[count].priority > queuePriority) {
        queuePriority = SoundEffectQueue[count].priority;
        highestPrioritySound = count;
      } else if (SoundEffectQueue[count].priority == queuePriority) {
        if (highestPrioritySound != 0xFF) {
          if (SoundEffectQueue[highestPrioritySound].requestedPlayTime > SoundEffectQueue[count].requestedPlayTime) {
            // The priorities are equal, but this sound was requested before, so switch to it
            highestPrioritySound = count;
          }
        }
      }
    }
  }

  if ((CurrentSoundPlaying.priority & 0x80) && (CurrentTime > (((unsigned long)CurrentSoundPlaying.playUntil) * 10 + CurrentSoundPlaying.requestedPlayTime))) {
    CurrentSoundPlaying.priority &= ~0x80;
  }

  if (highestPrioritySound != 0xFF) {

    if (DEBUG_MESSAGES) {
      char buf[128];
      sprintf(buf, "Ready to play sound 0x%04X\n\r", SoundEffectQueue[highestPrioritySound].soundEffectNum);
      Serial.write(buf);
    }

    if ((CurrentSoundPlaying.priority & 0x80) == 0 || ((CurrentSoundPlaying.priority & 0x80) && CurrentSoundPlaying.priority < queuePriority)) {
      // Play new sound
      CurrentSoundPlaying.soundEffectNum = SoundEffectQueue[highestPrioritySound].soundEffectNum;
      CurrentSoundPlaying.requestedPlayTime = SoundEffectQueue[highestPrioritySound].requestedPlayTime;
      CurrentSoundPlaying.playUntil = SoundEffectQueue[highestPrioritySound].playUntil;
      CurrentSoundPlaying.priority = SoundEffectQueue[highestPrioritySound].priority;
      CurrentSoundPlaying.priority |= 0x80;
      SoundEffectQueue[highestPrioritySound].priority &= ~0x80;
      RPU_PlaySoundDash51(CurrentSoundPlaying.soundEffectNum);
    }
  }
}



#endif


unsigned long NextSoundEffectTime = 0;




/*
   Start game = 0x08 (delay 180) 0x1F
   50 pt switches = [0x0F (delay 100)] x 5
   top rollovers (lit) = 0x17 (delay 1) [0x1D (delay 100)] x 5
   top rollovers (unlit) = 0x04
   pop explosion = 0x0D
   inlane rollover (lit) = 0x19
   kicker = [0x07 (delay 166)] x 5 (delay 281) 0x06
   inlane rollover (unlit) = 0x04
   end of ball (very little bonus) = 0x10 (delay 677) 0x1B (delay 138) 0x1F
   slingshot = 0x1E
   letter standup (lit) = 0x19
   end of ball (7 bonus) = 0x10 (delay 678) [0x1B (delay 50)] x 7 (delay 138) 0x1F
   Spinner = 0x1A (delay 690) [0x1A (delay 10)] x 4 0x1A (delay 1239) 0x1A (delay 0) 0x1A
   end of ball & game (8 bonus) = 0x10 (delay 677) [0x1B (delay 50)] x 8 (delay 282) 0x05
   stop everything (2 seconds after game) = 0x10
   credit = 0x0A

*/

#ifdef RPU_OS_USE_DASH51
void PlaySoundEffect51(byte soundEffectNum) {
  if (MusicLevel == 0) return;

  switch (soundEffectNum) {
    case SOUND_EFFECT_BACKGROUND_DRONE:
      PlaySoundEffectWhenPossible(0x11, 500, 1000, 100);
      RPU_PlaySoundDash51(0x11);
      break;
    case SOUND_EFFECT_STOP_SOUNDS:
    case SOUND_EFFECT_BONUS_START:
      // This kills other sounds
      PlaySoundEffectWhenPossible(0x10, 50, 100, 100);
      RPU_PlaySoundDash51(0x10);
      break;
    case SOUND_EFFECT_BONUS_1K:
      PlaySoundEffectWhenPossible(0x1B, 0, 40, 90);
      break;
    case SOUND_EFFECT_BONUS_15K:
    case SOUND_EFFECT_BONUS_30K:
    case SOUND_EFFECT_BONUS_KS:
      PlaySoundEffectWhenPossible(0x09, 0, 40, 90);
      break;
    case SOUND_EFFECT_BONUS_OVER:
      PlaySoundEffectWhenPossible(0x1F, 0, 40, 90);
      break;
    case SOUND_EFFECT_BUMPER_HIT_1:
    case SOUND_EFFECT_BUMPER_HIT_2:
    case SOUND_EFFECT_BUMPER_HIT_3:
      PlaySoundEffectWhenPossible(0x0D, 0, 50, 10);
      break;
    case SOUND_EFFECT_SLING_SHOT:
      PlaySoundEffectWhenPossible(0x1E, 0, 50, 10);
      break;
    case SOUND_EFFECT_KICKER_WATCH:
      PlaySoundEffectWhenPossible(0x07, 0, 100, 40);
      break;
    case SOUND_EFFECT_KICKER_LAUNCH:
      PlaySoundEffectWhenPossible(0x06, 0, 100, 40);
      break;
    case SOUND_EFFECT_ANIMATION_TICK:
      PlaySoundEffectWhenPossible(0x0F, 0, 50, 5);
      break;
    case SOUND_EFFECT_50PT_SWITCH:
      PlaySoundEffectWhenPossible(15, 0, 50, 10);
      PlaySoundEffectWhenPossible(15, 100, 50, 10);
      PlaySoundEffectWhenPossible(15, 200, 50, 10);
      PlaySoundEffectWhenPossible(15, 300, 50, 10);
      PlaySoundEffectWhenPossible(15, 400, 50, 10);
      break;
    case SOUND_EFFECT_BONUS_X_INCREASED:
      PlaySoundEffectWhenPossible(0x07, 0, 750, 90);
      PlaySoundEffectWhenPossible(0x0A, 1000, 500, 90);
      break;
    case SOUND_EFFECT_LIGHT_LETTER:
      PlaySoundEffectWhenPossible(0x19, 0, 500, 90);
      break;
    case SOUND_EFFECT_BALL_OVER:
      PlaySoundEffectWhenPossible(0x0D, 0, 1000, 100);
      break;
    case SOUND_EFFECT_GAME_OVER:
      PlaySoundEffectWhenPossible(0x05, 0, 0, 90);
      PlaySoundEffectWhenPossible(0x10, 2, 0, 100);
      break;
    case SOUND_EFFECT_SKILL_SHOT:
    case SOUND_EFFECT_SKILL_SHOT_ALT:
    case SOUND_EFFECT_EXTRA_BALL:
      PlaySoundEffectWhenPossible(0x10, 0, 5, 100);
      PlaySoundEffectWhenPossible(0x04, 10, 50, 100);
      PlaySoundEffectWhenPossible(0x05, 500, 250, 100);
      PlaySoundEffectWhenPossible(0x06, 1000, 250, 100);
      break;
    case SOUND_EFFECT_MACHINE_START:
      PlaySoundEffectWhenPossible(0x10, 3000, 5, 100);
      PlaySoundEffectWhenPossible(0x04, 3010, 50, 100);
      PlaySoundEffectWhenPossible(0x05, 3500, 250, 100);
      PlaySoundEffectWhenPossible(0x06, 4000, 250, 100);
      break;
    case SOUND_EFFECT_ADD_CREDIT_1:
    case SOUND_EFFECT_ADD_CREDIT_2:
    case SOUND_EFFECT_ADD_CREDIT_3:
      PlaySoundEffectWhenPossible(0x07, 0, 500, 100); // siren
      break;
    case SOUND_EFFECT_ADD_PLAYER_1:
    case SOUND_EFFECT_ADD_PLAYER_2:
    case SOUND_EFFECT_ADD_PLAYER_3:
    case SOUND_EFFECT_ADD_PLAYER_4:
      PlaySoundEffectWhenPossible(3, 0);
      PlaySoundEffectWhenPossible(2, 200);
      PlaySoundEffectWhenPossible(19, 400);
      PlaySoundEffectWhenPossible(18, 600);
      PlaySoundEffectWhenPossible(10, 800);
      PlaySoundEffectWhenPossible(10, 1000);
      PlaySoundEffectWhenPossible(8, 1200);
      break;
    case SOUND_EFFECT_UNLIT_LAMP_1:
      PlaySoundEffectWhenPossible(0x1C, 0, 50, 5);
      break;
    case SOUND_EFFECT_UNLIT_LAMP_2:
      PlaySoundEffectWhenPossible(0x0D, 0, 50, 5);
      break;
    case SOUND_EFFECT_ROLLOVER:
      PlaySoundEffectWhenPossible(0x1E, 0, 50, 5);
      break;
    case SOUND_EFFECT_PLACEHOLDER_LETTER:
      PlaySoundEffectWhenPossible(0x14, 0, 200, 40);
      break;
    case SOUND_EFFECT_TILT:
      PlaySoundEffectWhenPossible(14); // warning klaxon
      break;
    case SOUND_EFFECT_TILT_WARNING:
      PlaySoundEffectWhenPossible(12); // warning klaxon
      break;
    case SOUND_EFFECT_ADDED_BONUS_QUALIFIED:
      PlaySoundEffectWhenPossible(30, 0);
      PlaySoundEffectWhenPossible(30, 200);
      PlaySoundEffectWhenPossible(30, 400);
      PlaySoundEffectWhenPossible(31, 600);
      break;
    case SOUND_EFFECT_ADDED_BONUS_COLLECT:
      PlaySoundEffectWhenPossible(29, 0);
      PlaySoundEffectWhenPossible(25, 200);
      PlaySoundEffectWhenPossible(29, 400);
      PlaySoundEffectWhenPossible(31, 600);
      break;
    case SOUND_EFFECT_HORSESHOE:
      PlaySoundEffectWhenPossible(0x17, 0, 400, 95);
      PlaySoundEffectWhenPossible(0x1D, 420, 80, 95);
      PlaySoundEffectWhenPossible(0x1D, 520, 80, 95);
      PlaySoundEffectWhenPossible(0x1D, 620, 80, 95);
      PlaySoundEffectWhenPossible(0x1D, 720, 80, 95);
      PlaySoundEffectWhenPossible(0x1D, 820, 80, 95);
      break;
    case SOUND_EFFECT_SILVERBALL_COMPLETION:
      PlaySoundEffectWhenPossible(0x07, 0, 500, 100);
      PlaySoundEffectWhenPossible(0x0A, 500, 1200, 100);
      break;
    case SOUND_EFFECT_SPINNER_HIGH:
      PlaySoundEffectWhenPossible(0x1D, 0, 99, 50);
      break;
    case SOUND_EFFECT_SPINNER_LOW:
      PlaySoundEffectWhenPossible(0x1A, 0, 99, 50);
      break;
    case SOUND_EFFECT_MATCH_SPIN:
      PlaySoundEffectWhenPossible(0x13, 0, 20, 50);
      break;
  }

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }

}
#endif

inline void StopSoundEffect(byte soundEffectNum) {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.trackStop(soundEffectNum);
#else
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }
#endif
}


#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)

void PlaySoundEffect(byte soundEffectNum, int gain) {

  if (MusicLevel == 0) return;
  if (MusicLevel < 3) {
#ifdef RPU_OS_USE_DASH51
    PlaySoundEffect51(soundEffectNum);
#endif
    return;
  }

#ifndef RPU_OS_USE_WAV_TRIGGER_1p3
  if (  soundEffectNum == SOUND_EFFECT_THUMPER_BUMPER_HIT ||
        SOUND_EFFECT_SPINNER ) wTrig.trackStop(soundEffectNum);
#endif
  if (gain == 100) gain = SoundEffectsNormalVolume;
  wTrig.trackPlayPoly(soundEffectNum);
  wTrig.trackGain(soundEffectNum, gain);
}
#endif


#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)

#define VOICE_NOTIFICATION_STACK_SIZE   10
#define VOICE_NOTIFICATION_STACK_EMPTY  0xFFFF
byte VoiceNotificationStackFirst;
byte VoiceNotificationStackLast;
unsigned int VoiceNotificationNumStack[VOICE_NOTIFICATION_STACK_SIZE];
unsigned long NextVoiceNotificationPlayTime;

byte VoiceNotificationDurations[NUM_VOICE_NOTIFICATIONS] = {
  2, 2, 2, 2, 3, 3, 3, 3, 4, 3, 3, 3, 4
};


int SpaceLeftOnNotificationStack() {
  if (VoiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE || VoiceNotificationStackLast >= VOICE_NOTIFICATION_STACK_SIZE) return 0;
  if (VoiceNotificationStackLast >= VoiceNotificationStackFirst) return ((VOICE_NOTIFICATION_STACK_SIZE - 1) - (VoiceNotificationStackLast - VoiceNotificationStackFirst));
  return (VoiceNotificationStackFirst - VoiceNotificationStackLast) - 1;
}


void PushToNotificationStack(unsigned int notification) {
  // If the switch stack last index is out of range, then it's an error - return
  if (SpaceLeftOnNotificationStack() == 0) return;

  VoiceNotificationNumStack[VoiceNotificationStackLast] = notification;

  VoiceNotificationStackLast += 1;
  if (VoiceNotificationStackLast == VOICE_NOTIFICATION_STACK_SIZE) {
    // If the end index is off the end, then wrap
    VoiceNotificationStackLast = 0;
  }
}


unsigned int PullFirstFromVoiceNotificationStack() {
  // If first and last are equal, there's nothing on the stack
  if (VoiceNotificationStackFirst == VoiceNotificationStackLast) return VOICE_NOTIFICATION_STACK_EMPTY;

  unsigned int retVal = VoiceNotificationNumStack[VoiceNotificationStackFirst];

  VoiceNotificationStackFirst += 1;
  if (VoiceNotificationStackFirst >= VOICE_NOTIFICATION_STACK_SIZE) VoiceNotificationStackFirst = 0;

  return retVal;
}



void QueueNotification(unsigned int soundEffectNum, byte priority) {

  if (soundEffectNum < SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum >= (SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START + NUM_VOICE_NOTIFICATIONS)) return;

  // If there's nothing playing, we can play it now
  if (NextVoiceNotificationPlayTime == 0) {
    if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
      wTrig.trackFade(CurrentBackgroundSong, SongDuckedVolume, 500, 0);
    }
    NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[soundEffectNum - SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START]) * 1000;
    PlaySoundEffect(soundEffectNum, 2);
  } else {
    if (priority == 0) {
      PushToNotificationStack(soundEffectNum);
    }
  }
}

unsigned long SongVolumeRampDone = 0;
int LastVolumeRamp;

void ServiceNotificationQueue() {
  if (NextVoiceNotificationPlayTime != 0 && CurrentTime > NextVoiceNotificationPlayTime) {
    // Current notification done, see if there's another
    unsigned int nextNotification = PullFirstFromVoiceNotificationStack();
    if (nextNotification != VOICE_NOTIFICATION_STACK_EMPTY) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
        wTrig.trackFade(CurrentBackgroundSong, SongDuckedVolume, 500, 0);
      }
      NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[nextNotification - SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START]) * 1000;
      PlaySoundEffect(nextNotification, 2);
    } else {
      // No more notifications -- set the volume back up and clear the variable
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) {
        wTrig.trackFade(CurrentBackgroundSong, SongNormalVolume, 1500, 0);
      }
      NextVoiceNotificationPlayTime = 0;
    }
  }

}

#endif
