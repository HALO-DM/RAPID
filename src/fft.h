#pragma once

#include <cstddef>
#include <fftw3.h>
#include <memory>
//#include <print>
#include <span>
#include <stdexcept>

template <typename SampleT, size_t N> class FFTWrapper {
public:
  FFTWrapper()
      : in_array(fftw_alloc_complex(N), FFTWDeleter{}),
        out_array(fftw_alloc_complex(N), FFTWDeleter{}) {
    plan_forward =
        fftw_plan_dft_1d(static_cast<int>(N), in_array.get(), out_array.get(),
                         FFTW_FORWARD, FFTW_MEASURE);

    if (!plan_forward)
      throw std::runtime_error("FFTW plan creation failed");
  }

  ~FFTWrapper() {
    if (plan_forward)
      fftw_destroy_plan(plan_forward);
  }

  FFTWrapper(FFTWrapper &&other) noexcept
      : in_array(std::move(other.in_array)),
        out_array(std::move(other.out_array)),
        plan_forward(other.plan_forward) {
    other.plan_forward = nullptr;
  }

  FFTWrapper &operator=(FFTWrapper &&other) noexcept {
    if (this != &other) {
      if (plan_forward)
        fftw_destroy_plan(plan_forward);

      in_array = std::move(other.in_array);
      out_array = std::move(other.out_array);
      plan_forward = other.plan_forward;

      other.plan_forward = nullptr;
    }
    return *this;
  }

  FFTWrapper(const FFTWrapper &) = delete;
  FFTWrapper &operator=(const FFTWrapper &) = delete;

  // Forward FFT: pack I/Q into complex array, execute FFT, unpack to outputs
  void process(std::span<const SampleT> I, std::span<const SampleT> Q,
               std::span<SampleT> I_out, std::span<SampleT> Q_out) {

    //std::println("FFT: processing");

    if (I.size() < N || Q.size() < N || I_out.size() < N || Q_out.size() < N)
      throw std::runtime_error("FFTWrapper: input/output spans too small");

    // Pack I + iQ into FFTW complex array
    for (size_t i = 0; i < N; ++i) {
      in_array[i][0] = I[i]; // real
      in_array[i][1] = Q[i]; // imag
    }

    fftw_execute(plan_forward);

    // Unpack FFT output to I_out / Q_out
    for (size_t i = 0; i < N; ++i) {
      I_out[i] = out_array[i][0];
      Q_out[i] = out_array[i][1];
    }
  }

  static consteval size_t static_output_size() noexcept { return N; }

  static consteval size_t static_input_size() noexcept { return N; }

private:
  struct FFTWDeleter {
    void operator()(fftw_complex *ptr) const noexcept { fftw_free(ptr); }
  };

  std::unique_ptr<fftw_complex[], FFTWDeleter> in_array;
  std::unique_ptr<fftw_complex[], FFTWDeleter> out_array;
  fftw_plan plan_forward;
};
