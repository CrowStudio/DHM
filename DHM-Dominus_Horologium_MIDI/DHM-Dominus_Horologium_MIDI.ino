/* DHM - Dominus Horologium MIDIremoved
 * Master Clock for MIDI, USB MIDI, Pocket Operators, Volcas, and sync start for LiveTrak L12.
 * https://github.com/CrowStudio/DHM
 * 
 *
 * Inspired by the two following projects:
 *
 * arduino-midi-sync 
 * MIDI master clock/sync/divider for MIDI instruments.
 * https://github.com/ejlabs/arduino-midi-sync 
 * 
 * uClock
 * BPM clock generator for Arduino and Teensy.
 * https://github.com/midilab/uClock 
 * 
 *
 *
 *******************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *******************************************************************************
 */

#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Bounce2.h>
#include <Encoder.h>
#include <MIDI.h>
#include <uClock.h>


#define LED1_OUTPUT 13 // Tempo LED

#define CV1_SYNC_OUTPUT 26 // 1st CV Sync Pin
#define CV2_SYNC_OUTPUT 27 // 2nd CV Sync Pin
#define CONTROL_OUTPUT 28 // Start/Stopp Sync for L-12/R8/R24 Control IN jack

#define BUTTON_ROTARY_INPUT 10 // Rotary Encoder Button
#define BUTTON_START_INPUT 9 // Start/Stop Push Button
#define BUTTON_ALT_INPUT 24 // Alt Push Button

#define OLED_RESET_INPUT 4

#define MINIMUM_BPM 20
#define MAXIMUM_BPM 300


int oldPosition,
    newPosition;
    
byte i,
     p;

uint8_t bpm_blink_timer = 1,
        cv_pulse_timer = 1;

long bpm,
     CV1SyncPPQN = 1,
     CV2SyncPPQNDisplay,
     CV2SyncPPQN,
     compensation;

unsigned long FSSyncTime = 100,
              syncPulse,
              currentTime;

boolean display_update = false,
        playing = false,
        bpm_editing = !false,
        inc_dec = false,
        offSet = false;


Encoder myEnc(29, 30); // Rotary Encoder Pin 29,30 

Bounce bRotary = Bounce(); // Instantiate Bounce objects
Bounce bStart = Bounce(); 
Bounce bAlt = Bounce();

Adafruit_SSD1306 display(OLED_RESET_INPUT);

MIDI_CREATE_DEFAULT_INSTANCE();


void setup(void) {
  MIDI.begin(); // MIDI init
  MIDI.turnThruOff();


  bpm = EEPROMReadInt(0);
  if (bpm > MAXIMUM_BPM || bpm < MINIMUM_BPM) {
    bpm = 120;
  }
  CV2SyncPPQN = EEPROMReadInt(3);
  if (CV2SyncPPQN > 48 || CV2SyncPPQN < 1) {
    CV2SyncPPQN = 12;
  }
  compensation = EEPROMReadInt(6);
  if (compensation > 1000 || compensation < 0) {
    compensation = 0;
  }

  
  uClock.setDrift(11); // compensate latency - setDrift = 1 for USB Teensy, setDrift = 11 for MIDI
  uClock.init(); // Inits the clock
  uClock.setClock96PPQNOutput(ClockOut96PPQN); // Set the callback function for the clock output to send MIDI Sync message.
  uClock.setOnClockStartOutput(onClockStart);  // Set the callback function for MIDI Start and Stop messages.
  uClock.setOnClockStopOutput(onClockStop);
  uClock.setTempo(bpm); // Set the clock BPM to last value
  // uClock.start(); // Starts the clock, tick-tac-tick-tac...
  

  bRotary.attach(BUTTON_ROTARY_INPUT,INPUT_PULLUP); // Attach the debouncer to BUTTON_ROTARY_INPUT_INPUT pin with INPUT_PULLUP mode
  bRotary.interval(5); // Use a debounce interval of 5 milliseconds
  
  bStart.attach(BUTTON_START_INPUT,INPUT_PULLUP); // Attach the debouncer to BUTTON_START_INPUT pin with INPUT_PULLUP mode
  bStart.interval(5); // Use a debounce interval of 5 milliseconds

  bAlt.attach(BUTTON_ALT_INPUT,INPUT_PULLUP); // Attach the debouncer to BUTTON_ALT_INPUT pin with INPUT_PULLUP mode
  bAlt.interval(5); // Use a debounce interval of 5 milliseconds

  pinMode(OLED_RESET_INPUT,INPUT);
  
  pinMode(LED1_OUTPUT,OUTPUT);
  pinMode(CV1_SYNC_OUTPUT,OUTPUT);
  pinMode(CV2_SYNC_OUTPUT,OUTPUT);
  pinMode(CONTROL_OUTPUT,OUTPUT);
  

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);  
  display.setTextSize(2);
  if (bpm >= 100) {
  display.setCursor(0,8);
  display.print(bpm);  
  }
  else if(bpm < 100) {
  display.setCursor(12,8);
  display.print(bpm); 
  }
  display.setCursor(45,8);
  display.print("BPM");
  display.display();
}


// BPM led indicator
void bpmLed(uint32_t * tick) {
  if ( !(*tick % 96) || (*tick == 1) ) {  // start of bar led will flash longer
    bpm_blink_timer = 4;
    digitalWrite(LED1_OUTPUT, HIGH);
  } else if ( !(*tick % 24) ) {   // each quarter pulse led on 
    digitalWrite(LED1_OUTPUT, HIGH);
    bpm_blink_timer = 1;
  } else if ( !(*tick % bpm_blink_timer) ) { // get led off
    digitalWrite(LED1_OUTPUT, LOW);
  }
}

// CV1 sync pulse
void CV1SyncPulse(uint32_t * tick) {
  if ( !(*tick % 96) || (*tick == 1) ) {   // start of bar CV1_SYNC_OUTPUT HIGH
    digitalWrite(CV1_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % CV1SyncPPQN) ) {   // pulses per quarter note CV1_SYNC_OUTPUT HIGH
    digitalWrite(CV1_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % cv_pulse_timer) ) { //  CV1_SYNC_OUTPUT LOW
    digitalWrite(CV1_SYNC_OUTPUT, LOW);
  }
}

// CV2 sync pulse
void CV2SyncPulse(uint32_t * tick) {
  if ( !(*tick % 96) || (*tick == 1) ) {   //  start of bar CV2_SYNC_OUTPUT HIGH
    digitalWrite(CV2_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % CV2SyncPPQN) ) {   // pulses per quarter note CV2_SYNC_OUTPUT HIGH
    digitalWrite(CV2_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % cv_pulse_timer) ) { //  CV2_SYNC_OUTPUT LOW
    digitalWrite(CV2_SYNC_OUTPUT, LOW);
  }
}

// The callback function wich will be called by uClock each Pulse of 96PPQN clock resolution.
void ClockOut96PPQN(uint32_t * tick) {
  // Send MIDI_CLOCK to external gears
  MIDI.sendRealTime(midi::Clock);
  usbMIDI.sendRealTime(usbMIDI.Clock);
  CV1SyncPulse(tick);
  CV2SyncPulse(tick);
  bpmLed(tick);
}

// The callback function wich will be called when clock starts by using uClock.start() method.
void onClockStart() {
  MIDI.sendRealTime(midi::Start);
  usbMIDI.sendRealTime(usbMIDI.Start);
}


// The callback function wich will be called when clock stops by using uClock.stop() method.
void onClockStop() {
  MIDI.sendRealTime(midi::Stop);
  usbMIDI.sendRealTime(usbMIDI.Stop);
  all_off();
}

void detectButtonPress() {
  i = 0;
  p = 0;

  bRotary.update();
  bStart.update();
  bAlt.update();
  
  if (bRotary.fell()) {
    p = 1;
  } else if (bStart.fell()) {
    playing = !playing;
    if (playing) {
      syncPulse = millis();
      digitalWrite(CONTROL_OUTPUT, HIGH);
      delay(compensation);
      uClock.start();
    } else {
      uClock.stop();
      syncPulse = millis();
      digitalWrite(CONTROL_OUTPUT, HIGH);
    }
  } else if (bAlt.fell()) {
    if (bpm_editing && !offSet) {
      inc_dec = !inc_dec;
    } else if (!bpm_editing) {
      offSet = !offSet;
    }
  }
}

int rotaryReadout() {
  newPosition = (myEnc.read()/4);
  if (newPosition != oldPosition) {    
    if (oldPosition < newPosition) {
      i = 2;
    } else if (oldPosition > newPosition) {
      i = 1;
    }
    oldPosition = newPosition;
  }
  return i;
}

void EEPROMWriteInt(int p_address, int p_value) {
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);
  
  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}

unsigned int EEPROMReadInt(int p_address) {
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);
  
  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

void detailedTimer() {
  display.setTextSize(1);
  display.setCursor(0,0);  
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Play Time: ");
  display.setCursor(64, 0);
  display.println(uClock.getNumberOfMinutes(uClock.getPlayTime()));
  display.setCursor(84, 0);
  display.println("m");
  display.setCursor(96, 0);
  display.println(uClock.getNumberOfSeconds(uClock.getPlayTime()));  
  display.setCursor(114, 0);
  display.println("s");
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.println(bpm);
  display.setCursor(40, 15);
  display.println("BPM");
  display.setTextSize(1);
  display.setCursor(86, 14);
  display.println("2nd CV");
  if (CV2SyncPPQNDisplay == 12) {
    display.setCursor(82, 24);
    display.println(CV2SyncPPQNDisplay);
    display.setCursor(100, 24);
    display.println("PPQN");
  } else if (CV2SyncPPQNDisplay <= 8 && CV2SyncPPQNDisplay > 0) {
    display.setCursor(86, 24);
    display.println(CV2SyncPPQNDisplay);
    display.setCursor(92, 24);
    display.println(" PPQN");
  } else if (CV2SyncPPQNDisplay == 0) {
    display.setCursor(80, 24);
    display.println("0.5");
    display.setCursor(98, 24);
    display.println(" PPQN");
  }
  
//  display.setTextSize(1);
//  display.setCursor(0, 38);
//  display.println("Clock Division:");
//  display.setCursor(92, 38);
//  display.println("None");
//  display.setCursor(0, 50);
//  display.println("Device: ");
//  display.setCursor(44, 50);
//  display.println("LiveTrak L-12");
  display.display();
  // display_update = false;
}

void sync_display() {
  //EEPROMWriteInt(3,CV2SyncPPQN);
  display.setTextSize(2);
  display.setCursor(0,8);
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();  
  display.setCursor(0,8);
  display.print("PPQN:");
  if (CV2SyncPPQNDisplay == 48) {
    display.setCursor(64,8);
    display.print(CV2SyncPPQNDisplay);  
  } else if(CV2SyncPPQNDisplay < 48 && CV2SyncPPQNDisplay >= 1 ) {
    display.setCursor(76,8);
    display.print(CV2SyncPPQNDisplay); 
  } else if(CV2SyncPPQNDisplay == 0 ) {
    display.setCursor(76,8);
    display.print("0.5"); 
  }
  display.display();
}

void offset_display() {
  EEPROMWriteInt(6,compensation);
  display.setTextSize(1);
  display.setCursor(0,12);  
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();
  if (compensation == 1000) {
    display.setCursor(0,12);
    display.print(compensation);  
  } else if (compensation >= 100 && compensation < 1000) {
    display.setCursor(6,12);
    display.print(compensation);  
  } else if(compensation > 0 && compensation < 100) {
    display.setCursor(12,12);
    display.print(compensation); 
  } else if(compensation == 0) {
    display.setCursor(18,12);
    display.print(compensation); 
  }
  display.setCursor(28,12);
  display.print("ms  Start Offset");
  display.display();
  // display_update = false;
}

void setDisplayPPQN() {
  if (CV2SyncPPQN == 2) { CV2SyncPPQNDisplay = 12; }
  else if (CV2SyncPPQN == 3) { CV2SyncPPQNDisplay = 8; }
  else if (CV2SyncPPQN == 6) { CV2SyncPPQNDisplay = 4; }
  else if (CV2SyncPPQN == 12) { CV2SyncPPQNDisplay = 2; }
  else if (CV2SyncPPQN == 24) { CV2SyncPPQNDisplay = 1; }
  else if (CV2SyncPPQN == 48) { CV2SyncPPQNDisplay = 0; } // 0.5 PPQN
}

void editDisplay(int i, int p) {
  if (bpm_editing) {
    setDisplayPPQN();
    detailedTimer();
    if (i == 2 ) {
      bpm = bpm + 1;
      if (bpm > MAXIMUM_BPM) {
        bpm = MAXIMUM_BPM;
      }
      uClock.setTempo(bpm);
      detailedTimer();          
    } else if (i == 1) {
      bpm = bpm - 1;
      if (bpm < MINIMUM_BPM) {
        bpm = MINIMUM_BPM;
      }
        uClock.setTempo(bpm);
        detailedTimer();
    } else if (p == 1) {
      sync_display();
      bpm_editing = false;
    }
  } else if (p == 1) {      
        detailedTimer();
        offSet = false;
        bpm_editing = true;
      } else if (!offSet && i == 2) {      
        if (CV2SyncPPQN == 48) { CV2SyncPPQN = 24; }
        else if (CV2SyncPPQN == 24) { CV2SyncPPQN = 12; }
        else if (CV2SyncPPQN == 12) { CV2SyncPPQN = 6; }
        else if (CV2SyncPPQN == 6) { CV2SyncPPQN = 3; }
        else if (CV2SyncPPQN < 2 || CV2SyncPPQN == 3) { CV2SyncPPQN = 2; }
        setDisplayPPQN();
        sync_display();
      } else if (!offSet && i == 1) {      
        if (CV2SyncPPQN > 48 || CV2SyncPPQN == 2) { CV2SyncPPQN = 3; }
        else if (CV2SyncPPQN == 3) { CV2SyncPPQN = 6; }
        else if (CV2SyncPPQN == 6) { CV2SyncPPQN = 12; }
        else if  (CV2SyncPPQN == 12) { CV2SyncPPQN = 24; }
        else if  (CV2SyncPPQN == 24) { CV2SyncPPQN = 48; }
        setDisplayPPQN();
        sync_display();
      } else if (offSet) {
        offset_display();
         if (i == 2 ) {
          compensation = compensation + 10; 
          if (compensation > 1000) {
            compensation = 1000;
          }
          offset_display();          
        } else if (i == 1) {
            compensation = compensation - 10;
            if (compensation < 0) {
              compensation = 0;
            }
            offset_display();
        }
      } else {
        sync_display();
      }
}

  
void all_off() { // make sure all sync, led pin stat to low
  digitalWrite(CV1_SYNC_OUTPUT, LOW);
  digitalWrite(CV2_SYNC_OUTPUT, LOW);
  digitalWrite(CONTROL_OUTPUT, LOW);
  digitalWrite(LED1_OUTPUT, LOW);
}


void loop(void) {
  detectButtonPress();
  currentTime = millis();
  if(currentTime - syncPulse >= FSSyncTime) {digitalWrite(CONTROL_OUTPUT, LOW);}

  i = rotaryReadout();

  editDisplay(i,p);  

  while (usbMIDI.read());
}
