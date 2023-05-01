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

#include "yamaha_fm_params.h"

static Uint8 OPN_TL_mapping[128] = 
{ //OPN TL; klys vol
	/* 0 */   0x6e, /* or 0x6e? */
	/* 1 */   0x6C,
	/* 2 */   0x69,
	/* 3 */   0x66,
	/* 4 */   0x62,
	/* 5 */   0x5F,
	/* 6 */   0x5C,
	/* 7 */   0x5a,
	/* 8 */   0x58,
	/* 9 */   0x55,
	/* 10 */  0x52,
	/* 11 */  0x50,
	/* 12 */  0x4D,
	/* 13 */  0x4B,
	/* 14 */  0x49,
	/* 15 */  0x46,
	/* 16 */  0x43,
	/* 17 */  0x41,
	/* 18 */  0x3F,
	/* 19 */  0x3D,
	/* 20 */  0x3B,
	/* 21 */  0x38,
	/* 22 */  0x36,
	/* 23 */  0x34,
	/* 24 */  0x33,
	/* 25 */  0x31,
	/* 26 */  0x2E,
	/* 27 */  0x2B,
	/* 28 */  0x2A,
	/* 29 */  0x29,
	/* 30 */  0x28,
	/* 31 */  0x27,
	/* 32 */  0x26,
	/* 33 */  0x25,
	/* 34 */  0x23,
	/* 35 */  0x20,
	/* 36 */  0x1F,
	/* 37 */  0x1E,
	/* 38 */  0x1D,
	/* 39 */  0x1C,
	/* 40 */  0x1A,
	/* 41 */  0x1A,
	/* 42 */  0x19,
	/* 43 */  0x18,
	/* 44 */  0x17,
	/* 45 */  0x16,
	/* 46 */  0x15,
	/* 47 */  0x14,
	/* 48 */  0x14,
	/* 49 */  0x14,
	/* 50 */  0x13,
	/* 51 */  0x12,
	/* 52 */  0x12,
	/* 53 */  0x11,
	/* 54 */  0x10,
	/* 55 */  0xF,
	/* 56 */  0xF,
	/* 57 */  0xF,
	/* 58 */  0xE,
	/* 59 */  0xE,
	/* 60 */  0xD,
	/* 61 */  0xD,
	/* 62 */  0xD,
	/* 63 */  0xC,
	/* 64 */  0xC,
	/* 65 */  0xC,
	/* 66 */  0xC,
	/* 67 */  0xC,
	/* 68 */  0xB,
	/* 69 */  0xB,
	/* 70 */  0xB,
	/* 71 */  0xB,  
	/* 72 */  0xB,
	/* 73 */  0xB,
	/* 74 */  0xB,
	/* 75 */  0xA,
	/* 76 */  0xA,
	/* 77 */  0xA,
	/* 78 */  0xA,
	/* 79 */  0xA,
	/* 80 */  9,
	/* 81 */  9,
	/* 82 */  9,
	/* 83 */  9,
	/* 84 */  9,
	/* 85 */  8,
	/* 86 */  8,
	/* 87 */  8,
	/* 88 */  8,
	/* 89 */  7,
	/* 90 */  7,
	/* 91 */  7,
	/* 92 */  7,
	/* 93 */  7,
	/* 94 */  7,
	/* 95 */  6,
	/* 96 */  6,
	/* 97 */  6,
	/* 98 */  6,
	/* 99 */  6,
	/* 100 */ 5,
	/* 101 */ 5,
	/* 102 */ 5,
	/* 103 */ 5,
	/* 104 */ 4,
	/* 105 */ 4,
	/* 106 */ 4,
	/* 107 */ 4,
	/* 108 */ 4,
	/* 109 */ 4,
	/* 110 */ 4, 
	/* 111 */ 4,
	/* 112 */ 3,
	/* 113 */ 3,
	/* 114 */ 3,
	/* 115 */ 3,
	/* 116 */ 2,
	/* 117 */ 2,
	/* 118 */ 2,
	/* 119 */ 2,
	/* 120 */ 1,
	/* 121 */ 1,
	/* 122 */ 1,
	/* 123 */ 1,
	/* 124 */ 0,
	/* 125 */ 0,
	/* 126 */ 0,
	/* 127 */ 0,
};

static Uint8 OPN_FL_mapping[8] = 
{ //OPN FL; klys feedback
	/* 0 */ 0,
	/* 1 */ 2,
	/* 2 */ 4,
	/* 3 */ 6,
	/* 4 */ 8,
	/* 5 */ 0xB,
	/* 6 */ 0xD,
	/* 7 */ 0xF,
};

static Uint8 OPN_attack_rate_lengths[OPN_AR_VALUES] = 
{
	0xFF, //999999.0, //INFINITY,
	0x31, //7088.0,
	0x28, //4706.0,
	0x23, //3599.0,
	0x1D, //2410.0,
	0x19, //1800.0,
	0x14, //1204.0,
	0x12, //900.0,
	0xE, //602.0,
	0xC, //450.0,
	9, //225.0,
	7, //151.0,
	6, //113.0,
	5, //76.0,
	4, //57.0,
	3, //38.0,
	3, //28.0,
	2, //19.0,
	2, //14.0,
	2, //10.0,
	2, //8.0,
	1, //5.0,
	1, //4.0,
	1, //3.0,
	1, //2.0,
	1, //1.0,
	1, //1.0,
	1, //0.9,
	1, //0.5,
	0, //0.0,
	0, //0.0,
	
	/* here and later data in ms */
	/* measured from wav exported from BambooTracker */
};

static Uint8 OPN_decay_rate_lengths[OPN_DR_VALUES] = 
{
	0xFF, //999999.0, //INFINITY,
	0x9C, //70000.0,
	0x75, //40000.0,
	0x69, //32000.0, /* NICE! */
	0x55, //21000.0,
	0x4A, //16000.0,
	0x3E, //11000.0,
	0x38, //9000.0,
	0x2D, //5804.0,
	0x27, //4353.0,
	0x20, //2902.0,
	0x1B, //2177.0,
	0x16, //1452.0,
	0x13, //1088.0,
	0x10, //726.0,
	0xE, //543.0,
	0xB, //362.0,
	0xA, //272.0,
	7, //181.0,
	7, //136.0,
	6, //90.0,
	5, //68.0,
	4, //45.0,
	3, //34.0,
	3, //23.0,
	2, //17.0,
	2, //11.0,
	2, //9.0,
	2, //6.5,
	2, //5.4,
	1, //3.9,
	1, //3.8,
};

static Uint8 OPN_sustain_rate_lengths[OPN_SR_VALUES] = 
{
	0xFF - 0xFF, //999999.0, //INFINITY,
	0xFF - 0x9C, //70000.0,
	0xFF - 0x75, //40000.0,
	0xFF - 0x69, //32000.0, /* NICE! */
	0xFF - 0x55, //21000.0,
	0xFF - 0x4A, //16000.0,
	0xFF - 0x3E, //11000.0,
	0xFF - 0x38, //9000.0,
	0xFF - 0x2D, //5804.0,
	0xFF - 0x27, //4353.0,
	0xFF - 0x20, //2902.0,
	0xFF - 0x1B, //2177.0,
	0xFF - 0x16, //1452.0,
	0xFF - 0x13, //1088.0,
	0xFF - 0x10, //726.0,
	0xFF - 0xE, //543.0,
	0xFF - 0xB, //362.0,
	0xFF - 0xA, //272.0,
	0xFF - 7, //181.0,
	0xFF - 7, //136.0,
	0xFF - 6, //90.0,
	0xFF - 5, //68.0,
	0xFF - 4, //45.0,
	0xFF - 3, //34.0,
	0xFF - 3, //23.0,
	0xFF - 2, //17.0,
	0xFF - 2, //11.0,
	0xFF - 2, //9.0,
	0xFF - 2, //6.5,
	0xFF - 2, //5.4,
	0xFF - 1, //3.9,
	0xFF - 1, //3.8,
};

static Uint8 OPN_release_rate_lengths[OPN_RR_VALUES] = 
{
	0xA7, //80845.0,
	0x76, //40420.0,
	0x54, //20223.0,
	0x3A, //9733.0,
	0x29, //4865.0,
	0x1D, //2437.0,
	0x14, //1218.0,
	0xE, //607.0,
	0xA, //304.0,
	7, //151.0,
	5, //75.0,
	4, //38.0,
	3, //19.0,
	2, //9.5,
	1, //4.5,
	1, //4.5,
};

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
					//return OPN_attack_rate_lengths[param] > 1 ? OPN_attack_rate_lengths[param] / 2 : OPN_attack_rate_lengths[param];
					return OPN_attack_rate_lengths[param];
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
					return param == 0 ? OPN_decay_rate_lengths[param] : (OPN_decay_rate_lengths[param] > 1 ? OPN_decay_rate_lengths[param] / 2 : OPN_decay_rate_lengths[param]);
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
					return OPN_sustain_rate_lengths[param] < 0x80 ? OPN_sustain_rate_lengths[param] * 2 : OPN_sustain_rate_lengths[param];
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
					return param == 0 ? OPN_release_rate_lengths[param] : (OPN_release_rate_lengths[param] > 1 ? OPN_release_rate_lengths[param] / 2 : OPN_release_rate_lengths[param]);
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
					return OPN_FL_mapping[param]; //mapping feedback
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
			return (31 - (param == 15 ? 31 : param * 2));
			break;
		}
		
		case YAMAHA_PARAM_TL:
		{
			return chip_type == YAMAHA_CHIP_OPL ? OPN_TL_mapping[param * 2] : OPN_TL_mapping[param];
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