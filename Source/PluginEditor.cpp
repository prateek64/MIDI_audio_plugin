
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// This is a handy slider subclass that controls an AudioProcessorParameter
// (may move this class into the library itself at some point in the future..)
class JuceDemoPluginAudioProcessorEditor::ParameterSlider   : public Slider,
                                                              private Timer
{
public:
    ParameterSlider (AudioProcessorParameter& p)
        : Slider (p.getName (256)), param (p)
    {
        setRange (0.0, 1.0, 0.0);
        startTimerHz (30);
        updateSliderPos();
    }

    void valueChanged() override
    {
        if (isMouseButtonDown())
            param.setValueNotifyingHost ((float) Slider::getValue());
        else
            param.setValue ((float) Slider::getValue());
    }

    void timerCallback() override       { updateSliderPos(); }

    void startedDragging() override     { param.beginChangeGesture(); }
    void stoppedDragging() override     { param.endChangeGesture();   }

    double getValueFromText (const String& text) override   { return param.getValueForText (text); }
    String getTextFromValue (double value) override         { return param.getText ((float) value, 1024); }

    void updateSliderPos()
    {
        const float newValue = param.getValue();

        if (newValue != (float) Slider::getValue() && ! isMouseButtonDown())
            Slider::setValue (newValue);
    }

    AudioProcessorParameter& param;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

//==============================================================================
JuceDemoPluginAudioProcessorEditor::JuceDemoPluginAudioProcessorEditor (JuceDemoPluginAudioProcessor& owner)
    : AudioProcessorEditor (owner),
      midiKeyboard (owner.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
      timecodeDisplayLabel (String()),
      gainLabel (String(), "Throughput level:"),
      delayLabel (String(), "Delay:"),
	  lastMidiInputIndex(0)
{
    
	nChans = 2;  // Number of output channels 
	audioDeviceManager.initialise(0, nChans, nullptr, true, String(), nullptr); // Initializes 0 input channels , 2 output channels 


 
	audioDeviceManager.addMidiInputCallback(String(), &(getProcessor().midiCollector));

	// MIDI Inputs


	initialize_midi();  // Initializes midi components 
	initialize_sliders(); // Initializes the echo and pitch shifting sliders
	initialize_labels();  // Initializes all the labels 
	pads_midi_show(); // Displays the digital  midi pads 

	setSize(1300, 500);  // Sets the size of the plugin window

    // start a timer which will keep our timecode display updated
 //   startTimerHz (30);
}

JuceDemoPluginAudioProcessorEditor::~JuceDemoPluginAudioProcessorEditor()
{

	audioSourcePlayer.setSource(nullptr);
	audioDeviceManager.removeMidiInputCallback(String(), &(getProcessor().midiCollector));
	audioDeviceManager.removeAudioCallback(&audioSourcePlayer);
	audioDeviceManager.removeMidiInputCallback(MidiInput::getDevices()[midiInputList.getSelectedItemIndex()], this);
	midiInputList.removeListener(this); 


}

//==============================================================================
void JuceDemoPluginAudioProcessorEditor::paint (Graphics& g)
{   
	File file(file_path); // Opens the image file 
	img = ImageFileFormat::loadFrom(file);
	g.fillAll(Colours::darkgrey);  // Sets the color of the plugin 

	g.drawImageAt(img, 801, 0); // Draws the image at (801,0)

	
}

void JuceDemoPluginAudioProcessorEditor::resized()  // Function gets called when the window is resized
{
    
	const int labelWidth = 90;
	midiInputList.setBounds(labelWidth, 0, getWidth() - labelWidth, 20);
	midiKeyboard.setBounds(0, 300, 800, 200);

	echo_knob.setBounds(20 + 90 * 4 + 50, 100, 150, 150);
	pitshift_knob.setBounds(20 + 90 * 4 + 50 + 150, 100, 150, 150);

	for (int i = 0; i < 4; i++) {


		pads_midi[i].setBounds(20 + 90 * i, 100, 60, 60);  // Sets the size of the midi pads on the plugin


	}

	for (int i = 0; i < 4; i++) {


		pads_midi[i+4].setBounds(20 + 90 * i, 200, 60, 60);


	}

}

//==============================================================================
void JuceDemoPluginAudioProcessorEditor::timerCallback()
{
    updateTimecodeDisplay (getProcessor().lastPosInfo);
}

//==============================================================================
// quick-and-dirty function to format a timecode string
static String timeToTimecodeString (double seconds)
{
    const int millisecs = roundToInt (seconds * 1000.0);
    const int absMillisecs = std::abs (millisecs);

    return String::formatted ("%02d:%02d:%02d.%03d",
                              millisecs / 360000,
                              (absMillisecs / 60000) % 60,
                              (absMillisecs / 1000) % 60,
                              absMillisecs % 1000);
}

// quick-and-dirty function to format a bars/beats string
static String quarterNotePositionToBarsBeatsString (double quarterNotes, int numerator, int denominator)
{
    if (numerator == 0 || denominator == 0)
        return "1|1|000";

    const int quarterNotesPerBar = (numerator * 4 / denominator);
    const double beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

    const int bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
    const int beat   = ((int) beats) + 1;
    const int ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

    return String::formatted ("%d|%d|%03d", bar, beat, ticks);
}

// Updates the text in our position label.
void JuceDemoPluginAudioProcessorEditor::updateTimecodeDisplay (AudioPlayHead::CurrentPositionInfo pos)
{
    MemoryOutputStream displayText;

    displayText << "[" << SystemStats::getJUCEVersion() << "]   "
                << String (pos.bpm, 2) << " bpm, "
                << pos.timeSigNumerator << '/' << pos.timeSigDenominator
                << "  -  " << timeToTimecodeString (pos.timeInSeconds)
                << "  -  " << quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                                    pos.timeSigNumerator,
                                                                    pos.timeSigDenominator);

    if (pos.isRecording)
        displayText << "  (recording)";
    else if (pos.isPlaying)
        displayText << "  (playing)";

    timecodeDisplayLabel.setText (displayText.toString(), dontSendNotification);
}
