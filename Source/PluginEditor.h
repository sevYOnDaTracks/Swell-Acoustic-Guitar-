#pragma once
#include "PluginProcessor.h"

class GuitarLook:public juce::LookAndFeel_V4 {public:GuitarLook();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool)override;
 void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&)override;};

// Small pill switch, orange when on.
class Switch:public juce::Button {public:Switch():juce::Button("switch"){setClickingTogglesState(true);}void paintButton(juce::Graphics&,bool,bool)override;};

// Thin-ring knob with caption and value, under it or beside it (module columns).
class Knob:public juce::Component {
public:Knob(GuitarProcessor&,const char* id,juce::String caption,std::function<juce::String(float)> format,bool side=false);
 void paint(juce::Graphics&)override;void resized()override;juce::Slider slider;bool active=true;
private:juce::String caption;std::function<juce::String(float)> format;bool side;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;};

// Which string plays each note: chords take one string each from the low E up, a single note the lowest fret.
struct StringPos {int string,fret;};
std::vector<StringPos> assignStrings(std::vector<int> notes);

// C2..B5 keyboard: shows what is sounding and plays the engine when clicked.
class Keys:public juce::Component {public:explicit Keys(GuitarProcessor& p):proc(p){}void paint(juce::Graphics&)override;
 void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;void mouseUp(const juce::MouseEvent&)override;
 static constexpr int low=36,high=83;
private:int noteAt(juce::Point<float>,float& velocity)const;juce::Rectangle<float> keyRect(int note)const;
 GuitarProcessor& proc;int down=-1;};

class GuitarEditor:public juce::AudioProcessorEditor,private juce::Timer {
public:explicit GuitarEditor(GuitarProcessor&);~GuitarEditor()override;void paint(juce::Graphics&)override;void resized()override;
private:void timerCallback()override;void chooseFolder();void paintHero(juce::Graphics&);
 GuitarProcessor& proc;GuitarLook look;std::array<juce::Image,guitarTypeCount> photos;juce::Image grain;
 juce::ComboBox guitar,preset;juce::TextButton loadButton{"Samples..."};
 std::array<Switch,moduleCount> switches;std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchAttach;
 std::vector<std::unique_ptr<Knob>> moduleKnobs,fxKnobsUi;std::array<juce::TextButton,4> chordButtons;
 Keys keys{proc};std::unique_ptr<juce::FileChooser> chooser;
 std::array<float,6> glow{};float phase=0;int shownProgram=-1,shownType=-1;
 juce::String noteText="E2",subText="Joue une note (clavier MIDI ou clic)",articText{juce::CharPointer_UTF8("PRÊT")};
};
