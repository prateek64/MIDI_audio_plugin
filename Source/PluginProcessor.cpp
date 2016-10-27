
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Sine.h"
#include "Smooth.h"
#include "Drummer.h"
#include "Echo.h"
#include "PitShift.h"

using namespace stk;

AudioProcessor* JUCE_CALLTYPE createPluginFilter();


//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
struct FMSound : public SynthesiserSound
{
	FMSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
	bool appliesToChannel(int /*midiChannel*/) override { return true; }
};

//==============================================================================
/** A simple demo synth voice that just plays a sine wave.. */

class SineWaveVoice  : public SynthesiserVoice
{
public:
    SineWaveVoice() : 
		carrierFrequency(440.0),
		index(0.0),
		level(0.0),
		envelope(0.0),
		onOff(false),
		tailOff(true)
    {

		carrier.setSamplingRate(getSampleRate());
		modulator.setSamplingRate(getSampleRate());
		Stk::setRawwavePath("C:/Users/pmurgai/Desktop/Quarter-1/256A/JUCE/examples/audio_plugin/Source/rawfiles"); // Sets the raw wave path files
		// audioBuffer = new float*[2];
		ech.setMaximumDelay(Stk::sampleRate()); //Sets the maximum 
		ech.setDelay(0);

    }

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<FMSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override  // Function gets called when a note is pressed on the midi
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
		drum.noteOn(carrierFrequency, 0.5);  // 

    }

	void smoothing(double smooth_level) { // Smoothing Function 

		for (int i = 0; i<4; i++) {

			smooth[i].setSmooth(smooth_level);

		}


	}

    void stopNote (float /*velocity*/, bool allowTailOff) override  // Function gets called when the note on midi is released
    {

		onOff = false; // end the note
		level = 0; // ramp envelope to 0 if tail off is allowed

		tailOff = allowTailOff;
		drum.noteOff(0.1);
       

    }

    void pitchWheelMoved (int /*newValue*/) override
    {
        // can't be bothered implementing this for the demo!
    }

    void controllerMoved (int controllerNumber, int newValue) override  // Function gets called when any of the controllers on the midi are changed 
    {
        
		if (controllerNumber == 1) {


			ech.setDelay((Stk::sampleRate() / max_control_val)*newValue);


		}


		if (controllerNumber == 2) {



			pshift.setShift((max_pitch_shift_factor / max_control_val)*newValue);


		}

    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
      //  processBlock (outputBuffer, startSample, numSamples);
		// only compute block if note is on!
		if (envelope != 0 || onOff) {
			while (--numSamples >= 0) {
				envelope = smooth[1].tick(level); // here we use a smoother as an envelope generator
				carrier.setFrequency(smooth[0].tick(carrierFrequency) + modulator.tick()*index);
				//      const float currentSample = pshift.tick((float) carrier.tick()*envelope);
				//	const float currentSample = pshift.tick(smooth[3].tick((float)drum.tick()));
				const float currentSample = pshift.tick((ech.tick((float)drum.tick())));

				for (int i = outputBuffer.getNumChannels(); --i >= 0;) {

					outputBuffer.addSample(i, startSample, currentSample);

				}

				++startSample;

				// if tail off is disabled, we end the note right away, otherwise, we wait for envelope
				// to reach a safe value
				if (!onOff && (envelope < 0.001 || !tailOff)) {
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

/*    template <typename FloatType>
    void processBlock (AudioBuffer<FloatType>& outputBuffer, int startSample, int numSamples)
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0)
            {
                while (--numSamples >= 0)
                {
                    const FloatType currentSample =
                        static_cast<FloatType> (std::sin (currentAngle) * level * tailOff);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    const FloatType currentSample = static_cast<FloatType> (std::sin (currentAngle) * level);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    } */

	// double currentAngle, angleDelta, level;
};

//==============================================================================
JuceDemoPluginAudioProcessor::JuceDemoPluginAudioProcessor()
    : lastUIWidth (400),
      lastUIHeight (200),
      gainParam (nullptr),
      delayParam (nullptr),
      delayPosition (0)
{
    lastPosInfo.resetToDefault();

    // This creates our parameters. We'll keep some raw pointers to them in this class,
    // so that we can easily access them later, but the base class will take care of
    // deleting them for us.
    addParameter (gainParam  = new AudioParameterFloat ("gain",  "Gain",           0.0f, 1.0f, 0.9f));
    addParameter (delayParam = new AudioParameterFloat ("delay", "Delay Feedback", 0.0f, 1.0f, 0.5f));

    initialiseSynth();
}

JuceDemoPluginAudioProcessor::~JuceDemoPluginAudioProcessor()
{
}

void JuceDemoPluginAudioProcessor::initialiseSynth()  // Initializes how many sounds do you want at once
{
    const int numVoices = 5;

    // Add some voices...
    for (int i = numVoices; --i >= 0;)
        synth.addVoice (new SineWaveVoice());

    // ..and give the synth a sound to play
    synth.addSound (new FMSound());

}

//==============================================================================
void JuceDemoPluginAudioProcessor::prepareToPlay (double newSampleRate, int /*samplesPerBlock*/)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
 /*   synth.setCurrentPlaybackSampleRate (newSampleRate);
    keyboardState.reset();

    if (isUsingDoublePrecision())
    {
        delayBufferDouble.setSize (2, 12000);
        delayBufferFloat.setSize (1, 1);
    }
    else
    {
        delayBufferFloat.setSize (2, 12000);
        delayBufferDouble.setSize (1, 1);
    }

    reset(); */

	keyboardState.reset();
    midiCollector.reset(newSampleRate);
	synth.setCurrentPlaybackSampleRate(newSampleRate);
	Stk::setSampleRate(newSampleRate);
	// reset();

}

void JuceDemoPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    keyboardState.reset();
}

void JuceDemoPluginAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
    delayBufferFloat.clear();
    delayBufferDouble.clear();
}

template <typename FloatType>
void JuceDemoPluginAudioProcessor::process (AudioBuffer<FloatType>& buffer,
                                            MidiBuffer& midiMessages,
                                            AudioBuffer<FloatType>& delayBuffer)
{
    const int numSamples = buffer.getNumSamples();


	midiCollector.removeNextBlockOfMessages(midiMessages, numSamples);
    // Now pass any incoming midi messages to our keyboard state object, and let it
    // add messages to the buffer if the user is clicking on the on-screen keys
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

    // and now get our synth to process these midi events and generate its output.
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    // Apply our delay effect to the new output..
 //   applyDelay (buffer, delayBuffer);

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    // Now ask the host for the current time so we can store it to be displayed later...
   // updateCurrentTimeInfoFromHost(); 



}





template <typename FloatType>
void JuceDemoPluginAudioProcessor::applyGain (AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer)
{
    ignoreUnused (delayBuffer);
    const float gainLevel = *gainParam;

    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gainLevel);
}

template <typename FloatType>
void JuceDemoPluginAudioProcessor::applyDelay (AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer)
{
    const int numSamples = buffer.getNumSamples();
    const float delayLevel = *delayParam;

    int delayPos = 0;

    for (int channel = 0; channel < getTotalNumInputChannels(); ++channel)
    {
        FloatType* const channelData = buffer.getWritePointer (channel);
        FloatType* const delayData = delayBuffer.getWritePointer (jmin (channel, delayBuffer.getNumChannels() - 1));
        delayPos = delayPosition;

        for (int i = 0; i < numSamples; ++i)
        {
            const FloatType in = channelData[i];
            channelData[i] += delayData[delayPos];
            delayData[delayPos] = (delayData[delayPos] + in) * delayLevel;

            if (++delayPos >= delayBuffer.getNumSamples())
                delayPos = 0;
        }
    }

    delayPosition = delayPos;
}

void JuceDemoPluginAudioProcessor::updateCurrentTimeInfoFromHost()
{
    if (AudioPlayHead* ph = getPlayHead())
    {
        AudioPlayHead::CurrentPositionInfo newTime;

        if (ph->getCurrentPosition (newTime))
        {
            lastPosInfo = newTime;  // Successfully got the current time from the host..
            return;
        }
    }

    // If the host fails to provide the current time, we'll just reset our copy to a default..
    lastPosInfo.resetToDefault();
}

//==============================================================================
AudioProcessorEditor* JuceDemoPluginAudioProcessor::createEditor()
{
    return new JuceDemoPluginAudioProcessorEditor (*this);
}

//==============================================================================
void JuceDemoPluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:

    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);

    // Store the values of all our parameters, using their param ID as the XML attribute
    for (int i = 0; i < getNumParameters(); ++i)
        if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
            xml.setAttribute (p->paramID, p->getValue());

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}

void JuceDemoPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our last window size..
            lastUIWidth  = jmax (xmlState->getIntAttribute ("uiWidth", lastUIWidth), 400);
            lastUIHeight = jmax (xmlState->getIntAttribute ("uiHeight", lastUIHeight), 200);

            // Now reload our parameters..
            for (int i = 0; i < getNumParameters(); ++i)
                if (AudioProcessorParameterWithID* p = dynamic_cast<AudioProcessorParameterWithID*> (getParameters().getUnchecked(i)))
                    p->setValueNotifyingHost ((float) xmlState->getDoubleAttribute (p->paramID, p->getValue()));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JuceDemoPluginAudioProcessor();
}
