#ifndef __ELX_CONV_WINO_HPP__
#define __ELX_CONV_WINO_HPP__

#include <tuple>

#include "el_def.hpp"
#include "el_utils.hpp"
#include "elx_conv.hpp"
#include "euler.hpp"
#include "elk_conv_wino.hpp"
#include "kernel/elk_gemm_otj.hxx"

#include "kernel/elk_conv_wino_2x2_3x3_input.hxx"
#include "kernel/elk_conv_wino_2x2_3x3_output.hxx"
#include "kernel/elk_conv_wino_2x2_3x3_weights.hxx"

#include "kernel/elk_conv_wino_3x3_3x3_input.hxx"
#include "kernel/elk_conv_wino_3x3_3x3_output.hxx"
#include "kernel/elk_conv_wino_3x3_3x3_weights.hxx"

#include "kernel/elk_conv_wino_4x4_3x3_input.hxx"
#include "kernel/elk_conv_wino_4x4_3x3_output.hxx"
#include "kernel/elk_conv_wino_4x4_3x3_weights.hxx"

#include "kernel/elk_conv_wino_5x5_3x3_input.hxx"
#include "kernel/elk_conv_wino_5x5_3x3_output.hxx"
#include "kernel/elk_conv_wino_5x5_3x3_weights.hxx"

#include "kernel/elk_conv_wino_2x2_3x3_input_gen.hxx"
#include "kernel/elk_conv_wino_2x2_3x3_output_gen.hxx"
#include "kernel/elk_conv_wino_2x2_3x3_weights_gen.hxx"

#include "kernel/elk_conv_wino_3x3_3x3_input_gen.hxx"
#include "kernel/elk_conv_wino_3x3_3x3_output_gen.hxx"
#include "kernel/elk_conv_wino_3x3_3x3_weights_gen.hxx"

#include "kernel/elk_conv_wino_4x4_3x3_input_gen.hxx"
#include "kernel/elk_conv_wino_4x4_3x3_output_gen.hxx"
#include "kernel/elk_conv_wino_4x4_3x3_weights_gen.hxx"

#include "kernel/elk_conv_wino_5x5_3x3_input_gen.hxx"
#include "kernel/elk_conv_wino_5x5_3x3_output_gen.hxx"
#include "kernel/elk_conv_wino_5x5_3x3_weights_gen.hxx"

#include "kernel/elk_conv_wino_kernels.cosim.hxx"

namespace euler {

template <typename Type, const int A, const int K, const int V, const int I>
class elx_conv_wino_t : public elx_conv_t<Type> {
public:
  // Configurable parameters
  using elx_conv_t<Type>::IC;
  using elx_conv_t<Type>::OC;
  using elx_conv_t<Type>::T;
  using elx_conv_t<Type>::I2;
  using elx_conv_t<Type>::O2;
  using elx_conv_t<Type>::oc4;
  using elx_conv_t<Type>::ic3;
  using elx_conv_t<Type>::oc3;
  constexpr static size_t elem_sz = sizeof(Type);
  constexpr static bool is_border = true;
  constexpr static bool has_bias = true;
  constexpr static bool has_relu = true;
  constexpr static bool has_sum = true;
  constexpr static bool no = false;
public:
  elx_conv_wino_t(eld_conv_t<Type> &dc);
  virtual ~elx_conv_wino_t();

  virtual void execute(Type *output, Type *input, Type *weights, Type *bias);

  virtual void clflush();

  class exe_plan {
  public:
    exe_plan(int tiles, int IC, int OC):
      tiles_(tiles), tb_(tiles), ocd_(1), icb_(IC/V), ocb_(OC/V),
      weights_total(A * A * IC * OC * elem_sz), mode_(0xa061) {
    }

    inline bool bifurcate_oc() {
      if ((ocb_ & 0x1) == 0) {
        ocb_ /= 2;
        ocd_ *= 2;
        return true;
      }
      return false;
    }

    inline bool threading_fit(int num_cpu, int num_socket, std::size_t l2) {
      constexpr int reg_max = 32;
      /* double L3 effect, L2/L3 effect.
       * We still don't have clear boundaries between these */
      const int reg_min = (weights_total < (l2/2))?
        (13 - 1)/num_socket +1:
        (15 - 1)/num_socket +1;

      int n = 1;

      if ( tiles_ > reg_min * (num_cpu -1) + 1 ) {
        do { /* need something exponential */
          tb_ = (tiles_ - 1) / (n ++ * num_cpu) + 1;
        } while (tb_ > reg_max);
      } else {
        tb_ = (tiles_ * ocd_ - 1) / num_cpu + 1;
        while (tb_ < reg_min) {
          if (!bifurcate_oc())
            break;
          tb_ = (tiles_ * ocd_ - 1) / num_cpu + 1;
        }
      }

      return tb_ >= reg_min && tb_ < reg_max;
    }

    // Guarantees outputs L2 reside, then hiding output transform
    //
    inline bool l2_fit(std::size_t cache_sz) {
      while(gemm_output_reuse_set() > cache_sz) {
        if (!bifurcate_oc())
          break;
      }

      return gemm_output_reuse_set() < cache_sz;
    }

    // Is this necessary??? Don't know if it help.
    // Guarantees inputs L1 reside
    //
    inline bool l1_fit(std::size_t cache_sz) {
      while(gemm_input_reuse_set() > cache_sz) {
        if ( (icb_ & 0x1) == 0 )
          icb_ /= 2;
        else
          break;
      }

      return (gemm_input_reuse_set() < cache_sz);
    }

    inline bool fit(int num_cpu, int num_socket, std::size_t l2, std::size_t l1) {
      if (!(threading_fit(num_cpu, num_socket, l2) && l2_fit(l2) && l1_fit(l1))) {
        mode_ = 0xa000;
        // A000 execution tuning
      }

      return true;
    }

    // queries
    inline std::size_t input_unit() const {
      return elem_sz * tb_ * V;
    }

    inline std::size_t weights_unit() const {
      return elem_sz * V * V;
    }

    inline std::size_t output_unit() const {
      return elem_sz * tb_ * V;
    }

    inline std::size_t gemmker_input_footprint() const {
      return input_unit() * icb_;
    }

    inline std::size_t gemmker_weights_footprint() const {
      return weights_unit() * icb_;
    }

    inline std::size_t gemmker_output_footprint() const {
      return output_unit();
    }

    inline std::size_t gemm_input_reuse_set() const {
      return gemmker_input_footprint() +
        gemmker_weights_footprint() + gemmker_output_footprint();
    }

    inline std::size_t trans_output_footprint() const {
      return elem_sz * K * K * ocb_ * V * tb_;
    }

    inline std::size_t gemm_output_reuse_set() const {
      auto wtile_sz = elem_sz * A * A * icb_ * ocb_ * V * V;
      return wtile_sz + (gemmker_input_footprint() +
        gemmker_output_footprint() * ocb_) * A * A +
        trans_output_footprint();
    }

    void dump() const {
      std::cout<<"tb="<<tb_<<", icb_="<<icb_<<", ocb_"
        <<ocb_<<", ocd_="<<ocd_<<std::endl;
      std::cout<<"Input footprint: "<<gemm_input_reuse_set()<<std::endl;
      std::cout<<"Total footprint: "<<gemm_output_reuse_set()<<std::endl;
    }

    const int tiles_;
    int tb_, ocd_, icb_, ocb_;
    std::size_t weights_total;
    unsigned int mode_;
  };

  exe_plan execute_plan(int num_cpu, int num_socket, std::size_t l2, std::size_t l1) {
    exe_plan plan(this->t, this->IC, this->OC);
    plan.fit(num_cpu, num_socket, l2, l1);
    return plan;
  }

  inline std::size_t input_unit() const {
    return elem_sz * this->T * V;
  }

  inline std::size_t weights_unit() const {
    return elem_sz * V * V;
  }

  inline std::size_t output_unit() const {
    return elem_sz * this->T * V;
  }

  inline std::size_t gemmker_input_footprint() const {
    return input_unit() * this->I2;
  }

  inline std::size_t gemmker_weights_footprint() const {
    return weights_unit() * this->I2;
  }

  inline std::size_t gemmker_output_footprint() const {
    return output_unit();
  }

  inline std::size_t gemm_input_reuse_set() const {
    return gemmker_input_footprint() +
      gemmker_weights_footprint() + gemmker_output_footprint();
  }

  inline std::size_t gemm_output_reuse_set() const {
    auto wtile_sz = elem_sz * A * A * IC * OC;
    return wtile_sz/oc4 + (gemmker_input_footprint() +
      gemmker_output_footprint() * this->O2) * A * A;
  }

private:
  void __execute_a000(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a010(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a040(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a060(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a061(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a0e1(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a0e0(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a071(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a073(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a079(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a07b(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a201(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a241(Type *output, Type *input, Type *weights, Type *bias);
  void __execute_a448(Type *output, Type *input, Type *weights, Type *bias);

  inline void __trans_input_plain(Type *tinput, Type *input, int _t2, int Tz);
  inline void __trans_input_blocked(Type *tinput, Type *input, int _t2, int Tz);
  void trans_input(Type *tinput, Type *input, int _t2, int Tz);

  inline void __trans_input_plain(Type *tinput, Type *input);
  inline void __trans_input_blocked(Type *tinput, Type *input);
  void trans_input(Type *tinput, Type *input);

  inline void __trans_inputa_plain(Type *tinput, Type *input, int _t2, int _wA, int Tz);
  inline void __trans_inputa_blocked(Type *tinput, Type *input, int _t2, int _wA, int Tz);
  void trans_inputa(Type *tinput, Type *input, int _t2, int _wA, int Tz);

  inline void __trans_output_plain(Type *output, Type *toutput, Type *bias, int _t2, int Tz, int _ic4);
  inline void __trans_output_blocked(Type *output, Type *toutput, Type *bias, int _t2, int Tz, int _ic4);
  void trans_output(Type *output, Type *toutput, Type *bias, int _t2, int Tz, int _ic4 = -1);

  inline void __trans_output_plain(Type *output, Type *toutput, Type *bias);
  inline void __trans_output_blocked(Type *output, Type *toutput, Type *bias);
  void trans_output(Type *output, Type *toutput, Type *bias);

  inline void __trans_outputa_bh_plain(Type *output, Type *toutputa, Type *bias);
  inline void __trans_outputa_bh_blocked(Type *output, Type *toutputa, Type *bias);
  void trans_outputa_bh(Type *output, Type *toutputa, Type *bias);

  void trans_outputa_th(Type *toutputa, Type *toutput, int Tz);

  inline void __trans_weights_plain(Type *tweights, Type *weights, int oc4);
  inline void __trans_weights_blocked(Type *tweights, Type *weights, int oc4);
  void trans_weights(Type *tweights, Type *weights, int oc4 = 1);

  inline void __trans_weightsf_plain(Type *tweights, Type *weights, int _ic4, int _oc4);
  inline void __trans_weightsf_blocked(Type *tweights, Type *weights, int _ic4, int _oc4);
  void trans_weightsf(Type *tweights, Type *weights, int _ic4, int _oc4);

  inline void __trans_weightsa_plain(Type *tweights, Type *weights);
  inline void __trans_weightsa_blocked(Type *tweights, Type *weights);
  void trans_weightsa(Type *tweights, Type *weights);

  void gemm(Type *toutput, Type *tinput, Type *tweights, int _t2, int Tz, int _ic4 = 0);
  void gemm_non_acc(Type *toutput, Type *tinput, Type *tweights, int _t2, int Tz, int _ic4);
  void gemm(Type *toutput, Type *tinput, Type *tweights, int _ic4 = 0);
  void gemma(Type *toutput, Type *tinput, Type *tweights, int _t2, int Tz);

  void set_trans_buffers();
  int prepare_execute_opt();
  void bind_execute_functions();

  gemm_kernel_binder::ker *ker_gemm_;
  gemm_kernel_binder::ker *ker_gemm0_;
  gemm_kernel_binder::ker *ker_gemm_tail_;
  gemm_kernel_binder::ker *ker_gemm0_tail_;

  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template trans_input<no>) *ker_trans_input_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template trans_input<is_border>) *ker_trans_input0_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template trans_inputa<no>) *ker_trans_inputa_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template trans_inputa<is_border>) *ker_trans_inputa0_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::trans_weights) *ker_trans_weights_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template
        trans_output<false, false, false, false>) *ker_trans_output_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template
        trans_output<false, false, false, false>) *ker_trans_output0_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template
        trans_output<false, false, false, false>) *ker_trans_output_acc_;
  decltype(
      convolution_winograd_kernel<
        Type, I, V, A, K>::template
        trans_output<false, false, false, false>) *ker_trans_output0_acc_;
  decltype(
      convolution_winograd_kernel<
      Type, I, V, A, K>::template
      trans_output<false, false, false, false>) *ker_trans_output_nobias_;
  decltype(
      convolution_winograd_kernel<
      Type, I, V, A, K>::template
      trans_output<false, false, false, false>) *ker_trans_output0_nobias_;
  decltype(
      convolution_winograd_kernel<
      Type, I, V, A, K>::template trans_outputa_th<
      false, false, false, false>) *ker_trans_outputa_th_;
  decltype(
      convolution_winograd_kernel<
      Type, I, V, A, K>::template
      trans_outputa_bh<false, false, false, false>) *ker_trans_outputa_bh_;
  decltype(
      convolution_winograd_kernel<
      Type, I, V, A, K>::template
      trans_outputa_bh<false, false, false, false>) *ker_trans_outputa0_bh_;

  void (elx_conv_wino_t::*execute_opt_)(Type *, Type *, Type *, Type *);

  unsigned int xopt_;
  bool is_first_run_;
  bool inference_acc_;
  bool stream_in_;
  bool stream_out_;
  bool stream_wei_;
  bool is_bfmt_;
  bool input_is_bfmt_;
  bool weights_is_bfmt_;
  bool output_is_bfmt_;
  bool input_as_bfmt_;
  bool weights_as_bfmt_;
  bool output_as_bfmt_;
  int attr_;
  int mthr_;
  size_t tweights_size_;
  size_t tinput_size_;
  size_t toutput_size_;
  size_t toutputa_size_;
  size_t binput_size_;
  size_t bweights_size_;
  size_t boutput_size_;
  Type *workspace_;
  Type *scratch_;

  Type *tweights_;
  Type *tinput_;
  Type *toutput_;
  Type *toutputa_;
  Type *binput_; // blocked input
  Type *bweights_;
  Type *boutput_;

  int hOA_end_;
  int wOA_end_;
  int hA_end_;
  int wA_end_;

#define MAX_THREAD_TEAMS (8)
  // tasks allocation per thread team
  struct { int start; int end; } ttm_[MAX_THREAD_TEAMS];
};

template <int A, int K>
class input_tile_iter {
  constexpr static int output_line = A - K +1;
public:
  input_tile_iter(int t_init, int ht, int wt, int h, int w, int tp, int lp)
    : ht_(ht), wt_(wt), tp_(tp), lp_(lp),
    hA_end_(h + tp - (ht -1) * output_line -1),
    wA_end_(w + lp - (wt -1) * output_line -1),
    tile_h_(t_init / wt),
    tile_w_(t_init % wt),
    anchor_t_(tile_h_ * output_line - tp),
    anchor_l_(tile_w_ * output_line - lp),
    t_ (tile_h_ > 0 ? 0 : tp),
    l_ (tile_w_ > 0 ? 0 : lp),
    d_ (tile_h_ < ht -1 ? A -1 : hA_end_),
    r_ (tile_w_ < wt -1 ? A -1 : wA_end_) {}

  inline input_tile_iter &operator ++() {
    if ( ++ tile_w_ < wt_) {
      anchor_l_ += output_line;
    } else {
      tile_w_ = 0;
      anchor_l_ = -lp_;
      tile_h_ ++;
      anchor_t_ += output_line;
      t_ = tile_h_ > 0 ? 0 : tp_;
      d_ = tile_h_ < ht_ - 1 ? A -1 : hA_end_;
    }

    l_ = tile_w_ > 0 ? 0 : lp_;
    r_ = tile_w_ < wt_ - 1 ? A -1 : wA_end_;
    return *this;
  }

  inline input_tile_iter &operator --() {
    if ( -- tile_w_ >= 0) {
      anchor_l_ -= output_line;
    } else {
      tile_w_ = wt_ -1;
      anchor_l_ = wt_ * output_line - lp_;
      tile_h_ --;
      anchor_t_ -= output_line;
      t_ = tile_h_ > 0 ? 0 : tp_;
      d_ = tile_h_ < ht_ - 1 ? A -1 : hA_end_;
    }

    l_ = tile_w_ > 0 ? 0 : lp_;
    r_ = tile_w_ < wt_ - 1 ? A -1 : wA_end_;
    return *this;
  }

  inline void reset(int t = 0) {
    auto res = std::div(t, wt_);
    tile_h_ = res.quot;
    tile_w_ = res.rem;

    anchor_t_ = tile_h_ * output_line - tp_;
    anchor_l_ = tile_w_ * output_line - lp_;

    t_ = tile_h_ > 0 ? 0 : tp_;
    l_ = tile_w_ > 0 ? 0 : lp_;
    d_ = tile_h_ < ht_ -1 ? A -1 : hA_end_;
    r_ = tile_w_ < wt_ -1 ? A -1 : wA_end_;
  }

  inline bool is_border() const {
    return !(t_ == 0 && l_ == 0 && d_ == A-1 && r_ == A -1);
  }

protected:
  int ht_, wt_, tp_, lp_;

public:
  int hA_end_, wA_end_;
  int tile_h_, tile_w_;
  int anchor_t_, anchor_l_;
  int t_, l_, d_, r_;
};

template <int A, int K>
class output_tile_iter {
  constexpr static int output_line = A - K +1;
public:
  output_tile_iter(int t_init, int ht, int wt, int oh, int ow)
    : ht_(ht), wt_(wt),
    h_end_(oh - (ht -1) * output_line -1),
    w_end_(ow - (wt -1) * output_line -1),
    tile_h_(t_init / wt),
    tile_w_(t_init % wt),
    t_(tile_h_ * output_line),
    l_(tile_w_ * output_line),
    d_ (tile_h_ < ht -1 ? A -K : h_end_),
    r_ (tile_w_ < wt -1 ? A -K : w_end_) {}

  inline output_tile_iter &operator ++() {
    if ( ++ tile_w_ < wt_) {
      l_ += output_line;
    } else {
      tile_w_ = 0;
      l_ = 0;
      tile_h_ ++;
      t_ += output_line;
      d_ = tile_h_ < ht_ - 1 ? A -K : h_end_;
    }

    r_ = tile_w_ < wt_ - 1 ? A -K : w_end_;
    return *this;
  }

  inline output_tile_iter &operator --() {
    if ( -- tile_w_ >= 0) {
      l_ -= output_line;
    } else {
      tile_w_ = wt_ -1;
      l_ = wt_ * output_line;
      tile_h_ --;
      t_ -= output_line;
      d_ = tile_h_ < ht_ - 1 ? A -K : h_end_;
    }

    r_ = tile_w_ < wt_ - 1 ? A -K : w_end_;
    return *this;
  }

  inline void reset(int t = 0) {
    auto res = std::div(t, wt_);
    tile_h_ = res.quot;
    tile_w_ = res.rem;

    t_ = tile_h_ * output_line;
    l_ = tile_w_ * output_line;

    d_ = tile_h_ < ht_ -1 ? A -K : h_end_;
    r_ = tile_w_ < wt_ -1 ? A -K : w_end_;
  }

  inline bool is_border() const {
    return r_ < A - K || d_ < A - K;
  }

protected:
  int ht_, wt_;

public:
  int h_end_, w_end_;
  int tile_h_, tile_w_;
  int t_, l_, d_, r_;
};

#ifdef WITH_GK
template class elx_conv_wino_t<float, 4, 3, 16, ISA_GENERIC>;
template class elx_conv_wino_t<float, 4, 3, 16, ISA_COSIM_AVX512>;
template class elx_conv_wino_t<float, 5, 3, 16, ISA_GENERIC>;
template class elx_conv_wino_t<float, 5, 3, 16, ISA_COSIM_AVX512>;
template class elx_conv_wino_t<float, 6, 3, 16, ISA_GENERIC>;
template class elx_conv_wino_t<float, 6, 3, 16, ISA_COSIM_AVX512>;
template class elx_conv_wino_t<float, 7, 3, 16, ISA_GENERIC>;
template class elx_conv_wino_t<float, 7, 3, 16, ISA_COSIM_AVX512>;
// template class elx_conv_wino_t<float, 5, 3, 8, ISA_GENERIC>;
// template class elx_conv_wino_t<float, 5, 3, 8, ISA_COSIM_AVX512>;
// template class elx_conv_wino_t<float, 6, 3, 8, ISA_GENERIC>;
// template class elx_conv_wino_t<float, 6, 3, 8, ISA_COSIM_AVX512>;
// template class elx_conv_wino_t<float, 7, 3, 8, ISA_GENERIC>;
// template class elx_conv_wino_t<float, 7, 3, 8, ISA_COSIM_AVX512>;
#endif
template class elx_conv_wino_t<float, 4, 3, 16, ISA_SKX_AVX512>;
template class elx_conv_wino_t<float, 5, 3, 16, ISA_SKX_AVX512>;
template class elx_conv_wino_t<float, 6, 3, 16, ISA_SKX_AVX512>;
template class elx_conv_wino_t<float, 7, 3, 16, ISA_SKX_AVX512>;
// template class elx_conv_wino_t<float, 5, 3, 8, ISA_SKX_AVX512>;
// template class elx_conv_wino_t<float, 6, 3, 8, ISA_SKX_AVX512>;
// template class elx_conv_wino_t<float, 7, 3, 8, ISA_SKX_AVX512>;

}  // namespace euler
#endif  // __ELX_CONV_WINO_HPP__
