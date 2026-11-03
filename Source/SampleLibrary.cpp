#include "SampleLibrary.h"
#include <cmath>
#include <random>

namespace {
int pitchClass(juce::juce_wchar c){switch(juce::CharacterFunctions::toUpperCase(c)){case 'C':return 0;case 'D':return 2;case 'E':return 4;case 'F':return 5;case 'G':return 7;case 'A':return 9;case 'B':return 11;default:return -1;}}
// "C3", "F#2", "Fs2", "Bb4", "c-1" -> MIDI note (C4 = 60).
bool noteToken(const juce::String& t,int& note){
 if(t.length()<2||t.length()>4)return false;int pc=pitchClass(t[0]);if(pc<0)return false;int i=1;
 if(t[i]=='#'||t[i]=='s'||t[i]=='S'){++pc;++i;}else if(t[i]=='b'&&i+1<t.length()){--pc;++i;}
 auto octave=t.substring(i);if(octave.isEmpty()||!(octave.containsOnly("0123456789")||(octave[0]=='-'&&octave.length()>1&&octave.substring(1).containsOnly("0123456789"))))return false;
 note=(octave.getIntValue()+1)*12+pc;return note>=0&&note<128;}
int layerToken(juce::String t){t=t.toLowerCase();
 for(auto* s:{"pp","p","soft","doux","piano","v1","vel1","l1"})if(t==s)return 0;
 for(auto* s:{"mp","mf","m","medium","moyen","mezzo","med","v2","vel2","l2"})if(t==s)return 1;
 for(auto* s:{"f","ff","forte","fort","hard","v3","vel3","l3"})if(t==s)return 2;
 return -1;}
// Reads up to 20 s, trims silence before the pick attack (keeping 2 ms so the transient is untouched)
// and fades the last 20 ms so a sample cut short never clicks.
std::shared_ptr<Sample> readSample(juce::AudioFormatReader& reader,int root){
 auto length=(int)std::min<juce::int64>(reader.lengthInSamples,(juce::int64)(reader.sampleRate*20));int channels=(int)std::min(2u,reader.numChannels);
 juce::AudioBuffer<float> raw(channels,length);reader.read(&raw,0,length,0,true,channels>1);
 float peak=raw.getMagnitude(0,length);int start=0;
 while(start<length){bool loud=false;for(int c=0;c<channels;++c)loud|=std::abs(raw.getSample(c,start))>peak*.02f;if(loud)break;++start;}
 start=std::max(0,start-(int)(reader.sampleRate*.002));if(peak<=0||length-start<64)return nullptr;
 auto s=std::make_shared<Sample>();s->root=juce::jlimit(0,127,root);s->rate=reader.sampleRate;s->audio.setSize(channels,length-start);
 for(int c=0;c<channels;++c)s->audio.copyFrom(c,0,raw,c,start,length-start);
 int fade=std::min(s->audio.getNumSamples()/4,(int)(reader.sampleRate*.02));s->audio.applyGainRamp(s->audio.getNumSamples()-fade,fade,1,0);
 return s;}
int takeToken(juce::String t){t=t.toLowerCase();for(auto* p:{"rr","take","t"})if(t.startsWith(p)&&t.length()>(int)strlen(p)&&t.substring((int)strlen(p)).containsOnly("0123456789"))return t.substring((int)strlen(p)).getIntValue();return -1;}
}

bool SampleLibrary::parseName(const juce::String& fileName,int& note,int& layer,int& take){
 auto tokens=juce::StringArray::fromTokens(juce::File::createFileWithoutCheckingPath(fileName).getFileNameWithoutExtension(),"_- .()[]",{});
 bool found=false;layer=-1;take=1;
 for(auto& t:tokens){int n;if(!found&&noteToken(t,n)){note=n;found=true;continue;}int l=layerToken(t);if(l>=0){layer=l;continue;}int k=takeToken(t);if(k>=0)take=k;}
 return found;}

void SampleLibrary::add(std::shared_ptr<Sample> s,int layer){
 auto it=std::lower_bound(roots.begin(),roots.end(),s->root,[](const Root& r,int n){return r.note<n;});
 if(it==roots.end()||it->note!=s->root){it=roots.insert(it,Root{});it->note=s->root;}
 auto lit=std::lower_bound(it->layerIds.begin(),it->layerIds.end(),layer);auto index=(size_t)(lit-it->layerIds.begin());
 if(lit==it->layerIds.end()||*lit!=layer){it->layerIds.insert(lit,layer);it->layers.insert(it->layers.begin()+(long)index,std::vector<std::shared_ptr<Sample>>{});}
 it->layers[index].push_back(std::move(s));++count;}

SampleLibrary::Pick SampleLibrary::pick(int note,float velocity,uint32_t& counter)const{
 if(roots.empty())return {};
 auto it=std::lower_bound(roots.begin(),roots.end(),note,[](const Root& r,int n){return r.note<n;});
 // Nearest root; on a tie use the lower one (pitching up keeps the attack tighter).
 if(it==roots.end())--it;else if(it!=roots.begin()&&note-std::prev(it)->note<=it->note-note)--it;
 auto& layers=it->layers;int l=juce::jlimit(0,(int)layers.size()-1,(int)(velocity*(float)layers.size()));
 auto& takes=layers[(size_t)l];auto* s=takes[(counter++)%takes.size()].get();return {s,it->note};}

std::unique_ptr<SampleLibrary> SampleLibrary::load(const juce::File& folder,juce::String& report){
 juce::AudioFormatManager formats;formats.registerBasicFormats();
 struct Found {juce::File file;int note,layer,take;};std::vector<Found> found;int skipped=0;
 for(auto& f:folder.findChildFiles(juce::File::findFiles,true,"*.wav;*.aif;*.aiff;*.flac")){int n,l,t;if(parseName(f.getFileName(),n,l,t))found.push_back({f,n,l,t});else ++skipped;}
 if(found.empty()){report="Aucun sample avec une note dans le nom (ex. Guitare_C3.wav) dans "+folder.getFullPathName();return nullptr;}
 int lowest=127;for(auto& f:found)lowest=std::min(lowest,f.note);
 int shift=12*(int)std::ceil((36-lowest)/12.0);
 auto lib=std::make_unique<SampleLibrary>();lib->name=folder.getFileName();int failed=0;
 for(auto& f:found){
  std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(f.file));if(!reader){++failed;continue;}
  auto s=readSample(*reader,f.note+shift);if(!s){++failed;continue;}
  lib->add(std::move(s),f.layer<0?1:f.layer);}
 if(lib->count==0){report="Fichiers trouves mais illisibles dans "+folder.getFullPathName();return nullptr;}
 auto names=[](int n){return juce::MidiMessage::getMidiNoteName(n,true,true,4);};
 report=juce::String(lib->count)+" samples, "+names(lib->lowest())+" -> "+names(lib->highest())+", "+juce::String(lib->layerCount())+" velocite(s)";
 if(shift!=0)report<<" (octaves decalees de "<<(shift>0?"+":"")<<shift/12<<")";
 if(skipped+failed>0)report<<", "<<(skipped+failed)<<" fichier(s) ignore(s)";
 return lib;}

std::unique_ptr<SampleLibrary> SampleLibrary::single(const void* wavData,size_t size,int note,const juce::String& name){
 juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::MemoryInputStream(wavData,size,false),true));
 if(!reader)return nullptr;auto s=readSample(*reader,note);if(!s)return nullptr;
 // Built-ins come from different sources: same peak, so the presets drive the amp the same way whatever the file.
 s->audio.applyGain(.3f/std::max(s->audio.getMagnitude(0,s->audio.getNumSamples()),1e-6f));
 auto lib=std::make_unique<SampleLibrary>();lib->name=name;lib->add(std::move(s),1);return lib;}

std::unique_ptr<SampleLibrary> SampleLibrary::synth(double rate){
 auto lib=std::make_unique<SampleLibrary>();lib->synthetic=true;lib->name=juce::String(juce::CharPointer_UTF8("Synthèse provisoire"));std::mt19937 random(7);std::uniform_real_distribution<float> noise(-1,1);
 for(int note=36;note<=88;++note){
  double f=440*std::pow(2.,(note-69)/12.),period=rate/f;int n=(int)period;float frac=(float)(period-n);
  int length=(int)(rate*3.0);std::vector<float> line((size_t)(length+n+2));
  // Lowpassed noise burst, combed at a 13 % pick position for a guitar-like spectrum.
  float prev=0;for(int i=0;i<n+2;++i){prev+=.55f*(noise(random)-prev);line[(size_t)i]=prev;}
  int pickPos=std::max(1,(int)(n*.13f));for(int i=n+1;i>=pickPos;--i)line[(size_t)i]-=.6f*line[(size_t)(i-pickPos)];
  float decay=.9965f-std::max(0,note-60)*.00012f;
  for(int i=n+2;i<length+n+2;++i){float a=line[(size_t)(i-n)],b=line[(size_t)(i-n-1)],c=line[(size_t)(i-n+1)];float y=(1-frac)*a+frac*b;line[(size_t)i]=decay*(.5f*y+.25f*(y+c));}
  auto s=std::make_shared<Sample>();s->root=note;s->rate=rate;s->audio.setSize(1,length);float peak=0;
  for(int i=0;i<length;++i){float v=line[(size_t)(i+n+2)];s->audio.setSample(0,i,v);peak=std::max(peak,std::abs(v));}
  s->audio.applyGain(.8f/std::max(peak,1e-6f));int fade=(int)(rate*.05);s->audio.applyGainRamp(length-fade,fade,1,0);
  lib->add(std::move(s),1);}
 return lib;}
