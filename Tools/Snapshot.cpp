#include "../Source/PluginProcessor.h"
// Renders the editor to a PNG while a chord rings: Snapshot <out.png> [preset] [strum 0/1]
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;GuitarProcessor p;if(argc>2)p.setCurrentProgram(juce::String(argv[2]).getIntValue());
 bool strum=argc>3&&juce::String(argv[3])=="1";if(strum)p.setValue("strum",1);p.prepareToPlay(48000,256);
 juce::AudioBuffer<float> b(2,256);for(int i=0;i<20;++i){juce::MidiBuffer m;if(i==0){m.addEvent(juce::MidiMessage::noteOn(1,strum?48:52,.8f),0);}p.processBlock(b,m);}
 std::unique_ptr<juce::AudioProcessorEditor> e(p.createEditor());
 auto image=e->createComponentSnapshot(e->getLocalBounds(),true,1.f);
 juce::File out(juce::File::getCurrentWorkingDirectory().getChildFile(argc>1?argv[1]:"snapshot.png"));out.deleteFile();juce::FileOutputStream s(out);juce::PNGImageFormat().writeImageToStream(image,s);return 0;}
