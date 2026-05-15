#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <numbers>
// #include <print>
#include <span>
#include <type_traits>

template <typename SampleT, double Fs, double f_mix, size_t InputSize>
class DownMixer {
public:
  constexpr DownMixer() noexcept = default;

  void process(std::span<const SampleT> I_in, std::span<const SampleT> Q_in,
               std::span<SampleT> I_out, std::span<SampleT> Q_out) noexcept {

    // std::println("Mixer: processing");

    //std::cout << "Mixer: I: " << I_out.size() << ", Q: " << Q_in.size() << '\n';
    //assert(I_out.size() == Q_out.size());

    const size_t N{I_in.size()};
    SampleT oscI{m_oscI};
    SampleT oscQ{m_oscQ};

    for (size_t n{0}; n < N; ++n) {

      const SampleT I{I_in[n]};
      const SampleT Q{Q_in[n]};

      // Complex multiply (auto-vectorizable part)
      I_out[n] = I * oscI - Q * oscQ;
      Q_out[n] = I * oscQ + Q * oscI;

      // Oscillator update (recurrence)
      const SampleT oldI{oscI};

      // FMA-friendly structure
      oscI = oscI * m_cos_delta - oscQ * m_sin_delta;
      oscQ = oldI * m_sin_delta + oscQ * m_cos_delta;
    }

    m_oscI = oscI;
    m_oscQ = oscQ;
  }

  void reset_phase() noexcept {
    m_oscI = static_cast<SampleT>(1);
    m_oscQ = static_cast<SampleT>(0);
  }

  static consteval size_t static_output_size() noexcept { return InputSize; }

  static consteval size_t static_input_size() noexcept { return InputSize; }

private:
  static constexpr SampleT m_cos_delta{
      static_cast<SampleT>(std::cos(-2.0 * std::numbers::pi * f_mix / Fs))};

  static constexpr SampleT m_sin_delta{
      static_cast<SampleT>(std::sin(-2.0 * std::numbers::pi * f_mix / Fs))};

  SampleT m_oscI{static_cast<SampleT>(1)};
  SampleT m_oscQ{static_cast<SampleT>(0)};
};
