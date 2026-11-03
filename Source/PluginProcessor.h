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
 const juce::String getName() const override{return "Swell Guitar";}
 bool acceptsMidi() const override{return true;} bool producesMidi() const override{return false;}
 double getTailLengthSeconds() const override{return 4;}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void prepareToPlay(double,int) override;void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool hasEditor() const override{return true;}juce::AudioProcessorEditor* createEditor() override;
 int getNumPrograms() override{return presetCount;} int getCurrentProgram() override{return program.load();}
 void setCurrentProgram(int i) override;const juce::String getProgramName(int i) override{return juce::String(juce::CharPointer_UTF8(presets[(size_t)juce::jlimit(0,presetCount-1,i)].name));}
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;void setStateInformation(const void*,int) override;
 float value(const char* id)const{return state.getRawParameterValue(id)->load();}
 void setValue(const char* id,float v);

 // 0 = acoustique, 1 = electrique: picks the sound set and the amp of the FX chain.
 int guitarType()const{return juce::jlimit(0,guitarTypeCount-1,(int)value("guitarType"));}
 // Each type plays its built-in sample until a folder is loaded for it (on a background thread).
 void loadSamples(const juce::File& folder){loadSamples(guitarType(),folder);}
 void loadSamples(int type,const juce::File& folder);
 void useLibrary(int type,std::shared_ptr<const SampleLibrary>);
 // Built-in model (index into 'models') picked for a type, and picking one from the editor:
 // it replaces any sample folder loaded for that type.
 int selectedModel(int type)const;
 void selectModel(int type,int menuIndex);
 // Models load on a background thread; true once every type plays its selected model (tests).
 bool waitForModels(int timeoutMs);
 juce::File sampleFolder()const;
 static juce::File defaultSampleFolder(int type);
 juce::String libraryStatus()const{const juce::ScopedLock l(statusLock);return status[(size_t)guitarType()];}
 bool usingSamples()const{auto lib=engine.library();return lib&&!lib->isSynth();}
 PlaySettings settings()const;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 void setStatus(int type,const juce::String& s){const juce::ScopedLock l(statusLock);status[(size_t)type]=s;}
 FxChain fx;std::atomic<int> program{0};
 juce::CriticalSection statusLock;std::array<juce::String,guitarTypeCount> status,folderPath;
 std::array<std::shared_ptr<const SampleLibrary>,guitarTypeCount> builtIn,user;
 // Loader thread state: which model each builtIn holds, recently decoded models (loader thread only).
 void updateModels();std::shared_ptr<const SampleLibrary> loadModel(int index);
 std::array<std::atomic<int>,guitarTypeCount> builtInModel{};std::vector<std::pair<int,std::shared_ptr<const SampleLibrary>>> modelCache;
 std::shared_ptr<const SampleLibrary> synth;
 struct ModelLoader:juce::Thread {explicit ModelLoader(GuitarProcessor& p):juce::Thread("Swell Guitar models"),proc(p){}
  void run()override{while(!threadShouldExit()){proc.updateModels();wait(20);}}GuitarProcessor& proc;};
 ModelLoader modelLoader{*this};
 juce::ThreadPool loader{1};
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarProcessor)
};
