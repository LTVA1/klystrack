/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "stdlib.h"
#include "math.h"

#include "SDL.h"
#include "gfx/gfx.h"
#include "snd/music.h"
#include "gui/toolutil.h"
#include "copypaste.h"
#include "diskop.h"
#include "event.h"
#include "view.h"
#include "gui/slider.h"
#include "action.h"
#include "gui/mouse.h"
#include "gui/bevel.h"
#include "gui/menu.h"
#include "shortcutdefs.h"
#include "version.h"
#include "mused.h"
#include "config.h"
#include "mybevdefs.h"
#include <time.h>
#include "util/rnd.h"
#include "view/visu.h"
#include "view/pattern.h"
#include "view/sequence.h"
#include "view/wavetableview.h"
#include "view/timer.h"
#include "mymsg.h"
#include "key.h"
#include "nostalgy.h"
#include "theme.h"

long map_Arduino(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; //don't ask me just easiest wavegen+oscview setup involved arduino
}

//from jsSID emulator

void createCombinedWF(int wfarray[], float bitmul, float bitstrength,float treshold) 
{ //I found out how the combined waveform works (neighboring bits affect each other recursively)
  for (int i=0; i<4096; i++) 
  { 
   wfarray[i]=0; //neighbour-bit strength and DAC MOSFET treshold is approximately set by ears'n'trials
   
   for (int j=0; j<12;j++) 
   { 
    float bitlevel=0.0;
    
    for (int k=0; k<12; k++) 
    { 
      bitlevel += ( bitmul/pow(bitstrength,abs(k-j)) ) * (((i>>k)&1)-0.5); 
     }
      
     wfarray[i] += (bitlevel>=treshold)? pow(2,j) : 0;  
    }
    
    wfarray[i]*=12;  
   }

   for(int i = 0; i < 4096; i++)
   {
     wfarray[i] *= 1.26;
   }
}

void createPulseTri(int wfarray[], float bitmul, float bitstrength,float treshold) 
{ //I found out how the combined waveform works (neighboring bits affect each other recursively)
  for (int i=0; i<2048; i++) 
  { 
   wfarray[i]=0; //neighbour-bit strength and DAC MOSFET treshold is approximately set by ears'n'trials
   
   for (int j=0; j<12;j++) 
   { 
    float bitlevel=0.0;
    
    for (int k=0; k<12; k++) 
    { 
      bitlevel += (bitmul/pow(bitstrength,abs(k-j))) * (((2*i>>k)&1)-0.5); //bitlevel += (bitmul/pow(bitstrength,abs(k-j))) * (((i>>k)&1)-0.5); 
    }
      
    wfarray[i] += (bitlevel>=treshold) ? pow(2,j) : 0;  
   }
    
    wfarray[i]*=12;  
  }

  for(int i=2045; i<2049; i++)
  {
    wfarray[i] = wfarray[2045];
  }

  for(int i=0; i<2048; i++)
  {
    if(wfarray[i] > 0 && map_Arduino(i, 0, 2047, 0, 53535) < wfarray[i])
    {
      wfarray[i] = map_Arduino(i, 0, 2047, 0, 53535);
    }
  }

  for(int i=2047; i<4096; i++)
  {
    wfarray[i] = wfarray[4096 - i];
  }

  for(int i=0; i<4096; i++)
  {
    wfarray[i] = (wfarray[i] * 26) / 20;
  }

  for(int i=0; i<4096; i++)
  {
	wfarray[4096 + i] = 0;
    /*wfarray[4096 + i] = wfarray[i];
    wfarray[i] = 0;*/
  }
} 