#pragma once
#include <assert.h>
#include <x86intrin.h>
#include "elk_def.hpp"
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"
#include "elk_conv_wino.hpp"
#include "elk_conv_wino_5x5_3x3_input_gen.hxx"

namespace euler {
#define GENERIC_CALCULATE_O_0(z, n, nil)                                     \
  C(n) = T(n, 0) + T(n, 1) + T(n, 2) + T(n, 3)                               \
      + T(n, 4) + T(n, 5);
#define GENERIC_CALCULATE_O_1(z, n, nil)                                     \
  C(n) = T(n, 0) - T(n, 1) + a2 * (T(n, 2) - T(n, 3))                        \
      + a1_2 * (T(n, 4) - T(n, 5));
#define GENERIC_CALCULATE_O_2(z, n, nil)                                     \
  C(n) = T(n, 0) + T(n, 1) + a4 * (T(n, 2) + T(n, 3))                        \
      + a1_4* (T(n, 4) + T(n, 5));
#define GENERIC_CALCULATE_O_3(z, n, nil)                                     \
  C(n) = T(n, 0) - T(n, 1) + a8 * (T(n, 2) - T(n, 3))                        \
      + a1_8* (T(n, 4) - T(n, 5));
#define GENERIC_CALCULATE_O_4(z, n, nil)                                     \
  C(n) = T(n, 0) + T(n, 1) + a16 * (T(n, 2) + T(n, 3))                       \
      + a1_16* (T(n, 4) + T(n, 5)) + T(n, 6);

#define GENERIC_CALCULATE_O(n)                                               \
  S(0) = C(0) + C(1) + C(2) + C(3) + C(4) + C(5);                            \
  if (fuse_bias) S(0) += B;                                                  \
  if (fuse_ip_sum) S(0) += P(0, n);                                          \
  if (fuse_relu) P(0,n) = S(0) > 0 ? S(0) : 0;                               \
  else P(0, n) = S(0);                                                       \
  S(1) = C(0) - C(1) + a2 * (C(2) - C(3)) + a1_2 * (C(4) - C(5));            \
  if (fuse_bias) S(1) += B;                                                  \
  if (fuse_ip_sum) S(1) += P(1, n);                                          \
  if (fuse_relu) P(1, n) = S(1) > 0 ? S(1) : 0;                              \
  else P(1, n) = S(1);                                                       \
  S(2) = C(0) + C(1) + a4 * (C(2) + C(3)) + a1_4 * (C(4) + C(5));            \
  if (fuse_bias) S(2) += B;                                                  \
  if (fuse_ip_sum) S(2) += P(2, n);                                          \
  if (fuse_relu) P(2, n) = S(2) > 0 ? S(2) : 0;                              \
  else P(2, n) = S(2);                                                       \
  S(3) = C(0) - C(1) + a8 * (C(2) - C(3)) + a1_8 * (C(4) - C(5));            \
  if (fuse_bias) S(3) += B;                                                  \
  if (fuse_ip_sum) S(3) += P(3, n);                                          \
  if (fuse_relu) P(3, n) = S(3) > 0 ? S(3) : 0;                              \
  else P(3, n) = S(3);                                                       \
  S(4) = C(0) + C(1) + a16 * (C(2) + C(3)) + a1_16 * (C(4) + C(5));

#define GENERIC_ADD_TAIL_0(n, z)                                             \
  S(4) += T(6, 0) - T(6, 1) + a##z * (T(6, 2) - T(6, 3))                     \
      + a1_##z * (T(6, 4) - T(6, 5));
#define GENERIC_ADD_TAIL_1(n, z)                                             \
  S(4) += T(6, 0) + T(6, 1) + a##z * (T(6, 2) + T(6, 3))                     \
      + a1_##z * (T(6, 4) + T(6, 5));

template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 7, 3>::
__trans_output(elx_conv_t<float> &xc, float *output,
      float atoutput[A][A][V], float *bias, int hOA_end, int wOA_end) {
  const float a2 = 2.0f;
  const float a4 = 4.0f;
  const float a8 = 8.0f;
  const float a16 = 16.0f;
  const float a1_2 = 1.0f / 2.0f;
  const float a1_4 = 1.0f / 4.0f;
  const float a1_8 = 1.0f / 8.0f;
  const float a1_16 = 1.0f / 16.0f;

  float dummy[V];

  constexpr bool is_border = cd_traits<conditions...>::is_border;
  constexpr bool with_bias = cd_traits<conditions...>::with_bias;
  constexpr bool with_relu = cd_traits<conditions...>::with_relu;
  constexpr bool with_ip_sum = cd_traits<conditions...>::with_ip_sum;
  bool fuse_ip_sum = with_ip_sum && (wOA_end != -1);
  bool fuse_bias = with_bias && (bias != nullptr);
  bool fuse_relu = with_relu && (bias != nullptr);

  auto p_cb = [&](int _h, int _w, int _V) {
    if (wOA_end == -1) {
      MD3(float, aoutput, output, A - K + 1, A - K + 1, V);
      return &md3(aoutput, _h, _w, _V);
    } else {
      MD3(float, aoutput, output, xc.oh, xc.ow, V);
      if (is_border && (_h > hOA_end || _w > wOA_end))
        return &dummy[_V];
      else
        return &md3(aoutput, _h, _w, _V);
    }
  };

#undef T
#undef C
#undef P
#undef B
#undef S
#define T(_hA, _wA) atoutput[_wA][_hA][_V]
#define C(n) C##n[_V]
#define S(n) s##n[_V]
#define P(_h, _w) *p_cb(_h, _w, _V)
#define B bias[_V]

  float C0[V], C1[V], C2[V], C3[V], C4[V], C5[V];
  float s0[V], s1[V], s2[V], s3[V], s4[V];

#pragma omp simd
  for (int _V = 0; _V < V; ++_V) {
    BOOST_PP_REPEAT(6, GENERIC_CALCULATE_O_0, nil)
    GENERIC_CALCULATE_O(0)
    P(4, 0) += T(6, 0) + T(6, 1) + T(6, 2) + T(6, 3) + T(6, 4) + T(6, 5);
    if (fuse_bias) S(4) += B;
    if (fuse_ip_sum) S(4) += P(4, 0);
    if (fuse_relu) P(4, 0) = S(4) > 0 ? S(4) : 0;
    else P(4, 0) = S(4);


    BOOST_PP_REPEAT(6, GENERIC_CALCULATE_O_1, nil)
    GENERIC_CALCULATE_O(1)
    GENERIC_ADD_TAIL_0(1, 2)
    if (fuse_bias) S(4) += B;
    if (fuse_ip_sum) S(4) += P(4, 1);
    if (fuse_relu) P(4, 1) = S(4) > 0 ? S(4) : 0;
    else P(4, 1) = S(4);


    BOOST_PP_REPEAT(6, GENERIC_CALCULATE_O_2, nil)
    GENERIC_CALCULATE_O(2)
    GENERIC_ADD_TAIL_1(2, 4)
    if (fuse_bias) S(4) += B;
    if (fuse_ip_sum) S(4) += P(4, 2);
    if (fuse_relu) P(4, 2) = S(4) > 0 ? S(4) : 0;
    else P(4, 2) = S(4);


    BOOST_PP_REPEAT(6, GENERIC_CALCULATE_O_3, nil)
    GENERIC_CALCULATE_O(3)
    GENERIC_ADD_TAIL_0(3, 8)
    if (fuse_bias) S(4) += B;
    if (fuse_ip_sum) S(4) += P(4, 3);
    if (fuse_relu) P(4, 3) = S(4) > 0 ? S(4) : 0;
    else P(4, 3) = S(4);


    BOOST_PP_REPEAT(6, GENERIC_CALCULATE_O_4, nil)
    GENERIC_CALCULATE_O(4)
    GENERIC_ADD_TAIL_1(4, 16)
    S(4) += T(6, 6);
    if (fuse_bias) S(4) += B;
    if (fuse_ip_sum) S(4) += P(4, 4);
    if (fuse_relu) P(4, 4) = S(4) > 0 ? S(4) : 0;
    else P(4, 4) = S(4);
  }
}

template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 7, 3>::
__trans_outputa_th(elx_conv_t<float> &xc, float *toutputa, float *toutput,
    int Tz, bool stream_out) {
  MD4(float, atoutput, toutput, A, xc.oc3 * xc.O2, Tz, V);
  MD2(float, atoutputa, toutputa, A - K + 1, V);

#undef P
#undef T
#define T(_h) md4(atoutput, _h, 0, 0, _V)
#define P(_h) md2(atoutputa, _h, _V)

  const float z2 = 2.0f;
  const float z4 = 4.0f;
  const float z8 = 8.0f;
  const float z16 = 16.0f;
  const float z1_2 = 1.0f / 2.0f;
  const float z1_4 = 1.0f / 4.0f;
  const float z1_8 = 1.0f / 8.0f;
  const float z1_16 = 1.0f / 16.0f;
#pragma omp simd
  for (int _V = 0; _V < 16; ++_V) {
    P(0) = T(0) + T(1) + T(2) + T(3) + T(4) + T(5);
    P(1) = T(0) - T(1) + z2 * (T(2) - T(3)) + z1_2 * (T(4) - T(5));
    P(2) = T(0) + T(1) + z4 * (T(2) + T(3)) + z1_4 * (T(4) + T(5));
    P(3) = T(0) - T(1) + z8 * (T(2) - T(3)) + z1_8 * (T(4) - T(5));
    P(4) = T(0) + T(1) + z16 * (T(2) + T(3)) + z1_16 * (T(4) + T(5)) + T(6);
  }
}

#define GENERIC_CALCULATE_TILE_7(z, n, nil)                         \
  S(0) = T(n, 0) + T(n, 1) + T(n, 2) + T(n, 3) + T(n, 4)            \
      + T(n, 5);                                                    \
  if (fuse_bias) S(0) += B;                                         \
  if (fuse_ip_sum) S(0) += P(n, 0);                                 \
  if (fuse_relu) P(n, 0) = S(0) > 0 ? S(0) : 0;                     \
  else P(n, 0) = S(0);                                              \
  S(1) = T(n, 0) - T(n, 1) + z2 * (T(n, 2) - T(n, 3))               \
      + z1_2 * (T(n, 4) - T(n,5));                                  \
  if (fuse_bias) S(1) += B;                                         \
  if (fuse_ip_sum) S(1) += P(n, 1);                                 \
  if (fuse_relu) P(n, 1) = S(1) > 0 ? S(1) : 0;                     \
  else P(n, 1) = S(1);                                              \
  S(2) = T(n, 0) + T(n, 1) + z4 * (T(n, 2) + T(n, 3))               \
      + z1_4 * (T(n, 4) + T(n,5));                                  \
  if (fuse_bias) S(2) += B;                                         \
  if (fuse_ip_sum) S(2) += P(n, 2);                                 \
  if (fuse_relu) P(n, 2) = S(2) > 0 ? S(2) : 0;                     \
  else P(n, 2) = S(2);                                              \
  S(3) = T(n, 0) - T(n, 1) + z8 * (T(n, 2) - T(n, 3))               \
      + z1_8 * (T(n, 4) - T(n,5));                                  \
  if (fuse_bias) S(3) += B;                                         \
  if (fuse_ip_sum) S(3) += P(n, 3);                                 \
  if (fuse_relu) P(n, 3) = S(3) > 0 ? S(3) : 0;                     \
  else P(n, 3) = S(3);                                              \
  S(4) = T(n, 0) + T(n, 1) + z16 * (T(n, 2) + T(n, 3))              \
      + z1_16 * (T(n, 4) + T(n,5)) + T(n, 6);                       \
  if (fuse_bias) S(4) += B;                                         \
  if (fuse_ip_sum) S(4) += P(n, 4);                                 \
  if (fuse_relu) P(n, 4) = S(4) > 0 ? S(4) : 0;                     \
  else P(n, 4) = S(4);

template <bool ...conditions>
inline void convolution_winograd_kernel_base<float, ISA_GENERIC, 16, 7, 3>::
__trans_outputa_bh(elx_conv_t<float> &xc, float *output,
    float atoutput[A][A - K + 1][V], float *bias, int hOA_end, int wOA_end) {
  float dummy[V];
  constexpr bool is_border = cd_traits<conditions...>::is_border;
  constexpr bool with_bias = cd_traits<conditions...>::with_bias;
  constexpr bool with_relu = cd_traits<conditions...>::with_relu;
  constexpr bool with_ip_sum = cd_traits<conditions...>::with_ip_sum;
  bool fuse_ip_sum = with_ip_sum && (wOA_end != -1);
  bool fuse_bias = with_bias && (bias != nullptr);
  bool fuse_relu = with_relu && (bias != nullptr);

  auto p_cb = [&](int _h, int _w, int _V) {
    if (wOA_end == -1) {
      MD3(float, aoutput, output, A - K + 1, A - K + 1, V);
      return &md3(aoutput, _h, _w, _V);
    } else {
      MD3(float, aoutput, output, xc.oh, xc.ow, V);
      if (is_border && (_h > hOA_end || _w > wOA_end))
        return &dummy[_V];
      else
        return &md3(aoutput, _h, _w, _V);
    }
  };

#undef T
#undef C
#undef P
#undef B
#undef S
#define S(n) s##n[_V]
#define T(_hA, _wA) atoutput[_wA][_hA][_V]
#define P(_h, _w) *p_cb(_h, _w, _V)
#define B bias[_V]

  const float z2 = 2.0f;
  const float z4 = 4.0f;
  const float z8 = 8.0f;
  const float z16 = 16.0f;
  const float z1_2 = 1.0f / 2.0f;
  const float z1_4 = 1.0f / 4.0f;
  const float z1_8 = 1.0f / 8.0f;
  const float z1_16 = 1.0f / 16.0f;

  float s0[V], s1[V], s2[V], s3[V], s4[V];
#pragma omp simd
  for (int _V = 0; _V < V; ++_V) {
    BOOST_PP_REPEAT(5, GENERIC_CALCULATE_TILE_7, nil)
  }
}
} // namespace euler
