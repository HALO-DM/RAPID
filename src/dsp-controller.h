#pragma once

// #include <print>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "dsp-stage-traits.h"
#include "dsp-controller-traits.h"

template <typename SampleT, uint32_t Prescale, uint32_t AccumulatorSize,
          bool AccumulatorNorm, typename... Stages>
class DSPController {
public:
  template <typename... Args>
  DSPController(Args &&...args)
      : m_stages(std::forward<Args>(args)...),
        m_I_bufs(BufferTuple<SampleT, Stages...>::make()),
        m_Q_bufs(BufferTuple<SampleT, Stages...>::make()) {}

  // Main process function for a pipeline ending in a real last stage
  void process(std::span<const SampleT> I_in, std::span<const SampleT> Q_in,
               std::span<SampleT> out) {
    process_stage<0>(I_in, Q_in, out, out);
    // std::cout << "dsp\n";
  }

  // Overload for complex last stage
  void process(std::span<const SampleT> I_in, std::span<const SampleT> Q_in,
               std::span<SampleT> I_out, std::span<SampleT> Q_out) {
    process_stage<0>(I_in, Q_in, I_out, Q_out);
  }

  static consteval size_t static_output_size() noexcept {
    return StageElement_t<N - 1, Stages...>::static_output_size();
  }

  static consteval size_t static_input_size() noexcept {
    return StageElement_t<0, Stages...>::static_input_size();
  }

  static consteval uint32_t static_accumulator_size() noexcept {
    return AccumulatorSize;
  }

  static consteval bool static_accumulator_norm() noexcept {
    return AccumulatorNorm;
  }

  static constexpr uint32_t get_static_prescale() { return Prescale; }

private:
  static_assert((DSPStage<Stages, SampleT> && ...),
                "All stages must satisfy ImagStage or RealStage");
  static_assert(
      ValidPipeline<SampleT, Stages...>,
      "Invalid DSP pipeline: a RealStage can only appear as the final stage");

  static_assert(ValidSizes<SampleT, Stages...>,
                "Invalid DSP pipeline: the output size of stage n must equal "
                "the input size of stage n+1");

  static constexpr size_t N{sizeof...(Stages)};

  BufferTuple_t<SampleT, Stages...> m_I_bufs;
  BufferTuple_t<SampleT, Stages...> m_Q_bufs;

  StageTuple_t<Stages...> m_stages;

  // Recursive pipeline processor
  template <size_t I>
  void process_stage(std::span<const SampleT> I_in,
                     std::span<const SampleT> Q_in, std::span<SampleT> I_out,
                     std::span<SampleT> Q_out) {

    // std::cout << "dsp stage\n";
    //  std::println("DSPController: processing");
    using stage_t = StageElement_t<I, Stages...>;
    auto &stage_v = std::get<I>(m_stages);

    if constexpr (I == N - 1) {
      // Last stage: detect if it has 4-arg process
      if constexpr (ImagStage<stage_t, SampleT>) {
        stage_v.process(I_in, Q_in, I_out, Q_out);
      } else {
        stage_v.process(I_in, Q_in, I_out);
      }
    } else {
      // Intermediate stage
      auto &buf_I = std::get<I>(m_I_bufs);
      auto &buf_Q = std::get<I>(m_Q_bufs);

      stage_v.process(I_in, Q_in, buf_I, buf_Q);
      process_stage<I + 1>(buf_I, buf_Q, I_out, Q_out);
    }
  }
};
