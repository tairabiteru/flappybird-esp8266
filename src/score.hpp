/**
 * Module containing score functionality.
 * 
 * This just contains shortcut functions to interact with
 * the ESP8266's EEPROM, or Electronically Erasable Programmable
 * Read Only Memory. The entire program and all of its variables
 * are stored in RAM which is cleared once the machine loses power.
 * For most things, this is fine, but in the case of the highest
 * score, it would be kind of lame if the highest score got
 * deleted whenever the device was unplugged. For this reason,
 * the high score is written to EEPROM, which does not clear
 * when the machine loses power.
 */

#include <EEPROM.h>


/**
 * Shortcut function to read the current high score from EEPROM.
 * 
 * @return An integer corresponding to the current highest score.
*/
int get_high_score() {
    return EEPROM.read(0);
}


/**
 * Shortcut function to write a new high score to EEPROM.
*/
void set_high_score(int score) {
    EEPROM.write(0, score);
    EEPROM.commit();
}