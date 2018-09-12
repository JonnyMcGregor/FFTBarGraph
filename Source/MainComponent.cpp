/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : forwardFFT(fftOrder), spectrogramImage(Image::RGB, 700, 500, true), drawItBoi(spectrogramImage), currentAngle(0.0), angleDelta(0.0), fundamentalFrequency(0)
{
    // Make sure you set the size of the component after
    // you add any child components.
	setOpaque(true);
	
	startTimerHz(20);
	setSize(700, 256);
    // specify the number of input and output channels that we want to open
	setAudioChannels(2, 2);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
	//Ok cool, so this function is identifying the sample rate of the audio channels
	//and setting all of the variables for signal processing which rely on this value.
	//Then we print the info to console to make sure errythings going swell
	updateAngleDelta();
	currentSampleRate = sampleRate;
	freqRes = sampleRate / fftSize;
	
	lowestBinForVocals = 200 / int(freqRes);
	
	String message;
	message << "Frequency Resolution: " << freqRes << " Lowest Bin for Vocals: " << lowestBinForVocals;
	Logger::getCurrentLogger()->writeToLog(message);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
	//All of this stuff below sets up the loop to record and play audio. 

	AudioIODevice* device = deviceManager.getCurrentAudioDevice();
	const BigInteger activeInputChannels = device->getActiveInputChannels();
	const BigInteger activeOutputChannels = device->getActiveOutputChannels();

	const int maxInputChannels = activeInputChannels.getHighestBit() + 1;
	const int maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

	for (int channel = 0; channel < maxOutputChannels; ++channel) {

		if ((!activeOutputChannels[channel]) || maxInputChannels == 0) {

			bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
		}

		{
			const int actualInputChannel = channel % maxInputChannels;

			if (!activeInputChannels[channel]) { //clear buffer if input channels arent active
				bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
			}

			else {

				auto* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel, bufferToFill.startSample);
				auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
				
				//This for loop is where the important input/output stuff happens,
				//it works by assigning audio data between -1 and +1 to each individual sample
				//it will also send data to the fft to be processed
				for (int sample = 0; sample < bufferToFill.numSamples; ++sample) {
					pushNextSampleIntoFifo(inBuffer[sample]);//push sample to fft array				
					const float currentSample = (float)std::sin(currentAngle);//setting up a sine wave
					currentAngle += angleDelta;//updating sine wave position
					outBuffer[sample] = /*inBuffer[sample] */ currentSample * currentLevel;//send sine wave data to output
					

				}

			}

		}
	}

}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
	
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colours::black);
	g.setOpacity(1.0f);
	g.drawImage(spectrogramImage, getBounds().toFloat());
	
    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::timerCallback()
{
	//This is where the FFT processing gets implemented. nextBlockReady means that data from
	//fifo has been pushed into fftData and the FFT can then be applied to it.
	if (nextFFTBlockReady)
	{
		
		forwardFFT.performFrequencyOnlyForwardTransform(fftData);
		calculateFundamental();
		drawNextLineOfSpectrogram();
		nextFFTBlockReady = false;
		repaint();
		
		
		
	}
}

void MainComponent::pushNextSampleIntoFifo(float sample) noexcept
{
	//Ok, so here we are filling fifo with sample data. When fifo gets full, that data
	//gets copied to the first half of fftData. fifo then gets rewritten by next set of data.
	//Also to make the loop work fftData needs to be cleared 
	if (fifoIndex == fftSize)
	{
		if (!nextFFTBlockReady)
		{
			zeromem(fftData, sizeof(fftData));//clears fftData
			memcpy(fftData, fifo, sizeof(fifo));//copies memory from fifo to fftData
			nextFFTBlockReady = true;//ready to put more shit into fifo
		}
		fifoIndex = 0;//resests the array counter on fifo to 0
	}
	fifo[fifoIndex++] = sample;//information from sample gets pushed into fifo at array point fifoIndex.
}

void MainComponent::drawNextLineOfSpectrogram()
{
	//These floats are used to define the parameters of drawing shit

	float imageWidth = spectrogramImage.getWidth();//width of window
	float imageHeight = spectrogramImage.getHeight() - 50.0f;//height of window
	float binWidth = imageWidth / (fftSize*0.25f);
	
	auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize);
	drawItBoi.fillAll(Colours::black);
	for (auto y = 0; y < imageWidth; y++)
	{
		auto skewedProportionX = 1.0f - std::exp(std::log(y / (float)imageWidth) * 0.2f);
		auto fftDataIndex = jlimit(0,fftSize/2, (int)(skewedProportionX * fftSize/2));
		auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(),1e-5f), imageHeight, 0.0f);
		drawItBoi.setColour(Colours::aliceblue);
		drawItBoi.drawLine(fftDataIndex*binWidth, imageHeight, fftDataIndex*binWidth, level, binWidth);
		String message;
		message << fftData[y];
		//Logger::getCurrentLogger()->writeToLog(message);
	}
}

void MainComponent::calculateFundamental()
{
	int highestIndex = 0;
	fundamentalMagnitude = 0;
	float currentValue;
	for (int i = lowestBinForVocals; i < numberOfBins; i++)
	{
			currentValue = fftData[i];
			if (currentValue > fundamentalMagnitude && currentValue > 10) {
				fundamentalMagnitude = currentValue;
				highestIndex = i;
			}
	}

	

	fundamentalFrequency = highestIndex*freqRes;
	updateAngleDelta();
	String message;
	message << "Index of Fundamental: " << highestIndex << " Approx Frequency of Fundamental " << fundamentalFrequency;
	Logger::getCurrentLogger()->writeToLog(message);
}

void MainComponent::updateAngleDelta()
{
	auto cyclesPerSample = fundamentalFrequency / currentSampleRate;
	angleDelta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
}