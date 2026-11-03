#pragma once
#include "SampleLibrary.h"
#include "Params.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <random>

// Playing-module settings, read once per block from the parameters.
struct PlaySettings {
 bool slide=false,legato=false,vibrato=false,strum=false,humanize=true,palm=false;
 float slideTime=30,legatoSoft=60,vibDepth=40,strumSpeed=55,humAmount=35,palmAmount=70;int chord=0;
};

enum class Articulation {none,pluck,slide,hammer,pulloff,strum,palm};

// Polyphonic sampler with guitar playing modules: slide, legato, vibrato, strum, humanize, palm mute.
class GuitarEngine {
public:
 static constexpr int maxVoices=24;
 void prepare(double sampleRate);
 // Swaps the sound set; call from any thread, the audio thread picks it up between blocks.
 void setLibrary(std::shared_ptr<const SampleLibrary>);
 std::shared_ptr<const SampleLibrary> library()const{return std::atomic_load(&pending);}
 // Renders MIDI into 'out' (adds to it). 'out' has 1 or 2 channels.
 void render(juce::AudioBuffer<float>& out,const juce::MidiBuffer&,const PlaySettings&);
 void allNotesOff();

 // Display state for the editor (lock-free).
 std::array<std::atomic<uint32_t>,4> sounding{};
 std::atomic<int> lastNote{-1},lastVelocity{0},voiceCount{0},lastChord{-1},lastFrom{-1};
 std::atomic<Articulation> lastArticulation{Articulation::none};
 bool isSounding(int note)const{return (sounding[(size_t)(note>>5)].load()>>(note&31))&1u;}
 int activeVoices()const;
private:
 struct Voice {
  const Sample* sample=nullptr;int root=60,key=-1;bool held=false,releasing=false,palm=false;
  double pos=0;float semis=0,targetSemis=0,glideCoef=1,detune=0;int delay=0;
  float gain=0,amp=1,attackStep=1,releaseCoef=.999f,palmDecay=1;
  float lp1[2]{},lp2[2]{},lpCoef=1;float vibPhase=0,vibRamp=0,vibDepth=0;uint32_t age=0;
  bool active()const{return sample!=nullptr;}
 };
 void noteOn(int note,float velocity,int offset,const PlaySettings&);
 void noteOff(int note);
 Voice& startVoice(int key,int note,float velocity,int offset,const PlaySettings&,float startSeconds=0,float attackSeconds=.001f);
 void release(Voice&,float seconds);
 Voice* heldVoice();
 void renderVoice(Voice&,juce::AudioBuffer<float>&,int start,int count,const PlaySettings&);
 float rnd(){return dist(random);}

 std::shared_ptr<const SampleLibrary> current,pending;
 std::array<Voice,maxVoices> voices;std::array<bool,128> keys{},sustained{};
 double rate=44100;uint32_t ageCounter=0,roundRobin=0;bool sustainPedal=false,nextStrumUp=false;
 float bend=0,modWheel=0;std::mt19937 random{1234};std::uniform_real_distribution<float> dist{-1.f,1.f};
};
