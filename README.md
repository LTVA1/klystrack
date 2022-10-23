# klystrack-plus

A fork of klystrack chiptune tracker. A bit outdated but very good and comprehensive tutorial is [here](http://n00bstar.blogspot.com/2013/02/klystrack-tutorial-basics.html). To compile from github:

```
git clone --recursive https://github.com/LTVA1/klystrack
cd klystrack
make -j10
```

## Future plans

- ~~additive mode for current 2-op fm synth~~ done in v0.4
- ~~4-op fm synth with all 12 algorithms I've found. If algorithm is from OPL3 or Sega chip special tip emerges. Each op has its own macro, filter, envelope, etc. Filter of the main instrument filters overall output of the 4-op stack AFTER individual ops' filters.~~ sort of done in v0.10.0-alpha1
- ~~commands for setting start and end points of the sample from instrument macro (the result is you can use less samples, e.g. use sine and make lower half sine and upper half sine from the same sample for different instruments)~~ done in v0.9
- Song info or song comment (separate window with arbitrary amount of text as in e.g. Impulse tracker)
- ~~Oscilloscope in instrument editing window which shows current waveform produced by instrument~~ done in v0.8
- ~~Exponential wave in wavegen (mainly for OPL3)~~ done in v0.3
- DPCMator in wavegen (convert any sample into DPCM NES format (only 1 step up or down for each step, 6-bit resolution) and save as 8-bit PCM sample) 
- ~~Killing duplicate samples and relinking instruments to remaining 1 sample (useful for imported xms and its, combined with klystrack's sample compression it would give huge (or not) size reduction)~~ done in v0.5
- ~~saving wavegen settings as sort of "synth patches"~~ done in v0.3
- ~~moving klystrack config file from `C:/Users/%USERNAME%` to app folder which would make app portable~~ done in v0.4
- Renaming current wavetable thing to «samples» and doing a simpler wavetable-per-instrument setup. Max 256 wavetables per instrument. They all share bit depth and length, and do not have base note and rate settings so saving them is more optimal for Gameboy and N163-style songs. Also it would be easier to import FamiTracker files. Import from clipboard as in FamiTracker (`$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $10 $11 $12 $13 $14 $15`, `$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $a $b $c $d $e $f`, `0 1 2 3 4 5 6 7 8 9 a b c d e f` will yield 16-step sawtooth, `$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $a $b $c $d $e $f $f $e $d $c $b $a $9 $8 $7 $6 $5 $4 $3 $2 $1 $0; $0 $0 $1 $1 $2 $2 $3 $3 $4 $4 $5 $5 $6 $6 $7 $7 $8 $8 $9 $9 $a $a $b $b $c $c $d $d $e $e $f $f` will give you 32-step triangle at slot 0 and sawtooth at slot 1).
- ~~16-bit rate setting instead of current 8-bit for crazy rate songs (400 Hz, 800 Hz and so on). If rate is lower or equal to 255, save as 8-bit value (song would be 1 byte smaller!!!)~~ done in v0.8
- ~~Full range speed setting (not 0-F as now, but 0-FF, it’s anyway saved as 8-bit value). Commands for setting each speed in 00-FF range. Helps in case of crazy rates.~~ done in v0.8
- ~~More command columns (up to 8, absolutely not related to [Furnace](https://github.com/tildearrow/furnace))~~ done in v0.8
- `.vgm` file export (oh god it will be crazy). Before export unnecessary complex algorithm checks where user fucked up (used FM instrument or sawtooth wave in case of exporting for 2A03, used anything except noise of 10th channel for Genesis etc.) and tells where exactly.
- `.sid` export, possibly steal algorithm from goattracker or smth. 1-8 SIDs, user would specify each SID address (e.g. `$6A0F`). Sample support (4-bit and 8-bit).
- FamiTracker files import (`.ftm`, `.0cc`, `.dnm`, `.eft` and others, one song at a time)
- `.dmf` import (probably steal algorithm from… [you know](https://github.com/tildearrow/furnace))
- `.a2m` (Adlib Tracker II) file import
- `.rmt` (Raster Music Tracker) file import
- `.s3m` (Schism Tracker) file import
- `.it` (Impulse Tracker) file import
- `.MED` (OctaMED tracker) file import
- `.mid`/`.midi` file import along with some kind of klystrack own file for creating instrument banks
- MIDI input support
- `.mptm` import (OpenMPT file)
- [`.fur`](https://github.com/tildearrow/furnace) import. I love how this tracker also uses 16-bit commands (and will probably steal some algorithms from it). Deflemask killer.
- Custom envelope for instruments and thus more precise `.xm` (and maybe other formats) import. Maybe make Yamaha FM chips (YM2612 envelope, `TL-AR-D1R-TL1-D2R-RR` style) envelope as separate option.


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