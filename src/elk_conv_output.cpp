#include <assert.h>
#include <x86intrin.h>
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"
#include "elk_conv.hpp"

namespace euler {

template <typename T, const int A, const int K, const int V, const int I>
void elk_product_trans_output(elx_conv_t<T> &xc, T *tinput, T *tweights,
                              T *output, int _ih2, int _iw2) {}




template <>
void elk_product_trans_output<float, 5, 3, 16, ISA_GENERIC>(
    elx_conv_t<float> &xc, float *tinput, float *tweights, float *output,
    int _ih2, int _iw2) {
#undef T
#undef F
#undef W
#define T(h, w) t##h##w[_OV]
#define F(h, w) atinput[_ic2][_ih2][_iw2][h][w][_IV]
#define W(h, w) atweights[_ic2][h][w][_IV][_OV]

  float t00[16], t01[16], t02[16], t03[16], t04[16], t10[16], t11[16], t12[16],
      t13[16], t14[16], t20[16], t21[16], t22[16], t23[16], t24[16], t30[16],
      t31[16], t32[16], t33[16], t34[16], t40[16], t41[16], t42[16], t43[16],
      t44[16];

  MD(float, atinput, [xc.ic2][xc.ht][xc.wt][5][5][16], tinput);
  MD(float, atweights, [xc.ic2][5][5][16][16], tweights);
  MD(float, aoutput, [xc.oh][xc.ow][16], output);

#pragma omp simd
  for (int _OV = 0; _OV < 16; ++_OV) {
    T(0, 0) = 0.0f;
    T(0, 1) = 0.0f;
    T(0, 2) = 0.0f;
    T(0, 3) = 0.0f;
    T(0, 4) = 0.0f;
    T(1, 0) = 0.0f;
    T(1, 1) = 0.0f;
    T(1, 2) = 0.0f;
    T(1, 3) = 0.0f;
    T(1, 4) = 0.0f;
    T(2, 0) = 0.0f;
    T(2, 1) = 0.0f;
    T(2, 2) = 0.0f;
    T(2, 3) = 0.0f;
    T(2, 4) = 0.0f;
    T(3, 0) = 0.0f;
    T(3, 1) = 0.0f;
    T(3, 2) = 0.0f;
    T(3, 3) = 0.0f;
    T(3, 4) = 0.0f;
    T(4, 0) = 0.0f;
    T(4, 1) = 0.0f;
    T(4, 2) = 0.0f;
    T(4, 3) = 0.0f;
    T(4, 4) = 0.0f;
  }
  for (int _ic2 = 0; _ic2 < xc.ic2; ++_ic2) {
    for (int _IV = 0; _IV < 16; ++_IV) {
#pragma omp simd
      for (int _OV = 0; _OV < 16; ++_OV) {
        T(0, 0) += W(0, 0) * F(0, 0);
        T(0, 1) += W(0, 1) * F(0, 1);
        T(0, 2) += W(0, 2) * F(0, 2);
        T(0, 3) += W(0, 3) * F(0, 3);
        T(0, 4) += W(0, 4) * F(0, 4);
        T(1, 0) += W(1, 0) * F(1, 0);
        T(1, 1) += W(1, 1) * F(1, 1);
        T(1, 2) += W(1, 2) * F(1, 2);
        T(1, 3) += W(1, 3) * F(1, 3);
        T(1, 4) += W(1, 4) * F(1, 4);
        T(2, 0) += W(2, 0) * F(2, 0);
        T(2, 1) += W(2, 1) * F(2, 1);
        T(2, 2) += W(2, 2) * F(2, 2);
        T(2, 3) += W(2, 3) * F(2, 3);
        T(2, 4) += W(2, 4) * F(2, 4);
        T(3, 0) += W(3, 0) * F(3, 0);
        T(3, 1) += W(3, 1) * F(3, 1);
        T(3, 2) += W(3, 2) * F(3, 2);
        T(3, 3) += W(3, 3) * F(3, 3);
        T(3, 4) += W(3, 4) * F(3, 4);
        T(4, 0) += W(4, 0) * F(4, 0);
        T(4, 1) += W(4, 1) * F(4, 1);
        T(4, 2) += W(4, 2) * F(4, 2);
        T(4, 3) += W(4, 3) * F(4, 3);
        T(4, 4) += W(4, 4) * F(4, 4);
      }
    }
  }

#undef C
#define C(n) c##n[_OV]
#define P(_h, _w) \
  aoutput[xc.ht * 3 + _h][xc.wt * 3 + _w][_OV]  // TODO: overflow
  float c0[16], c1[16], c2[16], c3[16], c4[16];

#pragma omp simd
  for (int _OV = 0; _OV < 16; ++_OV) {
    C(0) = T(0, 0) + T(0, 1) + T(0, 2) + T(0, 3);
    C(1) = T(1, 0) + T(1, 1) + T(1, 2) + T(1, 3);
    C(2) = T(2, 0) + T(2, 1) + T(2, 2) + T(2, 3);
    C(3) = T(3, 0) + T(3, 1) + T(3, 2) + T(3, 3);
    C(4) = T(4, 0) + T(4, 1) + T(4, 2) + T(4, 3);
    P(0, 0) = C(0) + C(1) + C(2) + C(3);
    P(1, 0) = C(2) - C(1) + 2 * C(3);
    P(2, 0) = C(1) + C(2) + 4 * C(3) + C(4);

    C(0) = T(0, 2) - T(0, 1) + 2 * T(0, 3);
    C(1) = T(1, 2) - T(1, 1) + 2 * T(1, 3);
    C(2) = T(2, 2) - T(2, 1) + 2 * T(2, 3);
    C(3) = T(3, 2) - T(3, 1) + 2 * T(3, 3);
    C(4) = T(4, 2) - T(4, 1) + 2 * T(4, 3);
    P(0, 1) = C(0) + C(1) + C(2) + C(3);
    P(1, 1) = C(2) - C(1) + 2 * C(3);
    P(2, 1) = C(1) + C(2) + 4 * C(3) + C(4);

    C(0) = T(0, 1) + T(0, 2) + 4 * T(0, 3) + T(0, 4);
    C(1) = T(1, 1) + T(1, 2) + 4 * T(1, 3) + T(1, 4);
    C(2) = T(2, 1) + T(2, 2) + 4 * T(2, 3) + T(2, 4);
    C(3) = T(3, 1) + T(3, 2) + 4 * T(3, 3) + T(3, 4);
    C(4) = T(4, 1) + T(4, 2) + 4 * T(4, 3) + T(4, 4);
    P(0, 2) = C(0) + C(1) + C(2) + C(3);
    P(1, 2) = C(2) - C(1) + 2 * C(3);
    P(2, 2) = C(1) + C(2) + 4 * C(3) + C(4);
  }
}

template <>
void elk_product_trans_output<float, 5, 3, 16, ISA_SKX_AVX512>(
    elx_conv_t<float> &xc, float *tinput, float *tweights, float *output,
    int _ih2, int _iw2) {
  ENABLE_AVX512F();

  __m512 t00, t10, t20, t30, t40, t01, t11, t21, t31, t41, t02, t12, t22, t32,
      t42, t03, t13, t23, t33, t43, t04, t14, t24, t34, t44;

  MD(float, atinput, [xc.ic2][xc.ht][xc.wt][5][5][16], tinput);
  MD(float, atweights, [xc.ic2][5][5][16][16], tweights);
  MD(float, aoutput, [xc.oh][xc.ow][16], output);

#undef T
#undef F
#undef W
#define T(_h, _w) t##_h##_w
#define F(_h, _w) atinput[_ic2][_ih2][_iw2][_h][_w][_V]
#define W(_h, _w) atweights[_ic2][_h][_w][_V]

#define FMA(_h, _w)                                               \
  do {                                                            \
    __m512 f##_h##_w = _mm512_set1_ps(F(_h, _w));                 \
    __m512 w##_h##_w = _mm512_load_ps(W(_h, _w));                 \
    t##_h##_w = _mm512_fmadd_ps(w##_h##_w, f##_h##_w, t##_h##_w); \
  } while (0)
#define STORE(_h, _w)                      \
  do {                                     \
    _mm512_store_ps(T(_h, _w), t##_h##_w); \
  } while (0)

  t00 = _mm512_setzero_ps();
  t01 = _mm512_setzero_ps();
  t02 = _mm512_setzero_ps();
  t03 = _mm512_setzero_ps();
  t04 = _mm512_setzero_ps();
  t10 = _mm512_setzero_ps();
  t11 = _mm512_setzero_ps();
  t12 = _mm512_setzero_ps();
  t13 = _mm512_setzero_ps();
  t14 = _mm512_setzero_ps();
  t20 = _mm512_setzero_ps();
  t21 = _mm512_setzero_ps();
  t22 = _mm512_setzero_ps();
  t23 = _mm512_setzero_ps();
  t24 = _mm512_setzero_ps();
  t30 = _mm512_setzero_ps();
  t31 = _mm512_setzero_ps();
  t32 = _mm512_setzero_ps();
  t33 = _mm512_setzero_ps();
  t34 = _mm512_setzero_ps();
  t40 = _mm512_setzero_ps();
  t41 = _mm512_setzero_ps();
  t42 = _mm512_setzero_ps();
  t43 = _mm512_setzero_ps();
  t44 = _mm512_setzero_ps();

  for (int _ic2 = 0; _ic2 < xc.ic2; ++_ic2) {
    for (int _V = 0; _V < 16; ++_V) {
      //__m512 w00 = _mm512_load_ps(W(0,0));
      //__m512 f00 = _mm512_set1_ps(F(0,0));
      // t00 = _mm512_fmadd_ps(w00, f00, t00);
      FMA(0, 0);
      FMA(0, 1);
      FMA(0, 2);
      FMA(0, 3);
      FMA(0, 4);
      FMA(1, 0);
      FMA(1, 1);
      FMA(1, 2);
      FMA(1, 3);
      FMA(1, 4);
      FMA(2, 0);
      FMA(2, 1);
      FMA(2, 2);
      FMA(2, 3);
      FMA(2, 4);
      FMA(3, 0);
      FMA(3, 1);
      FMA(3, 2);
      FMA(3, 3);
      FMA(3, 4);
      FMA(4, 0);
      FMA(4, 1);
      FMA(4, 2);
      FMA(4, 3);
      FMA(4, 4);
    }
  }

#undef P
#define P(_h, _w) (aoutput[xc.ht * 3 + _h][xc.wt * 3 + _w])  // TODO: overflow
  __m512 c0, c1, c2, c3, c4;
  __m512 p00, p01, p02, p10, p11, p12, p20, p21, p22;
  __m512 z2 = _mm512_set_ps(IMM_BCAST16(2.0f));
  __m512 z4 = _mm512_set_ps(IMM_BCAST16(4.0f));

  c0 = _mm512_add_ps(t00, t01);
  c0 = _mm512_add_ps(c0, t02);
  c0 = _mm512_add_ps(c0, t03);
  c1 = _mm512_add_ps(t10, t11);
  c1 = _mm512_add_ps(c1, t12);
  c1 = _mm512_add_ps(c1, t13);
  c2 = _mm512_add_ps(t20, t21);
  c2 = _mm512_add_ps(c2, t22);
  c2 = _mm512_add_ps(c2, t23);
  c3 = _mm512_add_ps(t30, t31);
  c3 = _mm512_add_ps(c3, t32);
  c3 = _mm512_add_ps(c3, t33);
  c4 = _mm512_add_ps(t40, t41);
  c4 = _mm512_add_ps(c4, t42);
  c4 = _mm512_add_ps(c4, t43);

  p00 = _mm512_add_ps(c0, c1);
  p00 = _mm512_add_ps(p00, c2);
  p00 = _mm512_add_ps(p00, c3);
  _mm512_store_ps(P(0, 0), p00);  //
  p10 = _mm512_sub_ps(c2, c1);
  p10 = _mm512_fmadd_ps(z2, c3, p10);
  _mm512_store_ps(P(1, 0), p10);  //
  p20 = _mm512_add_ps(c0, c1);
  p20 = _mm512_add_ps(p20, c4);
  p20 = _mm512_fmadd_ps(z4, c3, p20);
  _mm512_store_ps(P(2, 0), p20);  //

  c0 = _mm512_sub_ps(t01, t02);
  c0 = _mm512_fmadd_ps(z2, t03, c0);
  c1 = _mm512_sub_ps(t11, t12);
  c1 = _mm512_fmadd_ps(z2, t13, c1);
  c2 = _mm512_sub_ps(t21, t22);
  c2 = _mm512_fmadd_ps(z2, t23, c2);
  c3 = _mm512_sub_ps(t31, t32);
  c3 = _mm512_fmadd_ps(z2, t33, c3);
  c4 = _mm512_sub_ps(t41, t42);
  c4 = _mm512_fmadd_ps(z2, t43, c4);
  p01 = _mm512_add_ps(c0, c1);
  p01 = _mm512_add_ps(p01, c2);
  p01 = _mm512_add_ps(p01, c3);
  _mm512_store_ps(P(0, 1), p01);  //
  p11 = _mm512_sub_ps(c2, c1);
  p11 = _mm512_fmadd_ps(z2, c3, p11);
  _mm512_store_ps(P(1, 1), p11);  //
  p21 = _mm512_add_ps(c1, c2);
  p21 = _mm512_add_ps(p21, c4);
  p21 = _mm512_fmadd_ps(z4, c3, p21);
  _mm512_store_ps(P(2, 1), p21);  //

  c0 = _mm512_add_ps(t01, t02);
  c0 = _mm512_add_ps(c0, t04);
  c0 = _mm512_fmadd_ps(z4, t03, c0);
  c1 = _mm512_add_ps(t11, t12);
  c1 = _mm512_add_ps(c1, t14);
  c1 = _mm512_fmadd_ps(z4, t13, c1);
  c2 = _mm512_add_ps(t21, t22);
  c2 = _mm512_add_ps(c2, t24);
  c2 = _mm512_fmadd_ps(z4, t23, c2);
  c3 = _mm512_add_ps(t31, t32);
  c3 = _mm512_add_ps(c3, t34);
  c3 = _mm512_fmadd_ps(z4, t33, c3);
  c4 = _mm512_add_ps(t41, t42);
  c4 = _mm512_add_ps(c4, t44);
  c4 = _mm512_fmadd_ps(z4, t43, c4);
  p02 = _mm512_add_ps(c0, c1);
  p02 = _mm512_add_ps(p02, c2);
  p02 = _mm512_add_ps(p02, c3);
  _mm512_store_ps(P(0, 2), p02);  //
  p12 = _mm512_sub_ps(c2, c1);
  p12 = _mm512_fmadd_ps(z2, c3, p12);
  _mm512_store_ps(P(1, 2), p12);  //
  p22 = _mm512_add_ps(c1, c2);
  p22 = _mm512_add_ps(p22, c4);
  p22 = _mm512_fmadd_ps(z4, c3, p22);
  _mm512_store_ps(P(2, 2), p22);  //
}

template <typename Type, int A, const int K, int V, int I>
void elk_trans_output(elx_conv_t<Type> &xc, Type *output,
                      Type atoutput[A][A][V]) {}

template <typename Type, int A, const int K, int V, int I>
void elk_trans_output(elx_conv_t<Type> &xc, Type *output,
                      Type atoutput[A][A][V],
                      int _hOA_end, int _wOA_end) {}

template <>
void elk_trans_output<float, 5, 3, 16, ISA_GENERIC>(elx_conv_t<float> &xc,
                                                    float *output,
                                                    float atoutput[5][5][16],
                                                    int _hOA_end,
                                                    int _wOA_end) {
  float dummy[16];
  auto p = [&](int _h, int _w, int _V) {
    int _i = _h * xc.output_strides[2] + _w * 16 + _V;
    if (_h > _hOA_end || _w > _wOA_end)
      return &dummy[_V];
    else
      return output + _i;
  };

#undef T
#undef C
#undef P
#define T(_hA, _wA) atoutput[_hA][_wA][_V]
#define C(n) c##n[_V]
#define P(_h, _w) *p(_h, _w, _V)
  float c0[16], c1[16], c2[16], c3[16], c4[16];
#pragma omp simd
  for (int _V = 0; _V < 16; ++_V) {
    C(0) = T(0, 0) + T(0, 1) + T(0, 2) + T(0, 3);
    C(1) = T(1, 0) + T(1, 1) + T(1, 2) + T(1, 3);
    C(2) = T(2, 0) + T(2, 1) + T(2, 2) + T(2, 3);
    C(3) = T(3, 0) + T(3, 1) + T(3, 2) + T(3, 3);
    C(4) = T(4, 0) + T(4, 1) + T(4, 2) + T(4, 3);
    P(0, 0) = C(0) + C(1) + C(2) + C(3);
    P(1, 0) = C(2) - C(1) + 2 * C(3);
    P(2, 0) = C(1) + C(2) + 4 * C(3) + C(4);

    C(0) = T(0, 2) - T(0, 1) + 2 * T(0, 3);
    C(1) = T(1, 2) - T(1, 1) + 2 * T(1, 3);
    C(2) = T(2, 2) - T(2, 1) + 2 * T(2, 3);
    C(3) = T(3, 2) - T(3, 1) + 2 * T(3, 3);
    C(4) = T(4, 2) - T(4, 1) + 2 * T(4, 3);
    P(0, 1) = C(0) + C(1) + C(2) + C(3);
    P(1, 1) = C(2) - C(1) + 2 * C(3);
    P(2, 1) = C(1) + C(2) + 4 * C(3) + C(4);

    C(0) = T(0, 1) + T(0, 2) + 4 * T(0, 3) + T(0, 4);
    C(1) = T(1, 1) + T(1, 2) + 4 * T(1, 3) + T(1, 4);
    C(2) = T(2, 1) + T(2, 2) + 4 * T(2, 3) + T(2, 4);
    C(3) = T(3, 1) + T(3, 2) + 4 * T(3, 3) + T(3, 4);
    C(4) = T(4, 1) + T(4, 2) + 4 * T(4, 3) + T(4, 4);
    P(0, 2) = C(0) + C(1) + C(2) + C(3);
    P(1, 2) = C(2) - C(1) + 2 * C(3);
    P(2, 2) = C(1) + C(2) + 4 * C(3) + C(4);
  }
}

#define AVX512_LOAD0(z, n, nil) t0##n = _mm512_load_ps(T(0, n));
#define AVX512_LOAD1(z, n, nil) t1##n = _mm512_load_ps(T(1, n));
#define AVX512_LOAD2(z, n, nil) t2##n = _mm512_load_ps(T(2, n));
#define AVX512_LOAD3(z, n, nil) t3##n = _mm512_load_ps(T(3, n));
#define AVX512_LOAD4(z, n, nil) t4##n = _mm512_load_ps(T(4, n));
#define LOAD_ZMMS()                        \
  do {                                     \
    BOOST_PP_REPEAT(5, AVX512_LOAD0, nil); \
    BOOST_PP_REPEAT(5, AVX512_LOAD1, nil); \
    BOOST_PP_REPEAT(5, AVX512_LOAD2, nil); \
    BOOST_PP_REPEAT(5, AVX512_LOAD3, nil); \
    BOOST_PP_REPEAT(5, AVX512_LOAD4, nil); \
  } while (0)

#define AVX512_STORE0(z, n, nil) _mm512_stream_ps(P(0, n), p0##n);
#define AVX512_STORE1(z, n, nil) _mm512_stream_ps(P(1, n), p1##n);
#define AVX512_STORE2(z, n, nil) _mm512_stream_ps(P(2, n), p2##n);
#define STORE_ZMMS()                        \
  do {                                      \
    BOOST_PP_REPEAT(3, AVX512_STORE0, nil); \
    BOOST_PP_REPEAT(3, AVX512_STORE1, nil); \
    BOOST_PP_REPEAT(3, AVX512_STORE2, nil); \
  } while (0)

template <>
void elk_trans_output<float, 5, 3, 16, ISA_GENERIC>(elx_conv_t<float> &xc,
                                                    float *output,
                                                    float atoutput[5][5][16]) {
  elk_trans_output<float, 5, 3, 16, ISA_GENERIC>(xc, output, atoutput, 2, 2);
}

template <>
void elk_trans_output<float, 5, 3, 16, ISA_SKX_AVX512>(
    elx_conv_t<float> &xc, float *output, float atoutput[5][5][16]) {

  ENABLE_AVX512F();

  auto p = [&](int _h, int _w) {
    int _i = _h * xc.output_strides[2] + _w * 16;
    return output + _i;
  };

#undef P
#undef T
#define T(_h, _w) atoutput[_h][_w]
#define P(_h, _w) p(_h, _w)

  __m512 c0, c1, c2, c3, c4;
  __m512 t00, t01, t02, t03, t04, t10, t11, t12, t13, t14, t20, t21, t22, t23,
      t24, t30, t31, t32, t33, t34, t40, t41, t42, t43, t44;
  __m512 p00, p01, p02, p10, p11, p12, p20, p21, p22;
  __m512 z2 = _mm512_set_ps(IMM_BCAST16(2.0f));
  __m512 z4 = _mm512_set_ps(IMM_BCAST16(4.0f));

  LOAD_ZMMS();

  c0 = _mm512_add_ps(t00, t01);
  c0 = _mm512_add_ps(c0, t02);
  c0 = _mm512_add_ps(c0, t03);
  c1 = _mm512_add_ps(t10, t11);
  c1 = _mm512_add_ps(c1, t12);
  c1 = _mm512_add_ps(c1, t13);
  c2 = _mm512_add_ps(t20, t21);
  c2 = _mm512_add_ps(c2, t22);
  c2 = _mm512_add_ps(c2, t23);
  c3 = _mm512_add_ps(t30, t31);
  c3 = _mm512_add_ps(c3, t32);
  c3 = _mm512_add_ps(c3, t33);
  c4 = _mm512_add_ps(t40, t41);
  c4 = _mm512_add_ps(c4, t42);
  c4 = _mm512_add_ps(c4, t43);

  p00 = _mm512_add_ps(c0, c1);
  p00 = _mm512_add_ps(p00, c2);
  p00 = _mm512_add_ps(p00, c3);
  p10 = _mm512_sub_ps(c2, c1);
  p10 = _mm512_fmadd_ps(z2, c3, p10);
  p20 = _mm512_add_ps(c0, c1);
  p20 = _mm512_add_ps(p20, c4);
  p20 = _mm512_fmadd_ps(z4, c3, p20);

  c0 = _mm512_sub_ps(t01, t02);
  c0 = _mm512_fmadd_ps(z2, t03, c0);
  c1 = _mm512_sub_ps(t11, t12);
  c1 = _mm512_fmadd_ps(z2, t13, c1);
  c2 = _mm512_sub_ps(t21, t22);
  c2 = _mm512_fmadd_ps(z2, t23, c2);
  c3 = _mm512_sub_ps(t31, t32);
  c3 = _mm512_fmadd_ps(z2, t33, c3);
  c4 = _mm512_sub_ps(t41, t42);
  c4 = _mm512_fmadd_ps(z2, t43, c4);
  p01 = _mm512_add_ps(c0, c1);
  p01 = _mm512_add_ps(p01, c2);
  p01 = _mm512_add_ps(p01, c3);
  p11 = _mm512_sub_ps(c2, c1);
  p11 = _mm512_fmadd_ps(z2, c3, p11);
  p21 = _mm512_add_ps(c1, c2);
  p21 = _mm512_add_ps(p21, c4);
  p21 = _mm512_fmadd_ps(z4, c3, p21);

  c0 = _mm512_add_ps(t01, t02);
  c0 = _mm512_add_ps(c0, t04);
  c0 = _mm512_fmadd_ps(z4, t03, c0);
  c1 = _mm512_add_ps(t11, t12);
  c1 = _mm512_add_ps(c1, t14);
  c1 = _mm512_fmadd_ps(z4, t13, c1);
  c2 = _mm512_add_ps(t21, t22);
  c2 = _mm512_add_ps(c2, t24);
  c2 = _mm512_fmadd_ps(z4, t23, c2);
  c3 = _mm512_add_ps(t31, t32);
  c3 = _mm512_add_ps(c3, t34);
  c3 = _mm512_fmadd_ps(z4, t33, c3);
  c4 = _mm512_add_ps(t41, t42);
  c4 = _mm512_add_ps(c4, t44);
  c4 = _mm512_fmadd_ps(z4, t43, c4);
  p02 = _mm512_add_ps(c0, c1);
  p02 = _mm512_add_ps(p02, c2);
  p02 = _mm512_add_ps(p02, c3);
  p12 = _mm512_sub_ps(c2, c1);
  p12 = _mm512_fmadd_ps(z2, c3, p12);
  p22 = _mm512_add_ps(c1, c2);
  p22 = _mm512_add_ps(p22, c4);
  p22 = _mm512_fmadd_ps(z4, c3, p22);

  STORE_ZMMS();
}

template <>
void elk_trans_output<float, 5, 3, 16, ISA_SKX_AVX512>(elx_conv_t<float> &xc,
                                                       float *output,
                                                       float atoutput[5][5][16],
                                                       int _hOA_end,
                                                       int _wOA_end) {
  ENABLE_AVX512F();

  alignas(64) float dummy[16];
  auto p = [&](int _h, int _w) {
    int _i = _h * xc.output_strides[2] + _w * 16;
    if (_h > _hOA_end || _w > _wOA_end)
      return dummy;
    else
      return output + _i;
  };

#undef P
#undef T
#define T(_h, _w) atoutput[_h][_w]
#define P(_h, _w) p(_h, _w)

  __m512 c0, c1, c2, c3, c4;
  __m512 t00, t01, t02, t03, t04, t10, t11, t12, t13, t14, t20, t21, t22, t23,
      t24, t30, t31, t32, t33, t34, t40, t41, t42, t43, t44;
  __m512 p00, p01, p02, p10, p11, p12, p20, p21, p22;
  __m512 z2 = _mm512_set_ps(IMM_BCAST16(2.0f));
  __m512 z4 = _mm512_set_ps(IMM_BCAST16(4.0f));

  LOAD_ZMMS();

  c0 = _mm512_add_ps(t00, t01);
  c0 = _mm512_add_ps(c0, t02);
  c0 = _mm512_add_ps(c0, t03);
  c1 = _mm512_add_ps(t10, t11);
  c1 = _mm512_add_ps(c1, t12);
  c1 = _mm512_add_ps(c1, t13);
  c2 = _mm512_add_ps(t20, t21);
  c2 = _mm512_add_ps(c2, t22);
  c2 = _mm512_add_ps(c2, t23);
  c3 = _mm512_add_ps(t30, t31);
  c3 = _mm512_add_ps(c3, t32);
  c3 = _mm512_add_ps(c3, t33);
  c4 = _mm512_add_ps(t40, t41);
  c4 = _mm512_add_ps(c4, t42);
  c4 = _mm512_add_ps(c4, t43);

  p00 = _mm512_add_ps(c0, c1);
  p00 = _mm512_add_ps(p00, c2);
  p00 = _mm512_add_ps(p00, c3);
  p10 = _mm512_sub_ps(c2, c1);
  p10 = _mm512_fmadd_ps(z2, c3, p10);
  p20 = _mm512_add_ps(c0, c1);
  p20 = _mm512_add_ps(p20, c4);
  p20 = _mm512_fmadd_ps(z4, c3, p20);

  c0 = _mm512_sub_ps(t01, t02);
  c0 = _mm512_fmadd_ps(z2, t03, c0);
  c1 = _mm512_sub_ps(t11, t12);
  c1 = _mm512_fmadd_ps(z2, t13, c1);
  c2 = _mm512_sub_ps(t21, t22);
  c2 = _mm512_fmadd_ps(z2, t23, c2);
  c3 = _mm512_sub_ps(t31, t32);
  c3 = _mm512_fmadd_ps(z2, t33, c3);
  c4 = _mm512_sub_ps(t41, t42);
  c4 = _mm512_fmadd_ps(z2, t43, c4);
  p01 = _mm512_add_ps(c0, c1);
  p01 = _mm512_add_ps(p01, c2);
  p01 = _mm512_add_ps(p01, c3);
  p11 = _mm512_sub_ps(c2, c1);
  p11 = _mm512_fmadd_ps(z2, c3, p11);
  p21 = _mm512_add_ps(c1, c2);
  p21 = _mm512_add_ps(p21, c4);
  p21 = _mm512_fmadd_ps(z4, c3, p21);

  c0 = _mm512_add_ps(t01, t02);
  c0 = _mm512_add_ps(c0, t04);
  c0 = _mm512_fmadd_ps(z4, t03, c0);
  c1 = _mm512_add_ps(t11, t12);
  c1 = _mm512_add_ps(c1, t14);
  c1 = _mm512_fmadd_ps(z4, t13, c1);
  c2 = _mm512_add_ps(t21, t22);
  c2 = _mm512_add_ps(c2, t24);
  c2 = _mm512_fmadd_ps(z4, t23, c2);
  c3 = _mm512_add_ps(t31, t32);
  c3 = _mm512_add_ps(c3, t34);
  c3 = _mm512_fmadd_ps(z4, t33, c3);
  c4 = _mm512_add_ps(t41, t42);
  c4 = _mm512_add_ps(c4, t44);
  c4 = _mm512_fmadd_ps(z4, t43, c4);
  p02 = _mm512_add_ps(c0, c1);
  p02 = _mm512_add_ps(p02, c2);
  p02 = _mm512_add_ps(p02, c3);
  p12 = _mm512_sub_ps(c2, c1);
  p12 = _mm512_fmadd_ps(z2, c3, p12);
  p22 = _mm512_add_ps(c1, c2);
  p22 = _mm512_add_ps(p22, c4);
  p22 = _mm512_fmadd_ps(z4, c3, p22);

  STORE_ZMMS();
}

}  // namespace euler