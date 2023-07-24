/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "importutil.h"

extern Mused mused;
extern GfxDomain *domain;

//next two functions taken from https://www.geeksforgeeks.org/implement-itoa/
// A utility function to reverse a string
static void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;

    while (start < end)
	{
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}

// Implementation of citoa()
char* citoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitly, otherwise empty string is
     * printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled
    // only with base 10. Otherwise numbers are
    // considered unsigned.
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

void read_uint32(FILE *f, Uint32* number) //little-endian
{
	*number = 0;
	Uint8 temp = 0;
	fread(&temp, 1, 1, f);
	*number |= temp;
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 8);
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 16);
	fread(&temp, 1, 1, f);
	*number |= ((Uint32)temp << 24);
}

int checkbox_simple(GfxDomain *dest, const SDL_Event *event, const SDL_Rect *area, GfxSurface *gfx, const Font * font, int offset, int offset_pressed, int decal, const char* _label, Uint32 *flags, Uint32 mask)
{
	SDL_Rect tick, label;
	copy_rect(&tick, area);
	copy_rect(&label, area);
	tick.h = tick.w = 8;
	label.w -= tick.w + 4;
	label.x += tick.w + 4;
	label.y += 1;
	label.h -= 1;
	int pressed = button_event(dest, event, &tick, gfx, offset, offset_pressed, (*flags & mask) ? decal : -1, NULL, 0, 0, 0);
	font_write(font, dest, &label, _label);
	
	return pressed;
}

void generic_flags_simple(const SDL_Event *e, const SDL_Rect *_area, const char *label, Uint32 *_flags, Uint32 mask)
{
	SDL_Rect area;
	copy_rect(&area, _area);
	area.y += 1;

	int hit = check_event(e, _area, NULL, NULL, NULL, NULL);

	if (checkbox_simple(domain, e, &area, mused.slider_bevel, &mused.smallfont, BEV_BUTTON, BEV_BUTTON_ACTIVE, DECAL_TICK, label, _flags, mask))
	{

	}
	
	if (hit)
	{
		// so that the gap between the box and label works too
		Uint32 flags = *_flags;
		flags ^= mask;
		*_flags = flags;
	}
}

Uint8 find_empty_command_column(MusStep* step)
{
	for(int i = 0; i < MUS_MAX_COMMANDS; i++)
	{
		if(step->command[i] == 0)
		{
			return i;
		}
	}

	return 0xff;
}

void find_command_xm(Uint16 command, MusStep* step)
{
	if ((command & 0xff00) == 0x0000 && (command & 0xff) != 0)
	{
		step->command[0] = MUS_FX_SET_EXT_ARP | (command & 0xff); 
		return;
	}
	
	else if ((command & 0xff00) == 0x0100 || (command & 0xff00) == 0x0200 || (command & 0xff00) == 0x0300)
	{
		step->command[0] = (command & 0xff00) | my_min(0xff, (command & 0xff)); // assuming linear xm freqs
		return;
	}
	
	else if ((command & 0xff00) == 0x0400)
	{
		step->command[0] = MUS_FX_VIBRATO | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0500)
	{
		step->ctrl |= MUS_CTRL_SLIDE;
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	else if ((command & 0xff00) == 0x0600)
	{
		step->ctrl |= MUS_CTRL_VIB;
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	else if ((command & 0xff00) == 0x0700)
	{
		step->command[0] = MUS_FX_TREMOLO | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0800)
	{
		step->command[0] = MUS_FX_SET_PANNING | (command & 0xff);
		return;
	}
	
	if ((command & 0xff00) == 0x0900)
	{ //the conversion will be finished later
		step->command[0] = MUS_FX_WAVETABLE_OFFSET | (command & 0xff);
		return;
	}
	
	else if ((command & 0xff00) == 0x0a00)
	{
		step->command[0] = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return;
	}
	
	//Bxx not currently supported in klystrack
	else if ((command & 0xff00) == 0x0b00) //Bxx finally :euphoria:
	{
		step->command[0] = MUS_FX_JUMP_SEQUENCE_POSITION | (command & 0xff);
		return ;
	}
	
	if ((command & 0xff00) == 0x0c00)
	{
		step->command[0] = MUS_FX_SET_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command & 0xff00) == 0x0d00)
	{
		//step->command[0] = MUS_FX_SKIP_PATTERN | (command & 0xff);
		//command = MUS_FX_SKIP_PATTERN | (command & 0xff);

		Uint8 new_param = (command & 0xf) + ((command & 0xff) >> 4) * 10; //hex to decimal, Protracker (and XM too!) lol
		step->command[0] = MUS_FX_SKIP_PATTERN | new_param;
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e10)
	{
		step->command[0] = MUS_FX_EXT_PORTA_UP | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e20)
	{
		step->command[0] = MUS_FX_EXT_PORTA_DN | (command & 0xf);
		return;
	}

	else if ((command & 0xfff0) == 0x0e30)
	{
		step->command[0] = MUS_FX_GLISSANDO_CONTROL | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e60)
	{
		step->command[0] = MUS_FX_FT2_PATTERN_LOOP | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0e90)
	{
		step->command[0] = MUS_FX_EXT_RETRIGGER | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ea0 || (command & 0xfff0) == 0x0eb0)
	{
		step->command[0] = ((command & 0xfff0) == 0x0ea0 ? 0x0eb0 : 0x0ea0) | (my_min(0xf, (command & 0xf) * 2));
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ec0)
	{
		step->command[0] = MUS_FX_EXT_NOTE_CUT | (command & 0xf);
		return;
	}
	
	else if ((command & 0xfff0) == 0x0ed0)
	{
		step->command[0] = MUS_FX_EXT_NOTE_DELAY | (command & 0xf);
		return;
	}
	
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) < 32)
	{
		step->command[0] = MUS_FX_SET_SPEED | my_min(0xf, (command & 0xff));
		return;
	}
	
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) >= 32)
	{
		step->command[0] = MUS_FX_SET_RATE | (((command & 0xff)) * 50 / 125);
		return;
	}
	
	else if ((command >> 8) == 'G')
	{
		step->command[0] = MUS_FX_SET_GLOBAL_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command >> 8) == 'H')
	{
		step->command[0] = MUS_FX_FADE_GLOBAL_VOLUME | ((command & 0xff) * 2);
		return;
	}
	
	else if ((command >> 8) == 'K')
	{
		step->command[0] = MUS_FX_TRIGGER_RELEASE | (command & 0xff);
		return;
	}
	
	else if ((command >> 8) == 'P')
	{
		if(command & 0xf0) //pan right
		{
			step->command[0] = MUS_FX_PAN_RIGHT | ((command & 0xf0) >> 4);
			return;
		}
		
		if(command & 0xf) //pan left
		{
			if(step->command[0] == 0)
			{
				step->command[0] = MUS_FX_PAN_LEFT | (command & 0xf);
			}

			else
			{
				step->command[1] = MUS_FX_PAN_LEFT | (command & 0xf); //so we can have both commands if e.g. P8f is the orig command (makes no sense but theoretically such command can be encountered)
			}

			return;
		}
	}
	
	else if ((command >> 8) == 'X')
	{
		if(((command & 0xf0) >> 4) == 1)
		{
			step->command[0] = MUS_FX_EXT_FINE_PORTA_UP | (command & 0xf);
			return;
		}
		
		if(((command & 0xf0) >> 4) == 2)
		{
			step->command[0] = MUS_FX_EXT_FINE_PORTA_DN | (command & 0xf);
			return;
		}
	}

	else if ((command >> 8) == 'Y')
	{
		step->command[0] = MUS_FX_PANBRELLO | (command & 0xff);
		return;
	}
	
	else
	{
		command = 0;
		return;
	}
}

void convert_volume_and_command_xm(Uint16 command, Uint8 volume, MusStep* step)
{
	if (volume >= 0x10 && volume <= 0x50)
	{
		step->volume = (volume - 0x10) * 2;
		goto next;
	}
	
	else if (volume >= 0xc0 && volume <= 0xcf)
	{
		step->volume = MUS_NOTE_VOLUME_SET_PAN | (volume & 0xf);
		goto next;
	}
	
	else if (volume >= 0xd0 && volume <= 0xdf)
	{
		step->volume = MUS_NOTE_VOLUME_PAN_LEFT | (volume & 0xf);
		goto next;
	}

	else if (volume >= 0xe0 && volume <= 0xef)
	{
		step->volume = MUS_NOTE_VOLUME_PAN_RIGHT | (volume & 0xf);
		goto next;
	}
	
	else if (volume >= 0x60 && volume <= 0x6f)
	{
		step->volume = MUS_NOTE_VOLUME_FADE_DN | (volume & 0xf);
		goto next;
	}

	else if (volume >= 0x70 && volume <= 0x7f)
	{
		step->volume = MUS_NOTE_VOLUME_FADE_UP | (volume & 0xf);
		goto next;
	}

	else if (volume >= 0x80 && volume <= 0x8f)
	{
		step->volume = MUS_NOTE_VOLUME_FADE_DN_FINE | (volume & 0xf);
		goto next;
	}

	else if (volume >= 0x90 && volume <= 0x9f)
	{
		if(volume & 0xf)
		{
			step->volume = MUS_NOTE_VOLUME_FADE_UP_FINE | (volume & 0xf);
		}

		goto next;
	}
	
	else if (volume >= 0xf0 && volume <= 0xff)
	{
		step->ctrl = MUS_CTRL_SLIDE|MUS_CTRL_LEGATO;
		goto next;
	}

	else if (volume >= 0xb0 && volume <= 0xbf)
	{
		step->ctrl = MUS_CTRL_VIB;
		goto next;
	}
	
	next:;

	find_command_xm(command, step);
	
	if (volume >= 0x80 && volume <= 0x8f)
	{
		Uint16 vol_command = MUS_FX_EXT_FADE_VOLUME_DN | (volume & 0xf);
		
		if(step->command[0] == 0)
		{
			step->command[0] = vol_command;
			goto next2;
		}
		
		else if(step->command[1] == 0)
		{
			step->command[1] = vol_command;
			goto next2;
		}
		
		else if(step->command[2] == 0)
		{
			step->command[2] = vol_command;
			goto next2;
		}
	}
	
	else if (volume >= 0x90 && volume <= 0x9f)
	{
		Uint16 vol_command = MUS_FX_EXT_FADE_VOLUME_UP | (volume & 0xf);
		
		if(step->command[0] == 0)
		{
			step->command[0] = vol_command;
			goto next2;
		}
		
		else if(step->command[1] == 0)
		{
			step->command[1] = vol_command;
			goto next2;
		}
		
		else if(step->command[2] == 0)
		{
			step->command[2] = vol_command;
			goto next2;
		}
	}
	
	next2:;
}

Uint16 find_command_pt(Uint16 command, int sample_length)
{
	if ((command & 0xff00) == 0x0000 && (command & 0xff) != 0)
	{
		command = MUS_FX_SET_EXT_ARP | (command & 0xff);
		return command;
	}
	else if ((command & 0xff00) == 0x0c00)
	{
		command = MUS_FX_SET_VOLUME | ((command & 0xff) * 2);
		//command = MUS_FX_SET_ABSOLUTE_VOLUME | ((command & 0xff) * 2);
		return command;
	}
	else if ((command & 0xff00) == 0x0a00)
	{
		command = MUS_FX_FADE_VOLUME | (my_min(0xf, (command & 0x0f) * 2)) | (my_min(0xf, ((command & 0xf0) >> 4) * 2) << 4);
		return command;
	}
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) < 32)
	{
		command = MUS_FX_SET_SPEED | my_min(0xf, (command & 0xff));
		return command;
	}
	else if ((command & 0xff00) == 0x0f00 && (command & 0xff) >= 32)
	{
		command = MUS_FX_SET_RATE | (((command & 0xff)) * 50 / 125);
		return command;
	}
	else if ((command & 0xff00) == 0x0100 || (command & 0xff00) == 0x0200)
	{
		command = (command & 0xff00) | my_min(0xff, (command & 0xff) * 8);
		return command;
	}
	else if ((command & 0xff00) == 0x0300)
	{
		command = MUS_FX_FAST_SLIDE | (command & 0xff);
		return command;
	}
	else if ((command & 0xff00) == 0x0900 && sample_length)
	{
		command = MUS_FX_WAVETABLE_OFFSET | ((Uint64)(command & 0xff) * 256 * 0x1000 / (Uint64)sample_length);
		return command;
	}

	else if ((command & 0xff00) == 0x0b00) //Bxx finally :euphoria:
	{
		command = MUS_FX_JUMP_SEQUENCE_POSITION | (command & 0xff);
		return command;
	}
	
	else if ((command & 0xff00) == 0x0d00)
	{
		//command = MUS_FX_SKIP_PATTERN | (command & 0xff);

		Uint8 new_param = (command & 0xf) + ((command & 0xff) >> 4) * 10; //hex to decimal, Protracker lol
		command = MUS_FX_SKIP_PATTERN | new_param;
		return command;
	}
	
	else if ((command & 0xff00) != 0x0400 && (command & 0xff00) != 0x0000) 
	{
		command = 0;
		return command;
	}
	else if ((command & 0xfff0) == 0x0e50)
	{
		Uint8 finetune = 0;

		if((command & 0xf) < 8)
		{
			finetune = 0x80 + ((command & 0xf) << 4);
		}

		else
		{
			finetune = 0x80 - ((0x10 - (command & 0xf)) << 4);
		}

		command = MUS_FX_PITCH | finetune;
		return command;
	}
	else if ((command & 0xfff0) == 0x0e60)
	{
		command = MUS_FX_FT2_PATTERN_LOOP | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0ec0)
	{
		command = MUS_FX_EXT_NOTE_CUT | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0ed0)
	{
		command = MUS_FX_EXT_NOTE_DELAY | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0e90)
	{
		command = MUS_FX_EXT_RETRIGGER | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0e10)
	{
		command = MUS_FX_EXT_PORTA_UP | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0e20)
	{
		command = MUS_FX_EXT_PORTA_DN | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0e30)
	{
		command = MUS_FX_GLISSANDO_CONTROL | (command & 0xf);
		return command;
	}
	else if ((command & 0xfff0) == 0x0ea0 || (command & 0xfff0) == 0x0eb0)
	{
		command = ((command & 0xfff0) == 0x0ea0 ? 0x0eb0 : 0x0ea0) | (my_min(0xf, (command & 0x0f) * 2));
		return command;
	}
	
	return command;
}

void find_command_s3m(Uint16 command, MusStep* step)
{
	Uint8 param = command & 0xff;

	switch(command >> 8)
	{
		case 1: //Axx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_SET_SPEED1 | param;
			step->command[find_empty_command_column(step)] = MUS_FX_SET_SPEED2 | param;

			break;
		}

		case 2: //Bxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_JUMP_SEQUENCE_POSITION | param;
			break;
		}

		case 3: //Cxx
		{
			Uint8 new_param = (param & 0xf) + (param >> 4) * 10; //hex to decimal, Scream Tracker 3 lol
			step->command[find_empty_command_column(step)] = MUS_FX_SKIP_PATTERN | new_param;
			break;
		}

		case 4: //Dxy
		{
			if((param & 0xf0) == 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_DN | my_min(0xf, (param & 0xf) * 2);
			}

			else if((param & 0xf) == 0xf)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_UP | my_min(0xf, ((param & 0xf0) >> 4) * 2);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_FADE_VOLUME | param;
			}
			
			break;
		}

		case 5: //Exx
		{
			if(param < 0xe0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PORTA_DN | param;
			}

			else if(param >= 0xe0 && param < 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_PORTA_DN | (param & 0xf);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FINE_PORTA_DN | (param & 0xf);
			}

			break;
		}

		case 6: //Fxx
		{
			if(param < 0xe0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PORTA_UP | param;
			}

			else if(param >= 0xe0 && param < 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_PORTA_UP | (param & 0xf);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FINE_PORTA_UP | (param & 0xf);
			}

			break;
		}

		case 7: //Gxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_FAST_SLIDE | param;
			break;
		}

		case 8: //Hxy
		{
			step->command[find_empty_command_column(step)] = MUS_FX_VIBRATO | param;
			break;
		}

		case 9: //Ixy
		{
			break;
		}

		case 10: //Jxy
		{
			step->command[find_empty_command_column(step)] = MUS_FX_SET_EXT_ARP | param;
			break;
		}

		case 11: //Kxy
		{
			if((param & 0xf0) == 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_DN | my_min(0xf, (param & 0xf) * 2);
			}

			else if((param & 0xf) == 0xf)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_UP | my_min(0xf, ((param & 0xf0) >> 4) * 2);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_FADE_VOLUME | param;
			}

			step->ctrl |= MUS_CTRL_VIB;

			break;
		}

		case 12: //Lxy
		{
			if((param & 0xf0) == 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_DN | my_min(0xf, (param & 0xf) * 2);
			}

			else if((param & 0xf) == 0xf)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_EXT_FADE_VOLUME_UP | my_min(0xf, ((param & 0xf0) >> 4) * 2);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_FADE_VOLUME | param;
			} //portamento will be filled in later
			break;
		}

		case 13: //Mxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_SET_CHANNEL_VOLUME | (param * 2);
			break;
		}

		case 14: //Nxy
		{
			//klystrack does not have this command yet
			break;
		}

		case 15: //Oxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_WAVETABLE_OFFSET | param; //conversion will be finished later
			break;
		}

		case 16: //Pxy
		{
			if((param & 0xf0) == 0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PAN_RIGHT | my_max(1, (param & 0xf) / 2);
			}

			else if((param & 0xf) == 0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PAN_LEFT | my_max(1, ((param & 0xf0) >> 4) / 2);
			}

			else if((param & 0xf0) == 0xf0)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PAN_RIGHT_FINE | my_max(1, (param & 0xf) / 2);
			}

			else if((param & 0xf) == 0xf)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_PAN_LEFT_FINE | my_max(1, ((param & 0xf0) >> 4) / 2);
			}

			else
			{
				step->command[find_empty_command_column(step)] = MUS_FX_SET_PANNING | my_min(0xff, (param * 2));
			}

			break;
		}

		case 17: //Qxy
		{
			step->command[find_empty_command_column(step)] = MUS_FX_EXT_RETRIGGER | (param & 0xf); //fuck the volume thing it's too complex
			break;
		}

		case 18: //Rxy
		{
			step->command[find_empty_command_column(step)] = MUS_FX_TREMOLO | param;
			break;
		}

		case 19: //SXx
		{
			switch(param >> 4)
			{
				case 1: //S1x
				{
					step->command[find_empty_command_column(step)] = MUS_FX_GLISSANDO_CONTROL | (param & 0xf);
					break;
				}

				case 2: //S2x
				{
					Uint8 finetune = 0;

					if((param & 0xf) < 8)
					{
						finetune = 0x80 + ((command & 0xf) << 4);
					}

					else
					{
						finetune = 0x80 - ((0x10 - (command & 0xf)) << 4);
					}

					step->command[find_empty_command_column(step)] = MUS_FX_PITCH | finetune;

					break;
				}

				case 3: //S3x
				{
					//klystrack does not have this command yet
					break;
				}

				case 4: //S4x
				{
					//klystrack does not have this command yet
					break;
				}

				case 5: //S5x
				{
					//klystrack does not have this command yet
					break;
				}

				case 6: //S6x
				{
					//klystrack does not have this command yet
					break;
				}

				case 8: //S8x
				{
					step->command[find_empty_command_column(step)] = MUS_FX_SET_PANNING | ((param & 0xf) << 4);
					break;
				}

				case 9: //S9x
				{
					//klystrack does not have this command yet
					break;
				}

				case 0xA: //SAx
				{
					//klystrack does not have this command yet
					break;
				}

				case 0xB: //SBx
				{
					step->command[find_empty_command_column(step)] = MUS_FX_FT2_PATTERN_LOOP | (param & 0xf);
					break;
				}

				case 0xC: //SBx
				{
					step->command[find_empty_command_column(step)] = MUS_FX_EXT_NOTE_CUT | (param & 0xf);
					break;
				}

				case 0xD: //SBx
				{
					step->command[find_empty_command_column(step)] = MUS_FX_EXT_NOTE_DELAY | (param & 0xf);
					break;
				}

				default: break;
			}

			break;
		}

		case 20: //Txx
		{
			if(param >= 0x20)
			{
				step->command[find_empty_command_column(step)] = MUS_FX_SET_RATE | ((Uint16)param * 50 / 125);
			}
			
			break;
		}

		case 21: //Uxy
		{
			step->command[find_empty_command_column(step)] = MUS_FX_VIBRATO | my_max(1, ((param & 0xf0) >> 4 / 4 << 4)) | my_max(1, ((param & 0xf) / 4));
			break;
		}

		case 22: //Vxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_SET_GLOBAL_VOLUME | (param * 2);
			break;
		}

		case 23: //Wxy
		{
			//nope no command in klystrack yet cope
			break;
		}

		case 24: //Xxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_SET_PANNING | my_min(0xff, (param * 2));
			break;
		}

		case 25: //Yxx
		{
			step->command[find_empty_command_column(step)] = MUS_FX_PANBRELLO | param;
			break;
		}

		default: break;
	}
}