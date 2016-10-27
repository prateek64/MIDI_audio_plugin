// Music 256a / CS 476a | fall 2016
// CCRMA, Stanford University
//
// Author: Romain Michon (rmichonATccrmaDOTstanfordDOTedu)
// Description: A MIDI controlled FM polyphonic FM synth

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "Sine.h"
#include "Smooth.h"
#include "Drummer.h"
#include "Echo.h"
#include "PitShift.h"

using namespace stk;


// Not really using this here, but we need it for things to work
struct FMSound : public SynthesiserSound
{
    FMSound() {}
	
    bool appliesToNote (int /*midiNoteNumber*/) override        { return true; }
    bool appliesToChannel (int /*midiChannel*/) override        { return true; }
};

// The FM synth voice. The FM synth is hardcoded here but ideally it should be in its own class
// to have a clear hierarchy (Sine -> FMSynth -> FMVoice)
struct FMVoice : public SynthesiserVoice
	             

{
	//double ech_val, pit_val;
    FMVoice():
    carrierFrequency(440.0),
    index(0.0),
    level(0.0),
    envelope(0.0),
    onOff (false),
    tailOff(true)

    {

	
        carrier.setSamplingRate(getSampleRate());
        modulator.setSamplingRate(getSampleRate());
		Stk::setRawwavePath("C:/Users/pmurgai/Desktop/Quarter-1/256A/Week2/stk-4.5.1/rawwaves");
		audioBuffer = new float*[2];
		ech.setMaximumDelay(Stk::sampleRate());
		ech.setDelay(0);

		
    };

	

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<FMSound*> (sound) != nullptr;
    }

	
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*sound, int /*currentPitchWheelPosition*/) override
    {

		
		double smoothing_parameters = 0.999;
        // converting MIDI note number into freq
		

        carrierFrequency = MidiMessage::getMidiNoteInHertz(midiNoteNumber);

        // we don't want an ugly sweep when the note begins...
        smooth[0].setSmooth(0);
        smooth[0].tick(carrierFrequency);

		smoothing(smoothing_parameters);
        // standard smoothing...
        

       // level = velocity;
        level = velocity; // if we want linear dynamic

        // tells the note to begin!
        onOff = true;

        // These parameters could be controlled with UI elements and could
        // be assigned to specific MIDI controllers. If you do so,
        // don't forget to smooth them!

        modulator.setFrequency(2);
        index = 150;
		drum.noteOn(carrierFrequency, 0.5);
		

		
    }

	void smoothing(double smooth_level) {

		for (int i = 0; i<4; i++) {

			smooth[i].setSmooth(smooth_level);

		}


	}
    void stopNote (float /*velocity*/, bool allowTailOff) override  // allowTailoff ()  should be true  for release not equal to zero
    {
        onOff = false; // end the note
        level = 0; // ramp envelope to 0 if tail off is allowed

        tailOff = allowTailOff;
		drum.noteOff(0.1);
		
    }

    void pitchWheelMoved (int newValue) override
    {
		
        
    }

    void controllerMoved (int controllerNumber, int newValue) override
    {

		if (controllerNumber == 1) {


			ech.setDelay((Stk::sampleRate()/max_control_val)*newValue);
			

		}
		

		if (controllerNumber == 2) {

		

			pshift.setShift((max_pitch_shift_factor/max_control_val)*newValue);
		
		
		}



    }

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {

		
        // only compute block if note is on!
        if(envelope != 0 || onOff){
            while (--numSamples >= 0){
                envelope = smooth[1].tick(level); // here we use a smoother as an envelope generator
                carrier.setFrequency(smooth[0].tick(carrierFrequency)+modulator.tick()*index);
         //      const float currentSample = pshift.tick((float) carrier.tick()*envelope);
			//	const float currentSample = pshift.tick(smooth[3].tick((float)drum.tick()));
		        const float currentSample = pshift.tick((ech.tick((float)drum.tick()))) ;

                for (int i = outputBuffer.getNumChannels(); --i >= 0;){

                    outputBuffer.addSample (i, startSample, currentSample);
					
                }

                ++startSample;

                // if tail off is disabled, we end the note right away, otherwise, we wait for envelope
                // to reach a safe value
                if(!onOff && (envelope < 0.001 || !tailOff)){
                    envelope = 0;
                    clearCurrentNote();
                }
            }
        }
		
    }

private:

    Sine carrier, modulator;
    Smooth smooth[4];
    double carrierFrequency, index, level, envelope;
    bool onOff, tailOff;
	float** audioBuffer;
	Echo ech;
    Drummer drum;
	PitShift pshift;

	int flag = 0;
	double max_control_val = 127;
	double max_pitch_shift_factor = 5;

	// MainContentComponent obj;
	
	
};

struct SynthAudioSource : public AudioSource
{
    SynthAudioSource (MidiKeyboardState& keyState) : keyboardState (keyState)
    {


		
        // Add some voices to our synth, to play the sounds..
		int nVoices = 5;
        for (int i = nVoices; --i >= 0;)
        {
            synth.addVoice (new FMVoice());
			
        }

        synth.clearSounds();
        synth.addSound (new FMSound());
    }

    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {

        midiCollector.reset (sampleRate);
        synth.setCurrentPlaybackSampleRate (sampleRate);
		Stk::setSampleRate(sampleRate);

    }

    void releaseResources() override
    {
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // the synth always adds its output to the audio buffer, so we have to clear it
        // first..
        bufferToFill.clearActiveBufferRegion();


        // fill a midi buffer with incoming messages from the midi input.
        MidiBuffer incomingMidi;
		
        midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
        //midiCollector.

        // pass these messages to the keyboard state so that it can update the component
        // to show on-screen which keys are being pressed on the physical midi keyboard.
        // This call will also add midi messages to the buffer which were generated by
        // the mouse-clicking on the on-screen keyboard.
        keyboardState.processNextMidiBuffer (incomingMidi, 0, bufferToFill.numSamples, true);

        // and now get the synth to process the midi events and generate its output.
        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, 0, bufferToFill.numSamples);
    }

    // this collects real-time midi messages from the midi input device, and
    // turns them into blocks that we can process in our audio callback
    MidiMessageCollector midiCollector;

    // this represents the state of which keys on our on-screen keyboard are held
    // down. When the mouse is clicked on the keyboard component, this object also
    // generates midi messages for this, which we can pass on to our synth.
    MidiKeyboardState& keyboardState;

    // the synth itself!
    Synthesiser synth;
};

class MainContentComponent :
public Component,    //Not required in Plugin
private Slider::Listener,  //Copied to Plugin A
private ComboBox::Listener, //Copied to Plugin A
private MidiInputCallback   //Copied to Plugin A

{
public:
    MainContentComponent() :  //Completely copied 
    keyboardComponent (keyboardState, MidiKeyboardComponent::horizontalKeyboard), 
    synthAudioSource (keyboardState),
    lastMidiInputIndex (0)
    {
		
		
		nChans = 2;
        audioDeviceManager.initialise (0, nChans, nullptr, true, String(), nullptr);
		

        audioSourcePlayer.setSource (&synthAudioSource);

        audioDeviceManager.addAudioCallback (&audioSourcePlayer);
        audioDeviceManager.addMidiInputCallback (String(), &(synthAudioSource.midiCollector));

        // MIDI Inputs
       
		
		initialize_midi();
		initialize_sliders();
		initialize_labels();
        setSize (1300, 500);
    }

	

	void initialize_midi() {   //Copied to Plugin A


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
		setMidiInput(0);

		// The MIDI keyboard
		addAndMakeVisible(keyboardComponent);
		

	}



	void paint(Graphics &g) override {   //Copied to Plugin A

		File file("C:/Users/pmurgai/Desktop/Quarter-1/256A/Week3/MIDIPolySynth/Source/disc_4.jpg");
		img = ImageFileFormat::loadFrom(file);
		g.fillAll(Colours::darkgrey);

		for (int i = 0; i < 4; i++) {

			pads[i].setBounds(20 + 90 * i, 100, 60, 60);
			g.setColour(Colours::aqua);
			g.fillRect(pads[i]);

		}

	
		g.drawImageAt(img, 801,0);
		
		for (int i = 4; i < 8; i++) {

			pads[i].setBounds(20 + 90 * (8-i-1), 200, 60, 60);
			g.setColour(Colours::cornflowerblue);
			g.fillRect(pads[i]);
			
		}


	}




	void initialize_labels() {   //Copied to Plugin A

		addAndMakeVisible(echo_label);
		echo_label.setText("Echo Sample Delay", dontSendNotification);
		echo_label.attachToComponent(&echo_knob, false);
		echo_label.setEditable(true);

		addAndMakeVisible(pitshift_label);
		pitshift_label.setText("Pitch Shifting Factor", dontSendNotification);
		pitshift_label.attachToComponent(&pitshift_knob, false);



	}

	void initialize_sliders() {   //Copied to Plugin A


		addAndMakeVisible(echo_knob);
		echo_knob.setRange(0, Stk::sampleRate());
		echo_knob.setSliderStyle(Slider::Rotary);
		echo_knob.setRotaryParameters(0.0, 2*PI,true);
		echo_knob.addListener(this);
		echo_knob.setColour(Slider::rotarySliderFillColourId, Colours::cyan);
		echo_knob.setValue(0.0);
		echo_knob.setTextBoxStyle(echo_knob.TextBoxBelow,true, 60,30);

		addAndMakeVisible(pitshift_knob);
		pitshift_knob.setRange(0, max_pitch_shift_factor);
		pitshift_knob.setSliderStyle(Slider::Rotary);
		pitshift_knob.setRotaryParameters(0.0, 2 * PI, true);
		pitshift_knob.addListener(this);
		pitshift_knob.setColour(Slider::rotarySliderFillColourId, Colours::lavenderblush);
		pitshift_knob.setValue(0.0);
		pitshift_knob.setTextBoxStyle(echo_knob.TextBoxBelow, true, 60, 30);
	}

    ~MainContentComponent()   //Copied to Plugin A
    {
        audioSourcePlayer.setSource (nullptr);
        audioDeviceManager.removeMidiInputCallback (String(), &(synthAudioSource.midiCollector));
        audioDeviceManager.removeAudioCallback (&audioSourcePlayer);
        audioDeviceManager.removeMidiInputCallback (MidiInput::getDevices()[midiInputList.getSelectedItemIndex()], this);
        midiInputList.removeListener (this);
    }

	void mouseDown(const MouseEvent& e) {  //Copied to plugin A

		for (int i = 0; i < synthAudioSource.synth.getNumVoices(); i++) {
			
			SynthesiserSound*sound;
			if (e.getMouseDownX() >= 800) {

				synthAudioSource.synth.getVoice(i)->startNote(127, 0.8, sound, 1);
			}
		}
		


	}

    void resized() override   //Copied to Plugin A
    {
        const int labelWidth = 90;
        midiInputList.setBounds (labelWidth, 0, getWidth()-labelWidth, 20);
        keyboardComponent.setBounds (0, 300, 800, 200);

		echo_knob.setBounds(20 + 90*4 + 50 , 100, 150, 150);
		pitshift_knob.setBounds(20 + 90*4 + 50 + 150, 100, 150, 150);
    }

	

    void comboBoxChanged (ComboBox* box) override   //Copied to Plugin A
    {
        if (box == &midiInputList) setMidiInput  (midiInputList.getSelectedItemIndex());
    }

	void sliderValueChanged(Slider*slider) override {   //Copied to Plugin A

		if (slider == &echo_knob) {


			for (int i = 0; i < synthAudioSource.synth.getNumVoices(); i++) {



				synthAudioSource.synth.getVoice(i)->controllerMoved(1, (echo_knob.getValue()*max_control_val)/Stk::sampleRate());
				


			}
			

		}

		if (slider == &pitshift_knob) {

			for (int i = 0; i < synthAudioSource.synth.getNumVoices(); i++) {

				synthAudioSource.synth.getVoice(i)->controllerMoved(2, (pitshift_knob.getValue()*max_control_val)/max_pitch_shift_factor);

			}
			


		}  
		

	}


private:   //Copied to Plugin A

    AudioDeviceManager audioDeviceManager;
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboardComponent;
    AudioSourcePlayer audioSourcePlayer;
    SynthAudioSource synthAudioSource;

    ComboBox midiInputList;
    Label midiInputListLabel;
	


	Slider echo_knob;
	Slider pitshift_knob;
	double max_control_val = 127;
	double max_pitch_shift_factor = 6;
	Label echo_label, pitshift_label;

	Rectangle<int> pads[8];
    int lastMidiInputIndex;
	Image img;

	int nChans = 2;
	const double PI = 3.1415;

    void setMidiInput (int index) //Copied to Plugin 
    {
        const StringArray list (MidiInput::getDevices());

        audioDeviceManager.removeMidiInputCallback (list[lastMidiInputIndex], this);

        const String newInput (list[index]);

        if (! audioDeviceManager.isMidiInputEnabled (newInput))
            audioDeviceManager.setMidiInputEnabled (newInput, true);

        audioDeviceManager.addMidiInputCallback (newInput, this);
        midiInputList.setSelectedId (index + 1, dontSendNotification);

        lastMidiInputIndex = index;
    }

    // Needed virtual function
    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override {  // Copied to Plugin
        
		if (message.getControllerNumber() ==  1) {


			echo_knob.setValue((Stk::sampleRate() / max_control_val)*message.getControllerValue());

			
		}


		 if (message.getControllerNumber() == 2) {


			pitshift_knob.setValue((max_pitch_shift_factor / max_control_val)*message.getControllerValue());


		}
	
		
	}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
