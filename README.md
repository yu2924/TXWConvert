# TXWConvert

## What is this?

A set of programs to convert sound diskettes for the YAMAHA sampler TX16W.

## Requirement to build

Visual Studio 2015 or later

I haven't tried it, but maybe other compilers/toolchains can build it as well.

## How to use

### txw2wav

Converts TX16W sample files ".W??" to WAV.
```
txw2wav [input spec] [output spec] [-d][-h][-o][-v]
-d: use default output directory 'wav'
-h: help
-o: overwrite
-v: verbose

examples:
  single file   : txw2wav d:\dir\input-filename.W01 d:\dir\output-filename.wav
  multiple file : txw2wav d:\dir\input-directory d:\dir\output-directory
```
omit [input spec] and [output spec] to enter the interactive mode

### txw2sfz

Converts entire TX16W diskettes, containing performances ".U??", voices & timbles ".V??" and samples "*.W??" to SFZ format.

```
txw2sfz [input file] [output directory] [-d][-h][-o][-v]
-d: use default output directory 'sfz'
-h: help
-o: overwrite
-v: verbose

examples:
  txw2sfz d:\dir\performance.U01 d:\dir\output-directory
```

omit [input file] and [output directory] to enter the interactive mode

## Reference

[Yamaha TX-16W Technical Info](http://www.youngmonkey.ca/nose/audio_tech/synth/Yamaha-TX16W.html)  
vortex.c (the original URL is lost)  
many thanks🙂

## Written by

[yu2924](https://twitter.com/yu2924)

## License

CC0 1.0 Universal
