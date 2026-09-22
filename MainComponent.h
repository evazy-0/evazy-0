/* 
Audio - to - Visual Interactive Canvas
By Ziyi(Eva) Wang, built with JUCE C++
*/
#include "MainComponent.h"
#include <cmath>
#include <algorithm>

MainComponent::MainComponent()
{
    // basic configuration and audio input pipeline
    setSize(800, 600);          
    setAudioChannels(2, 2);     // dual channel stereo input

    // start the interaction loop
    startTimerHz(60);           // refresh to ensure smooth animation
    setWantsKeyboardFocus(true);
}

// safely release audio resources when closing the component
MainComponent::~MainComponent()
{
    shutdownAudio();
}


void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    sampleRateCache = sampleRate; 
}

// identify its major or minor key.
void MainComponent::evaluateChord(const std::vector<int>& noteNumbers)
{
    if (noteNumbers.empty()) return;

    // remove octave differences
    std::set<int> pitchClasses;
    for (int note : noteNumbers) 
    {
        pitchClasses.insert(note % 12);
    }

    bool foundMajor = false;
    bool foundMinor = false;

    // major or minor triad
    for (int root = 0; root < 12; ++root)
    {
        int majorThird = (root + 4) % 12;
        int fifth = (root + 7) % 12;
        int minorThird = (root + 3) % 12;

        // check major triad
        if (pitchClasses.count(root) && pitchClasses.count(majorThird) && pitchClasses.count(fifth)) 
        {
            foundMajor = true;
            break;
        }
        //check minor triad
        if (pitchClasses.count(root) && pitchClasses.count(minorThird) && pitchClasses.count(fifth)) 
        {
            foundMinor = true;
            break;
        }
    }

    // if no triad tone, downgraded to a feature assessment based on the single most recent note
    if (!foundMajor && !foundMinor)
    {
        int latestNote = noteNumbers.back() % 12;

        // major/minor
        if (latestNote == 0 || latestNote == 4 || latestNote == 7) {
            isMajorMode = true;
            currentModeCode = "SINGLE NOTE: MAJOR (HAPPY)";
        }
        else 
        {
            isMajorMode = false;
            currentModeCode = "SINGLE NOTE: MINOR (SOMBER)";
        }
    }
    // major
    else if (foundMajor)
    {
        isMajorMode = true;
        currentModeCode = "CHORD DETECTED: MAJOR (HAPPY)";
    }
    // minor
    else
    {
        isMajorMode = false;
        currentModeCode = "CHORD DETECTED: MINOR (SOMBER)";
    }
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // skip if empty
    if (bufferToFill.buffer == nullptr)
        return;

    // get to the current audio input data and the number of samples
    auto* channelData = bufferToFill.buffer->getReadPointer(0);
    int numSamples = bufferToFill.numSamples;
    float rms = 0.0f;
    // compute RMS and FFT cache
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = channelData[i];
        rms += sample * sample;

        if (fifoIndex < fftSize)
        {
            fifo[fifoIndex++] = sample;
        }
    }

    // overall volume level
    currentMicLevel = std::sqrt(rms / (float)numSamples);

    // if fifo full, fft and pitch analysis
    if (fifoIndex >= fftSize)
    {
        std::fill(fftData, fftData + fftSize * 2, 0.0f);
        std::copy(fifo, fifo + fftSize, fftData);

        window.multiplyWithWindowingTable(fftData, fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);


        // dominant frequency
        float peakValue = 0.0f;
        int peakIndex = 0;
        for (int i = 0; i < fftSize / 2; ++i)
        {
            if (fftData[i] > peakValue)
            {
                peakValue = fftData[i];
                peakIndex = i;
            }
        }

        // convert the peak value in the frequency domain to a specific frequency in Hz
        float dominantFreq = (static_cast<float>(peakIndex) * static_cast<float>(sampleRateCache)) / static_cast<float>(fftSize);

        // ensure recognition accuracy 
        if (currentMicLevel > 0.02f && dominantFreq > 65.0f && dominantFreq < 2000.0f)
        {
            int estimatedMidiNote = static_cast<int>(std::round(69.0f + 12.0f * std::log2(dominantFreq / 440.0f)));
            currentNoteNumber = estimatedMidiNote;

            recentNotes.push_back(estimatedMidiNote);
            if (recentNotes.size() > 3) recentNotes.erase(recentNotes.begin());

            evaluateChord(recentNotes);
        }

        fifoIndex = 0;
    }
    // clear 
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // free audio resources
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    
    char c = key.getTextCharacter();

    // manual switch major(M) or minor(N)
    if (c == 'm' || c == 'M')
    {
        isMajorMode = true;
        currentModeCode = "MANUAL: MAJOR (HAPPY)";
        return true;
    }
    else if (c == 'n' || c == 'N')
    {
        isMajorMode = false;
        currentModeCode = "MANUAL: MINOR (SOMBER)";
        return true;
    }
    return false;
}


void MainComponent::timerCallback()
{
    // smoother animation transitions
    float targetRadius = 40.0f + (currentMicLevel * 400.0f);
    animatedRadius += (targetRadius - animatedRadius) * 0.2f;
  
    repaint();
}

void MainComponent::resized()
{

}


void MainComponent::paint(juce::Graphics& g)
{
    if (isMajorMode)
    {
        g.fillAll(juce::Colour(40, 90, 160)); // major: Bright
    }
    else
    {
        g.fillAll(juce::Colour(15, 12, 25)); // minor: deep dark 
    }

    g.setColour(isMajorMode ? juce::Colours::lightgreen : juce::Colours::darkorange);
    auto centerX = getWidth() / 2.0f;
    auto centerY = getHeight() / 2.0f;
    g.fillEllipse(centerX - animatedRadius, centerY - animatedRadius,
        animatedRadius * 2.0f, animatedRadius * 2.0f);


    // Dynamically calculate the eye positions based on the current radius of the circle.
    g.setColour(juce::Colours::white);
    float eyeY = centerY - (animatedRadius * 0.3f);       
    float eyeOffsetX = animatedRadius * 0.4f;             
    float eyeSize = juce::jmax(4.0f, animatedRadius * 0.12f); 

    g.fillEllipse(centerX - eyeOffsetX - eyeSize / 2.0f, eyeY, eyeSize, eyeSize);
    g.fillEllipse(centerX + eyeOffsetX - eyeSize / 2.0f, eyeY, eyeSize, eyeSize);

    // Use Path instead of drawArc to draw the smiley or sad line.
    g.setColour(juce::Colours::white);
    if (isMajorMode)
    {
        // major happy face
        juce::Path smilePath;
        smilePath.startNewSubPath(centerX - 22.0f, centerY + 2.0f);
        smilePath.quadraticTo(centerX, centerY + 20.0f, centerX + 22.0f, centerY + 2.0f);
        g.strokePath(smilePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        // minor sad face
        g.drawLine(centerX - 15.0f, centerY + 8.0f, centerX + 15.0f, centerY + 8.0f, 4.0f);
    }

    g.setFont(18.0f);
    g.drawText(currentModeCode, 30, 30, 700, 30, juce::Justification::left);

    juce::String infoText = "Note: " + juce::String(currentNoteNumber) + " | Mic RMS: " + juce::String(currentMicLevel, 3);
    g.drawText(infoText, 30, 70, 700, 30, juce::Justification::left);
}
