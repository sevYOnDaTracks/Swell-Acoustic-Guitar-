# Swell Guitar

Guitare **acoustique et électrique** VST3 / AU / Standalone de la suite **Swell** (Swell-audio, by @sevyondatracks). JUCE 8 + CMake.

> **v0.3** : 95 samples intégrés (one-shots en FLAC), rangés en 37 modèles acoustiques et 58 électriques.
> Menu **Modèle** à côté du choix de guitare ; chaque modèle est accordé (cents) et chargé en arrière-plan.

## Captures

**Strum** (preset Strum Pop, accord de C : 5 cordes allumées)

![Swell Acoustic Guitar - strum](docs/ui-strum.png)

**Note seule** (preset Trap Mélodique, E3 sur la corde D, case 2)

![Swell Acoustic Guitar - note](docs/ui-note.png)

## Ce qu'il y a
- **Moteur sampler** (`Source/GuitarEngine.*`, `Source/SampleLibrary.*`) : lit un dossier de WAV/AIFF/FLAC nommés par note
  (`Guitare_C3.wav`, `Guitare_F#2_forte_rr2.wav`), couches de vélocité (p / m / f), round-robin, recalage automatique de l'octave
  (la note la plus grave tombe sur C2..B2). Sans samples, une guitare synthétisée (Karplus-Strong) joue à la place.
- **Modules de jeu** : Slide (notes liées), Legato (hammer-on / pull-off), Vibrato, Strum (accord gratté, Maj / Min / 7 / Sus4),
  Humanize, Palm mute. Pitch bend ±2, molette = vibrato, pédale de sustain.
- **Effets** (`Source/FxChain.h`) : préampli (acoustique) ou ampli + baffle (électrique), compresseur, EQ 3 bandes, delay, reverb, volume.
- **Presets** : 6 acoustiques, 5 électriques (Clean Jazz, Crunch Rock, Lead Saturé, Palm Mute Heavy, Ambient Électrique).
- **Interface "Noir & Cordes"** (`Source/PluginEditor.*`) : photo noir et blanc, les 6 cordes s'allument en orange quand elles jouent.
  Prototype jouable dans `design/project/Main.dc.html`.

## Samples
**Modèles intégrés** (`assets/samples/`, liste dans `assets/samples/models.json`) :
- Acoustique : 6ix, Bamboo, Big, CLA, Fish, Flema, Goat, Guitar A, Guitar B, Guitar C, Guitar D, Guitar E, Harp, Kuun, Lean, Light, Luffy, Mellow, Naruto, Naruto FX, Obito, Obito FX, Plucked, RC20, Real, Sakura, Sakura FX, Sasuke, Sasuke FX, Stick, Stringy, Talk, Third, Travis, True, Wenna, Zamboni
- Électrique : Bean, BGuitar A, BGuitar B, BGuitar C, BGuitar D, BGuitar E, BGuitar F, Broom, Burst, Command, Dirt, Done, Droid, Dyno, Electric, Fender, Finnin, Fox, Hours, Itachi, Itachi FX, Kakashi, Kakashi FX, Kanye, Kanye FX, Leave, Lee, Lifeguard, Lunch, Madara, Madara FX, Mouse, Ninnie, Over, Pulse, Purple, Reverse, RGuitar A, RGuitar B, RGuitar C, RGuitar D, Run, Saver, Shaky, Singer, Softy, Soul, Sout, Space, Steel, Stirge, Stressed, Tronic, Viking, Wammy, Warm, Whip, Wonky

Classement fait à l'analyse (nom du fichier, pack d'origine, puis attaque et extinction : attaque franche et extinction
rapide = acoustique, sustain long ou swell = électrique). Pour déplacer un modèle ou corriger son accordage : modifier
`models.json` (`type`, `root`, `cents`), puis `python3 Tools/models_header.py` et recompiler.

**Dossier perso** : Sans dossier, chaque guitare joue son sample intégré. Dossier par défaut : `Documents/Swell/Acoustic Guitar/Samples` et
`Documents/Swell/Electric Guitar/Samples` (ou bouton « Samples... », qui charge pour la guitare affichée). Samples **dry**, une note par fichier,
44,1 ou 48 kHz, laissés sonner jusqu'au bout, sans normalisation note par note.

## Build
Windows : `./build.ps1` (VS 2022 Build Tools + CMake). Mac : `./build-mac.sh`.

## Crédits
Photos (licence Pixabay, usage commercial autorisé, passées en noir et blanc) :
- Acoustique : [guitar-acoustic-music-5807916](https://pixabay.com/photos/guitar-acoustic-music-5807916/) par bunditua
- Électrique : [electric-guitar-guitar-strings-1846582](https://pixabay.com/photos/electric-guitar-guitar-strings-1846582/)
