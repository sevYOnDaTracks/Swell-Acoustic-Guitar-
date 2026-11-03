#include "GuitarEngine.h"
#include <cmath>

void GuitarEngine::prepare(double sampleRate){rate=sampleRate;allNotesOff();for(auto& v:voices)v=Voice{};}

void GuitarEngine::setLibrary(std::shared_ptr<const SampleLibrary> lib){std::atomic_store(&pending,std::move(lib));}

void GuitarEngine::allNotesOff(){for(auto& v:voices)if(v.active())release(v,.03f);keys.fill(false);sustained.fill(false);sustainPedal=false;}

int GuitarEngine::activeVoices()const{int n=0;for(auto& v:voices)n+=v.active()?1:0;return n;}

void GuitarEngine::release(Voice& v,float seconds){v.releasing=true;v.held=false;v.releaseCoef=std::exp(-1.f/(float)(std::max(.002f,seconds)*rate));}

GuitarEngine::Voice* GuitarEngine::heldVoice(){Voice* best=nullptr;for(auto& v:voices)if(v.active()&&v.held&&!v.releasing&&(!best||v.age>best->age))best=&v;return best;}

GuitarEngine::Voice& GuitarEngine::startVoice(int key,int note,float velocity,int offset,const PlaySettings& s,float startSeconds,float attackSeconds){
 // Same key retriggered: the string is re-picked, so damp the old note quickly.
 for(auto& v:voices)if(v.active()&&v.key==key&&!v.releasing)release(v,.04f);
 Voice* slot=nullptr;for(auto& v:voices)if(!v.active()){slot=&v;break;}
 if(!slot){slot=&voices[0];for(auto& v:voices)if(v.age<slot->age)slot=&v;}
 auto pick=current->pick(note,velocity,roundRobin);auto& v=*slot;v=Voice{};
 v.sample=pick.sample;v.root=pick.root;v.key=key;v.held=true;v.age=++ageCounter;v.delay=offset;
 v.semis=v.targetSemis=(float)(note-pick.root);
 v.pos=std::min((double)v.sample->audio.getNumSamples()-4,startSeconds*v.sample->rate);
 v.amp=.25f+.75f*velocity;v.attackStep=1.f/std::max(1.f,attackSeconds*(float)rate);
 v.palm=s.palm&&s.palmAmount>5;
 if(v.palm){v.palmDecay=std::exp(-1.f/(float)((.09f+(100-s.palmAmount)*.004f)*rate));}
 // Velocity opens the tone like a harder pick; palm mute closes it.
 float cutoff=v.palm?400+(100-s.palmAmount)*35:(current->isSynth()?1800+velocity*9000:3500+velocity*16500);
 v.lpCoef=1-std::exp(-6.2831853f*std::min(cutoff,(float)rate*.45f)/(float)rate);
 if(s.humanize)v.detune=rnd()*s.humAmount*.06f;
 if(s.vibrato||modWheel>0){v.vibDepth=1;v.vibRamp=0;}
 return v;}

void GuitarEngine::noteOn(int note,float velocity,int offset,const PlaySettings& s){
 if(!current)return;
 keys[(size_t)note]=true;sustained[(size_t)note]=false;
 int humanDelay=0;
 if(s.humanize){velocity=juce::jlimit(.05f,1.f,velocity+rnd()*s.humAmount/400);humanDelay=(int)(std::abs(rnd())*s.humAmount*.00015f*rate);}
 lastNote=note;lastVelocity=(int)std::round(velocity*127);lastFrom=-1;lastChord=-1;
 if(s.strum){
  auto& shape=chordShapes[(size_t)juce::jlimit(0,3,s.chord)];int root=note;while(root+shape.back()>92)root-=12;
  for(auto& v:voices)if(v.active()&&!v.releasing)release(v,.05f);
  bool up=nextStrumUp;nextStrumUp=!nextStrumUp;int gap=(int)(strumGapSeconds(s.strumSpeed)*rate);
  for(int i=0;i<5;++i){int n=root+shape[(size_t)(up?4-i:i)];int jitter=s.humanize?(int)(std::abs(rnd())*s.humAmount*.0001f*rate):0;
   auto& v=startVoice(note,n,velocity*(1-.06f*(float)i),offset+humanDelay+i*gap+jitter,s);v.key=note;}
  // startVoice damps same-key voices; the 5 chord voices share the key, so re-hold them.
  for(auto& v:voices)if(v.active()&&v.key==note&&v.age>ageCounter-5){v.releasing=false;v.held=true;}
  lastChord=root;lastArticulation=Articulation::strum;return;}
 auto* held=heldVoice();
 if(s.slide&&held&&held->key!=note){int from=held->key;
  if(std::abs(note-from)<=12){
   keys[(size_t)from]=false;held->key=note;held->targetSemis=held->semis+(float)(note-from);
   held->glideCoef=1-std::exp(-1.f/(slideSeconds(s.slideTime)*.35f*(float)rate));
   lastFrom=from;lastArticulation=Articulation::slide;return;}}
 if(s.legato&&held&&held->key!=note){
  int from=held->key;release(*held,.02f);float soft=s.legatoSoft/100;
  // Skip the pick transient and fade in: the fretting finger excites the string, not the pick.
  auto& v=startVoice(note,note,velocity*(1-soft*.4f),offset,s,.004f+soft*.03f,.002f+soft*.025f);(void)v;
  lastFrom=from;lastArticulation=note>from?Articulation::hammer:Articulation::pulloff;return;}
 startVoice(note,note,velocity,offset+humanDelay,s);
 lastArticulation=s.palm?Articulation::palm:Articulation::pluck;}

void GuitarEngine::noteOff(int note){
 keys[(size_t)note]=false;
 if(sustainPedal){sustained[(size_t)note]=true;return;}
 for(auto& v:voices)if(v.active()&&v.key==note&&!v.releasing)release(v,.12f);}

void GuitarEngine::renderVoice(Voice& v,juce::AudioBuffer<float>& out,int start,int count,const PlaySettings& s){
 auto& a=v.sample->audio;int length=a.getNumSamples(),channels=a.getNumChannels(),outChannels=out.getNumChannels();
 const float* src[2]={a.getReadPointer(0),a.getReadPointer(std::min(1,channels-1))};
 double base=v.sample->rate/rate;float vibAmount=(s.vibrato?s.vibDepth/100*.35f:0)+modWheel*.35f;
 float vibStep=6.2831853f*5.2f/(float)rate,vibRampStep=1.f/(.45f*(float)rate);
 for(int i=start;i<start+count;++i){
  if(v.delay>0){--v.delay;continue;}
  v.semis+=(v.targetSemis-v.semis)*v.glideCoef;
  float vib=0;if(vibAmount>0){v.vibPhase+=vibStep;if(v.vibPhase>6.2831853f)v.vibPhase-=6.2831853f;v.vibRamp=std::min(1.f,v.vibRamp+vibRampStep);vib=std::sin(v.vibPhase)*vibAmount*v.vibRamp;}
  double step=base*std::exp2((v.semis+bend+vib+v.detune/100)/12.);
  int p=(int)v.pos;if(p>=length-3){v.sample=nullptr;return;}
  float t=(float)(v.pos-p);
  if(!v.releasing){v.gain=std::min(1.f,v.gain+v.attackStep);if(v.palm)v.amp*=v.palmDecay;}else{v.gain*=v.releaseCoef;if(v.gain<1e-4f){v.sample=nullptr;return;}}
  float g=v.gain*v.amp;
  for(int c=0;c<2;++c){const float* x=src[c];float xm=p>0?x[p-1]:x[p],x0=x[p],x1=x[p+1],x2=x[p+2];
   // 4-point Hermite interpolation keeps pitched-down samples clean.
   float c1=.5f*(x1-xm),c2=xm-2.5f*x0+2*x1-.5f*x2,c3=.5f*(x2-xm)+1.5f*(x0-x1);float y=((c3*t+c2)*t+c1)*t+x0;
   v.lp1[c]+=v.lpCoef*(y-v.lp1[c]);v.lp2[c]+=v.lpCoef*(v.lp1[c]-v.lp2[c]);
   if(c<outChannels)out.addSample(c,i,v.lp2[c]*g);}
  v.pos+=step;}}

void GuitarEngine::render(juce::AudioBuffer<float>& out,const juce::MidiBuffer& midi,const PlaySettings& s){
 auto incoming=std::atomic_load(&pending);
 if(incoming!=current){for(auto& v:voices)v.sample=nullptr;current=incoming;}
 int pos=0,total=out.getNumSamples();
 auto renderUntil=[&](int end){if(end>pos&&current)for(auto& v:voices)if(v.active())renderVoice(v,out,pos,end-pos,s);pos=std::max(pos,end);};
 for(const auto meta:midi){auto m=meta.getMessage();int at=juce::jlimit(0,total,meta.samplePosition);renderUntil(at);
  if(m.isNoteOn())noteOn(m.getNoteNumber(),m.getFloatVelocity(),0,s);
  else if(m.isNoteOff())noteOff(m.getNoteNumber());
  else if(m.isPitchWheel())bend=(float)(m.getPitchWheelValue()-8192)/8192*2;
  else if(m.isController()&&m.getControllerNumber()==1)modWheel=(float)m.getControllerValue()/127;
  else if(m.isSustainPedalOn())sustainPedal=true;
  else if(m.isSustainPedalOff()){sustainPedal=false;for(int n=0;n<128;++n)if(sustained[(size_t)n]){sustained[(size_t)n]=false;noteOff(n);}}
  else if(m.isAllNotesOff()||m.isAllSoundOff())allNotesOff();}
 renderUntil(total);
 std::array<uint32_t,4> bits{};for(auto& v:voices)if(v.active()&&!v.releasing&&v.key>=0){int n=juce::jlimit(0,127,v.root+(int)std::lround(v.semis));bits[(size_t)(n>>5)]|=1u<<(n&31);}
 for(size_t i=0;i<4;++i)sounding[i]=bits[i];voiceCount=activeVoices();}
