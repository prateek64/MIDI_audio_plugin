
#ifndef __PLUGINEDITOR_H_4ACCBAA__
#define __PLUGINEDITOR_H_4ACCBAA__

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"


//==============================================================================
/** This is the editor component that our filter will display.
*/


class JuceDemoPluginAudioProcessorEditor  : public AudioProcessorEditor,
                                            private Timer,
											private Slider::Listener,  
										    private ComboBox::Listener,
											private MidiInputCallback,  // Class that looks for MidiInput CallBacks
											private	ToggleButton::Listener

{
public:
    JuceDemoPluginAudioProcessorEditor (JuceDemoPluginAudioProcessor&);
    ~JuceDemoPluginAudioProcessorEditor();

    //==============================================================================

	String file_path = "C:/Users/pmurgai/Desktop/Quarter-1/256A/Week3/MIDIPolySynth/Source/disc_4.jpg";
    void paint (Graphics&) override;
    void resized() override;
    void timerCallback() override;

	void initialize_midi() {   // Initializes Midi Components ( KeyBoard, InputLabel, Midi Input List)


		addAndMakeVisible(midiInputListLabel);
		midiInputListLabel.setText("MIDI Input:", dontSendNotification);
		midiInputListLabel.attachToComponent(&midiInputList, true);

		// menu listing all the available MIDI inputs
		addAndMakeVisible(midiInputList);
		midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
		const StringArray midiInputs(MidiInput::getDevices());
		midiInputList.addItemList(midiInputs, 1);
		midiInputList.setSelectedId(0); // default
		midiInputList.addListener(this);

		// by default we use the first MIDI input
	   	setMidiInput(0);  // Initializes the default midi input on the input combo box 
		int i = 5;

		// The MIDI keyboard
		addAndMakeVisible(midiKeyboard);


	}

	void pads_midi_show() {  // Displays the midi pads on the plugin 

		char buffer[128];
		char buffer_1[128];
		for (int i = 0; i < 4; i++) {
			snprintf(buffer, sizeof(buffer), "%s %d", "Pads", i+1);
			addAndMakeVisible(pads_midi[i]);
			pads_midi[i].setButtonText(buffer);
			pads_midi[i].setColour(0x1000100, Colours::cyan);
			pads_midi[i].addListener(this);
		}

		for (int i = 4; i < 8; i++) {
			snprintf(buffer_1, sizeof(buffer), "%s %d", "Pads", i+1); // Concatenates int values with string values 
			addAndMakeVisible(pads_midi[i]);
			pads_midi[i].setButtonText(buffer_1);
			pads_midi[i].setColour(0x1000100, Colours::aqua);
			pads_midi[i].addListener(this);
		}



	}

	void initialize_labels() {   // Initializes the labels for the echo and the pitch shifting knobs 

		addAndMakeVisible(echo_label);
		echo_label.setText("Echo Sample Delay", dontSendNotification);
		echo_label.attachToComponent(&echo_knob, false);
		echo_label.setEditable(true);

		addAndMakeVisible(pitshift_label);
		pitshift_label.setText("Pitch Shifting Factor", dontSendNotification);
		pitshift_label.attachToComponent(&pitshift_knob, false);



	}

	void initialize_sliders() {   // Initializes the value for the echo and pitch shifting sliders


		addAndMakeVisible(echo_knob);
		echo_knob.setRange(0,44100 );
	
		echo_knob.setSliderStyle(Slider::Rotary);
		echo_knob.setRotaryParameters(0.0, 2 * PI, true);
		echo_knob.addListener(this);
		echo_knob.setColour(Slider::rotarySliderFillColourId, Colours::cyan);
		echo_knob.setValue(0.0);
		echo_knob.setTextBoxStyle(echo_knob.TextBoxBelow, true, 60, 30);

		addAndMakeVisible(pitshift_knob);
		pitshift_knob.setRange(0, max_pitch_shift_factor);
		pitshift_knob.setSliderStyle(Slider::Rotary);
		pitshift_knob.setRotaryParameters(0.0, 2 * PI, true);
		pitshift_knob.addListener(this);
		pitshift_knob.setColour(Slider::rotarySliderFillColourId, Colours::lavenderblush);
		pitshift_knob.setValue(0.0);
		pitshift_knob.setTextBoxStyle(echo_knob.TextBoxBelow, true, 60, 30);
	}

	void mouseDown(const MouseEvent& e) {  // Callback function which gets called when mouse is preseed 

  		for (int i = 0; i < getProcessor().synth.getNumVoices(); i++) {

			SynthesiserSound*sound;
			if (e.getMouseDownX() >= 800) {

				getProcessor().synth.getVoice(i)->startNote(127, 0.8, sound, 1); // Starts the midi note to 127 and amplitude level to 0.8
			}
		} 
	}

	void buttonClicked(Button* button) override { // Callback function when a butoon is pressed 

		SynthesiserSound*sound[8];
		for (int i = 0; i < 8; i++) {


			if (button == &pads_midi[i]) {
				pads_midi[i].setColour(0x1000100, Colours::cadetblue);
				SynthesiserSound*sound;

				for (int k = 0; k < getProcessor().synth.getNumVoices(); k++) {

					getProcessor().synth.getVoice(k)->startNote(44+i, 0.8, sound, 1); // Maps the midi pads to the digital mid pads

				}
			}
		}


	}
	void comboBoxChanged(ComboBox* box) override   // Gets called when the combo box gets chnaged 
	{
		if (box == &midiInputList) setMidiInput(midiInputList.getSelectedItemIndex());
	}

	void sliderValueChanged(Slider*slider) override {   // Gets called when thhe slider is moved( echo and pitch shift knobs)

		if (slider == &echo_knob) {


			for (int i = 0; i < getProcessor().synth.getNumVoices(); i++) {



				getProcessor().synth.getVoice(i)->controllerMoved(1, (echo_knob.getValue()*max_control_val) / 44100);




			}


		}

		if (slider == &pitshift_knob) {

			for (int i = 0; i < getProcessor().synth.getNumVoices(); i++) {

				getProcessor().synth.getVoice(i)->controllerMoved(2, (pitshift_knob.getValue()*max_control_val) / max_pitch_shift_factor);

			}



		} 


	} 



private:
    class ParameterSlider;

   // JuceDemoPluginAudioProcessor& processor;
    MidiKeyboardComponent midiKeyboard;  // The software midiKeyboard 
    Label timecodeDisplayLabel, gainLabel, delayLabel;
    ScopedPointer<ParameterSlider> gainSlider, delaySlider;

	AudioDeviceManager audioDeviceManager;
   // MidiKeyboardState keyboardState;
      AudioSourcePlayer audioSourcePlayer;
	

	ComboBox midiInputList;
	Label midiInputListLabel;


	Slider echo_knob; // Echo delaying knob 
	Slider pitshift_knob; // Pitch Shifting Knob 
	double max_control_val = 127;
	double max_pitch_shift_factor = 6;
	Label echo_label, pitshift_label;

	TextButton pads_midi[8]; // TextButtons for the pidi pads on the plugin 
	int lastMidiInputIndex;
	Image img;

	int nChans = 2; // Number of Channels ( Output / Input)
	const double PI = 3.1415;

	void setMidiInput(int index) //Copied to Plugin 
	{
		const StringArray list(MidiInput::getDevices());

  		audioDeviceManager.removeMidiInputCallback(list[lastMidiInputIndex], this);

		const String newInput(list[index]);

		if (!audioDeviceManager.isMidiInputEnabled(newInput))
			audioDeviceManager.setMidiInputEnabled(newInput, true);

		audioDeviceManager.addMidiInputCallback(newInput, this);
		midiInputList.setSelectedId(index + 1, dontSendNotification);

		lastMidiInputIndex = index;
	} 


	// Needed virtual function
	void handleIncomingMidiMessage(MidiInput*, const MidiMessage& message) override {  // This function gets called when midi messages are received 

		if (message.getControllerNumber() == 1) {


			echo_knob.setValue((44100 / max_control_val)*message.getControllerValue());


		}


		if (message.getControllerNumber() == 2) {


			pitshift_knob.setValue((max_pitch_shift_factor / max_control_val)*message.getControllerValue()); // Sets the value of the pitch shifting knobs when a controller is called


		} 

		for (int i = 0; i < 8; i++) {

			if (message.getNoteNumber() == 44 + i) {

				pads_midi[i].setColour(0x1000100, Colours::cadetblue);
			}

			if (message.isNoteOff()) {

				pads_midi[i].setColour(0x1000100, Colours::cyan);

			}

		}


	} 



    //==============================================================================
    JuceDemoPluginAudioProcessor& getProcessor() const
    {
        return static_cast<JuceDemoPluginAudioProcessor&> (processor);
    }

    void updateTimecodeDisplay (AudioPlayHead::CurrentPositionInfo);
};


#endif  // __PLUGINEDITOR_H_4ACCBAA__
