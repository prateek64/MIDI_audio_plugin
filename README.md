# MIDI_audio_plugin - Drum Kit 

In this week's project I have developed a MIDI plugin using JUCE. I have developed a drum kit along with a dj touch to it. My audio plugin (VST3)
has the following components :- 

1. A digital midi keyboard which can be played on the computer screen or can be controlled using an external midi keyboard.
2. Echo Delay Knob - Echo knob lets you add echo to you midi notes (Maximum delay is the sampling rate of the plugin). 
3. Pitch Shifting - Pitch Shifting knob lets you change the pitch of the midi notes by a certain factor ( Maximum is a factor of 5). 
4. Eight Pads to play certain drum sounds.

Here I have used the drummer kit from the STK and modified it with my own sounds and a MIDI map. The echo and the pitch shifiting is also done
using the STK Echo and PitShift classes. 

The knobs ,keyboard and the pads on the plugin react to the external knobs and plugin. 

AddOn: You get to scratch your own DJ disc( feels better if its a touch screen). 

Note ( Very Important) : Before running the code, please modify paths for the following :

1. Raw wave files - By changing the path in the PluginProcessor.cpp
2. Image file path - By changing the path in the PluginEditor.h



