# klystrack-plus

A fork of klystrack chiptune tracker. A bit outdated but very good and comprehensive tutorial is [here](http://n00bstar.blogspot.com/2013/02/klystrack-tutorial-basics.html). To compile from github (not from source archive) you would need to download klystron and do some black magic I think, so better use source.zip.

## Future plans

- ~~additive mode for current 2-op fm synth~~ done in v0.4
- 4-op fm synth with all 12 algorithms I've found. If algorithm is from OPL3 or Sega chip special tip emerges. Each op has its own macro, filter, envelope, etc. Filter of the first op which would be edited in main window filters overall output of the 4-op stack. 
- commands for setting start and end points of the sample from instrument macro (the result is you can use less samples, e.g. use sine and make lower half sine and upper half sine from the same sample for different instruments) 
- Song info or song comment (separate window with arbitrary amount of text as in e.g. Impulse tracker)
- Oscilloscope in instrument editing window which shows current waveform produced by instrument
- ~~Exponential wave in wavegen (mainly for OPL3)~~ done in v0.3
- DPCMator in wavegen (convert any sample into DPCM NES format (only 1 step up or down for each step, 6-bit resolution) and save as 8-bit PCM sample) 
- ~~Killing duplicate samples and relinking instruments to remaining 1 sample (useful for imported xms and its, combined with klystrack's sample compression it would give huge (or not) size reduction)~~ done in v0.5
- ~~saving wavegen settings as sort of "synth patches"~~ done in v0.3
- ~~moving klystrack config file from C:/Users/%USERNAME% to app folder which would make app portable~~ done in v0.4

Below are links related to original klystrack. You will not find my releases there. Instead check releases of my fork. 

# klystrack

Klystrack is a chiptune tracker for making chiptune-like music on a modern computer.

1. Download the latest [release](https://github.com/kometbomb/klystrack/releases) or: 
    - [Build your own](https://github.com/kometbomb/klystrack/wiki/HowToCompile).
    - [Get a prebuilt](https://repology.org/metapackage/klystrack/versions).
    - [klystrack for OSX](https://plugins.ro/klystrack/) (thanks, TBD!)
    - [Install on Linux using the snap](https://snapcraft.io/klystrack) ([repo](https://github.com/kometbomb/klystrack-snap)) [![klystrack](https://snapcraft.io/klystrack/badge.svg)](https://snapcraft.io/klystrack)
    - [Run on Linux using the AppImage](http://sid.ethz.ch/appimage/Klystrack-x86_64.AppImage)
2. Google for a klystrack tutorial or start exploring, it's mostly just like any tracker.

## Discord

Join the [klystrack Discord](https://discord.gg/udba7HG) for help, listen to tunes or just to hang out.
