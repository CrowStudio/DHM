# DHM - Dominus Horologium MIDI
Master Clock for MIDI, USB MIDI, CV Sync, and Sync Start for ZOOM LiveTrak L-12, ZOOM R8/R24.

<br/>Breadboard Prototype with 1 CV out, 1 MIDI OUT, and 6.3mm Start/Stop Pulse out for the L-12 Control In jack.
<img src="https://github.com/CrowStudio/DHM/blob/main/media/prototype_breadboard.jpg" alt="breadboard prototype" width="450">

Features:<br/>
MIDI 
 - 1x MIDI (DIN) in/through.
 - 1x USB MIDI in/through.
 - 4x MIDI (DIN) out.
 - 1x USB MIDI out.

CV
 - 2x CV Sync (3.5 mm mini TRS jack) out.
 
 Start/Stop Pulse to Recording gear.
 - 1x Control (6.3 mm TRS jack) out.
 
OLED display.<br/>
<img src="https://github.com/CrowStudio/DHM/blob/main/media/Main_screen.jpg" alt="closeup on display" width="450">

Rotary Enconder
 - Adjust BPM 20-300 with +/- 1 increments.<br/>

Rotary Encoder Push Button (Menu mode)
 - Individual PPQN Resolution for CV Sync jack: 0.5, 1, 2, 4, 8, or 12 PPQN.<br/>
 
Alt Button
 - Adjust Latency Compensation 0-1000 ms with +/- 10 increments, when in Menu mode.

Start/Stop Button
 - Send Start/Stop to MIDI (DIN) and/or MIDI USB, and Start/Stop Pulse to Control In jack for LiveTrak L-12, or ZOOM R8/R24.
  

## To do list
- Add speed-up for Rotary Encoder.
  - Replace with +/- 1 per step at slow speed and icrease when truning faster.
- Create Menu structure for Rotary Encoder Push Button.<br/>
  Select option with Rotary Encoder, edit with Alt(Select) Button, change value with Rotary Encoder, and press Alt(Select) Button to save/exit
  - Individual PPQN Resolution for CV1 and CV2 Sync jack.
  - Adjust Latency Compensation.
  - MIDI through ON/OFF.
- Implement save values to SD.
  - Replace save values to EEPROM. 
