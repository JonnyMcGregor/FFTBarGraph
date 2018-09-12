/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent, private Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;//sets up a few variables for audio processing
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;//manages audio input and output across channels
    void releaseResources() override;//closes audio stuff

    //==============================================================================
    void paint (Graphics& g) override;//spectrogram is drawn in this function
    void resized() override;//doesnt do too much in this program but needs to be overridden
	
	//==============================================================================

	void timerCallback() override;//used to routinely call other functions
	void pushNextSampleIntoFifo(float sample) noexcept;//prepares fft array with audio data
	void drawNextLineOfSpectrogram();//sets up the drawing of the fft freq data
	void calculateFundamental();//takes fft freq magnitudes and converts to rough frequency values
	enum 
	{
		fftOrder = 11,
		fftSize = 1 << fftOrder
	};
private:
    //==============================================================================
    
	dsp::FFT forwardFFT;//fft object
	Image spectrogramImage;//canvas upon which freq data from fft will be drawn
	Graphics drawItBoi;//object that draws data upon canvas

	float fifo[fftSize];//first in first out processing of data, also represents the window size of fft
	float fftData[2 * fftSize];//array which fft will process and return to.
	int fifoIndex = 0;//used as a counter for the fft window
	bool nextFFTBlockReady = false;//changes when fft window is full or empty
	
	float freqRes;//resolution of each frequency bin
	float fundamentalFrequency;//the approximate fundamental frequency of note (accuracy dependent on fftSize)
	float fundamentalMagnitude;//magnitude of fundamental within bin
	float numberOfBins = fftSize / 2;//is what it is amirite?
	int lowestBinForVocals;//calculate the bin containing 80Hz, is roughly the lowest note a human can make/lowest note worth making xD
	double currentSampleRate, currentAngle, angleDelta, currentLevel = 0.025, levelScale, freqLevel, Freq;//variables for generating output audio

	void updateAngleDelta();//function to create a sine wave
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
