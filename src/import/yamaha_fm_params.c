/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2022 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

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

#include "yamaha_fm_params.h"

#define envelope_length(slope) (slope != 0 ? (float)(((slope) * (slope) * 256 / (ENVELOPE_SCALE * ENVELOPE_SCALE))) / ((float)CYD_BASE_FREQ / 1000.0f) : 0.0f)

static float OPN_attack_rate_lengths[OPN_AR_VALUES] = 
{
	INFINITY,
	7088.0,
	4706.0,
	3599.0,
	2410.0,
	1800.0,
	1204.0,
	900.0,
	602.0,
	450.0,
	225.0,
	151.0,
	113.0,
	76.0,
	57.0,
	38.0,
	28.0,
	19.0,
	14.0,
	10.0,
	8.0,
	5.0,
	4.0,
	3.0,
	2.0,
	1.0,
	1.0,
	0.9,
	0.5,
	0.0,
	0.0,
	
	/* here and later data in ms */
	/* measured from wav exported from BambooTracker */
};

static float OPN_decay_rate_lengths[OPN_DR_VALUES] = 
{
	999999.0, //INFINITY,
	70000.0,
	40000.0,
	32000.0,
	21000.0,
	16000.0,
	11000.0,
	9000.0,
	5804.0,
	4353.0,
	2902.0,
	2177.0,
	1452.0,
	1088.0,
	726.0,
	543.0,
	362.0,
	272.0,
	181.0,
	136.0,
	90.0,
	68.0,
	45.0,
	34.0,
	23.0,
	17.0,
	11.0,
	9.0,
	6.5,
	5.4,
	3.9,
	3.8,
};

static float OPN_sustain_rate_lengths[OPN_SR_VALUES] = 
{
	999999.0, //INFINITY,
	70000.0,
	40000.0,
	32000.0,
	21000.0,
	16000.0,
	11000.0,
	9000.0,
	5804.0,
	4353.0,
	2902.0,
	2177.0,
	1452.0,
	1088.0,
	726.0,
	543.0,
	362.0,
	272.0,
	181.0,
	136.0,
	90.0,
	68.0,
	45.0,
	34.0,
	23.0,
	17.0,
	11.0,
	9.0,
	6.5,
	5.4,
	3.9,
	3.8,
};

static float OPN_release_rate_lengths[OPN_RR_VALUES] = 
{
	80845.0,
	40420.0,
	20223.0,
	9733.0,
	4865.0,
	2437.0,
	1218.0,
	607.0,
	304.0,
	151.0,
	75.0,
	38.0,
	19.0,
	9.5,
	4.5,
	4.5,
};

Sint8 find_match(Uint8 param, CydEngine* cyd, Uint8 arr_len, float* array, Uint8 type)
{
	float min = 9999999.0;
	Sint8 best_index = 0;
	
	for(int i = 0; i < 64; ++i)
	{
		if(fabs(array[param] - envelope_length(i)) < min)
		{
			min = fabs(array[param] - envelope_length(i));
			best_index = i;
		}
	}
	
	return type == YAMAHA_PARAM_SR ? 63 - best_index : best_index;
}

Sint8 find_closest_match(Sint8 param, Uint8 type, CydEngine* cyd, Uint8 chip_type)
{
	switch(type)
	{
		case YAMAHA_PARAM_AR:
		{
			switch(chip_type)
			{
				case YAMAHA_CHIP_OPN:
				{
					return find_match(param, cyd, OPN_AR_VALUES, (float*)&OPN_attack_rate_lengths, type);
					break;
				}
				
				default: break;
			}
		}
		
		case YAMAHA_PARAM_DR:
		{
			switch(chip_type)
			{
				case YAMAHA_CHIP_OPN:
				{
					return find_match(param, cyd, OPN_DR_VALUES, (float*)&OPN_decay_rate_lengths, type);
					break;
				}
				
				default: break;
			}
		}
		
		case YAMAHA_PARAM_SR:
		{
			switch(chip_type)
			{
				case YAMAHA_CHIP_OPN:
				{
					return find_match(param, cyd, OPN_SR_VALUES, (float*)&OPN_sustain_rate_lengths, type);
					break;
				}
				
				default: break;
			}
		}
		
		case YAMAHA_PARAM_RR:
		{
			switch(chip_type)
			{
				case YAMAHA_CHIP_OPN:
				{
					return find_match(param, cyd, OPN_RR_VALUES, (float*)&OPN_release_rate_lengths, type);
					break;
				}
				
				default: break;
			}
		}
		
		default: break;
	}
	
	return -1;
}

Sint8 reinterpret_yamaha_params(Sint8 param, Uint8 type, Uint16 song_rate, /* <- for LFO speed */ CydEngine* cyd, /* <- for envelope length */ Uint8 chip_type, /* OPNx, OPLx or OPM */ Uint8 dialect)
{
	switch(type)
	{
		case YAMAHA_PARAM_PAN:
		{
			switch(dialect)
			{
				case BTN_PMD:
				{
					
					break;
				}
				
				default: break;
			}
			
			break;
		}
		
		case YAMAHA_PARAM_FL:
		{
			switch(dialect)
			{
				case BTN_PMD:
				{
					return param > 1 ? (param - 1) * 2 : param * 2; //mapping feedback
					break;
				}
				
				default: break;
			}
			
			break;
		}
		
		case YAMAHA_PARAM_CON:
		{
			switch(dialect)
			{
				case BTN_PMD:
				{
					if(chip_type == YAMAHA_CHIP_OPM || chip_type == YAMAHA_CHIP_OPN) //in OPL case we would process it manually in process_mml_string()
					{
						switch(param)
						{
							case 0: return 1;
							case 1: return 2;
							case 2: case 3: return 3; //but feedback has different position
							case 4: return 6;
							case 5: return 11;
							case 6: return 9;
							case 7: return 13;
							
							default: break;
						}
					}
					
					break;
				}
				
				default: break;
			}
			
			break;
		}
		
		case YAMAHA_PARAM_AR:
		case YAMAHA_PARAM_DR:
		case YAMAHA_PARAM_SR:
		case YAMAHA_PARAM_RR:
		{
			return find_closest_match(param, type, cyd, chip_type);
			
			break;
		}
		
		case YAMAHA_PARAM_SL:
		{
			return (31 - param);
			break;
		}
		
		case YAMAHA_PARAM_TL:
		{
			return chip_type == YAMAHA_CHIP_OPL ? 127 - (param * 2) : (127 - param) * 30 / 53;
			break;
		}
		
		case YAMAHA_PARAM_MUL:
		{
			return (1 << 4) | param; //so we divide freq by 2 to account for klystrack mults being exactly two times larger
			break;
		}
		
		case YAMAHA_PARAM_DT1:
		{            //if 0..7 range   //if -3..3 range
			return param > 3 ? (4 - param) : param;
		}
		
		case YAMAHA_PARAM_DT2:
		{
			return param;
			break;
		}
		
		/*case 
		{
			
			break;
		}*/
		
		default: break;
	}
}