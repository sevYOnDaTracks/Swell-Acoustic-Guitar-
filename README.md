# Swell Acoustic Guitar

Guitare acoustique VST3 / AU / Standalone de la suite **Swell** (Swell-audio, by @sevyondatracks). JUCE 8 + CMake.

> **Travail en cours (v0.1, pas encore utilisable).** Le code compile, mais le plugin plante au chargement
> (l'outil VST3 de JUCE et les tests crashent à l'instanciation). C'est la prochaine chose à corriger.

## Ce qu'il y a
- **Moteur sampler** (`Source/GuitarEngine.*`, `Source/SampleLibrary.*`) : lit un dossier de WAV/AIFF/FLAC nommés par note
  (`Guitare_C3.wav`, `Guitare_F#2_forte_rr2.wav`), couches de vélocité (p / m / f), round-robin, recalage automatique de l'octave
  (la note la plus grave tombe sur C2..B2). Sans samples, une guitare synthétisée (Karplus-Strong) joue à la place.
- **Modules de jeu** : Slide (notes liées), Legato (hammer-on / pull-off), Vibrato, Strum (accord gratté, Maj / Min / 7 / Sus4),
  Humanize, Palm mute. Pitch bend ±2, molette = vibrato, pédale de sustain.
- **Effets** (`Source/FxChain.h`) : préampli, compresseur, EQ 3 bandes, delay, reverb, volume.
- **Interface "Noir & Cordes"** (`Source/PluginEditor.*`) : photo noir et blanc, les 6 cordes s'allument en orange quand elles jouent.
  Prototype jouable dans `design/project/Main.dc.html`.

## Samples
Dossier par défaut : `Documents/Swell/Acoustic Guitar/Samples` (ou bouton « Samples... »). Samples **dry**, une note par fichier,
44,1 ou 48 kHz, laissés sonner jusqu'au bout, sans normalisation note par note.

## Build
Windows : `./build.ps1` (VS 2022 Build Tools + CMake). Mac : `./build-mac.sh`.

## Crédits
Photo : [Pixabay — guitar-acoustic-music-5807916](https://pixabay.com/photos/guitar-acoustic-music-5807916/) par bunditua, licence Pixabay (usage commercial autorisé).
