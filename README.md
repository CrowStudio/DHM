# DHM - Dominus Horologium MIDI
Master Clock for MIDI DIN/USB, CV Sync, and Sync Start for ZOOM LiveTrak L-12/L-20/L-20R, or ZOOM R8/R24.

<br/>Breadboard Prototype with 1 CV Sync out, 1 MIDI DIN out, 1 MIDI USB out, and 6.3mm Start/Stop Pulse for the L-12 Control In jack.
<img src="https://github.com/CrowStudio/DHM/blob/main/media/prototype_breadboard.jpg" alt="breadboard prototype" width="450">

Features:<br/>
MIDI
 - 1x MIDI DIN out.
 - 1x USB MIDI out.

CV
 - 1x CV(2) Sync (3.5 mm mini TRS jack) out.
 
 Start/Stop Pulse for Recording gear.
 - 1x Control (6.3 mm TR jack) out.
 
OLED display.<br/>
<img src="https://github.com/CrowStudio/DHM/blob/main/media/Main_screen.jpg" alt="closeup on display" width="450">

Rotary Enconder
 - Adjust BPM 20-300 with +/- 1 increments.<br/>

Rotary Encoder Push Button (Menu mode)
 - Individual PPQN Resolution for CV(2) Sync jack: 0.5, 1, 2, 4, 8, or 12 PPQN.<br/>
 
Alt Button
 - Adjust Latency Compensation for MIDI start/stop, 0-1000 ms with +/- 5 increments when in Menu mode.

Start/Stop Button
 - Send Start/Stop Pulse for the Control In jack on LiveTrak L-12 (or L-20/L-20R,ZOOM R8/R24), and Start/Stop for MIDI DIN/USB, CV Sync.
  

## To do list
HARDWARE
 - Add 1x MIDI DIN in/through.
 - Add 3x MIDI DIN out.
 - Add 1x CV(1) Sync (3.5 mm mini TRS jack) out.<br/>

FIRMWARE
- Add speed-up for Rotary Encoder.
  - Replace with +/- 1 per step at slow speed and icrease when truning faster.
- Create Menu structure for Rotary Encoder Push Button.<br/>
  Select option with Rotary Encoder, edit with Alt(Select) Button, change value with Rotary Encoder, and press Alt(Select) Button to save/exit
  - Add individual PPQN Resolution for CV(1) Sync.
  - MIDI through ON/OFF.
- Implement save values to SD.
  - Replace save values to EEPROM.

