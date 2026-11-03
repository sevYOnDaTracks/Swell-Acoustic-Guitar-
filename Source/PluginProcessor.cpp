#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout GuitarProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;auto percent=juce::NormalisableRange<float>(0,100,.1f);
 for(auto& m:modules){
  l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{m.id,1},juce::String(m.name).toLowerCase(),juce::String(m.id)=="humanize"));
  l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{m.knob.id,1},juce::String(m.name).toLowerCase()+" "+juce::String(m.knob.label).toLowerCase(),percent,m.knob.def));}
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"chord",1},"strum chord",juce::StringArray(chordNames.data(),(int)chordNames.size()),0));
 for(auto& k:fxKnobs)l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k.id,1},k.id,percent,k.def));
 return l;}

GuitarProcessor::GuitarProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"SwellAcousticGuitar",layout()){
 synthLibrary=SampleLibrary::synth();engine.setLibrary(synthLibrary);status=synthLibrary->name;
 auto folder=defaultSampleFolder();if(folder.isDirectory())loadSamples(folder);}

GuitarProcessor::~GuitarProcessor(){loader.removeAllJobs(true,5000);}

bool GuitarProcessor::isBusesLayoutSupported(const BusesLayout& l) const{auto o=l.getMainOutputChannelSet();return o==juce::AudioChannelSet::stereo()||o==juce::AudioChannelSet::mono();}

void GuitarProcessor::prepareToPlay(double sr,int block){engine.prepare(sr);fx.prepare(sr,block);keyboard.reset();}

PlaySettings GuitarProcessor::settings()const{PlaySettings s;
 s.slide=value("slide")>.5f;s.legato=value("legato")>.5f;s.vibrato=value("vibrato")>.5f;s.strum=value("strum")>.5f;s.humanize=value("humanize")>.5f;s.palm=value("palm")>.5f;
 s.slideTime=value("slideTime");s.legatoSoft=value("legatoSoft");s.vibDepth=value("vibDepth");s.strumSpeed=value("strumSpeed");s.humAmount=value("humAmount");s.palmAmount=value("palmAmount");
 s.chord=(int)value("chord");return s;}

void GuitarProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;buffer.clear();
 keyboard.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
 engine.render(buffer,midi,settings());
 FxChain::Values v;float* f[]={&v.drive,&v.tone,&v.comp,&v.low,&v.mid,&v.high,&v.dtime,&v.dmix,&v.size,&v.rmix,&v.vol};
 for(int i=0;i<fxKnobCount;++i)*f[i]=value(fxKnobs[(size_t)i].id);
 fx.process(buffer,v);midi.clear();}

void GuitarProcessor::setValue(const char* id,float v){if(auto* p=state.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(v));}

void GuitarProcessor::setCurrentProgram(int i){i=juce::jlimit(0,presetCount-1,i);program=i;auto& p=presets[(size_t)i];
 for(int k=0;k<fxKnobCount;++k)setValue(fxKnobs[(size_t)k].id,p.fx[(size_t)k]);
 for(int m=0;m<moduleCount;++m)setValue(modules[(size_t)m].id,p.on[(size_t)m]?1.f:0.f);}

juce::File GuitarProcessor::defaultSampleFolder(){return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Swell").getChildFile("Acoustic Guitar").getChildFile("Samples");}
juce::File GuitarProcessor::sampleFolder()const{const juce::ScopedLock l(statusLock);return folderPath.isEmpty()?defaultSampleFolder():juce::File(folderPath);}

void GuitarProcessor::loadSamples(const juce::File& folder){
 {const juce::ScopedLock l(statusLock);folderPath=folder.getFullPathName();status="Chargement des samples...";}
 loader.addJob([this,folder]{juce::String report;std::shared_ptr<const SampleLibrary> lib=SampleLibrary::load(folder,report);
  if(lib){engine.setLibrary(lib);loaded=lib;setStatus(report);}else setStatus(report+"  -  synthese provisoire active");});}

void GuitarProcessor::getStateInformation(juce::MemoryBlock& data){auto s=state.copyState();s.setProperty("program",program.load(),nullptr);
 {const juce::ScopedLock l(statusLock);s.setProperty("sampleFolder",folderPath,nullptr);}
 if(auto xml=s.createXml())copyXmlToBinary(*xml,data);}
void GuitarProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size)){auto s=juce::ValueTree::fromXml(*xml);if(!s.isValid())return;
 program=juce::jlimit(0,presetCount-1,(int)s.getProperty("program",0));juce::String folder=s.getProperty("sampleFolder","");s.removeProperty("sampleFolder",nullptr);state.replaceState(s);
 if(folder.isNotEmpty()&&folder!=sampleFolder().getFullPathName()&&juce::File(folder).isDirectory())loadSamples(juce::File(folder));}}

juce::AudioProcessorEditor* GuitarProcessor::createEditor(){return new GuitarEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GuitarProcessor();}
