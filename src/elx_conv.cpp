#include <assert.h>
#include "euler.hpp"
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"
#include "elk_conv_wino.hpp"

namespace euler {

template <typename UserTypes>
elx_conv_t<UserTypes>::elx_conv_t(eld_conv_t<UserTypes> &dc)
{
  this->n = dc.dims.input.n;
  this->ic = dc.dims.input.c;
  this->oc = dc.dims.output.c;
  this->ih = dc.dims.input.h;
  this->iw = dc.dims.input.w;
  this->oh = dc.dims.output.h;
  this->ow = dc.dims.output.w;
  this->kh = dc.dims.weights.h;
  this->kw = dc.dims.weights.w;
  this->lp = dc.pads.l;
  this->rp = dc.pads.r;
  this->tp = dc.pads.t;
  this->bp = dc.pads.b;
  this->hs = dc.strides.h;
  this->ws = dc.strides.w;
  this->hd = dc.dilations.h;
  this->wd = dc.dilations.w;

  this->input_fmt = dc.formats.input;
  this->weights_fmt = dc.formats.weights;
  this->output_fmt = dc.formats.output;
  this->with_relu = dc.with_relu;
  this->with_bias = dc.with_bias;
  this->with_ip_sum = dc.with_ip_sum;
  this->with_op_sum = dc.with_op_sum;
  this->f16c_opt = dc.f16c_opt;
  this->fp_mode = dc.fp_mode;

  this->prop_kind = dc.prop_kind;

  this->nthreads = dc.nthreads;
  this->execution_mode = dc.execution_mode;

  /* Automatical parameters */
  this->O = dc.flatting.o;
  this->T = dc.flatting.t;

  this->I2 = dc.blocking.i;
  this->O1 = dc.blocking.o;

  this->ic4 = dc.partition.i;
  this->oc4 = dc.partition.o;

  this->streaming_weights = dc.streaming_hint.weights;
  this->streaming_input = dc.streaming_hint.input;
  this->streaming_output = dc.streaming_hint.output;

  this->input_as_blocked = dc.format_as_blocked.input;
  this->weights_as_blocked = dc.format_as_blocked.weights;
  this->output_as_blocked = dc.format_as_blocked.output;

  this->quantization_calibration_min = dc.quantization_calibration_min;
  this->quantization_calibration_max = dc.quantization_calibration_max;
}

template <typename UserTypes>
int elx_conv(eld_conv_t<UserTypes> &desc,
    typename UserTypes::OutputType *output,
    typename UserTypes::InputType *input,
    typename UserTypes::WeightsType *weights,
    typename UserTypes::BiasType *bias)
{
  elx_conv_t<UserTypes> &xc = *desc.xc;

  // Sanity check
  if (input == nullptr || weights == nullptr || output == nullptr
      || (desc.with_bias && bias == nullptr)) {
    el_error("Parameter error. Invalid input data!");
    return ELX_GENERAL_ERROR;
  }

  xc.execute(output, input, weights, bias);
  return ELX_OK;
}

template int elx_conv<conv::FP32>(
    eld_conv_t<conv::FP32> &, float *, float *, float *, float *);

template int elx_conv<conv::FP16>(
    eld_conv_t<conv::FP16> &, short *, short *, short *, short *);

template int elx_conv<conv::FP16O>(
    eld_conv_t<conv::FP16O> &, short *, float *, float *, float *);
}  // namespace euler
