#include "../Source/PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
void set(GuitarProcessor& p,const char* id,float v){auto* a=p.state.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(v));}
struct Event {double at;int note;bool on;float velocity=.8f;};
// Renders 'seconds' at 48 kHz with the given MIDI events; returns the left channel.
std::vector<float> run(GuitarProcessor& p,double seconds,std::vector<Event> events,std::function<void(int)> perBlock={}){
 const int block=256;p.prepareToPlay(48000,block);juce::AudioBuffer<float> b(2,block);std::vector<float> out;
 for(int start=0,index=0;start<(int)(48000*seconds);start+=block,++index){juce::MidiBuffer m;
  for(auto& e:events){int at=(int)(e.at*48000)-start;if(at>=0&&at<block)m.addEvent(e.on?juce::MidiMessage::noteOn(1,e.note,e.velocity):juce::MidiMessage::noteOff(1,e.note),at);}
  p.processBlock(b,m);if(perBlock)perBlock(index);
  for(int n=0;n<block;++n){require(std::isfinite(b.getSample(0,n))&&std::isfinite(b.getSample(1,n)),"non-finite output");out.push_back(b.getSample(0,n));}}
 return out;}
float rms(const std::vector<float>& x,double from,double to){double s=0;int a=(int)(from*48000),z=std::min((int)x.size(),(int)(to*48000));for(int i=a;i<z;++i)s+=x[(size_t)i]*x[(size_t)i];return (float)std::sqrt(s/std::max(1,z-a));}
// Dominant period by autocorrelation between 'lo' and 'hi' Hz.
float pitchHz(const std::vector<float>& x,double from,float lo=60,float hi=1200){int a=(int)(from*48000),len=4096;int best=0;double bestScore=-1;
 for(int lag=(int)(48000/hi);lag<=(int)(48000/lo);++lag){double s=0;for(int i=0;i<len;++i)s+=x[(size_t)(a+i)]*x[(size_t)(a+i+lag)];if(s>bestScore){bestScore=s;best=lag;}}return 48000.f/(float)best;}

int main(){try{juce::ScopedJuceInitialiser_GUI gui;
 {int n,l,t;
  require(SampleLibrary::parseName("Acoustique_C3_forte_rr2.wav",n,l,t)&&n==48&&l==2&&t==2,"parse C3 forte rr2");
  require(SampleLibrary::parseName("Guitar F#2 p.wav",n,l,t)&&n==42&&l==0&&t==1,"parse F#2 p");
  require(SampleLibrary::parseName("gtr-Bb4.aif",n,l,t)&&n==70&&l==-1,"parse Bb4");
  require(SampleLibrary::parseName("Fs3_mf.wav",n,l,t)&&n==54&&l==1,"parse Fs3 mf");
  require(!SampleLibrary::parseName("Guitar.wav",n,l,t),"name without note accepted");}
 // Folder loading: a fake guitar named in FL Studio convention (E3 = low E) must be shifted to E2.
 {auto dir=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("swell-guitar-test");dir.deleteRecursively();dir.createDirectory();
  juce::WavAudioFormat wav;for(auto [name,hz]:{std::pair{"Test_E3.wav",82.41},std::pair{"Test_A3.wav",110.0}}){
   juce::AudioBuffer<float> b(1,48000);for(int i=0;i<48000;++i)b.setSample(0,i,i<480?0.f:(float)(.5*std::sin(6.2831853*hz*i/48000)*std::exp(-2.0*i/48000)));
   std::unique_ptr<juce::OutputStream> os(dir.getChildFile(name).createOutputStream());std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(os.get(),48000,1,24,{},0));require(w!=nullptr,"wav writer");os.release();w->writeFromAudioSampleBuffer(b,0,48000);}
  juce::String report;auto lib=SampleLibrary::load(dir,report);require(lib&&lib->sampleCount()==2,"library not loaded");require(lib->lowest()==40&&lib->highest()==45,"octave not normalised to E2");
  uint32_t c=0;auto pick=lib->pick(43,.5f,c);require(pick.root==45||pick.root==40,"nearest root");require(pick.sample->audio.getNumSamples()<48000-400,"leading silence not trimmed");
  // Plays the loaded library: E2 should sound at ~82 Hz.
  GuitarProcessor p;p.engine.setLibrary(std::shared_ptr<const SampleLibrary>(std::move(lib)));set(p,"humanize",0);set(p,"dmix",0);set(p,"rmix",0);set(p,"drive",0);
  auto out=run(p,.6,{{0,40,true}});float f=pitchHz(out,.1,50,400);require(std::abs(f-82.4f)<3,"sample plays at wrong pitch");dir.deleteRecursively();}
 // Synth fallback, every preset: sound, no NaN, reasonable level.
 for(int i=0;i<presetCount;++i){GuitarProcessor p;p.setCurrentProgram(i);auto out=run(p,1.5,{{0,52,true},{.4,55,true},{.8,52,false},{.8,55,false}});
  float level=rms(out,0,.8);require(level>.005f&&level<.7f,"preset level out of range");}
 {GuitarProcessor p;auto out=run(p,1,{});require(rms(out,0,1)<1e-5f,"noise without notes");}
 // Pitch: A3 (220 Hz) with humanize off.
 {GuitarProcessor p;set(p,"humanize",0);set(p,"dmix",0);set(p,"rmix",0);auto out=run(p,.5,{{0,57,true}});float f=pitchHz(out,.1);require(std::abs(f-220)<4,"A3 not at 220 Hz");}
 // Strum: one key sounds five strings.
 {GuitarProcessor p;set(p,"strum",1);int most=0;run(p,.6,{{0,48,true}},[&](int){most=std::max(most,p.engine.voiceCount.load());});require(most==5,"strum does not play 5 strings");}
 // Slide: overlapping notes glide on one voice and end at the new pitch.
 {GuitarProcessor p;set(p,"slide",1);set(p,"humanize",0);set(p,"dmix",0);set(p,"rmix",0);int most=0;
  auto out=run(p,1,{{0,57,true},{.3,59,true},{.35,57,false}},[&](int){most=std::max(most,p.engine.voiceCount.load());});
  require(most==1,"slide started a second voice");float f=pitchHz(out,.6);require(std::abs(f-246.9f)<5,"slide does not reach B3");}
 // Legato: overlapping notes hand over to a new voice with a softer attack.
 {GuitarProcessor p;set(p,"legato",1);set(p,"humanize",0);auto out=run(p,.8,{{0,57,true},{.3,59,true}});require(p.engine.lastArticulation.load()==Articulation::hammer,"legato not detected as hammer-on");}
 // Palm mute dies away much faster than an open note.
 {GuitarProcessor a,b;set(a,"humanize",0);set(b,"humanize",0);set(b,"palm",1);set(a,"rmix",0);set(b,"rmix",0);set(a,"dmix",0);set(b,"dmix",0);
  auto open=run(a,1.2,{{0,45,true}}),muted=run(b,1.2,{{0,45,true}});require(rms(muted,.8,1.2)<rms(open,.8,1.2)*.3f,"palm mute does not damp");}
 // Note off releases the string.
 {GuitarProcessor p;set(p,"rmix",0);set(p,"dmix",0);auto out=run(p,1.5,{{0,50,true},{.3,50,false}});require(rms(out,1.,1.5)<rms(out,.05,.3)*.02f,"note off does not release");}
 {GuitarProcessor p;p.setCurrentProgram(4);set(p,"vol",42);juce::MemoryBlock state;p.getStateInformation(state);GuitarProcessor restored;restored.setStateInformation(state.getData(),(int)state.getSize());
  require(restored.getCurrentProgram()==4&&restored.value("slide")>.5f&&std::abs(restored.value("vol")-42)<.2f,"state not restored");}
 std::cout<<"PASS: name parsing, folder load + octave fix + pitch, 6 presets, silence, pitch, strum, slide, legato, palm mute, release, state\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
