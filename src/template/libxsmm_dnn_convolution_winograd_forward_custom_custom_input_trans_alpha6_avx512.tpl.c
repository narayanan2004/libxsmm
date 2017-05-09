/******************************************************************************
** Copyright (c) 2016-2017, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Kunal Banerjee (Intel Corp.), Jongsoo Park (Intel Corp.)
******************************************************************************/

const int total_tiles = handle->cwino_fwd.itiles*handle->cwino_fwd.jtiles;
LIBXSMM_VLA_DECL(4, const float, input, inp, handle->ifhp, handle->ifwp, TDVLEN);
LIBXSMM_VLA_DECL(5, float, output, tinp, ALPHA, handle->blocksifm*handle->cwino_fwd.bimg, total_tiles, TDVLEN);
LIBXSMM_VLA_DECL(4, float, Iw, Iwp, ALPHA, ALPHA, TDVLEN);
__m512 I[ALPHA];
unsigned int ti, tj;
int i, j;
int xdim, ydim;
__m512 T[ALPHA][ALPHA]; /* FIXME: too big and causing spills */
__m512 t0, t1, t2, t3, t4, t5;

for (tj = 0; tj < handle->cwino_fwd.jtiles; tj++) {
  for (ti = 0; ti < handle->cwino_fwd.itiles; ti++) { /* for each tile */
    if (ti*(ALPHA-2) >= handle->desc.pad_w && ti*(ALPHA-2) + ALPHA <= handle->desc.W + handle->desc.pad_w &&
        tj*(ALPHA-2) >= handle->desc.pad_h && tj*(ALPHA-2) + ALPHA <= handle->desc.H + handle->desc.pad_h) { /* common case */

      /* left multiplication */
      /* this unrolling didn't help performance much so we may want to remove later if code size becomes an issue */
      LIBXSMM_PRAGMA_UNROLL_N(ALPHA)
      for (i = 0; i < ALPHA; i++) {
        xdim = ti*(ALPHA - 2) - handle->desc.pad_w + handle->desc.pad_w_in + i;
        ydim = tj*(ALPHA - 2) - handle->desc.pad_h + handle->desc.pad_h_in;

        /* HW prefetcher should be able to cover these sequential accesses */
        I[0] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 0, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
        I[1] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 1, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
        I[2] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 2, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
        I[3] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 3, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
        I[4] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 4, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
        I[5] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim + 5, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));

        t0 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), I[2], I[4]);
        t1 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), I[1], I[3]);
        t2 = _mm512_sub_ps(I[4], I[2]);
        t3 = _mm512_sub_ps(I[3], I[1]);
        t4 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), I[2], I[4]);
        t5 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), I[3], I[5]);

        T[0][i] = _mm512_fmadd_ps(_mm512_set1_ps(4.0f), I[0], t4);
        T[1][i] = _mm512_add_ps(t0, t1);
        T[2][i] = _mm512_sub_ps(t0, t1);
        T[3][i] = _mm512_fmadd_ps(_mm512_set1_ps(2.0f), t3, t2);
        T[4][i] = _mm512_fnmadd_ps(_mm512_set1_ps(2.0f), t3, t2);
        T[5][i] = _mm512_fmadd_ps(_mm512_set1_ps(4.0f), I[1], t5);
      }

      /* right multiplication */
      /* this unrolling didn't help performance much so we may want to remove later if code size becomes an issue */
      LIBXSMM_PRAGMA_UNROLL_N(ALPHA)
      for (j = 0; j < ALPHA; j++) {
        t0 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), T[j][2], T[j][4]);
        t1 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), T[j][1], T[j][3]);
        t2 = _mm512_sub_ps(T[j][4], T[j][2]);
        t3 = _mm512_sub_ps(T[j][3], T[j][1]);
        t4 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), T[j][2], T[j][4]);
        t5 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), T[j][3], T[j][5]);

        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 0, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(4.0f), T[j][0], t4));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 1, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_add_ps(t0, t1));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 2, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_sub_ps(t0, t1));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 3, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(2.0f), t3, t2));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 4, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fnmadd_ps(_mm512_set1_ps(2.0f), t3, t2));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 5, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(4.0f), T[j][1], t5));
      }
    }
    else { /* corner case */
      /* left multiplication */
      for (i = 0; i < ALPHA; i++) {
        xdim = ti*(ALPHA - 2) - handle->desc.pad_w + handle->desc.pad_w_in + i;
        if ((xdim < handle->desc.pad_w_in) || (xdim >= handle->desc.W + handle->desc.pad_w_in)) {
          T[0][i] = _mm512_setzero_ps();
          T[1][i] = _mm512_setzero_ps();
          T[2][i] = _mm512_setzero_ps();
          T[3][i] = _mm512_setzero_ps();
          T[4][i] = _mm512_setzero_ps();
          T[5][i] = _mm512_setzero_ps();
        } else {
          for (j = 0; j < LIBXSMM_MIN(handle->desc.pad_h - (int)tj*(ALPHA - 2), ALPHA); j++) {
            I[j] = _mm512_setzero_ps();
          }
          for ( ; j < LIBXSMM_MIN(handle->desc.H + handle->desc.pad_h - (int)tj*(ALPHA - 2), ALPHA); j++) {
            ydim = tj*(ALPHA - 2) - handle->desc.pad_h + handle->desc.pad_h_in + j;
            I[j] = LIBXSMM_INTRINSICS_MM512_LOAD_PS(&LIBXSMM_VLA_ACCESS(4, input, 0, ydim, xdim, 0, handle->ifhp, handle->ifwp, TDVLEN));
          }
          for ( ; j < ALPHA; j++) {
            I[j] = _mm512_setzero_ps();
          }

          t0 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), I[2], I[4]);
          t1 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), I[1], I[3]);
          t2 = _mm512_sub_ps(I[4], I[2]);
          t3 = _mm512_sub_ps(I[3], I[1]);
          t4 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), I[2], I[4]);
          t5 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), I[3], I[5]);

          T[0][i] = _mm512_fmadd_ps(_mm512_set1_ps(4.0f), I[0], t4);
          T[1][i] = _mm512_add_ps(t0, t1);
          T[2][i] = _mm512_sub_ps(t0, t1);
          T[3][i] = _mm512_fmadd_ps(_mm512_set1_ps(2.0f), t3, t2);
          T[4][i] = _mm512_fnmadd_ps(_mm512_set1_ps(2.0f), t3, t2);
          T[5][i] = _mm512_fmadd_ps(_mm512_set1_ps(4.0f), I[1], t5);
        }
      }

      /* right multiplication */
      /* this unrolling didn't help performance much so we may want to remove later if code size becomes an issue */
      LIBXSMM_PRAGMA_UNROLL_N(ALPHA)
      for (j = 0; j < ALPHA; j++) {
        t0 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), T[j][2], T[j][4]);
        t1 = _mm512_fnmadd_ps(_mm512_set1_ps(4.0f), T[j][1], T[j][3]);
        t2 = _mm512_sub_ps(T[j][4], T[j][2]);
        t3 = _mm512_sub_ps(T[j][3], T[j][1]);
        t4 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), T[j][2], T[j][4]);
        t5 = _mm512_fnmadd_ps(_mm512_set1_ps(5.0f), T[j][3], T[j][5]);

        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 0, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(4.0f), T[j][0], t4));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 1, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_add_ps(t0, t1));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 2, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_sub_ps(t0, t1));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 3, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(2.0f), t3, t2));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 4, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fnmadd_ps(_mm512_set1_ps(2.0f), t3, t2));
        _mm512_store_ps(
            &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, 5, 0, ALPHA, ALPHA, TDVLEN),
            _mm512_fmadd_ps(_mm512_set1_ps(4.0f), T[j][1], t5));
      }
    }
  } /* for each tile */
}
/* temporal buffer Iw has shape total_tiles*ALPHA*ALPHA*TDVLEN */
/* We have this separate loop to avoid long stride accesses */
/* If we directly write to output in the loop above, we'd have blocksifm*bimg*4*4*64 Byte strides */
/* In this loop, the stride is reduced to 64 B */
/* Since now we're using streaming stores long stride accesses might not be an issue but
 * moving streaming stores to the loop above slowed things down. */
/* the order of this loop is optimized for the target array which bigger */
for (j = 0; j < ALPHA; j++) {
  for (i = 0; i < ALPHA; i++) {
    for (tj = 0; tj < handle->cwino_fwd.jtiles; tj++) {
      for (ti = 0; ti < handle->cwino_fwd.itiles; ti++) {
        /* this prefetch didn't help performance much so we may want to remove it later */
        _mm_prefetch(
            (const char *)&LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, i + 1, 0, ALPHA, ALPHA, TDVLEN),
            _MM_HINT_T0);
        /* using streaming store here may actually hurt performance if batch size is small
         * so that the entire output fits in on-chip cache */
        LIBXSMM_INTRINSICS_MM512_STREAM_PS(
            &LIBXSMM_VLA_ACCESS(5, output, j, i, 0, tj*handle->cwino_fwd.itiles + ti, 0, ALPHA, handle->blocksifm*handle->cwino_fwd.bimg, total_tiles, TDVLEN),
            LIBXSMM_INTRINSICS_MM512_LOAD_PS(
                &LIBXSMM_VLA_ACCESS(4, Iw, tj*handle->cwino_fwd.itiles + ti, j, i, 0, ALPHA, ALPHA, TDVLEN)));
      }
    }
  }
}
/* output buffer has shape ALPHA*ALPHA*(blocksifm*cwino_fwd.bimg)*total_tiles*TDVLEN */
/* typically blocksifm = C/TDVLEN ifmblock = TDVLEN */
