#pragma once
#include <array>
#include <cmath>
#include <juce_core/juce_core.h>
#include "Models.h"
// Every continuous control is 0..100 like the prototype; the engine maps it to real units.
struct KnobInfo {const char* id;const char* label;float def;};
inline constexpr int moduleCount=6;
struct ModuleInfo {const char* id;const char* name;const char* desc;KnobInfo knob;};
inline constexpr std::array<ModuleInfo,moduleCount> modules{{
 {"slide","SLIDE","Glisse vers la note suivante jouée liée.",{"slideTime","TEMPS",30}},
 {"legato","LEGATO","Hammer-on, pull-off : sans réattaque.",{"legatoSoft","DOUCEUR",60}},
 {"vibrato","VIBRATO","Vibrato de doigt après l'attaque.",{"vibDepth","PROFONDEUR",40}},
 {"strum","STRUM","Une touche, un accord gratté.",{"strumSpeed","VITESSE",55}},
 {"humanize","HUMAN","Écarts de timing, de force, de justesse.",{"humAmount","DOSE",35}},
 {"palm","PALM","Corde étouffée : court et sourd.",{"palmAmount","MUTE",70}}}};
inline constexpr int fxKnobCount=11;
struct FxGroup {const char* name;int first,count;};
inline constexpr std::array<KnobInfo,fxKnobCount> fxKnobs{{
 {"drive","DRIVE",15},{"tone","TONE",55},{"comp","DOSE",35},{"low","GRAVE",55},{"mid","MÉDIUM",50},{"high","AIGU",55},
 {"dtime","TEMPS",35},{"dmix","MIX",10},{"size","TAILLE",45},{"rmix","MIX",22},{"vol","VOLUME",75}}};
inline constexpr std::array<FxGroup,6> fxGroups{{{"PRÉAMPLI",0,2},{"COMP",2,1},{"ÉGALISEUR",3,3},{"DELAY",6,2},{"REVERB",8,2},{"SORTIE",10,1}}};
inline constexpr std::array<const char*,4> chordNames{"Majeur","Mineur","7","Sus4"};
// Strum voicings in semitones above the played key (5 strings).
inline constexpr std::array<std::array<int,5>,4> chordShapes{{{0,4,7,12,16},{0,3,7,12,15},{0,4,7,10,16},{0,5,7,12,17}}};
inline constexpr std::array<const char*,4> chordSuffix{"","m","7","sus4"};

inline constexpr int guitarTypeCount=2;
inline constexpr std::array<const char*,guitarTypeCount> guitarTypes{"Acoustique","Électrique"};
// Built-in samples (Models.h is sorted by type, then name): one menu per guitar type.
inline constexpr std::array<const char*,guitarTypeCount> modelParams{"acousticModel","electricModel"};
inline int modelFirst(int type){for(int i=0;i<modelCount;++i)if(models[(size_t)i].type==type)return i;return 0;}
inline int modelsOfType(int type){int n=0;for(auto& m:models)n+=m.type==type?1:0;return n;}
// Menu position of the sound each guitar opens with.
inline int defaultModel(int type){auto* want=type==1?"Run":"RC20";for(int i=0;i<modelsOfType(type);++i)if(juce::String(models[(size_t)(modelFirst(type)+i)].name)==want)return i;return 0;}

struct Preset {const char* name;int type;std::array<float,fxKnobCount> fx;std::array<bool,moduleCount> on;};
inline constexpr int presetCount=11;
inline constexpr std::array<Preset,presetCount> presets{{
 {"Folk Warm",0,{15,55,35,55,50,55,35,10,45,22,75},{false,false,false,false,true,false}},
 {"Fingerstyle Intime",0,{5,45,25,50,45,50,30,0,30,14,75},{false,true,false,false,true,false}},
 {"Strum Pop",0,{20,65,55,45,50,65,40,8,40,18,72},{false,false,false,true,true,false}},
 {"Lo-fi Bedroom",0,{40,25,50,60,55,25,55,22,55,25,72},{false,false,true,false,true,false}},
 {"Trap Mélodique",0,{25,60,45,40,50,60,62,30,75,35,70},{true,false,false,false,true,false}},
 {"Ambient Wash",0,{10,50,30,45,45,55,80,38,95,55,68},{false,false,true,false,true,false}},
 {"Clean Jazz",1,{10,40,30,55,45,45,35,0,40,18,75},{false,true,false,false,true,false}},
 {"Crunch Rock",1,{55,60,45,55,60,55,35,0,35,12,72},{false,false,false,true,true,false}},
 {"Lead Saturé",1,{80,65,55,50,65,55,55,18,50,20,70},{true,false,true,false,true,false}},
 {"Palm Mute Heavy",1,{85,55,60,65,40,60,30,0,25,8,70},{false,false,false,false,true,true}},
 {"Ambient Électrique",1,{20,50,30,45,45,55,80,40,95,50,70},{false,false,true,false,true,false}}}};
inline int firstPreset(int type){for(int i=0;i<presetCount;++i)if(presets[(size_t)i].type==type)return i;return 0;}

inline float eqDb(float v){return (v-50)*.24f;}
inline float slideSeconds(float v){return (30+v*4.7f)/1000;}
inline float strumGapSeconds(float v){return (70-v*.6f)/1000;}
inline float delaySeconds(float v){return (60+v*6.4f)/1000;}
inline float reverbSeconds(float v){return .6f+v*.054f;}
inline float outputGain(float v){return std::pow(v/75,2.f)*.9f;}
