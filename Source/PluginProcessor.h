#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "GuitarEngine.h"
#include "FxChain.h"

class GuitarProcessor : public juce::AudioProcessor {
public:
 GuitarProcessor();~GuitarProcessor()override;
 juce::AudioProcessorValueTreeState state;
 GuitarEngine engine;
 juce::MidiKeyboardState keyboard;  // on-screen keyboard
 const juce::String getName() const override{return "Swell Acoustic Guitar";}
 bool acceptsMidi() const override{return true;} bool producesMidi() const override{return false;}
 double getTailLengthSeconds() const override{return 4;}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void prepareToPlay(double,int) override;void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool hasEditor() const override{return true;}juce::AudioProcessorEditor* createEditor() override;
 int getNumPrograms() override{return presetCount;} int getCurrentProgram() override{return program.load();}
 void setCurrentProgram(int i) override;const juce::String getProgramName(int i) override{return presets[(size_t)juce::jlimit(0,presetCount-1,i)].name;}
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;void setStateInformation(const void*,int) override;
 float value(const char* id)const{return state.getRawParameterValue(id)->load();}
 void setValue(const char* id,float v);

 // Sample folder: loaded on a background thread; the synth plays until it is ready.
 void loadSamples(const juce::File& folder);
 juce::File sampleFolder()const;
 static juce::File defaultSampleFolder();
 juce::String libraryStatus()const{const juce::ScopedLock l(statusLock);return status;}
 bool usingSamples()const{auto lib=engine.library();return lib&&!lib->isSynth();}
 PlaySettings settings()const;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 void setStatus(const juce::String& s){const juce::ScopedLock l(statusLock);status=s;}
 FxChain fx;std::atomic<int> program{0};
 juce::CriticalSection statusLock;juce::String status;juce::String folderPath;
 std::shared_ptr<const SampleLibrary> synthLibrary,loaded;
 juce::ThreadPool loader{1};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarProcessor)
};
