#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include <vector>

// One recorded (or synthesised) note. Mono or stereo, at its own sample rate.
// tune: how sharp the recording is, in semitones (played back that much lower).
struct Sample {int root=60;float tune=0;juce::AudioBuffer<float> audio;double rate=44100;};

// Samples grouped by root note, then velocity layer (soft -> hard), then round-robin takes.
class SampleLibrary {
public:
 struct Pick {const Sample* sample=nullptr;int root=60;};
 struct Root {int note=60;std::vector<int> layerIds;std::vector<std::vector<std::shared_ptr<Sample>>> layers;};

 // Reads every audio file in 'folder' whose name holds a note like C3, F#2 or Bb4.
 // Octaves are shifted so the lowest note lands in C2..B2 (guitar low E = E2), whatever
 // naming convention (C3, C4 or C5 = middle C) the recording software used.
 static std::unique_ptr<SampleLibrary> load(const juce::File& folder,juce::String& report);
 // One embedded audio file (WAV, FLAC...) played across the whole keyboard: a built-in model.
 static std::unique_ptr<SampleLibrary> single(const void* data,size_t size,int note,float cents,const juce::String& name);
 // Karplus-Strong plucked strings, used until real samples are loaded.
 static std::unique_ptr<SampleLibrary> synth(double rate=44100);

 // Parses "Acoustique_C3_forte_rr2.wav" -> note 48 (C3, scientific), layer 2, take 2.
 // Returns false when no note is found. Layer: -1 when unnamed, else 0 soft .. 2 hard.
 static bool parseName(const juce::String& fileName,int& note,int& layer,int& take);

 Pick pick(int note,float velocity,uint32_t& counter)const;
 bool isSynth()const{return synthetic;}
 int lowest()const{return roots.empty()?0:roots.front().note;}
 int highest()const{return roots.empty()?0:roots.back().note;}
 int sampleCount()const{return count;}
 int layerCount()const{int n=0;for(auto& r:roots)n=std::max(n,(int)r.layers.size());return n;}
 juce::String name;
private:
 void add(std::shared_ptr<Sample>,int layer);
 std::vector<Root> roots;bool synthetic=false;int count=0;
};
