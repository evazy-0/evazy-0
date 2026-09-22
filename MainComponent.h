#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <set>

class MainComponent : public juce::AudioAppComponent,
    public juce::Timer

{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void evaluateChord(const std::vector<int>& noteNumbers);

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT forwardFFT{ fftOrder };
    juce::dsp::WindowingFunction<float> window{ fftSize, juce::dsp::WindowingFunction<float>::hann };

    float fifo[fftSize];
    float fftData[fftSize * 2];
    int fifoIndex = 0;
    double sampleRateCache = 44100.0;

    std::vector<int> recentNotes;

    bool isMajorMode = true;
    float animatedRadius = 50.0f;
    float currentMicLevel = 0.0f;
    float currentKeyVelocity = 0.0f;
    int currentNoteNumber = 60;
    juce::String currentModeCode = "STATUS: INITIALIZED";
private:
    void JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DECORATOR(MainComponent);
};
