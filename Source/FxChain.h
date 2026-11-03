#pragma once
#include "Params.h"
#include <juce_dsp/juce_dsp.h>

// Acoustic: preamp -> body -> 3-band EQ -> compressor -> delay -> reverb -> output. Values are 0..100.
// Electric: the preamp becomes a hotter, asymmetric amp and the body resonance a 1x12 cabinet (high-pass, presence, low-pass).
class FxChain {
public:
 struct Values {float drive=15,tone=55,comp=35,low=55,mid=50,high=55,dtime=35,dmix=10,size=45,rmix=22,vol=75;bool electric=false;};
 void prepare(double sampleRate,int block){
  rate=sampleRate;juce::dsp::ProcessSpec spec{sampleRate,(juce::uint32)block,2};
  tone=Coeffs::makeLowPass(rate,1000);body=Coeffs::makePeakFilter(rate,210,1.1f,juce::Decibels::decibelsToGain(3.f));
  cabHp=Coeffs::makeHighPass(rate,90,.7f);presence=Coeffs::makePeakFilter(rate,1800,.9f,juce::Decibels::decibelsToGain(4.f));cabLp=Coeffs::makeLowPass(rate,std::min(5200.,rate*.45),.8f);
  low=Coeffs::makeLowShelf(rate,180,.7f,1);mid=Coeffs::makePeakFilter(rate,1100,.8f,1);high=Coeffs::makeHighShelf(rate,5200,.7f,1);
  for(int c=0;c<2;++c){auto& f=filters[(size_t)c];size_t i=0;for(auto& k:{tone,body,cabHp,presence,cabLp,low,mid,high})f[i++].coefficients=k;for(auto& x:f)x.reset();}
  comp.prepare(spec);comp.setAttack(6);comp.setRelease(180);reverb.prepare(spec);reverb.reset();
  delay.setSize(2,(int)(sampleRate*1.2)+8);delay.clear();writePos=0;delayTail=0;dlp[0]=dlp[1]=0;
  last=Values{};last.tone=last.low=last.mid=last.high=last.comp=last.size=-1;}
 void process(juce::AudioBuffer<float>& b,const Values& v){
  update(v);int n=b.getNumSamples(),channels=std::min(2,b.getNumChannels());
  if(v.electric!=wasElectric){wasElectric=v.electric;for(auto& f:filters)for(auto& x:f)x.reset();}
  if(v.electric){
   // Up to 60x gain; the bias adds even harmonics like a tube stage, the cabinet high-pass removes its DC.
   float k=1+v.drive*.6f,norm=std::pow(k,-.34f),bias=.2f,offset=std::tanh(bias);
   for(int c=0;c<channels;++c){auto* x=b.getWritePointer(c);auto& f=filters[(size_t)c];
    for(int i=0;i<n;++i){float y=(std::tanh(k*x[i]+bias)-offset)*norm;for(int s:{0,2,3,4,5,6,7})y=f[(size_t)s].processSample(y);x[i]=y;}}}
  else{float k=1+v.drive*.15f,norm=std::pow(k,-.6f);
   for(int c=0;c<channels;++c){auto* x=b.getWritePointer(c);auto& f=filters[(size_t)c];
    for(int i=0;i<n;++i){float y=std::tanh(k*x[i])*norm;for(int s:{0,1,5,6,7})y=f[(size_t)s].processSample(y);x[i]=y;}}}
  juce::dsp::AudioBlock<float> block(b);comp.process(juce::dsp::ProcessContextReplacing<float>(block));
  b.applyGain(juce::Decibels::decibelsToGain(v.comp*.08f));
  float mix=v.dmix/100*.7f;
  if(mix>0||delayTail>0){int length=delay.getNumSamples(),d=juce::jlimit(1,length-1,(int)(delaySeconds(v.dtime)*rate));float lp=1-std::exp(-6.2831853f*3800/(float)rate);
   for(int i=0;i<n;++i){int readPos=(writePos-d+length)%length;
    for(int c=0;c<channels;++c){float echo=delay.getSample(c,readPos);dlp[c]+=lp*(echo-dlp[c]);float in=b.getSample(c,i);delay.setSample(c,writePos,in*mix+dlp[c]*.32f);b.setSample(c,i,in+dlp[c]);}
    writePos=(writePos+1)%length;}
   delayTail=mix>0?(int)(rate*4):std::max(0,delayTail-n);}
  reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
  b.applyGain(outputGain(v.vol));}
private:
 using Coeffs=juce::dsp::IIR::Coefficients<float>;
 void update(const Values& v){
  if(v.tone!=last.tone)*tone=*Coeffs::makeLowPass(rate,std::min(1500*std::pow(12.f,v.tone/100),(float)rate*.45f),.6f);
  if(v.low!=last.low)*low=*Coeffs::makeLowShelf(rate,180,.7f,juce::Decibels::decibelsToGain(eqDb(v.low)));
  if(v.mid!=last.mid)*mid=*Coeffs::makePeakFilter(rate,1100,.8f,juce::Decibels::decibelsToGain(eqDb(v.mid)));
  if(v.high!=last.high)*high=*Coeffs::makeHighShelf(rate,5200,.7f,juce::Decibels::decibelsToGain(eqDb(v.high)));
  if(v.comp!=last.comp){comp.setThreshold(-6-v.comp*.34f);comp.setRatio(1.5f+v.comp*.08f);}
  if(v.size!=last.size||v.rmix!=last.rmix){juce::dsp::Reverb::Parameters p;p.roomSize=.3f+v.size*.0068f;p.damping=.45f;p.wetLevel=v.rmix/100*.6f;p.dryLevel=1;p.width=1;reverb.setParameters(p);}
  last=v;}
 double rate=44100;Values last;
 Coeffs::Ptr tone,body,cabHp,presence,cabLp,low,mid,high;bool wasElectric=false;
 std::array<std::array<juce::dsp::IIR::Filter<float>,8>,2> filters;
 juce::dsp::Compressor<float> comp;juce::dsp::Reverb reverb;
 juce::AudioBuffer<float> delay;int writePos=0,delayTail=0;float dlp[2]{};
};
