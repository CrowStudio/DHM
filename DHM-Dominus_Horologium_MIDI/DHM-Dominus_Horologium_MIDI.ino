
/* DHM - Dominus Horologium MIDI
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

#define AUDIO_SYNC_OUTPUT 23 // Audio Sync Digital Pin
#define AUDIO_SYNC_PPQN_OUTPUT 22 // 2nd Audio Sync Pin

#define BUTTON_ROTARY_INPUT 10 // Rotary Encoder Button
#define BUTTON_START_INPUT 9 // Start/Stop Push Button
#define BUTTON_ALT_INPUT 24 // Alt Push Button

#define OLED_RESET_INPUT 4

#define BLINK_TIME 1 // LED blink time

#define AUDIO_SYNC 12 // Audio Sync Ticks
#define AUDIO_SYNC_PPQN 12 // 2nd Audio Sync Ticks

#define MINIMUM_BPM 20
#define MAXIMUM_BPM 300


int oldPosition;

uint8_t bpm_blink_timer = 1,
        PPQN_blink_timer = 1,
        audio_pulse_timer = 1,
        audio_PPQN_pulse_timer = 1;

long intervalMicroSeconds,
     bpm,
     audio_syncPPQN,
     compensation;

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
  audio_syncPPQN = EEPROMReadInt(3);
  if (audio_syncPPQN > 64 || audio_syncPPQN < 2) {
    audio_syncPPQN = 12;
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
  pinMode(AUDIO_SYNC_OUTPUT,OUTPUT);
  pinMode(AUDIO_SYNC_PPQN_OUTPUT,OUTPUT);

  
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
  if ( !(*tick % (96)) || (*tick == 1) ) {  // first quater pulse led will flash longer
    bpm_blink_timer = 4;
    digitalWrite(LED1_OUTPUT, HIGH);
  } else if ( !(*tick % (24)) ) {   // each quarter pulse led on 
    digitalWrite(LED1_OUTPUT, HIGH);
  } else if ( !(*tick % bpm_blink_timer) ) { // get led off
    digitalWrite(LED1_OUTPUT, LOW);
    bpm_blink_timer = 1;
  }
}

// CV sync pulse
void audioSyncPulse(uint32_t * tick) {
  if ( !(*tick % (24)) ) {   // each quarter pulse AUDIO_SYNC_OUTPUT HIGH
    digitalWrite(AUDIO_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % audio_pulse_timer) ) { //  AUDIO_SYNC_OUTPUT LOW
    digitalWrite(AUDIO_SYNC_OUTPUT, LOW);
  }
}

// CV sync pulse PPQN led indicator
void ppqnLed(uint32_t * tick) {
  if ( !(*tick % (96 + ((audio_syncPPQN - 12) * 4))) || (*tick == 1) ) {  // first quater pulse led will flash longer
    digitalWrite(LED1_OUTPUT, HIGH);
    PPQN_blink_timer = 4;
  } else if ( !(*tick % (24 + (audio_syncPPQN - 12))) ) {   // each quarter pulse led on
    digitalWrite(LED1_OUTPUT, HIGH);
  } else if ( !(*tick % PPQN_blink_timer) ) { // get led off
    digitalWrite(LED1_OUTPUT, LOW);
    PPQN_blink_timer = 1;
  }
}

// CV sync pulse PPQN
void audioSyncPpqnPulse(uint32_t * tick) {
  if ( !(*tick % (24 + (audio_syncPPQN - 12))) ) {   // each quarter pulse AUDIO_SYNC_OUTPUT HIGH
    digitalWrite(AUDIO_SYNC_OUTPUT, HIGH);
  } else if ( !(*tick % audio_pulse_timer) ) { //  AUDIO_SYNC_OUTPUT LOW
    digitalWrite(AUDIO_SYNC_OUTPUT, LOW);
    audio_pulse_timer = 1;
  }
}

// The callback function wich will be called by uClock each Pulse of 96PPQN clock resolution.
void ClockOut96PPQN(uint32_t * tick) {
  // Send MIDI_CLOCK to external gears
  MIDI.sendRealTime(midi::Clock);
  usbMIDI.sendRealTime(usbMIDI.Clock);
  audioSyncPulse(tick);
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

void EEPROMWriteInt(int p_address, int p_value)
     {
     byte lowByte = ((p_value >> 0) & 0xFF);
     byte highByte = ((p_value >> 8) & 0xFF);

     EEPROM.write(p_address, lowByte);
     EEPROM.write(p_address + 1, highByte);
     }

unsigned int EEPROMReadInt(int p_address)
     {
     byte lowByte = EEPROM.read(p_address);
     byte highByte = EEPROM.read(p_address + 1);

     return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

void bpm_display() { 
  EEPROMWriteInt(0,bpm);  
  display.setTextSize(2);
  display.setCursor(0,8);  
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();
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
  display_update = false;
}

void bpm_dec_display() {
  EEPROMWriteInt(0,bpm);  
  display.setTextSize(2);
  display.setCursor(0,8);  
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();
  if (bpm >= 100) {
  display.setCursor(0,8);
  display.print(bpm);  
  }
  else if(bpm < 100) {
  display.setCursor(12,8);
  display.print(bpm); 
  }
  display.setCursor(45,8);
  display.print("BPM *");
  display.display();
  display_update = false;
}

void sync_display() {
  EEPROMWriteInt(3,audio_syncPPQN);
  
  int sync_current;
  sync_current = audio_syncPPQN - 12;  
  
  if (sync_current < 0) {    
    sync_current = abs(sync_current);
  } else if (sync_current > 0) {
    sync_current = -sync_current;
  }
    
  display.setTextSize(2);
  display.setCursor(0,8);
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();  
  display.setCursor(0,8);
  display.print("PPQN:");
  if (sync_current >= 10) {
  display.setCursor(64,8);
  display.print(sync_current);  
  }
  else if(sync_current < 10 && sync_current >= 0 ) {
  display.setCursor(76,8);
  display.print(sync_current); 
  }
  else if(sync_current < 0 ) {
  display.setCursor(64,8);
  display.print(sync_current); 
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
  }
  if (compensation >= 100 && compensation < 1000) {
  display.setCursor(6,12);
  display.print(compensation);  
  }
  else if(compensation > 0 && compensation < 100) {
  display.setCursor(12,12);
  display.print(compensation); 
  }
  else if(compensation == 0) {
  display.setCursor(18,12);
  display.print(compensation); 
  }
  display.setCursor(28,12);
  display.print("ms  Start Offset");
  display.display();
  // display_update = false;
}

void editDisplay(byte i, byte p) {
  
  if (bpm_editing && inc_dec) {
    bpm_dec_display();
    if (i == 2 ) {
      bpm = bpm + 10;
      if (bpm > MAXIMUM_BPM) {
        bpm = MAXIMUM_BPM;
      }
      uClock.setTempo(bpm);
      bpm_dec_display();          
    } else if (i == 1) {
      bpm = bpm - 10;
      if (bpm < MINIMUM_BPM) {
        bpm = MINIMUM_BPM;
      }
        uClock.setTempo(bpm);
        bpm_dec_display();
      }
    
    else if (p == 1) {
      sync_display();
      bpm_editing = false;
    }
   
  } else if (bpm_editing && !inc_dec) {
      bpm_display();
      if (i == 2) {
        bpm++;
        if (bpm > MAXIMUM_BPM) {
          bpm = MAXIMUM_BPM;
        }
        uClock.setTempo(bpm);
        bpm_display();        
      } else if (i == 1) {
        bpm--;
        if (bpm < MINIMUM_BPM) {
          bpm = MINIMUM_BPM;
        }
        uClock.setTempo(bpm);
        bpm_display();
      }
    
    else if (p == 1) {
      sync_display();
      bpm_editing = false;
    }
    
  } else { // 2nd jack audio sync speed
      if (p == 1) {      
        bpm_display();
        offSet = false;
        bpm_editing = true;
      } else if (!offSet && i == 1) {      
        audio_syncPPQN++;
        if (audio_syncPPQN > 64) { audio_syncPPQN = 64; }
        sync_display();
      } else if (!offSet && i == 2) {
        audio_syncPPQN--;
        if (audio_syncPPQN < 2) { audio_syncPPQN = 2; }
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
}

void all_off() { // make sure all sync, led pin stat to low
  digitalWrite(AUDIO_SYNC_OUTPUT, LOW);
  digitalWrite(AUDIO_SYNC_PPQN_OUTPUT, LOW);
  digitalWrite(LED1_OUTPUT, LOW);
}


void loop(void) {
  byte i = 0;
  byte p = 0;

  bRotary.update();
  bStart.update();
  bAlt.update();

  if (bRotary.fell()) {
    p = 1;
  } else if (bStart.fell()) {
    playing = !playing;
    if (playing) {
      delay(compensation);
      uClock.start();
    } else {
      uClock.stop();
    }
  } else if (bAlt.fell()) {
    if (bpm_editing && !offSet) {
      inc_dec = !inc_dec;
    } else if (!bpm_editing) {
      offSet = !offSet;
    }
  }

  int newPosition = (myEnc.read()/4);
  if (newPosition != oldPosition) {    
    if (oldPosition < newPosition) {
      i = 2;
    } else if (oldPosition > newPosition) {
      i = 1;
    }
    oldPosition = newPosition;
  }

  editDisplay(i, p);
  
  while (usbMIDI.read());
}
