#pragma once

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

#include "dsp-stage-traits.h"

template <typename... Stages> struct StageTuple {
  using type = std::tuple<Stages...>;
};

template <typename... Stages>
using StageTuple_t = typename StageTuple<Stages...>::type;

template <std::size_t I, typename... Stages> struct StageTupleElement {
  using type = std::tuple_element_t<I, StageTuple_t<Stages...>>;
};

template <std::size_t I, typename... Stages>
using StageElement_t = typename StageTupleElement<I, Stages...>::type;

template <typename SampleT, typename... Stages> struct BufferTuple {
private:
  static constexpr size_t N{sizeof...(Stages)};

  template <size_t I> using stage_t = StageElement_t<I, Stages...>;

  template <size_t... Is> static auto make_buffers(std::index_sequence<Is...>) {
    return std::make_tuple(
        std::vector<SampleT>(stage_t<Is>::static_output_size())...);
  }

public:
  using type = decltype(make_buffers(std::make_index_sequence<N - 1>{}));

  static auto make() { return make_buffers(std::make_index_sequence<N - 1>{}); }
};

template <typename SampleT, typename... Stages>
using BufferTuple_t = typename BufferTuple<SampleT, Stages...>::type;

template <typename SampleT, std::uint32_t Prescale, uint32_t AccumulatorSize,
          bool AccumulatorNorm, typename... Stages>
class DSPController;

template <typename T> struct is_DSPController : std::false_type {};

template <typename SampleT, std::uint32_t Prescale, uint32_t AccumulatorSize,
          bool AccumulatorNorm, typename... Stages>
struct is_DSPController<DSPController<SampleT, Prescale, AccumulatorSize,
                                            AccumulatorNorm, Stages...>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_DSPController_v = is_DSPController<T>::value;

template <typename T> struct is_ImagDSPController : std::false_type {};

template <typename SampleT, uint32_t Prescale, uint32_t AccumulatorSize,
          bool AccumulatorNorm, typename... Stages>
struct is_ImagDSPController<DSPController<SampleT, Prescale, AccumulatorSize,
                                          AccumulatorNorm, Stages...>> {
private:
  static constexpr size_t N{sizeof...(Stages)};
  using last_stage_t = StageElement_t<N - 1, Stages...>;

public:
  static constexpr bool value = ImagStage<last_stage_t, SampleT>;
};

template <typename T>
inline constexpr bool is_ImagDSPController_v = is_ImagDSPController<T>::value;
