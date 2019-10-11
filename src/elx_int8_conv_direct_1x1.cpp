#include "elx_int8_conv_direct_1x1.hpp"
#include "el_parallel.hpp"

namespace euler {

Template_elx_int8_conv_direct_1x1_t
Instance_elx_int8_conv_direct_1x1_t::elx_int8_conv_direct_1x1_t(eld_conv_t &dc)
    : elx_conv_t(dc)
{
  // user input
  xopt_ = this->execution_mode;
  attr_ = 0x0;

  this->Vx = 4;
  this->V1 = V / this->Vx;
  this->IC = ALIGNUP(this->ic, V);
  this->OC = ALIGNUP(this->oc, V);

  if (this->I2 == 0) this->I2 = this->ic2;
  if (this->T == 0)  this->T = 1;
  if (this->O == 0)  this->O = 1;
  if (this->O1 == 0) this->O1 = 1;
  this->O2 = this->O * this->O1;

  this->O4 = this->O4 == 0 ? 1 : this->O4;
  this->I4 = this->I4 == 0 ? 1 : this->I4;

  this->ic2 = this->IC / V;
  this->oc2 = this->OC / V;

  no_pad_ = this->lp == 0 && this->rp == 0 && this->tp == 0 && this->bp == 0;
  if (!no_pad_)
    el_error("no support for padding in 1x1 u8s8 conv");

  // n, t2, (T, Tr)
  bool shape_ok = this->hs < 3 && this->ws < 3 && no_pad_;
  if (!shape_ok)
    el_error("direct_1x1: int8: Shape not supported");

  if (xopt_ == 0xc160) {
    this->ht = this->oh;
    this->wt = this->ow;
    this->nt = this->ht * this->wt;
    this->t = this->nt * this->n;
    this->t2 = (this->nt + this->T - 1) / this->T;
    this->Tr = this->nt % this->T ? this->nt % this->T : this->T;
  } else if (xopt_ == 0xb161){
    this->ht = this->oh;
    this->wt = this->ow / this->T;
    this->nt = this->oh * this->ow;
    this->t2 = this->nt / this->T;
    this->Tr = this->T;
    this->t = this->nt * this->n;
    if (this->ow % T != 0) {
      el_error("direct_1x1: int8: b161: No Tr support");
    }
  } else {
    el_error("direct_1x1: int8: xopt not supported");
  }

  this->Ir = this->ic % V ? this->ic % V : V;
  this->Or = this->oc % V ? this->oc % V : V;

  if (this->Ir != V)
    el_error("ic / 16 != 0 is not implement while doing int8 gemm");

  // O4, (O3, O3r), (O2, O2r)
  this->oc34 = (this->oc2 + this->O2 - 1) / this->O2;
  this->O2r = this->oc2 % this->O2;
  if (this->O2r == 0) this->O2r = this->O2;
  this->O3 = this->O4; // FIXME, swap order
  this->O4 = (this->oc34 + this->O3 - 1) / this->O3;
  this->O3r = this->oc34 % this->O3;
  if (this->O3r == 0) this->O3r = this->O3;

  if (this->O2r != this->O2 || this->O3r != this->O3) {
    el_error("No oc tailing for 0xa061, 0xb061, 0xe060, 0xf061");
  }

  // I4, I3, I3
  this->ic34 = this->ic2 / this->I2;
  this->I3 = this->ic34 / this->I4;
  if (this->I4 * this->I3 * this->I2 * V != this->IC)
    el_error("IC blocking error");

  if (this->Ir != V) {
    el_error("direct_1x1: int8: Ir not support");
  }
  if ((this->output_fmt != nChw16c || this->weights_fmt != OIhw16i16o) &&
      this->Or != V) {
    el_error("direct_1x1: int8: Or not support");
  }

  attr_ = set_bit(attr_, AT_FMAOPT_MASK);
  is_first_run_ = true;
  inference_acc_ = false;
  mthr_ = estl::max_concurrency();
  inference_acc_ = this->prop_kind == forward_inference;

  attr_ = this->with_bias ? set_bit(attr_, AT_BIAS_MASK) : attr_;
  attr_ = this->with_ip_sum ? set_bit(attr_, AT_INP_SUM_MASK) : attr_;
  toutput_opt_ = false;
  prepare_quant_calibration(dc);

  prepare_execute_opt();
  bind_execute_functions();

  // dbg
  printf("T=%d, Tr=%d, t2=%d, ht=%d, wt=%d, t=%d\n",
      this->T, this->Tr, this->t2, this->ht, this->wt, this->t);
  printf("V=%d, Vx=%d, Ir=%d, I2=%d, I3=%d, I4=%d, IC=%d\n",
      V, this->Vx, this->Ir, this->I2, this->I3, this->I4, this->IC);
  printf("V=%d, Or=%d, O2=%d (O=%d, O1=%d), O3=%d, O4=%d, O2r=%d, O3r=%d, OC=%d\n",
      V, this->Or, this->O2, this->O, this->O1,
      this->O3, this->O4, this->O2r, this->O3r, this->OC);
}

Template_elx_int8_conv_direct_1x1_t
int Instance_elx_int8_conv_direct_1x1_t::prepare_execute_opt()
{
  size_t tweights_size = 0, tinput_size = 0, toutput_size = 0;
  size_t binput_size = 0, bweights_size = 0, boutput_size = 0;
  size_t tweights_s8_size = 0, input_scale_size = 0, weights_scale_size = 0;

  stream_in_ = this->streaming_input
      ? (this->streaming_input == STORE_STREAMING) : false;
  stream_out_ = this->streaming_output
      ? (this->streaming_output == STORE_STREAMING) : false;

  input_is_bfmt_ = this->input_fmt == nChw16c; // nChw8c
  weights_is_bfmt_ = this->weights_fmt == OIhw16i16o;
  output_is_bfmt_ = this->output_fmt == nChw16c;
  input_as_bfmt_ = this->input_fmt == nchw && this->input_as_blocked;
  weights_as_bfmt_ = this->input_fmt == oihw && this->weights_as_blocked;
  output_as_bfmt_ = this->output_fmt == nchw && this->output_as_blocked;
  is_bfmt_ = input_is_bfmt_ && weights_is_bfmt_ && output_is_bfmt_;

  if (input_as_bfmt_)
    binput_size = this->n * this->IC * this->ih * this->iw * sizeof(InputType);
  if (weights_as_bfmt_)
    bweights_size = this->OC * this->IC * sizeof(WeightsType);
  if (output_as_bfmt_)
    boutput_size = this->n * this->OC * this->oh * this->ow * sizeof(OutputType);

  tweights_ = nullptr;
  tinput_ = nullptr;
  toutput_ = nullptr;
  binput_ = nullptr;
  bweights_ = nullptr;
  boutput_ = nullptr;
  if (this->n * this->O4 >= mthr_ && this->output_fmt == nChw16c)
    toutput_opt_ = true;

  switch (xopt_) {
  case 0xc160:
  case 0xb161:
    input_scale_size = this->T * 2 * sizeof(TscaleType);
    tweights_s8_size = this->IC * this->OC * sizeof(int8_t);
    weights_scale_size = this->OC * 2 * sizeof(TscaleType);
    toutput_size = (this->OC / this->O4) * this->oh * this->ow *
                   sizeof(ToutputType);
    toutput_size *= toutput_opt_ ? mthr_ : this->n * this->O4;
    break;
  default:
      el_error("Unknown xopt!");
      return -1;
    break;
  }

  const size_t align = PAGE_SIZE;
#define WEIGHTS_MAX_PRELOAD 4
  if (tweights_size > 0)
    tweights_size += WEIGHTS_MAX_PRELOAD * V;

  tweights_size_ = tweights_size > 0 ? alignup(tweights_size, align) : 0;
  tinput_size_ = tinput_size > 0 ? alignup(tinput_size, align) : 0;
  toutput_size_ = toutput_size > 0 ? alignup(toutput_size, align) : 0;
  binput_size_ = binput_size > 0 ? alignup(binput_size, align) : 0;
  bweights_size_ = bweights_size > 0 ? alignup(bweights_size, align) : 0;
  boutput_size_ = boutput_size > 0 ? alignup(boutput_size, align) : 0;
  tweights_s8_size_ = tweights_s8_size > 0 ? alignup(tweights_s8_size, align) : 0;
  input_scale_size_ = input_scale_size > 0 ? alignup(input_scale_size, align) : 0;
  weights_scale_size_ = weights_scale_size > 0 ? alignup(weights_scale_size, align) : 0;

  workspace_size_ = tweights_size_ + tweights_s8_size_
      + weights_scale_size_ + input_scale_size_;
  scratch_size_ = tinput_size_ + toutput_size_
      + binput_size_ + bweights_size_ + boutput_size_;

  return 0;
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::set_scratch_buffers(void *base)
{
  if (base != nullptr) {
    tinput_ = (TinputType *)base;
    toutput_ = (ToutputType *)((char *)tinput_ + tinput_size_);
    binput_ = (InputType *)((char *)toutput_ + toutput_size_);
    bweights_ = (WeightsType *)((char *)binput_ + binput_size_);
    boutput_ = (OutputType *)((char *)bweights_ + bweights_size_);
  }
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::set_workspace_buffers(void *base)
{
  if (base != nullptr) {
    tweights_ = (TweightsType *)base;
    input_scale_ = (TscaleType *)((char *)tweights_ + tweights_size_);
    weights_scale_ = (TscaleType *)((char *)input_scale_ + input_scale_size_);
    tweights_s8_ = (int8_t *)((char *)weights_scale_ + weights_scale_size_);
  }
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::prepare_quant_calibration(eld_conv_t &dc)
{
  this->input_quant_S = dc.input_quant.scale;
  this->input_quant_repS = 1 / dc.input_quant.scale;
  this->input_quant_z = dc.input_quant.z;
  this->output_quant_S = dc.output_quant.scale;
  this->output_quant_repS = 1 / dc.output_quant.scale;
  this->output_quant_z = dc.output_quant.z;
  this->sum_quant_S = dc.sum_quant.scale;
  this->sum_quant_z = dc.sum_quant.z;

  if (this->sampling_kind != CALIBRATED)
    el_error("Unsupported quantization mode in int8 direct 1x1");
}

Template_elx_int8_conv_direct_1x1_t
Instance_elx_int8_conv_direct_1x1_t::~elx_int8_conv_direct_1x1_t()
{
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::trans_weights_s8_blocked_oc(
    TscaleType *weights_scale, int8_t *tweights_s8, WeightsType *weights,
    BiasType *bias)
{
  __m<V> mmscale = _mm<V>::set1_ps(EL_INT8_MAX);

  // abs max
  estl::parallel_for<3>(mthr_, [&](int _O4, int _O3, int _O2) {
    MD5(TscaleType, aweights_scale, weights_scale,
        this->O4, this->O3, 2, this->O2, V);
    __m<V> mmabs_max = _mm<V>::set1_ps(0.0);
    iter_each (_I4, this->I4) {
    iter_each (_I3, this->I3) {
    iter_each (_I2, this->I2) {
    iter_each (_iV1, this->V1) {
    iter_each (_iVx, this->Vx) {
      MD9(WeightsType, aweights, weights, this->O4, this->O3, this->O2,
          this->I4, this->I3, this->I2, this->V1, this->Vx, V);
      mmabs_max = _mm<V>::max_ps(mmabs_max, _mm512_abs_ps(*(__m<V> *)
          &md9(aweights, _O4, _O3, _O2, _I4, _I3, _I2, _iV1, _iVx, 0)));
    }}}}}
    _mm<V>::store_ps(
        &md5(aweights_scale, _O4, _O3, 0, _O2, 0), mmabs_max);
  }, this->O4, this->O3, this->O2);

  // O4, (O3, O3r), (O2, O2r), I4, I3, I2, V1, Vx, V ->
  // O4, I4, (O3, O3r), I3, I2, V1, (O2, O2r), V, Vx
  // quantization
  estl::parallel_for<9>(mthr_, [&](int _O4, int _I4, int _O3,
    int _I3, int _O1, int _I2, int _iV1, int _O, int _iVx) {
    MD10(WeightsType, aweights, weights, this->O4, this->O3, this->O1, this->O,
        this->I4, this->I3, this->I2, this->V1, this->Vx, V);
    MD10(int8_t, atweights_s8, tweights_s8, this->O4, this->I4,
        this->O3, this->I3, this->O1, this->I2, this->V1, this->O, V, this->Vx);
    MD6(TscaleType, aweights_scale, weights_scale,
        this->O4, this->O3, 2, this->O1, this->O, V);

    auto mmresf32 = _mm<V>::mul_ps(
        *(__m<V> *)&md10(aweights, _O4, _O3, _O1, _O, _I4, _I3, _I2, _iV1, _iVx, 0),
        mmscale);
    mmresf32 = _mm<V>::div_ps(mmresf32,
        *(__m<V> *)&md6(aweights_scale, _O4, _O3, 0, _O1, _O, 0));
    mmresf32 = _mm<V>::roundscale_ps(
        mmresf32, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    TscaleType *resf32 = (TscaleType *)&mmresf32;
#pragma omp simd
    iter_each (_oV, V) {
      md10(atweights_s8, _O4, _I4, _O3, _I3, _O1, _I2, _iV1, _O, _oV, _iVx) =
          (int8_t)resf32[_oV];
    }
  }, this->O4, this->I4, this->O3, this->I3, this->O1, this->I2, this->V1, this->O, this->Vx);

  // accumulation
  estl::parallel_for<5>(mthr_, [&](int _O4, int _O3, int _O1, int _O, int _oV) {
    MD10(int8_t, atweights_s8, tweights_s8, this->O4, this->I4,
        this->O3, this->I3, this->O1, this->I2, this->V1, this->O, V, this->Vx);
    MD6(TscaleType, aweights_scale, weights_scale,
        this->O4, this->O3, 2, this->O1, this->O, V);
    TscaleType acc = 0;
    iter_each (_I4, this->I4) {
    iter_each (_I3, this->I3) {
    iter_each (_I2, this->I2) {
    iter_each (_iV1, this->V1) {
    iter_each (_iVx, this->Vx) {
      acc += (TscaleType)md10(atweights_s8,
          _O4, _I4, _O3, _I3, _O1, _I2, _iV1, _O, _oV, _iVx);
    }}}}}
    md6(aweights_scale, _O4, _O3, 1, _O1, _O, _oV) = acc;
  }, this->O4, this->O3, this->O1, this->O, V);

  // scale
  estl::parallel_for<3>(mthr_, [&](int _O4, int _O3, int _O2) {
    MD5(TscaleType, aweights_scale, weights_scale,
        this->O4, this->O3, 2, this->O2, V);
    __m<V> &mmqs = *(__m<V> *)&md5(
        aweights_scale, _O4, _O3, 0, _O2, 0);
    mmqs = mmqs / mmscale;
  }, this->O4, this->O3, this->O2);

  // combine
  __m<V> mmorepS = _mm<V>::set1_ps(this->output_quant_repS);
  __m<V> mmoz = _mm<V>::set1_ps(this->output_quant_z);
  __m<V> mmiS = _mm<V>::set1_ps(this->input_quant_S);
  __m<V> mmiz = _mm<V>::set1_ps(this->input_quant_z);
  estl::parallel_for<3>(mthr_, [&](int _O4, int _O3, int _O2) {
    MD5(TscaleType, aweights_scale, weights_scale,
        this->O4, this->O3, 2, this->O2, V);
    MD4(BiasType, abias, bias, this->O4, this->O3, this->O2, V);
    __m<V> &mmqs = *(__m<V> *)&md5(
        aweights_scale, _O4, _O3, 0, _O2, 0);
    __m<V> &mmqf = *(__m<V> *)&md5(
        aweights_scale, _O4, _O3, 1, _O2, 0);
    __m<V> mmbias = this->with_bias
                  ? *(__m<V> *)&md4(abias, _O4, _O3, _O2, 0)
                  : _mm<V>::setzero_ps();

    if (std::is_same<OutputType, float>::value) {
      mmqs = mmiS * mmqs;
      mmqf = mmbias - mmiz * mmqf * mmqs;
    } else {
      mmqs = mmiS * mmqs * mmorepS;
      mmqf = mmbias * mmorepS + mmoz - mmiz * mmqf * mmqs;
    }

    if (this->with_ip_sum) {
      __m<V> sum_S = _mm<V>::set1_ps(this->sum_quant_S);
      __m<V> sum_z = _mm<V>::set1_ps(this->sum_quant_z);
      mmqf -= sum_z * sum_S;
    }
  }, this->O4, this->O3, this->O2);
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::requant_output(
    OutputType *output, ToutputType *toutput)
{
  __m<V> mmorepS = _mm<V>::set1_ps(this->output_quant_repS);
  __m<V> mmoz = _mm<V>::set1_ps(this->output_quant_z);

  estl::parallel_for<4>(mthr_, [&](int _n, int _o, int _oh, int _ow) {
    MD5(ToutputType, atoutput, toutput,
        this->n, this->OC / V, this->oh, this->ow, V);
    MD5(OutputType, aoutput, output,
        this->n, this->OC / V, this->oh, this->ow, V);
    __m<V> mmres = *(__m<V> *)&md5(atoutput, _n, _o, _oh, _ow, 0);
    __m<V> mmresf32 = mmres * mmorepS + mmoz;
    __i<V> mmress32 = _mm<V>::cvt_roundps_epi32(
        mmresf32, _MM_FROUND_TO_NEAREST_INT  | _MM_FROUND_NO_EXC);
    __m128i mmresx8;
    if (std::is_same<OutputType, int8_t>::value)
      mmresx8 = _mm<V>::cvtsepi32_epi8(mmress32);
    else if (std::is_same<OutputType, uint8_t>::value)
      mmresx8 = _mm<V>::cvtusepi32_epi8(mmress32);
    else {
      mmresx8 = _mm_setzero_si128();
      el_error("Unsupported output type for int8 direct 1x1");
    }
    _mm_store_si128((__m128i *)&md5(aoutput, _n, _o, _oh, _ow, 0), mmresx8);
  }, this->n, this->OC / V, this->oh, this->ow);
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::gemm_b161(ToutputType *toutput,
    OutputType *output, uint8_t *input, int8_t *weights, TscaleType *input_scale,
    TscaleType *weights_scale, BiasType *bias, int _I4)
{
  MD3(int8_t, aweights, weights,
      this->O3, this->I3, this->O2 * this->I2 * V * V);
  MD2(BiasType, abias, bias, this->O3, this->O2 * V);
  MD2(TscaleType, ainput_scale, input_scale, 2, this->T);
  MD4(TscaleType, aweights_scale, weights_scale, this->O3, 2, this->O2, V);
  // blocked
  MD2(uint8_t, ainput_blocked, input,
      this->I3, this->I2 * this->ih * this->iw * V);
  MD2(OutputType, aoutput_blocked, output,
      this->O3, this->O2 * this->ht * this->wt *  this->T * V);
  MD2(ToutputType, atoutput_blocked, toutput,
      this->O3, this->O2 * this->ht * this->wt *  this->T * V);
  // nhwc
  MD2(uint8_t, ainput_nhwc, input, this->I3, this->I2 * V);
  MD2(OutputType, aoutput_nhwc, output, this->O3, this->O2 * V);
  MD2(ToutputType, atoutput_nhwc, toutput, this->O3, this->O2 * V);

  auto ker_gemm = ker_u8s8_gemm_I_O_T_;

  iter_each (_I3, this->I3) {
    int attr = _I4 == 0 && _I3 == 0
        ? set_bit(attr_, AT_CLEAR_OUTPUT_MASK)
        : attr_;
    if (_I4 == this->I4 - 1 && _I3 == this->I3 - 1) {
      if (this->with_relu)
        attr = set_bit(attr, AT_RELU_MASK);
      attr = set_bit(attr, AT_RESTORE_OUTPUT_MASK);
    }
    auto ain = this->input_fmt == nhwc
        ? &md2(ainput_nhwc, _I3, 0) : &md2(ainput_blocked, _I3, 0);
    iter_each (_O3, this->O3) {
      auto aout = this->output_fmt == nhwc
                ? &md2(aoutput_nhwc, _O3, 0) : &md2(aoutput_blocked, _O3, 0);
      auto atout = this->output_fmt == nhwc
                ? &md2(atoutput_nhwc, _O3, 0) : &md2(atoutput_blocked, _O3, 0);
      ker_gemm(*this, atout, aout, ain,
          &md3(aweights, _O3, _I3, 0),
          &md2(abias, _O3, 0),
          attr,
          &md2(ainput_scale, 0, 0),
          &md2(ainput_scale, 1, 0),
          &md4(aweights_scale, _O3, 0, 0, 0),
          &md4(aweights_scale, _O3, 1, 0, 0));
    }
  }
}

Template_elx_int8_conv_direct_1x1_t
void Instance_elx_int8_conv_direct_1x1_t::gemm_c160(ToutputType *toutput,
    OutputType *output, uint8_t *input, int8_t *weights_s8, TscaleType *input_scale,
    TscaleType *weights_scale, BiasType *bias, int _I4, int _O4, int _t2)
{
  // input
  MD3(uint8_t, ainput_blocked, input,
      this->I4, this->I3, this->I2 * this->ih * this->iw * V);
  MD3(uint8_t, ainput_nhwc, input, this->t2, T, this->ic);
  // output
  MD3(OutputType, aoutput_blocked, output,
      this->O4, this->O3, this->O2 * this->oh * this->ow * V);
  MD3(OutputType, aoutput_nhwc, output, this->t2, T, this->oc);
  // toutput
  MD3(ToutputType, atoutput_blocked, toutput,
      this->O4, this->O3, this->O2 * this->oh * this->ow * V);
  MD3(ToutputType, atoutput_nhwc, toutput, this->t2, T, this->oc);

  MD3(int8_t, aweights_s8, weights_s8,
      this->O3, this->I3, this->O2 * this->I2 * V * V);
  MD2(BiasType, abias, bias, this->O3, this->O2 * V);
  MD2(TscaleType, ainput_scale, input_scale, 2, this->T);
  MD4(TscaleType, aweights_scale, weights_scale,
      this->O3, 2, this->O2, V);

  auto ker_gemm = (_t2 == this->t2 - 1)
      ? ker_u8s8_gemm_I_O_Tr_
      : ker_u8s8_gemm_I_O_T_;

  iter_each (_I3, this->I3) {
    MD2(uint8_t, ainput2_blocked, &md3(ainput_blocked, _I4, _I3, 0), this->t2, this->T * V);
    MD3(uint8_t, ainput2_nhwc, &md3(ainput_nhwc, _t2, 0, 0), this->I4, this->I3, this->I2 * V);
    int attr = _I4 == 0 && _I3 == 0
        ? set_bit(attr_, AT_CLEAR_OUTPUT_MASK)
        : attr_;

    if (_I4 == this->I4 - 1 && _I3 == this->I3 - 1) {
      if (this->with_relu)
        attr = set_bit(attr, AT_RELU_MASK);
      attr = set_bit(attr, AT_RESTORE_OUTPUT_MASK);
    }

    auto ain = this->input_fmt == nhwc
             ? &md3(ainput2_nhwc, _I4, _I3, 0)
             : &md2(ainput2_blocked, _t2, 0);
    iter_each (_O3, this->O3) {
      int _O4_tout = toutput_opt_ ? 0 : _O4;
      MD2(OutputType, aoutput2_blocked, &md3(aoutput_blocked, _O4, _O3, 0),
          this->t2, this->T * V);
      MD3(OutputType, aoutput2_nhwc, &md3(aoutput_nhwc, _t2, 0, 0),
          this->O4, this->O3, this->O2 * V);
      MD2(ToutputType, atoutput2_blocked,
          &md3(atoutput_blocked, _O4_tout, _O3, 0), this->t2, this->T * V);
      MD3(ToutputType, atoutput2_nhwc, &md3(atoutput_nhwc, _t2, 0, 0),
          this->O4, this->O3, this->O2 * V);
      auto aout = this->output_fmt == nhwc
                ? &md3(aoutput2_nhwc, _O4, _O3, 0)
                : &md2(aoutput2_blocked, _t2, 0);
      auto atout = this->output_fmt == nhwc
                ? &md3(atoutput2_nhwc, _O4, _O3, 0)
                : &md2(atoutput2_blocked, _t2, 0);
      ker_gemm(*this, atout, aout, ain,
          &md3(aweights_s8, _O3, _I3, 0),
          &md2(abias, _O3, 0),
          attr,
          &md2(ainput_scale, 0, 0),
          &md2(ainput_scale, 1, 0),
          &md4(aweights_scale, _O3, 0, 0, 0),
          &md4(aweights_scale, _O3, 1, 0, 0));
    }
  }
}

} // namespace euler