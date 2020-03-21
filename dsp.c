/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <arm_math.h>
#include "nanovna.h"

#ifdef ENABLED_DUMP
int16_t samp_buf[SAMPLE_LEN];
int16_t ref_buf[SAMPLE_LEN];
#endif

const int16_t sincos_tbl[48][2] = {
  { 10533,  31029 }, { 27246,  18205 }, { 32698,  -2143 }, { 24636, -21605 },
  {  6393, -32138 }, {-14493, -29389 }, {-29389, -14493 }, {-32138,   6393 },
  {-21605,  24636 }, { -2143,  32698 }, { 18205,  27246 }, { 31029,  10533 },
  { 31029, -10533 }, { 18205, -27246 }, { -2143, -32698 }, {-21605, -24636 },
  {-32138,  -6393 }, {-29389,  14493 }, {-14493,  29389 }, {  6393,  32138 },
  { 24636,  21605 }, { 32698,   2143 }, { 27246, -18205 }, { 10533, -31029 },
  {-10533, -31029 }, {-27246, -18205 }, {-32698,   2143 }, {-24636,  21605 },
  { -6393,  32138 }, { 14493,  29389 }, { 29389,  14493 }, { 32138,  -6393 },
  { 21605, -24636 }, { 2143,  -32698 }, {-18205, -27246 }, {-31029, -10533 },
  {-31029,  10533 }, {-18205,  27246 }, {  2143,  32698 }, { 21605,  24636 },
  { 32138,   6393 }, { 29389, -14493 }, { 14493, -29389 }, { -6393, -32138 },
  {-24636, -21605 }, {-32698,  -2143 }, {-27246,  18205 }, {-10533,  31029 }
};
#if 0
void generate_DSP_Table(int offset){
  float audio_freq  = 48000.0;
  // N = offset * AUDIO_SAMPLES_COUNT / audio_freq; should be integer
  // AUDIO_SAMPLES_COUNT = N * audio_freq / offset; N - minimum integer value for get integer AUDIO_SAMPLES_COUNT
  // Bandwidth on one step = audio_freq / AUDIO_SAMPLES_COUNT
  float step = 2 * VNA_PI * offset / audio_freq;
  float v = step/2;
  for (int i=0; i<AUDIO_SAMPLES_COUNT; i++){
    sincos_tbl[i][0] = sin(v)*32768.0 + 0.5;
    sincos_tbl[i][1] = cos(v)*32768.0 + 0.5;
    v+=step;
  }
}
#endif

typedef float acc_t;
acc_t acc_samp_s;
acc_t acc_samp_c;
acc_t acc_ref_s;
acc_t acc_ref_c;

void
dsp_process(int16_t *capture, size_t length)
{
  uint32_t *p = (uint32_t*)capture;
  uint32_t len = length / 2;
  uint32_t i;
  int32_t samp_s = 0;
  int32_t samp_c = 0;
  int32_t ref_s = 0;
  int32_t ref_c = 0;

  for (i = 0; i < len; i++) {
    uint32_t sr = *p++;
    int16_t ref = sr & 0xffff;
    int16_t smp = (sr>>16) & 0xffff;
#ifdef ENABLED_DUMP
    ref_buf[i] = ref;
    samp_buf[i] = smp;
#endif
    int32_t s = sincos_tbl[i][0];
    int32_t c = sincos_tbl[i][1];
    samp_s += (smp * s)>>4;
    samp_c += (smp * c)>>4;
    ref_s  += (ref * s)>>4;
    ref_c  += (ref * c)>>4;
#if 0
    uint32_t sc = *(uint32_t)&sincos_tbl[i];
    samp_s = __SMLABB(sr, sc, samp_s);
    samp_c = __SMLABT(sr, sc, samp_c);
    ref_s = __SMLATB(sr, sc, ref_s);
    ref_c = __SMLATT(sr, sc, ref_c);
#endif
  }
  acc_samp_s += samp_s;
  acc_samp_c += samp_c;
  acc_ref_s += ref_s;
  acc_ref_c += ref_c;
}

void
calculate_gamma(float gamma[2])
{
#if 1
  // calculate reflection coeff. by samp divide by ref
  float rs = acc_ref_s;
  float rc = acc_ref_c;
  float rr = rs * rs + rc * rc;
  //rr = sqrtf(rr) * 1e8;
  float ss = acc_samp_s;
  float sc = acc_samp_c;
  gamma[0] =  (sc * rc + ss * rs) / rr;
  gamma[1] =  (ss * rc - sc * rs) / rr;
#elif 0
  gamma[0] =  acc_samp_s;
  gamma[1] =  acc_samp_c;
#else
  gamma[0] =  acc_ref_s;
  gamma[1] =  acc_ref_c;
#endif
}

void
fetch_amplitude(float gamma[2])
{
  gamma[0] =  acc_samp_s * 1e-9;
  gamma[1] =  acc_samp_c * 1e-9;
}

void
fetch_amplitude_ref(float gamma[2])
{
  gamma[0] =  acc_ref_s * 1e-9;
  gamma[1] =  acc_ref_c * 1e-9;
}

void
reset_dsp_accumerator(void)
{
  acc_ref_s = 0;
  acc_ref_c = 0;
  acc_samp_s = 0;
  acc_samp_c = 0;
}
