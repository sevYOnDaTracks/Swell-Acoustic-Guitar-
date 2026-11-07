#include "PluginEditor.h"
#include "BinaryData.h"
#include <random>

namespace {
const juce::Colour bg{0xff0b0b0c},ink{0xffededee},soft{0xffd9d9dc},muted{0xff8e8e94},faint{0xff6e6e74},rule{0xff1f1f22},track{0xff26262a},orange{0xffff8a3d},orangeLight{0xffffb27a};
juce::String utf8(const char* s){return juce::String(juce::CharPointer_UTF8(s));}
void drawLabel(juce::Graphics& g,const juce::String& s,float x,float y,float w,float h,float size,juce::Colour c,juce::Justification j=juce::Justification::centredLeft,bool bold=false,float tracking=0){
 g.setColour(c);g.setFont(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain).withKerningFactor(tracking));g.drawText(s,juce::Rectangle<float>(x,y,w,h),j);}
juce::String noteName(int n){return juce::MidiMessage::getMidiNoteName(n,true,true,4);}
constexpr std::array<int,6> openStrings{64,59,55,50,45,40};
constexpr std::array<const char*,6> stringNames{"e","B","G","D","A","E"};
// One photo per guitar type (1280x853, black and white). Strings e..E in image pixels: lit segment start, end;
// 'top' places the photo vertically, 'label' is the string name's offset from the segment start.
struct HeroPhoto {std::array<std::array<float,4>,6> strings;float top;juce::Point<float> label;};
const std::array<HeroPhoto,guitarTypeCount> heroPhotos{{
 {{{{478,604,1258,40},{545,604,1277,40},{612,604,1298,40},{682,604,1318,40},{752,604,1340,40},{822,604,1352,40}}},-150,{0,24}},
 {{{{607,545,907,285},{594,517,898,264},{580,488,888,244},{567,459,879,223},{554,430,869,203},{541,402,860,182}}},-127,{-26,14}}}};
constexpr float photoScale=1180.f/1280;
juce::Point<float> photoToScreen(const HeroPhoto& p,float x,float y){return {x*photoScale,y*photoScale+p.top};}
const char* articulationName(Articulation a){switch(a){case Articulation::pluck:return "PINCÉ";case Articulation::slide:return "SLIDE";case Articulation::hammer:return "HAMMER-ON";case Articulation::pulloff:return "PULL-OFF";case Articulation::strum:return "STRUM";case Articulation::palm:return "PALM MUTE";default:return "PRÊT";}}
}

std::vector<StringPos> assignStrings(std::vector<int> notes){
 std::sort(notes.begin(),notes.end());std::vector<StringPos> out;int used=6;
 for(int n:notes){int best=-1,bestFret=99;
  for(int s=used-1;s>=0;--s){int f=n-openStrings[(size_t)s];if(f>=0&&f<=15&&(notes.size()>1?best<0:f<bestFret)){best=s;bestFret=f;}}
  if(best<0)continue;used=best;out.push_back({best,bestFret});}
 return out;}

GuitarLook::GuitarLook(){
 setColour(juce::TextButton::buttonColourId,juce::Colours::transparentBlack);setColour(juce::TextButton::buttonOnColourId,orange);setColour(juce::TextButton::textColourOffId,soft);setColour(juce::TextButton::textColourOnId,bg);
 setColour(juce::ComboBox::backgroundColourId,juce::Colour(0x8c0b0b0c));setColour(juce::ComboBox::textColourId,ink);setColour(juce::ComboBox::arrowColourId,orange);setColour(juce::ComboBox::outlineColourId,juce::Colours::transparentBlack);
 setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff141416));setColour(juce::PopupMenu::textColourId,ink);setColour(juce::PopupMenu::highlightedBackgroundColourId,orange);setColour(juce::PopupMenu::highlightedTextColourId,bg);
 setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);}
void GuitarLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& s){
 auto r=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h);float size=std::min(r.getWidth(),r.getHeight());r=r.withSizeKeepingCentre(size,size);
 auto c=r.getCentre();float radius=size/2-4;bool on=s.getProperties().getWithDefault("active",true);auto accent=on?orange:juce::Colour(0xff5a5a60);
 juce::Path t,a;t.addCentredArc(c.x,c.y,radius,radius,0,start,end,true);a.addCentredArc(c.x,c.y,radius,radius,0,start,start+pos*(end-start),true);
 g.setColour(track);g.strokePath(t,juce::PathStrokeType(2,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 g.setColour(accent);g.strokePath(a,juce::PathStrokeType(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 auto dot=c.getPointOnCircumference(radius,start+pos*(end-start));g.fillEllipse(dot.x-3.5f,dot.y-3.5f,7,7);}
void GuitarLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool){
 auto r=b.getLocalBounds().toFloat().reduced(.5f);float radius=r.getHeight()/2;bool on=b.getToggleState();
 if(on){g.setColour(orange);g.fillRoundedRectangle(r,radius);}else{g.setColour(over?juce::Colour(0x14ffffff):juce::Colours::transparentBlack);g.fillRoundedRectangle(r,radius);g.setColour(juce::Colour(0xff3a3a40));g.drawRoundedRectangle(r,radius,1);}}
void GuitarLook::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& box){
 g.setColour(box.findColour(juce::ComboBox::backgroundColourId));g.fillRect(0,0,w,h);
 g.setColour(box.getProperties().getWithDefault("accent",false)?orange:juce::Colour(0xff5a5a60));g.fillRect(0,h-1,w,1);
 juce::Path arrow;arrow.addTriangle((float)w-16,(float)h/2-2,(float)w-8,(float)h/2-2,(float)w-12,(float)h/2+3);g.setColour(orange);g.fillPath(arrow);}

void Switch::paintButton(juce::Graphics& g,bool,bool){bool on=getToggleState();auto r=getLocalBounds().toFloat().withSizeKeepingCentre(30,16);
 if(on){g.setColour(orange);g.fillRoundedRectangle(r,8);}g.setColour(on?orange:juce::Colour(0xff4a4a50));g.drawRoundedRectangle(r,8,1);
 g.setColour(on?bg:faint);g.fillEllipse(on?r.getRight()-13:r.getX()+3,r.getY()+3,10,10);}

Knob::Knob(GuitarProcessor& p,const char* id,juce::String c,std::function<juce::String(float)> f,bool s):caption(std::move(c)),format(std::move(f)),side(s){
 slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
 attach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,id,slider);
 slider.setDoubleClickReturnValue(true,p.state.getParameter(id)->convertFrom0to1(p.state.getParameter(id)->getDefaultValue()));
 slider.onValueChange=[this]{repaint();};addAndMakeVisible(slider);}
void Knob::resized(){if(side)slider.setBounds(0,0,46,46);else slider.setBounds((getWidth()-46)/2,0,46,46);}
void Knob::paint(juce::Graphics& g){auto value=format((float)slider.getValue());
 if(side){drawLabel(g,caption,56,7,getWidth()-56.f,14,10,muted,juce::Justification::centredLeft,false,.15f);drawLabel(g,value,56,22,getWidth()-56.f,18,13,active?ink:muted);}
 else{drawLabel(g,caption,0,49,(float)getWidth(),13,10,muted,juce::Justification::centred,false,.12f);drawLabel(g,value,0,62,(float)getWidth(),16,12,ink,juce::Justification::centred);}}

juce::Rectangle<float> Keys::keyRect(int note)const{
 float kw=(float)getWidth()/28;int white=0;for(int n=low;n<note;++n){int pc=n%12;if(pc!=1&&pc!=3&&pc!=6&&pc!=8&&pc!=10)++white;}
 int pc=note%12;bool black=pc==1||pc==3||pc==6||pc==8||pc==10;
 return black?juce::Rectangle<float>(white*kw-12,0,24,(float)getHeight()*.62f):juce::Rectangle<float>(white*kw,0,kw,(float)getHeight());}
int Keys::noteAt(juce::Point<float> p,float& velocity)const{
 for(int n=low;n<=high;++n){int pc=n%12;if((pc==1||pc==3||pc==6||pc==8||pc==10)&&keyRect(n).contains(p)){velocity=juce::jlimit(.15f,1.f,p.y/keyRect(n).getHeight()*1.15f);return n;}}
 for(int n=low;n<=high;++n){int pc=n%12;if(!(pc==1||pc==3||pc==6||pc==8||pc==10)&&keyRect(n).contains(p)){velocity=juce::jlimit(.15f,1.f,p.y/(float)getHeight()*1.15f);return n;}}
 return -1;}
void Keys::paint(juce::Graphics& g){
 auto lit=[this](int n){return proc.engine.isSounding(n)||proc.keyboard.isNoteOn(1,n);};
 for(int pass=0;pass<2;++pass)for(int n=low;n<=high;++n){int pc=n%12;bool black=pc==1||pc==3||pc==6||pc==8||pc==10;if(black!=(pass==1))continue;
  auto r=keyRect(n);bool on=lit(n),out=n<40;
  if(black){g.setColour(on?orange:juce::Colour(0xff060607));g.fillRoundedRectangle(r,3);g.setColour(juce::Colours::black);g.drawRoundedRectangle(r,3,1);}
  else{auto k=r.withTrimmedRight(1);g.setColour(on?orange:out?juce::Colour(0xff141416):juce::Colour(0xff1e1e21));g.fillRoundedRectangle(k,4);g.fillRect(k.withHeight(8));
   g.setColour(on?orangeLight:out?juce::Colour(0xff1a1a1c):juce::Colour(0xff2e2e33));g.fillRect(k.withHeight(2));
   if(pc==0)drawLabel(g,noteName(n),k.getX(),k.getBottom()-20,k.getWidth(),14,9,on?bg:faint,juce::Justification::centred);}}}
void Keys::mouseDown(const juce::MouseEvent& e){float v;down=noteAt(e.position,v);if(down>=0)proc.keyboard.noteOn(1,down,v);}
void Keys::mouseDrag(const juce::MouseEvent& e){float v;int n=noteAt(e.position,v);if(n!=down){
 // Press the new key before releasing the old one so Slide and Legato see the overlap.
 if(n>=0)proc.keyboard.noteOn(1,n,.7f);if(down>=0)proc.keyboard.noteOff(1,down,0);down=n;}}
void Keys::mouseUp(const juce::MouseEvent&){if(down>=0)proc.keyboard.noteOff(1,down,0);down=-1;}

GuitarEditor::GuitarEditor(GuitarProcessor& p):AudioProcessorEditor(p),proc(p){
 setLookAndFeel(&look);photos[0]=juce::ImageCache::getFromMemory(BinaryData::guitarphoto_jpg,BinaryData::guitarphoto_jpgSize);
 photos[1]=juce::ImageCache::getFromMemory(BinaryData::guitarphotoelectric_jpg,BinaryData::guitarphotoelectric_jpgSize);
 // Film grain, generated once.
 grain=juce::Image(juce::Image::ARGB,590,225,true);{std::mt19937 r(3);std::uniform_int_distribution<int> d(0,255);for(int y=0;y<grain.getHeight();++y)for(int x=0;x<grain.getWidth();++x){int v=d(r);grain.setPixelAt(x,y,juce::Colour((juce::uint8)v,(juce::uint8)v,(juce::uint8)v,(juce::uint8)18));}}
 for(int t=0;t<guitarTypeCount;++t)guitar.addItem(utf8(guitarTypes[(size_t)t]),t+1);
 model.onChange=[this]{int i=model.getSelectedId()-1;if(i>=0)proc.selectModel(proc.guitarType(),i);};
 // Changing guitar loads that guitar's first preset, so the amp and EQ suit it straight away.
 guitar.onChange=[this]{int t=guitar.getSelectedId()-1;if(t>=0&&t!=proc.guitarType())proc.setCurrentProgram(firstPreset(t));};
 for(int t=0;t<guitarTypeCount;++t){preset.addSectionHeading(utf8(guitarTypes[(size_t)t]));for(int i=0;i<presetCount;++i)if(presets[(size_t)i].type==t)preset.addItem(utf8(presets[(size_t)i].name),i+1);}
 preset.getProperties().set("accent",true);
 preset.onChange=[this]{if(preset.getSelectedId()>0)proc.setCurrentProgram(preset.getSelectedId()-1);};
 loadButton.onClick=[this]{chooseFolder();};
 auto pct=[](float v){return juce::String((int)std::round(v))+" %";};
 std::array<std::function<juce::String(float)>,moduleCount> moduleFormats{[](float v){return juce::String((int)std::round(slideSeconds(v)*1000))+" ms";},pct,pct,[](float v){return juce::String((int)std::round(strumGapSeconds(v)*1000))+" ms";},pct,pct};
 for(int i=0;i<moduleCount;++i){auto& m=modules[(size_t)i];switchAttach.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,m.id,switches[(size_t)i]));
  switches[(size_t)i].onClick=[this]{repaint();};addAndMakeVisible(switches[(size_t)i]);
  moduleKnobs.push_back(std::make_unique<Knob>(proc,m.knob.id,utf8(m.knob.label),moduleFormats[(size_t)i],true));addAndMakeVisible(*moduleKnobs.back());}
 auto db=[](float v){float d=eqDb(v);return (d>=0?"+":"")+juce::String(d,1)+" dB";};
 std::array<std::function<juce::String(float)>,fxKnobCount> fxFormats{pct,pct,pct,db,db,db,[](float v){return juce::String((int)std::round(delaySeconds(v)*1000))+" ms";},pct,
  [](float v){return juce::String(reverbSeconds(v),1)+" s";},pct,[](float v){return v<=0?juce::String("-inf"):juce::String(juce::Decibels::gainToDecibels(outputGain(v)/.9f),1)+" dB";}};
 for(int i=0;i<fxKnobCount;++i){fxKnobsUi.push_back(std::make_unique<Knob>(proc,fxKnobs[(size_t)i].id,utf8(fxKnobs[(size_t)i].label),fxFormats[(size_t)i]));addAndMakeVisible(*fxKnobsUi.back());}
 for(int i=0;i<4;++i){chordButtons[(size_t)i].setButtonText(chordNames[(size_t)i]);chordButtons[(size_t)i].onClick=[this,i]{proc.setValue("chord",(float)i);};addAndMakeVisible(chordButtons[(size_t)i]);}
 for(juce::Component* c:{(juce::Component*)&guitar,(juce::Component*)&model,(juce::Component*)&preset,(juce::Component*)&loadButton,(juce::Component*)&keys})addAndMakeVisible(c);
 setSize(1180,940);timerCallback();startTimerHz(60);}
GuitarEditor::~GuitarEditor(){stopTimer();setLookAndFeel(nullptr);}

void GuitarEditor::chooseFolder(){
 chooser=std::make_unique<juce::FileChooser>("Dossier de samples (un WAV par note, ex. Guitare_C3.wav)",proc.sampleFolder());
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectDirectories,[this](const juce::FileChooser& c){auto f=c.getResult();if(f.isDirectory())proc.loadSamples(f);});}

void GuitarEditor::timerCallback(){
 int program=proc.getCurrentProgram();if(program!=shownProgram){shownProgram=program;preset.setSelectedId(program+1,juce::dontSendNotification);}
 int type=proc.guitarType();if(type!=shownType){shownType=type;guitar.setSelectedId(type+1,juce::dontSendNotification);repaint();
  // Each guitar has its own list of built-in samples.
  model.clear(juce::dontSendNotification);for(int i=0;i<modelsOfType(type);++i)model.addItem(juce::String(juce::CharPointer_UTF8(models[(size_t)(modelFirst(type)+i)].name)),i+1);shownModel=-1;}
 int m=selectedModelIndex();if(m!=shownModel){shownModel=m;model.setSelectedId(m+1,juce::dontSendNotification);}
 int chord=(int)proc.value("chord");for(int i=0;i<4;++i)chordButtons[(size_t)i].setToggleState(i==chord,juce::dontSendNotification);
 for(int i=0;i<moduleCount;++i){bool on=switches[(size_t)i].getToggleState();auto& k=*moduleKnobs[(size_t)i];if(k.active!=on){k.active=on;k.slider.getProperties().set("active",on);k.repaint();}}
 std::vector<int> notes;for(int n=0;n<128;++n)if(proc.engine.isSounding(n))notes.push_back(n);
 std::array<bool,6> lit{};for(auto& s:assignStrings(notes))lit[(size_t)s.string]=true;
 for(size_t i=0;i<6;++i)glow[i]=lit[i]?1.f:glow[i]*.93f;
 phase+=1;
 int note=proc.engine.lastNote.load();
 if(note>=0){auto a=proc.engine.lastArticulation.load();articText=utf8(articulationName(a));int chordRoot=proc.engine.lastChord.load();
  if(a==Articulation::strum&&chordRoot>=0){noteText=juce::MidiMessage::getMidiNoteName(chordRoot,true,false,4)+chordSuffix[(size_t)chord];subText="Strum  /  5 cordes";}
  else{noteText=noteName(note);int from=proc.engine.lastFrom.load();
   if(from>=0)subText=juce::String(a==Articulation::slide?"Slide":a==Articulation::hammer?"Hammer-on":"Pull-off")+" depuis "+noteName(from);
   else if(note<40)subText="Sous la tessiture de la guitare";
   else{auto pos=assignStrings({note});subText=pos.empty()?juce::String("Au-dessus de la case 15"):"Corde "+juce::String(stringNames[(size_t)pos[0].string])+"  /  "+(pos[0].fret==0?utf8("à vide"):"case "+juce::String(pos[0].fret));}}}
 repaint(0,0,getWidth(),450);keys.repaint();}

void GuitarEditor::resized(){
 loadButton.setBounds(560,22,110,30);guitar.setBounds(678,22,120,30);model.setBounds(806,22,180,30);preset.setBounds(994,22,154,30);
 float col=1116.f/moduleCount;
 for(int i=0;i<moduleCount;++i){int x=32+(int)(i*col);switches[(size_t)i].setBounds(x+(int)col-50,474,44,30);moduleKnobs[(size_t)i]->setBounds(x+16,546,(int)col-24,46);}
 // FX groups share the width by weight, like the prototype.
 std::array<float,6> grow{2,1,3,2,2,1};float total=0;for(float w:grow)total+=w;float x=32;
 for(size_t gi=0;gi<fxGroups.size();++gi){float w=1116*grow[gi]/total;auto& grp=fxGroups[gi];for(int k=0;k<grp.count;++k)fxKnobsUi[(size_t)(grp.first+k)]->setBounds((int)(x+14+k*66),660,56,80);x+=w;}
 for(int i=0;i<4;++i)chordButtons[(size_t)i].setBounds(1148-(4-i)*76,758,70,28);
 keys.setBounds(32,796,1116,104);}

void GuitarEditor::paintHero(juce::Graphics& g){
 juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(0,0,1180,450);
 int type=proc.guitarType();auto& hero=heroPhotos[(size_t)type];auto& photo=photos[(size_t)type];
 g.drawImage(photo,0,(int)hero.top,1180,(int)(853*photoScale),0,0,photo.getWidth(),photo.getHeight());
 // Lit strings: orange glow, slight vibration while sounding.
 for(size_t i=0;i<6;++i){float a=glow[i];if(a<.02f)continue;auto& s=hero.strings[i];
  float wobble=std::sin(phase*2.4f+(float)i)*1.4f*a;auto p1=photoToScreen(hero,s[0],s[1]),p2=photoToScreen(hero,s[2],s[3]);
  juce::Line<float> line(p1.translated(wobble,0),p2.translated(-wobble,0));
  for(float w:{16.f,10.f,6.f}){g.setColour(orange.withAlpha(.12f*a));g.drawLine(line,w);}
  g.setColour(orangeLight.withAlpha(a));g.drawLine(line,1.3f+(float)i*.42f);}
 g.drawImage(grain,0,0,1180,450,0,0,grain.getWidth(),grain.getHeight());
 g.setGradientFill(juce::ColourGradient(bg.withAlpha(0.f),0,330,bg,0,450,false));g.fillRect(0,330,1180,120);
 g.setGradientFill(juce::ColourGradient(juce::Colour(0xc7050506),0,0,juce::Colour(0x00050506),520,0,false));g.fillRect(0,0,520,450);
 for(size_t i=0;i<6;++i){auto p=photoToScreen(hero,hero.strings[i][0],hero.strings[i][1])+hero.label;drawLabel(g,stringNames[i],p.x-15,p.y-10,30,16,15.5f,glow[i]>.3f?orange:ink.withAlpha(.55f),juce::Justification::centred,true);}
 // Header
 juce::Path wave;wave.startNewSubPath(7,23);wave.cubicTo(10.5f,16,14,16,19,23);wave.cubicTo(24,30,27.5f,30,31,23);wave.startNewSubPath(7,15);wave.cubicTo(10.5f,8,14,8,19,15);wave.cubicTo(24,22,27.5f,22,31,15);
 wave.applyTransform(juce::AffineTransform::scale(30.f/38).translated(32,22));g.setColour(orange);g.strokePath(wave,juce::PathStrokeType(2.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 drawLabel(g,"swell",72,22,90,30,22,ink);drawLabel(g,proc.guitarType()==1?"E L E C T R I C   G U I T A R":"A C O U S T I C   G U I T A R",150,22,320,30,12,soft);
 // Readout
 drawLabel(g,articText,34,118,300,14,10,juce::Colour(0xffa3a3a8),juce::Justification::centredLeft,false,.3f);
 g.setColour(juce::Colour(0xfff4f4f5));g.setFont(juce::FontOptions(128.f).withKerningFactor(-.04f));g.drawText(noteText,juce::Rectangle<float>(30,134,560,130),juce::Justification::centredLeft);
 drawLabel(g,subText,34,266,480,20,14,juce::Colour(0xffc9c9cd));
 drawLabel(g,utf8("VÉLOCITÉ"),34,300,90,12,9.5f,muted,juce::Justification::centredLeft,false,.2f);drawLabel(g,juce::String(proc.engine.lastNote.load()>=0?proc.engine.lastVelocity.load():0),34,314,90,18,15,ink);
 drawLabel(g,"VOIX",132,300,60,12,9.5f,muted,juce::Justification::centredLeft,false,.2f);drawLabel(g,juce::String(proc.engine.voiceCount.load()),132,314,60,18,15,ink);
 drawLabel(g,"MOTEUR",196,300,80,12,9.5f,muted,juce::Justification::centredLeft,false,.2f);drawLabel(g,proc.libraryStatus(),196,316,560,16,13,orange);}

void GuitarEditor::paint(juce::Graphics& g){
 g.fillAll(bg);paintHero(g);
 auto section=[&](const char* name,float y,float lineFrom){drawLabel(g,name,32,y,120,14,10,muted,juce::Justification::centredLeft,false,.4f);g.setColour(juce::Colour(0xff232326));g.fillRect(lineFrom,y+7,1148.f-lineFrom,1.f);};
 section("JEU",454,74);section("SON",618,74);
 drawLabel(g,"CLAVIER",32,764,100,14,10,muted,juce::Justification::centredLeft,false,.4f);g.setColour(juce::Colour(0xff232326));g.fillRect(118.f,771.f,40.f,1.f);
 drawLabel(g,utf8("Haut de touche = doux, bas = fort  /  glisse pour tester le Slide"),168,764,420,14,11,muted);
 drawLabel(g,"ACCORD STRUM",1148-4*76-120,764,112,14,10,muted,juce::Justification::centredRight,false,.2f);
 float col=1116.f/moduleCount;
 for(int i=0;i<moduleCount;++i){float x=32+i*col;bool on=switches[(size_t)i].getToggleState();if(i>0){g.setColour(rule);g.fillRect(x,480.f,1.f,112.f);}
  drawLabel(g,utf8(modules[(size_t)i].name),x+16,480,col-60,18,12,on?orange:soft,juce::Justification::centredLeft,true,.2f);
  g.setColour(muted);g.setFont(juce::FontOptions(11.f));g.drawFittedText(utf8(modules[(size_t)i].desc),juce::Rectangle<int>((int)x+16,504,(int)col-28,34),juce::Justification::topLeft,2,1.f);}
 std::array<float,6> grow{2,1,3,2,2,1};float total=0;for(float w:grow)total+=w;float x=32;
 for(size_t gi=0;gi<fxGroups.size();++gi){float w=1116*grow[gi]/total;if(gi>0){g.setColour(rule);g.fillRect(x,640.f,1.f,100.f);}drawLabel(g,gi==0&&proc.guitarType()==1?juce::String("AMPLI + BAFFLE"):utf8(fxGroups[gi].name),x+14,638,w-20,16,11,soft,juce::Justification::centredLeft,true,.2f);x+=w;}
 drawLabel(g,utf8(presets[(size_t)proc.getCurrentProgram()].name)+"  /  "+utf8(guitarTypes[(size_t)proc.guitarType()])+"  /  "+(proc.usingSamples()?"samples":utf8("synthèse provisoire")),32,912,600,14,10.5f,faint);
 drawLabel(g,"Swell-audio  /  0.3  /  by @sevyondatracks",700,912,448,14,10.5f,faint,juce::Justification::centredRight);}
