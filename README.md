# DHM - Dominus Horologium MIDI
Master Clock for MIDI, USB MIDI, CV Sync, and Sync Start for ZOOM LiveTrak L-12, ZOOM R8/R24.

<br/>Breadboard Prototype with 1 CV out, 1 MIDI OUT, and 6.3mm start/stop sync out for the L-12 Foot Switch jack.
<img src="https://github.com/CrowStudio/DHM/blob/main/media/prototype_breadboard.jpg" alt="breadboard prototype 1 CV out, 1 MIDI OUT" width="450">

Features:<br/>
MIDI 
 - 1x MIDI (DIN) in/through.
 - 1x USB MIDI in/through.
 - 4x MIDI (DIN) out.
 - 1x USB MIDI out.

CV
 - 2x CV Sync (3.5 mm mini jack) out.
 
 Start/Stop sync for Recording gear
 - 1x "Foot Switch" (6.3mm jack) out.
 
OLED display, 1 Rotary Encoder with Push Button, and 2 separate Push Buttons.<br/>
<img src="https://github.com/CrowStudio/DHM/blob/main/media/Main_screen.jpg" alt="closeup on display" width="450">

Rotary Enconder
 - Adjust BPM 10-300 with +/- 1 increments.<br/>

Rotary Encoder Push Button (Menu mode)
 - Individual PPQN Resolution for CV Sync jack: 1, 2, 4, or 24 PPQN.<br/>
 
Alt Button
 - Adjust Latency Compensation 0-1000 ms with +/- 10 increments, when in Menu mode.

Start/Stop Button
 - Send Start/Stop to MIDI (DIN) and/or MIDI USB, and Start/Stop signal to Foot Switch jack for LiveTrak L-12, or ZOOM R8/R24.
  

## To do list
- Verify individual PPQN Resolution for CV Sync jack.
- Add speed-up for Rotary Encoder.
  - Replace with +/- 1 per step at slow speed and icrease when truning faster.
- Create Menu structure for Rotary Encoder Push Button.<br/>
  Select option with Rotary Encoder, edit with Alt(Select) Button, change value with Rotary Encoder, and press Alt(Select) Button to save/exit
  - Individual PPQN Resolution for CV Sync jack.
  - Individual Tempo Adjustment for CV Sync jack: x1/4, x1/2, x2, and x4.
  - Adjust Latency Compensation.
  - MIDI through ON/OFF.
- Implement save values to SD.
  - Replace save values to EEPROM. 
