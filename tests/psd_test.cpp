#include <cmath>
//#include <print>
#include <iostream>
#include <vector>

#include "helpers.h"
#include "timer.h"

#include "../src/psd.h"
#include "../src/hann-window.h"

namespace test {

void psd_test() {

  constexpr size_t FFTSize{1000};
  constexpr size_t N{10000};
  constexpr size_t entries{N / FFTSize};
  constexpr size_t Overlap{100};
  constexpr double Fs{31.25e6 / 4};
  constexpr double f0{1e6};

  using SampleT = double;

  std::vector<SampleT> I(N), Q(N);
  std::vector<SampleT> psd(FFTSize);

  // Generate test tone
  for (size_t n = 0; n < N; ++n) {
    double phase{2.0 * std::numbers::pi * f0 * n / Fs};
    I[n] = std::cos(phase);
    Q[n] = std::sin(phase);
  }

  WelchPSD<SampleT, FFTSize, Overlap, Fs, N, HannWindow> welch;

  double time_passed{};
  Timer t;

  welch.process(I, Q, psd);

  time_passed = t.elapsed();

  std::cout << "Entries (Digitiser Polls):" << entries << '\n';
  std::cout << "Time taken to process: " << time_passed * 1e3 << " ms\n";
  std::cout << "Time taken per entry:  " << time_passed * 1e6 / entries << " us\n";

  double F_est_i{help::estimate_frequency<SampleT>(I, Q, Fs)};
  double F_est_f{help::psd_frequency<SampleT>(psd, Fs)};

  std::cout << "Initial Frequency:   " << F_est_i * 1e-6 << " MHz\n";
  std::cout << "Frequency After FFT: " << F_est_f * 1e-6 << " MHz\n";
}

} // namespace test

int main() { test::psd_test(); }
