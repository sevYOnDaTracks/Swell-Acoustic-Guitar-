#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

juce::AudioProcessorValueTreeState::ParameterLayout GuitarProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;auto percent=juce::NormalisableRange<float>(0,100,.1f);
 for(auto& m:modules){
  l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{m.id,1},juce::String(m.name).toLowerCase(),juce::String(m.id)=="humanize"));
  l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{m.knob.id,1},juce::String(m.name).toLowerCase()+" "+juce::String(m.knob.label).toLowerCase(),percent,m.knob.def));}
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"guitarType",1},"guitare",juce::StringArray{juce::CharPointer_UTF8(guitarTypes[0]),juce::CharPointer_UTF8(guitarTypes[1])},0));
 for(int t=0;t<guitarTypeCount;++t){juce::StringArray names;for(int i=0;i<modelsOfType(t);++i)names.add(models[(size_t)(modelFirst(t)+i)].name);
  l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{modelParams[(size_t)t],1},t==0?"modele acoustique":"modele electrique",names,defaultModel(t)));}
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"chord",1},"strum chord",juce::StringArray(chordNames.data(),(int)chordNames.size()),0));
 for(auto& k:fxKnobs)l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{k.id,1},k.id,percent,k.def));
 return l;}

GuitarProcessor::GuitarProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"SwellAcousticGuitar",layout()){
 for(auto& m:builtInModel)m=-1;
 // The default models decode here so the first block already has sound; later changes load on the thread.
 updateModels();engine.setLibrary(std::atomic_load(&builtIn[0]));
 for(int t=0;t<guitarTypeCount;++t){auto folder=defaultSampleFolder(t);if(folder.isDirectory())loadSamples(t,folder);}
 modelLoader.startThread();}

GuitarProcessor::~GuitarProcessor(){modelLoader.stopThread(4000);loader.removeAllJobs(true,5000);}

int GuitarProcessor::selectedModel(int type)const{return modelFirst(type)+juce::jlimit(0,modelsOfType(type)-1,(int)value(modelParams[(size_t)type]));}

void GuitarProcessor::selectModel(int type,int menuIndex){
 useLibrary(type,nullptr);{const juce::ScopedLock l(statusLock);folderPath[(size_t)type]={};}
 setValue(modelParams[(size_t)type],(float)menuIndex);builtInModel[(size_t)type]=-1;modelLoader.notify();}

std::shared_ptr<const SampleLibrary> GuitarProcessor::loadModel(int index){
 for(auto& c:modelCache)if(c.first==index)return c.second;
 auto& m=models[(size_t)index];std::shared_ptr<const SampleLibrary> lib;
 for(int i=0;i<BinaryData::namedResourceListSize;++i)
  if(juce::String(BinaryData::getNamedResourceOriginalFilename(BinaryData::namedResourceList[i]))==m.file){
   int size=0;auto* data=BinaryData::getNamedResource(BinaryData::namedResourceList[i],size);
   lib=SampleLibrary::single(data,(size_t)size,m.root,m.cents,juce::String(m.name)+juce::String(juce::CharPointer_UTF8(" — sample intégré")));break;}
 // A broken embedded file should never leave the plugin silent.
 if(!lib){if(!synth)synth=SampleLibrary::synth();return synth;}
 // Keep the few last models decoded so going back and forth is instant.
 modelCache.insert(modelCache.begin(),{index,lib});if(modelCache.size()>6)modelCache.pop_back();
 return lib;}

void GuitarProcessor::updateModels(){
 for(int t=0;t<guitarTypeCount;++t){int want=selectedModel(t);if(want==builtInModel[(size_t)t])continue;
  auto lib=loadModel(want);std::atomic_store(&builtIn[(size_t)t],lib);builtInModel[(size_t)t]=want;
  if(!std::atomic_load(&user[(size_t)t]))setStatus(t,lib->name);}}

bool GuitarProcessor::waitForModels(int timeoutMs){auto end=juce::Time::getMillisecondCounter()+(juce::uint32)timeoutMs;
 while(juce::Time::getMillisecondCounter()<end){bool ready=true;for(int t=0;t<guitarTypeCount;++t)ready&=builtInModel[(size_t)t]==selectedModel(t);if(ready)return true;juce::Thread::sleep(5);}
 return false;}

bool GuitarProcessor::isBusesLayoutSupported(const BusesLayout& l) const{auto o=l.getMainOutputChannelSet();return o==juce::AudioChannelSet::stereo()||o==juce::AudioChannelSet::mono();}

void GuitarProcessor::prepareToPlay(double sr,int block){engine.prepare(sr);fx.prepare(sr,block);keyboard.reset();}

PlaySettings GuitarProcessor::settings()const{PlaySettings s;
 s.slide=value("slide")>.5f;s.legato=value("legato")>.5f;s.vibrato=value("vibrato")>.5f;s.strum=value("strum")>.5f;s.humanize=value("humanize")>.5f;s.palm=value("palm")>.5f;
 s.slideTime=value("slideTime");s.legatoSoft=value("legatoSoft");s.vibDepth=value("vibDepth");s.strumSpeed=value("strumSpeed");s.humAmount=value("humAmount");s.palmAmount=value("palmAmount");
 s.chord=(int)value("chord");return s;}

void GuitarProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;buffer.clear();
 keyboard.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
 int type=guitarType();auto lib=std::atomic_load(&user[(size_t)type]);if(!lib)lib=std::atomic_load(&builtIn[(size_t)type]);
 if(lib!=engine.library())engine.setLibrary(lib);
 engine.render(buffer,midi,settings());
 FxChain::Values v;v.electric=type==1;float* f[]={&v.drive,&v.tone,&v.comp,&v.low,&v.mid,&v.high,&v.dtime,&v.dmix,&v.size,&v.rmix,&v.vol};
 for(int i=0;i<fxKnobCount;++i)*f[i]=value(fxKnobs[(size_t)i].id);
 fx.process(buffer,v);midi.clear();}

void GuitarProcessor::setValue(const char* id,float v){if(auto* p=state.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(v));}

void GuitarProcessor::setCurrentProgram(int i){i=juce::jlimit(0,presetCount-1,i);program=i;auto& p=presets[(size_t)i];
 setValue("guitarType",(float)p.type);
 for(int k=0;k<fxKnobCount;++k)setValue(fxKnobs[(size_t)k].id,p.fx[(size_t)k]);
 for(int m=0;m<moduleCount;++m)setValue(modules[(size_t)m].id,p.on[(size_t)m]?1.f:0.f);}

juce::File GuitarProcessor::defaultSampleFolder(int type){return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Swell").getChildFile(type==1?"Electric Guitar":"Acoustic Guitar").getChildFile("Samples");}
juce::File GuitarProcessor::sampleFolder()const{int t=guitarType();const juce::ScopedLock l(statusLock);return folderPath[(size_t)t].isEmpty()?defaultSampleFolder(t):juce::File(folderPath[(size_t)t]);}

void GuitarProcessor::useLibrary(int type,std::shared_ptr<const SampleLibrary> lib){std::atomic_store(&user[(size_t)type],std::move(lib));}

void GuitarProcessor::loadSamples(int type,const juce::File& folder){
 {const juce::ScopedLock l(statusLock);folderPath[(size_t)type]=folder.getFullPathName();status[(size_t)type]="Chargement des samples...";}
 loader.addJob([this,type,folder]{juce::String report;std::shared_ptr<const SampleLibrary> lib=SampleLibrary::load(folder,report);
  if(lib){useLibrary(type,lib);setStatus(type,report);}else setStatus(type,report+juce::String(juce::CharPointer_UTF8("  -  sample intégré actif")));});}

void GuitarProcessor::getStateInformation(juce::MemoryBlock& data){auto s=state.copyState();s.setProperty("program",program.load(),nullptr);
 {const juce::ScopedLock l(statusLock);s.setProperty("sampleFolder",folderPath[0],nullptr);s.setProperty("sampleFolderElectric",folderPath[1],nullptr);}
 if(auto xml=s.createXml())copyXmlToBinary(*xml,data);}
void GuitarProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size)){auto s=juce::ValueTree::fromXml(*xml);if(!s.isValid())return;
 program=juce::jlimit(0,presetCount-1,(int)s.getProperty("program",0));
 std::array<juce::String,guitarTypeCount> folders{s.getProperty("sampleFolder","").toString(),s.getProperty("sampleFolderElectric","").toString()};
 s.removeProperty("sampleFolder",nullptr);s.removeProperty("sampleFolderElectric",nullptr);state.replaceState(s);
 for(int t=0;t<guitarTypeCount;++t){juce::String current;{const juce::ScopedLock l(statusLock);current=folderPath[(size_t)t];}
  if(folders[(size_t)t].isNotEmpty()&&folders[(size_t)t]!=current&&juce::File(folders[(size_t)t]).isDirectory())loadSamples(t,juce::File(folders[(size_t)t]));}}}

juce::AudioProcessorEditor* GuitarProcessor::createEditor(){return new GuitarEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GuitarProcessor();}
