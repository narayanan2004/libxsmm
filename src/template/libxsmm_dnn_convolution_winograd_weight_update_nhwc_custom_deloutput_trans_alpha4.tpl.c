/******************************************************************************
** Copyright (c) 2016, Intel Corporation                                     **
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
/* Kunal Banerjee (Intel Corp.)
******************************************************************************/

  int total_tiles = handle->cwino_upd.itiles*handle->cwino_upd.jtiles;
#ifdef __INTEL_COMPILER
  float (* __restrict input )[handle->ofwp][handle->blocksofm][TDVLEN] = (float (*)[*][*][TDVLEN])inp;
  float (* __restrict output)[ALPHA][(handle->blocksofm/VRATIO)*handle->cwino_upd.bimg][total_tiles][FDVLEN] = (float (*)[ALPHA][*][*][FDVLEN])tinp;
#else
  LIBXSMM_VLA_DECL(4, float, input, inp, handle->ofwp, handle->blocksofm, TDVLEN);
  LIBXSMM_VLA_DECL(5, float, output, tinp, ALPHA, (handle->blocksofm/VRATIO)*handle->cwino_upd.bimg, total_tiles, FDVLEN);
#endif
  float Iw[total_tiles][ALPHA][ALPHA][FDVLEN];
  float I[ALPHA][ALPHA][FDVLEN];
  int i;
  int j;
  int ti;
  int tj;
  int r;
  int k;
  int xdim;
  int ydim;
  float T[4][2][FDVLEN];

  for (tj = 0; tj < handle->cwino_upd.jtiles; tj++) {
    for (ti = 0; ti < handle->cwino_upd.itiles; ti++) {
      for (j = 0; j < ALPHA; j++) {
        ydim = tj*(ALPHA - 2) + j;
        if (ydim < handle->ofh) {
          for (i = 0; i < ALPHA; i++) {
            xdim = ti*(ALPHA - 2) + i;
            if (xdim < handle->ofw) {
              for (r = 0; r < VRATIO; r++) {
                LIBXSMM_PRAGMA_SIMD
                for (k = 0; k < TDVLEN; k++) {
                  I[j][i][r*TDVLEN + k] =
#ifdef __INTEL_COMPILER
                    input[ydim][xdim][r][k];
#else
                    LIBXSMM_VLA_ACCESS(4, input, ydim, xdim, r, k, handle->ofwp, handle->blocksofm, TDVLEN);
#endif
                }
              }
            } else {
              for (r = 0; r < VRATIO; r++) {
                LIBXSMM_PRAGMA_SIMD
                for (k = 0; k < TDVLEN; k++) {
                  I[j][i][r*TDVLEN + k] = 0.0f;
                }
              }
            }
          }
        } else {
          for (i = 0; i < ALPHA; i++) {
            for (r = 0; r < VRATIO; r++) {
              LIBXSMM_PRAGMA_SIMD
              for (k = 0; k < TDVLEN; k++) {
                I[j][i][r*TDVLEN + k] = 0.0f;
              }
            }
          }
        }
      }
      /*trans_F_3x3_2x2(ALPHA, FDVLEN, Iw[tj*handle->cwino_upd.itiles + ti], I);*/

      /* inline code start */
      for (i = 0; i < 2; i++) {
        LIBXSMM_PRAGMA_SIMD
        for (j = 0; j < FDVLEN; j++) {
          T[0][i][j] =  I[0][i][j];
          T[1][i][j] = (I[0][i][j] + I[1][i][j]) * 0.5f;
          T[2][i][j] = (I[0][i][j] - I[1][i][j]) * 0.5f;
          T[3][i][j] =  I[1][i][j];
        }
      }

      for (i = 0; i < 4; i++) {
        LIBXSMM_PRAGMA_SIMD
        for (j = 0; j < FDVLEN; j++) {
          Iw[tj*handle->cwino_upd.itiles + ti][i][0][j] =  T[i][0][j];
          Iw[tj*handle->cwino_upd.itiles + ti][i][1][j] = (T[i][0][j] + T[i][1][j]) * 0.5f;
          Iw[tj*handle->cwino_upd.itiles + ti][i][2][j] = (T[i][0][j] - T[i][1][j]) * 0.5f;
          Iw[tj*handle->cwino_upd.itiles + ti][i][3][j] =  T[i][1][j];
        }
      }
      /* inline code end */

    }
  }
  for (j = 0; j < ALPHA; j++) {
    for (i = 0; i < ALPHA; i++) {
      for (tj = 0; tj < handle->cwino_upd.jtiles; tj++) {
        for (ti = 0; ti < handle->cwino_upd.itiles; ti++) {
          LIBXSMM_PRAGMA_SIMD
          for (k = 0; k < FDVLEN; k++) {
#ifdef __INTEL_COMPILER
            output[j][i][0][tj*handle->cwino_upd.itiles + ti][k] =
#else
              LIBXSMM_VLA_ACCESS(5, output, j, i, 0, tj*handle->cwino_upd.itiles + ti, k, ALPHA, (handle->blocksofm/VRATIO)*handle->cwino_upd.bimg, total_tiles, FDVLEN) =
#endif
              Iw[tj*handle->cwino_upd.itiles + ti][j][i][k];
          }
        }
      }
    }
  }