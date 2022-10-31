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

#include "visu.h"
#include "gui/bevel.h"
#include "mybevdefs.h"
#include "mused.h"
#include "snd/freqs.h"
#include "gfx/gfx.h"
#include "theme.h"

extern Uint32 colors[NUM_COLORS];

//Fast Fourier Transform (I honestly don't know this black magic I just want a spectrum)
//From https://ru.m.wikibooks.org/wiki/%D0%A0%D0%B5%D0%B0%D0%BB%D0%B8%D0%B7%D0%B0%D1%86%D0%B8%D0%B8_%D0%B0%D0%BB%D0%B3%D0%BE%D1%80%D0%B8%D1%82%D0%BC%D0%BE%D0%B2/%D0%91%D1%8B%D1%81%D1%82%D1%80%D0%BE%D0%B5_%D0%BF%D1%80%D0%B5%D0%BE%D0%B1%D1%80%D0%B0%D0%B7%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5_%D0%A4%D1%83%D1%80%D1%8C%D0%B5#C

#define  NUMBER_IS_2_POW_K(x)   ((!((x)&((x)-1)))&&((x)>1))  // x is pow(2, k), k = 1, 2, ...
#define  FT_DIRECT        -1    // Direct transform.
#define  FT_INVERSE        1    // Inverse transform.

bool FFT(float *Rdat, float *Idat, int N, int LogN, int Ft_Flag)
{
	// parameters error check:
	if((Rdat == NULL) || (Idat == NULL)) return false;
	if((N > 16384) || (N < 1)) return false;
	if(!NUMBER_IS_2_POW_K(N)) return false;
	if((LogN < 2) || (LogN > 14)) return false;
	if((Ft_Flag != FT_DIRECT) && (Ft_Flag != FT_INVERSE)) return false;

	register int i, j, n, k, io, ie, in, nn;
	float ru, iu, rtp, itp, rtq, itq, rw, iw, sr;
	
	static const float Rcoef[14] =
	{	-1.0000000000000000F, 0.0000000000000000F, 0.7071067811865475F,
		0.9238795325112867F, 0.9807852804032304F, 0.9951847266721969F,
		0.9987954562051724F, 0.9996988186962042F, 0.9999247018391445F,
		0.9999811752826011F, 0.9999952938095761F, 0.9999988234517018F,
		0.9999997058628822F, 0.9999999264657178F
	};
	
	static const float Icoef[14] =
	{	 0.0000000000000000F, -1.0000000000000000F, -0.7071067811865474F,
		 -0.3826834323650897F, -0.1950903220161282F, -0.0980171403295606F,
		 -0.0490676743274180F, -0.0245412285229122F, -0.0122715382857199F,
		 -0.0061358846491544F, -0.0030679567629659F, -0.0015339801862847F,
		 -0.0007669903187427F, -0.0003834951875714F
	};
	
	nn = N >> 1;
	ie = N;
	
	for(n = 1; n <= LogN; n++)
	{
		rw = Rcoef[LogN - n];
		iw = Icoef[LogN - n];
	
		if(Ft_Flag == FT_INVERSE) iw = -iw;
	
		in = ie >> 1;
		ru = 1.0F;
		iu = 0.0F;
	
		for(j = 0; j < in; j++)
		{
			for(i = j; i < N; i += ie)
			{
				io = i + in;
				rtp = Rdat[i] + Rdat[io];
				itp = Idat[i] + Idat[io];
				rtq = Rdat[i] - Rdat[io];
				itq = Idat[i] - Idat[io];
				Rdat[io] = rtq * ru - itq * iu;
				Idat[io] = itq * ru + rtq * iu;
				Rdat[i] = rtp;
				Idat[i] = itp;
			}

			sr = ru;
			ru = ru * rw - iu * iw;
			iu = iu * rw + sr * iw;
		}

		ie >>= 1;
	}

	for(j = i = 1; i < N; i++)
	{
		if(i < j)
		{
			io = i - 1;
			in = j - 1;
			rtp = Rdat[in];
			itp = Idat[in];
			Rdat[in] = Rdat[io];
			Idat[in] = Idat[io];
			Rdat[io] = rtp;
			Idat[io] = itp;
		}

		k = nn;

		while(k < j)
		{
			j = j - k;
			k >>= 1;
		}

		j = j + k;
	}

	return true;
}

#define FFT_BUFFER_SIZE 8192

void do_fft(int num_peaks)
{
	for(int i = 0; i < FFT_BUFFER_SIZE; ++i)
	{
		mused.real_buffer[i] = (float)mused.output_buffer[i] / 32768.0; //0...1 amplitude
		mused.imaginary_buffer[i] = 0.0; //Jesus I just have an audio signal!!! WHY COMPLEX MAGIC?!!! WHY????!!!!!
	}
	
	if(FFT(mused.real_buffer, mused.imaginary_buffer, FFT_BUFFER_SIZE, 13, FT_DIRECT))
	{
		for(int i = 0; i < 64; ++i)
		{
			mused.real_buffer[i] /= 32.0 / ((float)i / 2.0); //suppressing the bottommost freqs
			mused.imaginary_buffer[i] /= 32.0 / ((float)i / 2.0);
		}
		
		for(int i = 0; i < num_peaks; ++i)
		{
			float amplitude = 0.0;
			
			int scale = FFT_BUFFER_SIZE / num_peaks / 6;
			
			for(int j = 0; j < scale; ++j)
			{
				float init_amplitude = sqrt(mused.real_buffer[(scale) * i + j] * mused.real_buffer[(scale) * i + j] + mused.imaginary_buffer[(scale) * i + j] * mused.imaginary_buffer[(scale) * i + j]); //raw data, amplitude is half the array size
				
				amplitude += init_amplitude / 18.0; //0..(8192 / num_peaks) range
			}
			
			amplitude /= (((float)FFT_BUFFER_SIZE / 256.0) / (float)num_peaks);  //0..1 range
			
			mused.vis.spec_peak[i] = (int)(amplitude);
		}
	}
}

void spectrum_analyzer_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect content;
	copy_rect(&content, dest);
	
	SDL_Rect clip;
	gfx_domain_get_clip(domain, &clip);
	gfx_domain_set_clip(domain, &content);
	
	int spec[255] = { 0 };
	
	if(mused.flags2 & SHOW_OLD_SPECTRUM_VIS)
	{
		for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
		{
			Uint8 note = (mused.stat_note[i] & 0xff00) >> 8;
			spec[note] = my_max(spec[note], mused.vis.cyd_env[i]);
			if (note > 0) spec[note - 1] = my_max(spec[note] / 3, spec[note - 1]);
			if (note < 255) spec[note + 1] = my_max(spec[note] / 3, spec[note + 1]);
		}
		
		for (int i = 0; i < 96; ++i)
		{
			if (spec[i] >= mused.vis.spec_peak[i])
				mused.vis.spec_peak_decay[i] = 0;
			mused.vis.spec_peak_decay[i] = my_min(64, mused.vis.spec_peak_decay[i] + 1);
			mused.vis.spec_peak[i] = my_max(0, my_max(mused.vis.spec_peak[i], spec[i]) - my_min(1, my_max(0, /*mused.vis.spec_peak_decay[i] - 20*/ 2)) * 4);
		}
		
		const int w = mused.analyzer->surface->w / 2;
		
		SDL_Rect bar = { content.x, 0, w, 0 };
		SDL_Rect src = { 0, 0, w, content.h };
		
		for (int i = (MIDDLE_C - content.w / w / 2 + 12); i < FREQ_TAB_SIZE && bar.x < content.x + content.w; ++i, bar.x += bar.w)
		{
			if (i >= 0)
			{
				/*bar.h = mused.vis.spec_peak[i] * content.h / MAX_VOLUME;
				bar.y = content.y + content.h - bar.h;
				
				SDL_FillRect(mused.screen, &bar, 0x404040);*/
				
				src.x = 0;
				src.y = 0;
				src.w = w;
				src.h = content.h;
				
				bar.h = content.h;
				bar.y = content.y;
				
				SDL_Rect temp;
				copy_rect(&temp, &bar);
				
				my_BlitSurface(mused.analyzer, &src, dest_surface, &temp);
				
				bar.h = my_min(MAX_VOLUME, mused.vis.spec_peak[i]) * content.h / MAX_VOLUME;
				bar.y = content.y + content.h - bar.h;
				
				src.h = my_min(MAX_VOLUME, mused.vis.spec_peak[i]) * content.h / MAX_VOLUME;
				src.y = mused.analyzer->surface->h - bar.h;
				src.h = bar.h;
				src.x = w;
				
				copy_rect(&temp, &bar);
				
				temp.y += 1;
				
				my_BlitSurface(mused.analyzer, &src, dest_surface, &temp);
			}
		}
	}
	
	else
	{
		gfx_rect(dest_surface, dest, colors[COLOR_WAVETABLE_BACKGROUND]);
		
		Uint16 num_peaks = (mused.analyzer->surface->w / 2) * 31 < (content.w) ? 64 : 32;
		
		if(num_peaks == 64)
		{
			num_peaks = (mused.analyzer->surface->w / 2) * 63 < (content.w) ? 128 : 64;
		}
		
		if(num_peaks == 128)
		{
			num_peaks = (mused.analyzer->surface->w / 2) * 127 < (content.w) ? 256 : 128;
		}
		
		do_fft(num_peaks);
		
		//const int w = mused.analyzer->surface->w / 2;
		const int w = (content.w + num_peaks + 1) / num_peaks - 1;
		
		//mused.analyzer->surface->w = w * 2;
		
		SDL_Rect bar = { content.x, 0, w, 0 };
		SDL_Rect src = { 0, 0, w, content.h };
		
		for (int i = 0; i < num_peaks; ++i, bar.x += bar.w + 1)
		{
			src.x = 0;
			src.y = 0;
			src.w = w;
			src.h = content.h;
			
			bar.h = content.h;
			bar.y = content.y;
			
			SDL_Rect temp;
			copy_rect(&temp, &bar);
			
			my_BlitSurface(mused.analyzer, &src, dest_surface, &temp);
			
			bar.h = my_min(128, mused.vis.spec_peak[i]) * content.h / 128;
			bar.y = content.y + content.h - bar.h;
			
			src.h = my_min(128, mused.vis.spec_peak[i]) * content.h / 128;
			src.y = mused.analyzer->surface->h - bar.h;
			src.h = bar.h;
			src.x = mused.analyzer->surface->w / 2;
			
			copy_rect(&temp, &bar);
			
			temp.y += 1;
			
			my_BlitSurface(mused.analyzer, &src, dest_surface, &temp);
		}
	}
	
	gfx_domain_set_clip(dest_surface, &clip);
}


void catometer_view(GfxDomain *dest_surface, const SDL_Rect *dest, const SDL_Event *event, void *param)
{
	SDL_Rect content;
	copy_rect(&content, dest);
	
	gfx_rect(dest_surface, &content, colors[COLOR_OF_BACKGROUND]);
	
	SDL_Rect clip, cat;
	copy_rect(&cat, &content);
	cat.w = mused.catometer->surface->w;
	cat.x = cat.x + content.w / 2 - mused.catometer->surface->w / 2;
	gfx_domain_get_clip(domain, &clip);
	gfx_domain_set_clip(domain, &content);
	my_BlitSurface(mused.catometer, NULL, dest_surface, &cat);
	
	int v = 0;
	
	for (int i = 0; i < MUS_MAX_CHANNELS; ++i)
	{
		v += mused.vis.cyd_env[i];
	}
	
	float a = ((float)v * M_PI / (MAX_VOLUME * 4) + M_PI) * 0.25 + mused.vis.prev_a * 0.75;
	
	if (a < M_PI)
		a = M_PI;
		
	if (a > M_PI * 2)
		a = M_PI * 2;
	
	mused.vis.prev_a = a;
	
	int ax = cos(a) * 12;
	int ay = sin(a) * 12;
	int eye1 = 31;
	int eye2 = -30;
	
	for (int w = -3; w <= 3; ++w)
	{
		gfx_line(dest_surface, dest->x + dest->w / 2 + eye1 + w, dest->y + dest->h / 2 + 6, dest->x + dest->w / 2 + ax + eye1, dest->y + dest->h / 2 + ay + 6, colors[COLOR_CATOMETER_EYES]);
		gfx_line(dest_surface, dest->x + dest->w / 2 + eye2 + w, dest->y + dest->h / 2 + 6, dest->x + dest->w / 2 + ax + eye2, dest->y + dest->h / 2 + ay + 6, colors[COLOR_CATOMETER_EYES]);
	}
	
	gfx_domain_set_clip(dest_surface, &clip);
}
