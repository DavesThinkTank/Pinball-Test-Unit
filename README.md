# Pinball Test Unit 2025
## Version 2001.01
## for the Arduino Mega 2560 Rev3

The new Pinball Test Unit (PTU) from Dave’s Think Tank improves tremendously on the self-tests of any pinball running a Bally AS-2518-17, AS-2518-35, Stern MPU-100, Stern MPU-200, or Alltek replacement MPU board. 

What kind of improvements are we talking about? Instead of just running through the solenoids over and over, the solenoid test now allows you to stop on and repeatedly fire a single solenoid, then stop it from firing while you make adjustments, then start it up again. It will tell you if vibration from a solenoid is setting off a switch. Similar improvements can be found in the switch test: fixing bouncing switches and even switch matrix errors just became a breeze! Not to mention new tests like the DIP switch review. Check out the new and extended tests below:

## Test 1: Light Test

The first test will repeatedly flash all the switched illumination lights on the playfield and in the backbox. This is similar to the regular Bally light test, except the PTU allows you to now press a button to stop all the lights from flashing except one. Then you can scroll through the lights individually.

So why would you want to do this? Well, maybe you just find it easier to work on a light when the whole machine isn’t blinking at you! In some machines, these light switches control not just lights, but also switched relays, and it can be important to find them, especially if they're not working! You can also make up a numbered list of lights, making it easier to locate problems in the future. A numbered list is essential if you intend to do any Arduino programming of your own. There are lots of advantages to expanding this test!

##Test 2: Display Test

Pressing the self-test button again will then take you to the display test. The first difference you’ll probably notice is, it’s a lot faster than the standard display test! Every digit gets through the numbers from 0 to 9 in 2 and a half seconds, instead of what used to seem like 2 and a half minutes. Trying to watch five displays of six or seven digits, each taking about 10 seconds to complete was always a tedious process. 

To make it easier though, you can press a button. Now you can watch one digit at a time going through its paces without distraction. Hold in the switch, and you can scroll to the digit you’re interested in. Scroll to the end and all the digits scroll again.

 
##Test 3: Solenoid Test

Pressing the self-test button again takes you to the solenoid test. This runs through all the solenoids, just like the regular Bally test (except in a different order). Note, the Coin Door Lockout and the K1 Relay Flipper Enable are not included in these tests. Note also that the numbering of solenoids is different than you will find in your game manual.

New Features: 

Pressing the primary switch at any point will cause the current solenoid to continue firing repeatedly, so you no longer have to cycle through all of the solenoids to see the one you are interested in. Press again to continue cycling. Press the secondary switch to turn firing of solenoids off, and back on. This allows you to both observe and work on a solenoid while remaining in test mode! Just remember to use caution, and keep hands and tools away from the electrical wiring!

Keep an eye on the credit window during this test. If vibration from a solenoid causes a switch to misfire, the switch number will be displayed here. The time between the solenoid firing and the switch activation is shown in display #4 (in milliseconds). If a solenoid is activating a switch, try cleaning and re-gapping that switch. Inspect the stiff metal blade between the two connecting blades to ensure it is situated properly, holding the blades apart and not shorting them. If the problem is persistent you may need to replace the switch. Press the secondary switch twice to clear the displays.

Note that, some solenoids are supposed to activate switches, and this is not an issue! For example, solenoids that pull down drop targets will activate the switch on that drop target.

The flippers are enabled throughout the solenoid test. This is therefore a good place to observe flipper issues.

 
##Test 4: Stuck Switch Test

Pressing the self-test again takes you to the switch test. Switches that are stuck on will be identified by number in the displays, like the original test. However, the PTU allows up to four stuck switches to be identified on four displays. The original Bally test displayed only the lowest-numbered stuck switch, making testing of multiple stuck switches and switch-matrix issues difficult. The number of closed switches is also displayed in the Credit display, for cases where more than four switches are closed at once. The switch numbers used by the PTU are the same as in your game manual.

In order to allow testing of the primary, secondary, and end switches, there are no special functions assigned to these buttons during the stuck switch test. The one exception to this is, double-clicking the primary switch will activate any solenoids you need to reset. This allows you to easily test and work with drop target and other tricky switches, and then quickly and easily reset them.

##Detecting Switch Matrix Issues with the Stuck Switch Test

The Stuck Switch test can also be used to locate switch matrix issues. The 40 (or more) switches of a pinball are wired together in an 8x5 grid. Diodes on each switch make sure one switch closing cannot affect any other switch, but a bad diode can cause problems. If a closed switch has a bad diode, and another switch in the same row is closed, and another in the same column is closed, then a fourth switch at the opposite intersection of the row and column will also register as closed.

Testing for switch matrix issues:

1.	Fix all stuck switches. Make sure all switches are open. All four displays should be blank.
2.	Test that all switches are working correctly. Make sure you know where they all are. Note that the switch matrix diagram in your schematics may be inaccurate. Note any errors.
3.	Start with switch 0. Close the switch, and hold it closed.
4.	Choose any other switch in the same row (refer to the switch matrix chart in your schematics). Close the switch, and hold it closed.
5.	Choose any other switch in the same column. Close the switch, and hold it closed.
6.	Three displays should show the three switches you are holding closed. If a fourth display indicates another switch, then switch zero has a bad diode and is causing a switch matrix error. (Note, the coin slot switches do not have diodes and should register as causing a switch matrix error.)
7.	If there are two or more switches with the same number, be sure to test them all by opening the one you are holding, and closing the next one.
8.	Open all the switches. Proceed to test switch 1, then 2, and every switch in sequence.

##Test 5: Switch Bounce (Double-Hit) Test

Pressing the self-test button again takes you to the switch bounce test. Switches on your pinball machine may develop a “bounce”, where hitting them registers two or more hits. If you suspect this may be happening with a switch on your machine, this test can help you to identify the issue. 

To determine whether a switch is bouncing, activate the suspected switch with a pinball. If it registers only once, the switch number will appear in the Player 1 display, and all other displays will be blank. If it registers two or more times, the time between hits will appear in the Player 2 display (measured in milliseconds). If a switch is bouncing, try cleaning and re-gapping that switch. If the problem is persistent you may need to replace the switch. 

##Test 6: Sound Test

Pressing self-test again takes you to the sound test. The original Bally test simply played a single sound. The PTU cycles through all the sounds. Pressing the primary switch plays the current sound repeatedly. Pressing it again will continue cycling sounds. 

Display #1 will indicate the sound number to be played. If the primary switch is pressed within one half second of the display changing, the current sound will be skipped. Holding the button will increase speed, skipping sounds.

Be aware, each sound will take five seconds before proceeding to the next. This is a bit slow, but it generally prevents the sounds from running over each other. Also, some sounds have odd effects, such as disallowing other sounds to begin until they have finished. In some cases, background sounds may play continuously for the rest of the testing.

Testing and reviewing the sounds can be challenging. If a sound is making it difficult to test subsequent sounds, try the “skip” feature.

The sound test currently only works with Bally Squawk & Talk boards, or their equivalents, such as the Geeteoh replacement boards, or a WAV Trigger board. It also does not work with the very early Bally Sound Module boards, or their Geeteoh replacements, on games like the 1979 Bally Star Trek. Later releases may be able to expand on this.
 
##Test 7A: DIP Switch Test for 7-Digit Displays

The DIP switch test takes advantage of the fact that many pinballs have four 7-digit displays, plus four digits in the credit and ball-in-play displays, for a total of 32 digits. This allows the PTU to display the values of the pinball’s 32 DIP switches. This also means that if the pinball uses 6-digit displays, there are only 4 x 6 + 4 = 28 digits to work with. If your pinball has only six-digit displays, go to the next section labeled “Test 7B”.

Pressing self-test again takes you to the DIP switch test. This completely new test shows you the setting of all 32 DIP switches, and allows you to change them temporarily. Turning the machine off and on again restores the DIP switches to the settings on the MPU board. 

All 32 DIP switches are shown in the 32 display digits as either 1 (ON) or 0 (OFF). Since the displays are only seven digits, the first seven of each bank of eight are shown in the four displays. The eighth digit of each bank is shown in the Ball-In-Play or the Credit window. The current switch is identified by a flashing number. 

By pressing the primary switch, you can scroll through switches 1 to 32. Stop on a switch and you can use the secondary switch to change its setting temporarily. 

This can be useful to detect defective DIP switches, or just to review the DIP settings without having to open the backbox.
 
##Test 7B: DIP Switch Test for 6-Digit Displays

Pressing self-test again takes you to the DIP switch test. This completely new test shows you the setting of all 32 DIP switches, and allows you to change them temporarily. Turning the machine off and on again restores the DIP switches to the settings on the MPU board. 

The first six DIP switches for DIP banks 1, 2, 3, and 4 are shown in displays 1, 2, 3, and 4, as either 1 (ON) or 0 (OFF). The last two DIP switches for the current DIP bank are shown in the two digits of the credit display. The current DIP switch of the current DIP bank will always flash. By pressing the primary switch, you can scroll through the 32 DIP switches, moving through all four banks, and the credit display will be updated with the final two digits of each DIP bank as it becomes the “current” DIP bank.

Stop on any switch and you can use the secondary switch to change its setting temporarily. 

This can be useful to detect defective DIP switches, or just to review the DIP settings without having to open the backbox.
