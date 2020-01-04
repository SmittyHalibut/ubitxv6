#include <Arduino.h>
#include <EEPROM.h>
#include "morse.h"
#include "settings.h"
#include "ubitx.h"
#include "nano_gui.h"

/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

void setupExit(){
  menuOn = 0;
}

 //this is used by the si5351 routines in the ubitx_5351 file
extern uint32_t si5351bx_vcoa;

void setupFreq(){
  int knob = 0;
  int32_t prev_calibration;

  displayDialog("Set Frequency", "Push TUNE to Save"); 

  //round off the the nearest khz
  {
    uint32_t freq = GetActiveVfoFreq();
    freq = (freq/1000l)* 1000l;
    setFrequency(freq);
  }

  displayRawText("You should have a", 20, 50, DISPLAY_CYAN, DISPLAY_NAVY);
  displayRawText("signal exactly at ", 20, 75, DISPLAY_CYAN, DISPLAY_NAVY);
  ltoa(GetActiveVfoFreq()/1000l, c, 10);
  strcat(c, " KHz");
  displayRawText(c, 20, 100, DISPLAY_CYAN, DISPLAY_NAVY);

  displayRawText("Rotate to zerobeat", 20, 180, DISPLAY_CYAN, DISPLAY_NAVY);
  //keep clear of any previous button press
  while (btnDown())
    active_delay(100);
  active_delay(100);
   
  prev_calibration = globalSettings.oscillatorCal;
  globalSettings.oscillatorCal = 0;

//  ltoa(calibration/8750, c, 10);
//  strcpy(b, c);
//  strcat(b, "Hz");
//  printLine2(b);

  while (!btnDown())
  {
   knob = enc_read();
   if (knob != 0)
      globalSettings.oscillatorCal += knob * 875;
 /*   else if (knob < 0)
      calibration -= 875; */
    else  
      continue; //don't update the frequency or the display
 
    si5351bx_setfreq(0, globalSettings.usbCarrierFreq);  //set back the cardrier oscillator anyway, cw tx switches it off  
    si5351_set_calibration(globalSettings.oscillatorCal);
    setFrequency(GetActiveVfoFreq());

    //displayRawText("Rotate to zerobeat", 20, 120, DISPLAY_CYAN, DISPLAY_NAVY);
    
    ltoa(globalSettings.oscillatorCal, b, 10);
    displayText(b, 100, 140, 100, 26, DISPLAY_CYAN, DISPLAY_NAVY, DISPLAY_WHITE);
  }

  SaveSettingsToEeprom();
  initOscillators();
  si5351_set_calibration(globalSettings.oscillatorCal);
  setFrequency(GetActiveVfoFreq());

  //debounce and delay
  while(btnDown())
    active_delay(50);
  active_delay(100);
}

void setupBFO(){
  int knob = 0;
  unsigned long prevCarrier;
   
  prevCarrier = globalSettings.usbCarrierFreq;

  displayDialog("Set BFO", "Press TUNE to Save"); 
  
  globalSettings.usbCarrierFreq = 11053000l;
  si5351bx_setfreq(0, globalSettings.usbCarrierFreq);
  printCarrierFreq(globalSettings.usbCarrierFreq);

  while (!btnDown()){
    knob = enc_read();

    if (knob != 0)
      globalSettings.usbCarrierFreq -= 50 * knob;
    else
      continue; //don't update the frequency or the display
      
    si5351bx_setfreq(0, globalSettings.usbCarrierFreq);
    setFrequency(GetActiveVfoFreq());
    printCarrierFreq(globalSettings.usbCarrierFreq);
    
    active_delay(100);
  }

  SaveSettingsToEeprom();
  si5351bx_setfreq(0, globalSettings.usbCarrierFreq);
  setFrequency(GetActiveVfoFreq());
  updateDisplay();
  setupExit();
}

void setupCwDelay(){
  int knob = 0;
  int prev_cw_delay;

  displayDialog("Set CW T/R Delay", "Press tune to Save"); 

  active_delay(500);
  prev_cw_delay = globalSettings.cwActiveTimeoutMs;

  ltoa(globalSettings.cwActiveTimeoutMs, b, 10);
  strcat(b, " msec");
  displayText(b, 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);

  while (!btnDown()){
    knob = enc_read();

    if (knob < 0 && globalSettings.cwActiveTimeoutMs > 100)
      globalSettings.cwActiveTimeoutMs -= 100;
    else if (knob > 0 && globalSettings.cwActiveTimeoutMs < 1000)
      globalSettings.cwActiveTimeoutMs += 100;
    else
      continue; //don't update the frequency or the display

    ltoa(globalSettings.cwActiveTimeoutMs, b, 10);
    strcat(b, " msec");
    displayText(b, 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
      
  }

  SaveSettingsToEeprom();
  active_delay(500);
  setupExit();
}

void setupKeyer(){
  displayDialog("Set CW Keyer", "Press tune to Save"); 

  if(KeyerMode_e::KEYER_STRAIGHT == globalSettings.keyerMode){
    displayText("< Hand Key >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
  }
  else if(KeyerMode_e::KEYER_IAMBIC_A == globalSettings.keyerMode){
    displayText("< Iambic A >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
  }
  else{
    displayText("< Iambic B >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
  }

  int knob = 0;
  uint32_t tmp_mode = globalSettings.keyerMode;
  while (!btnDown())
  {
    knob = enc_read();
    if(knob == 0){
      active_delay(50);
      continue;
    }
    if(knob < 0 && tmp_mode > KeyerMode_e::KEYER_STRAIGHT){
      tmp_mode--;
    }
    if(knob > 0 && tmp_mode < KeyerMode_e::KEYER_IAMBIC_B){
      tmp_mode++;
    }

    if (KeyerMode_e::KEYER_STRAIGHT == tmp_mode){
      displayText("< Hand Key >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
    }
    else if(KeyerMode_e::KEYER_IAMBIC_A == tmp_mode){
      displayText("< Iambic A >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
    }
    else if (KeyerMode_e::KEYER_IAMBIC_B == tmp_mode){
      displayText("< Iambic B >", 100, 100, 120, 26, DISPLAY_CYAN, DISPLAY_BLACK, DISPLAY_BLACK);
    }
  }

  active_delay(500);

  globalSettings.keyerMode = tmp_mode;
  SaveSettingsToEeprom();
  
  setupExit();
}

static const unsigned int COLOR_TEXT = DISPLAY_WHITE;
static const unsigned int COLOR_BACKGROUND = DISPLAY_BLACK;
static const unsigned int COLOR_TITLE_BACKGROUND = DISPLAY_NAVY;
static const unsigned int COLOR_ACTIVE_BORDER = DISPLAY_WHITE;
static const unsigned int COLOR_INACTIVE_BORDER = DISPLAY_DARKGREY;

static const unsigned int LAYOUT_TITLE_X = 10;
static const unsigned int LAYOUT_TITLE_Y = 10;
static const unsigned int LAYOUT_TITLE_WIDTH = 300;
static const unsigned int LAYOUT_TITLE_HEIGHT = 35;

static const unsigned int LAYOUT_ITEM_X = 30;
static const unsigned int LAYOUT_ITEM_Y = LAYOUT_TITLE_Y + LAYOUT_TITLE_HEIGHT + 5;
static const unsigned int LAYOUT_ITEM_WIDTH = 260;
static const unsigned int LAYOUT_ITEM_HEIGHT = 30;
static const unsigned int LAYOUT_ITEM_PITCH_Y = 31;

const char MI_SET_FREQ [] PROGMEM = "Set Freq...";
const char MI_SET_BFO [] PROGMEM = "Set BFO...";
const char MI_CW_DELAY [] PROGMEM = "CW Delay...";
const char MI_CW_KEYER [] PROGMEM = "CW Keyer...";
const char MI_TOUCH [] PROGMEM = "Touch Screen...";
const char MI_EXIT [] PROGMEM = "Exit";

enum MenuIds {
  MENU_SET_FREQ,
  MENU_SET_BFO,
  MENU_CW_DELAY,
  MENU_CW_KEYER,
  MENU_TOUCH,
  MENU_EXIT,
  MENU_TOTAL
};

const char* const menuItems [MENU_TOTAL] PROGMEM {
  MI_SET_FREQ,
  MI_SET_BFO,
  MI_CW_DELAY,
  MI_CW_KEYER,
  MI_TOUCH,
  MI_EXIT
};

void drawSetupMenu(){
  displayClear(COLOR_BACKGROUND);
  strcpy_P(b,(const char*)F("Setup"));
  displayText(b, LAYOUT_TITLE_X, LAYOUT_TITLE_Y, LAYOUT_TITLE_WIDTH, LAYOUT_TITLE_HEIGHT, COLOR_TEXT, COLOR_TITLE_BACKGROUND, COLOR_ACTIVE_BORDER);
  for(unsigned int i = 0; i < MENU_TOTAL; ++i){
    strcpy_P(b,(const char*)pgm_read_word(&(menuItems[i])));
    displayText(b, LAYOUT_ITEM_X, LAYOUT_ITEM_Y + i*LAYOUT_ITEM_PITCH_Y, LAYOUT_ITEM_WIDTH, LAYOUT_ITEM_HEIGHT, COLOR_TEXT, COLOR_BACKGROUND, COLOR_INACTIVE_BORDER);
  }
}

void movePuck(int i){
  static int prevPuck = 1;//Start value at 1 so that on init, when we get called with 0, we'll update

  //Don't update if we're already on the right selection
  if(prevPuck == i){
    return;
  }

  //Clear old
  displayRect(LAYOUT_ITEM_X, LAYOUT_ITEM_Y + (prevPuck*LAYOUT_ITEM_PITCH_Y), LAYOUT_ITEM_WIDTH, LAYOUT_ITEM_HEIGHT, COLOR_INACTIVE_BORDER);
  //Draw new
  displayRect(LAYOUT_ITEM_X, LAYOUT_ITEM_Y + (i*LAYOUT_ITEM_PITCH_Y), LAYOUT_ITEM_WIDTH, LAYOUT_ITEM_HEIGHT, COLOR_ACTIVE_BORDER);
  prevPuck = i;
  
}

void doSetup2(){
  static const unsigned int COUNTS_PER_ITEM = 10;
  int select=0, i, btnState;

  drawSetupMenu();
  movePuck(select);

   //wait for the button to be raised up
  while(btnDown())
    active_delay(50);
  active_delay(50);  //debounce
  
  menuOn = 2;
 
  while (menuOn){
    i = enc_read();

    if (i > 0){
      if (select + i < MENU_TOTAL*COUNTS_PER_ITEM)
        select += i;
      movePuck(select/COUNTS_PER_ITEM);
    }
    if (i < 0 && select + i >= 0){
      select += i;      //caught ya, i is already -ve here, so you add it
      movePuck(select/COUNTS_PER_ITEM);
    }

    if (!btnDown()){
      active_delay(50);
      continue;
    }

    //wait for the touch to lift off and debounce
    while(btnDown()){
      active_delay(50);
    }
    active_delay(300);
    
    switch(select/COUNTS_PER_ITEM){
      case MENU_SET_FREQ:
      {
        setupFreq();
        break;
      }
      case MENU_SET_BFO:
      {
        setupBFO();
        break;
      }
      case MENU_CW_DELAY:
      {
        setupCwDelay();
        break;
      }
      case MENU_CW_KEYER:
      {
        setupKeyer();
        break;
      }
      case MENU_TOUCH:
      {
        setupTouch();
        break;
      }
      case MENU_EXIT:
      default:
      {
        menuOn = 0;
        break;
      }
    }//switch
    //redraw
    drawSetupMenu();
  }

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);

  checkCAT();
  guiUpdate();
}
