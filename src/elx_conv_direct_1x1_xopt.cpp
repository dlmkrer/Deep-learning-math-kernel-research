#include <string.h>
#include "el_intrin.hpp"
#include "el_utils.hpp"
#include "elx_conv_direct_1x1.hpp"
#include "el_def.hpp"
#include "el_utils.hpp"
#include "elk_conv_wino.hpp"
#include "elx_conv.hpp"
#include "euler.hpp"

// XOPT
// kernel options:
//   - a: CCC, s1
//   - b: CCD, s1
//   - c: DDD: s1
//   - d: DDD: s2
//   - e: DCD: s2
// fusion:  same as winograd
// dup:     same as winograd
//
// ------+-----+--------+-----+--------------------------------------
//       | ker | fusion | dup |             notes
// ------+-----+--------+-----+--------------------------------------
//  a061 |  a  |   t+o  |  I  | plain, stride>=1, padded
// ------+-----+--------+-----+--------------------------------------
//  f061 |  a  |   t+o  |  I  | plain, stride=1
// ------+-----+--------+-----+--------------------------------------
//  b061 |  b  |   t+o  |  I  | blocked, stride>=1, large batch
// ------+-----+--------+-----+--------------------------------------
//  e060 |  e  |   t+o  |  -  | blocked, stride>=1, large batch
// ------+-----+--------+-----+--------------------------------------
//  c060 |  c  |   t+o  |  -  | Tr, Or, blocked, stride=1
// ------+-----+--------+-----+--------------------------------------
//  d060 |  d  |   t+o  |  -  | Or, blocked, stride>=1, small batch
// ------+-----+--------+-----+--------------------------------------
//
namespace euler {

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_c060(
    Type *output, Type *input, Type *weights, Type *bias)
{
  // weights: oc4*, oc3, O2(O2r), ic4*, ic3, I2, V, V
  // input:   t3*, ic4*, ic3, I2, t2*, T(Tr), V
  // output:  t3*, oc4*, oc3, O2(O2r), t2*, T(Tr), V
  MD2(Type, aweights, weights, this->oc4, this->oc3 * this->O2 * this->IC * V);
  MD3(Type, ainput, input, this->t3, this->ic4,
      this->ic3 * this->I2 * this->ih * this->iw * V);
  MD2(Type, aoutput, output, this->t3, this->OC * this->oh * this->ow);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  iter_each (_ic4, this->ic4) {
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(3)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_t2, this->t2) {
      MD2(Type, aoutput2, &md2(aoutput, _t3, 0), this->oc4,
          this->oc3 * this->O2 * this->oh * this->ow * V);
      gemm_c060(&md2(aoutput2, _oc4, 0), &md3(ainput, _t3, _ic4, 0),
          &md2(aweights, _oc4, 0), &md2(abias, _oc4, 0), _ic4, _oc4, _t2);
    }}}
  }
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_d060(
    Type *output, Type *input, Type *weights, Type *bias)
{
  // weights: oc4*, oc3, O2(O2r), ic4*, ic3, I2, V, V
  // input:   t3*, ic4*, ic3, I2, ht*, S, wt*, T, S, V
  // output:  t3*, oc4*, oc3, O2(O2r), ht*wt*, T, V
  MD2(Type, aweights, weights, this->oc4, this->oc3 * this->O2 * this->IC * V);
  MD7(Type, ainput, input, this->t3, this->ic4, this->ic3 * this->I2,
      this->ht, this->hs, this->wt, this->T * this->ws * V);
  MD2(Type, aoutput, output, this->t3, this->OC * this->oh * this->ow);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  iter_each (_ic4, this->ic4) {
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(4)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_ht, this->ht) {
    iter_each (_wt, this->wt) {
      MD5(Type, aoutput2, &md2(aoutput, _t3, 0), this->oc4,
          this->oc3 * this->O2, this->ht, this->wt, this->T * V);
      gemm_d060(&md5(aoutput2, _oc4, 0, _ht, _wt, 0),
          &md7(ainput, _t3, _ic4, 0, _ht, 0, _wt, 0),
          &md2(aweights, _oc4, 0), &md2(abias, _oc4, 0), _ic4, _oc4, _ht,
          _wt);
    }}}}
  }
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_e060(
    Type *output, Type *input, Type *weights, Type *bias)
{
  // weights: oc4*, oc3, O2, ic4*, ic3, I2, V, V
  // input:   t3*, ic4*, ic3, I2, ht*, S, wt*, S, T, V
  // output:  t3*, oc4*, oc3, O2, ht*, wt*, T, V
  MD7(Type, ainput, input, this->t3, this->ic4, this->ic3 * this->I2,
      this->ht, this->hs, this->wt, this->T * this->ws * V);
  MD6(Type, aoutput, output, this->t3, this->oc4, this->oc3 * this->O2,
      this->ht, this->wt, this->T * V);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  MD3(Type, atweights, tweights_, this->oc4, this->ic4,
      this->oc3 * this->ic3 * this->O2 * this->I2 * V * V);

  if (is_first_run_) {
    trans_weights(tweights_, weights);
  }

  iter_each (_ic4, this->ic4) {
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(4)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_ht, this->ht) {
    iter_each (_wt, this->wt) {
      gemm_e060(&md6(aoutput, _t3, _oc4, 0, _ht, _wt, 0),
          &md7(ainput, _t3, _ic4, 0, _ht, 0, _wt, 0),
          &md3(atweights, _oc4, _ic4, 0), &md2(abias, _oc4, 0), _ic4);
    }}}}
  }

  if (inference_acc_)
    is_first_run_ = false;
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_b061(
    Type *output, Type *input, Type *weights, Type *bias)
{
  // weights: oc4*, oc3, O2(O2r), ic4*, ic3, I2, V, V
  // input:   t3*, ic4*, ic3, I2, t2*, T(Tr), V
  // output:  t3*, oc4*, oc3, O2(O2r), t2*, T(Tr), V
  MD3(Type, ainput, input, this->t3, this->ic4,
      this->ic3 * this->I2 * this->ih * this->iw * V);
  MD2(Type, aoutput, output, this->t3, this->OC * this->oh * this->ow);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  MD3(Type, atweights, tweights_, this->oc4, this->ic4,
      this->oc3 * this->ic3 * this->O2 * this->I2 * V * V);

  if (is_first_run_) {
    trans_weights(tweights_, weights);
  }

  if (this->oc4 == 1) {
    MD2(Type, atinput, tinput_, mthr_, this->ic3 * this->I2 * this->T * V);
    iter_each (_ic4, this->ic4) {
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(4)
      iter_each (_t3, this->t3) {
      iter_each (_oc4, this->oc4) {
      iter_each (_ht, this->ht) {
      iter_each (_wt, this->wt) {
        size_t ithr = omp_get_thread_num();
        MD5(Type, aoutput2, &md2(aoutput, _t3, 0), this->oc4,
            this->oc3 * this->O2, this->ht, this->wt, this->T * V);

        trans_input(&md2(atinput, ithr, 0),
            &md3(ainput, _t3, _ic4, 0), _ht, _wt);
        gemm_b061(&md5(aoutput2, _oc4, 0, _ht, _wt, 0), &md2(atinput, ithr, 0),
            &md3(atweights, _oc4, _ic4, 0), &md2(abias, _oc4, 0), _ic4);
      }}}}
    }
  } else {
    MD4(Type, atinput, tinput_, mthr_, this->ht, this->wt,
        this->ic3 * this->I2 * this->T * V);
    MD3(unsigned char, atinput_msk, tinput_msk_, mthr_, this->ht, this->wt);
    iter_each (_ic4, this->ic4) {
      int t3_history = -1;
#pragma omp parallel num_threads(mthr_) proc_bind(close) firstprivate(t3_history)
#pragma omp for nowait collapse(4)
      iter_each (_t3, this->t3) {
      iter_each (_oc4, this->oc4) {
      iter_each (_ht, this->ht) {
      iter_each (_wt, this->wt) {
        size_t ithr = omp_get_thread_num();
        MD5(Type, aoutput2, &md2(aoutput, _t3, 0), this->oc4,
            this->oc3 * this->O2, this->ht, this->wt, this->T * V);

        if (_t3 != t3_history) {
          memset(&md3(atinput_msk, ithr, 0, 0), 0, this->ht * this->wt);
          t3_history = _t3;
        }
        if (md3(atinput_msk, ithr,  _ht, _wt) == 0) {
          trans_input(&md4(atinput, ithr, _ht, _wt, 0),
              &md3(ainput, _t3, _ic4, 0), _ht, _wt);
          md3(atinput_msk, ithr, _ht, _wt) = 1;
        }
        gemm_b061(&md5(aoutput2, _oc4, 0, _ht, _wt, 0),
            &md4(atinput, ithr, _ht, _wt, 0),
            &md3(atweights, _oc4, _ic4, 0), &md2(abias, _oc4, 0), _ic4);
      }}}}
    }
  }

  if (inference_acc_)
    is_first_run_ = false;
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_a061(
    Type *output, Type *input, Type *weights, Type *bias)
{
  MD2(Type, ainput, input, this->t3, this->ic * this->ih * this->iw);
  MD2(Type, aoutput, output, this->t3, this->oc * this->oh * this->ow);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  MD2(Type, atoutput, toutput_, mthr_, this->oc3 * this->O2 * this->T * V);
  MD3(Type, atweights, tweights_, this->oc4, this->ic4,
      this->oc3 * this->ic3 * this->O2 * this->I2 * V * V);

  if (is_first_run_) {
    trans_weights(tweights_, weights);
  }

  if (this->oc4 == 1) {
    MD2(Type, atinput, tinput_, mthr_, this->ic3 * this->I2 * this->T * V);
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(4)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_ht, this->ht) {
    iter_each (_wt, this->wt) {
      size_t ithr = omp_get_thread_num();
      trans_input(&md2(atinput, ithr, 0),
          &md2(ainput, _t3, 0), _ht, _wt);
      gemm_a061(&md2(atoutput, ithr, 0), &md2(atinput, ithr, 0),
          &md3(atweights, _oc4, 0, 0), &md2(abias, _oc4, 0), 0);
      trans_output(&md2(aoutput, _t3, 0), &md2(atoutput, ithr, 0),
          _oc4, _ht, _wt);
    }}}}
  } else {
    MD4(Type, atinput, tinput_, mthr_, this->ht, this->wt,
        this->ic3 * this->I2 * this->T * V);
    MD3(unsigned char, atinput_msk, tinput_msk_, mthr_, this->ht, this->wt);
    int t3_history = -1;
#pragma omp parallel num_threads(mthr_) proc_bind(close) firstprivate(t3_history)
#pragma omp for nowait collapse(4)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_ht, this->ht) {
    iter_each (_wt, this->wt) {
      size_t ithr = omp_get_thread_num();
      if (_t3 != t3_history) {
        memset(&md3(atinput_msk, ithr, 0, 0), 0, this->ht * this->wt);
        t3_history = _t3;
      }
      if (md3(atinput_msk, ithr,  _ht, _wt) == 0) {
        trans_input(&md4(atinput, ithr, _ht, _wt, 0),
            &md2(ainput, _t3, 0), _ht, _wt);
        md3(atinput_msk, ithr, _ht, _wt) = 1;
      }
      gemm_a061(&md2(atoutput, ithr, 0),
          &md4(atinput, ithr, _ht, _wt, 0),
          &md3(atweights, _oc4, 0, 0), &md2(abias, _oc4, 0), 0);
      trans_output(&md2(aoutput, _t3, 0), &md2(atoutput, ithr, 0),
          _oc4, _ht, _wt);
    }}}}
  }

  if (inference_acc_)
    is_first_run_ = false;
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::__execute_f061(
    Type *output, Type *input, Type *weights, Type *bias)
{
  MD2(Type, ainput, input, this->t3, this->ic * this->ih * this->iw);
  MD2(Type, aoutput, output, this->t3, this->oc * this->oh * this->ow);
  MD2(Type, abias, bias, this->oc4, this->oc3 * this->O2 * V);

  MD2(Type, atoutput, toutput_, mthr_, this->oc3 * this->O2 * this->T * V);
  MD3(Type, atweights, tweights_, this->oc4, this->ic4,
      this->oc3 * this->ic3 * this->O2 * this->I2 * V * V);

  if (is_first_run_) {
    trans_weights(tweights_, weights);
  }

  if (this->oc4 == 1) {
    MD2(Type, atinput, tinput_, mthr_, this->ic3 * this->I2 * this->T * V);
#pragma omp parallel num_threads(mthr_) proc_bind(close)
#pragma omp for nowait collapse(3)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_t2, this->t2) {
      size_t ithr = omp_get_thread_num();
      int Tz = _t2 == (this->t2 - 1) ? this->Tr : this->T;
      trans_input2(&md2(atinput, ithr, 0), &md2(ainput, _t3, 0), _t2, Tz);
      gemm_f061(&md2(atoutput, ithr, 0), &md2(atinput, ithr, 0),
          &md3(atweights, _oc4, 0, 0), &md2(abias, _oc4, 0), _t2, Tz);
      trans_output2(&md2(aoutput, _t3, 0), &md2(atoutput, ithr, 0),
          _oc4, _t2, Tz);
    }}}
  } else {
    MD3(Type, atinput, tinput_, mthr_, this->t2,
        this->ic3 * this->I2 * this->T * V);
    MD2(unsigned char, atinput_msk, tinput_msk_, mthr_, this->t2);
    int t3_history = -1;
#pragma omp parallel num_threads(mthr_) proc_bind(close) firstprivate(t3_history)
#pragma omp for nowait collapse(3)
    iter_each (_t3, this->t3) {
    iter_each (_oc4, this->oc4) {
    iter_each (_t2, this->t2) {
      size_t ithr = omp_get_thread_num();
      int Tz = _t2 == (this->t2 - 1) ? this->Tr : this->T;
      if (_t3 != t3_history) {
        memset(&md2(atinput_msk, ithr, 0), 0, this->t2);
        t3_history = _t3;
      }
      if (md2(atinput_msk, ithr, _t2) == 0) {
        trans_input2(&md3(atinput, ithr, _t2, 0),
            &md2(ainput, _t3, 0), _t2, Tz);
        md2(atinput_msk, ithr, _t2) = 1;
      }
      gemm_f061(&md2(atoutput, ithr, 0), &md3(atinput, ithr, _t2, 0),
          &md3(atweights, _oc4, 0, 0), &md2(abias, _oc4, 0), _t2, Tz);
      trans_output2(&md2(aoutput, _t3, 0), &md2(atoutput, ithr, 0),
          _oc4, _t2, Tz);
    }}}
  }

  if (inference_acc_)
    is_first_run_ = false;
}

template <typename Type, const int V, const int I>
void elx_conv_direct_1x1_t<Type, V, I>::execute(
    Type *output, Type *input, Type *weights, Type *bias)
{
  if (is_bfmt_)
    (this->*execute_opt_)(output, input, weights, bias);
  else {
    Type *in = input_as_bfmt_ ? binput_ : input;
    Type *wei = weights_as_bfmt_ ? bweights_ : weights;
    Type *out = output_as_bfmt_ ? boutput_ : output;

    if (input_as_bfmt_) {
      trans_input_2_blocked(in, input);
    }

    if (weights_as_bfmt_) {
      trans_weights_2_blocked(wei, weights);
    }

    // TODO: padding bias
    (this->*execute_opt_)(out, in, wei, bias);

    if (output_as_bfmt_) {
      trans_output_2_plain(output, out);
    }
  }
}

} // namespace euler