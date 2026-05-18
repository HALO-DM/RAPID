# RAPID - Real-time Acquisition Pipeline for IQ Data

RAPID is a high-performance, real-time data acquisition and digital signal processing (DSP) pipeline designed for IQ (In-phase/Quadrature) data collection and analysis. It integrates hardware digitizers with optimized DSP stages to enable real-time processing of sampled RF signals.

## Overview

RAPID combines several key components:

- **Hardware Digitization**: Interface with CAEN digitizer hardware for high-speed signal acquisition
- **Real-time DSP Pipeline**: Multi-stage processing including downmixing, filtering, and spectral analysis
- **MIDAS Integration**: Integration with the MIDAS data acquisition framework for flexible event handling and storage
- **Optimized Processing**: Lock-free queues and CPU affinity for deterministic, low-latency performance

### Key Features

- **Real-time Processing**: Multi-threaded pipeline architecture with CPU pinning for predictable performance
- **Configurable DSP Stages**: Flexible signal processing pipeline including:
  - Downmixing (frequency translation)
  - Decimating FIR filtering
  - Welch Power Spectral Density (PSD) estimation
- **Complex Signal Support**: Full I/Q (complex) signal handling throughout the pipeline
- **Hardware Integration**: Support for CAEN FELib and Dig2 libraries for digitizer control
- **FFTW Integration**: Fast Fourier Transform for spectral analysis using industry-standard FFTW3
- **Prescaling & Accumulation**: Configurable data reduction with optional accumulation and normalization

## Architecture

RAPID uses a modular, trait-based architecture:

```
Digitizer → Downmixer → FIR Filter → Welch PSD → MIDAS Events
   (Stage 0)   (Stage 1)   (Stage 2)    (Stage 3)
```

Each stage:
- Runs in its own thread with dedicated CPU core affinity
- Communicates via lock-free queues (SPSC - Single Producer Single Consumer)
- Supports data tapping for asynchronous snapshot capture
- Produces configurable output (raw waveforms or processed spectra)

### Pipeline Parameters

Default configuration (configurable in `src/main_midas.cpp`):

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Sampling Rate (Fs)** | 62.5 MHz | Hardware digitizer sample rate |
| **RF Frequency (f0)** | 6 MHz | Center frequency of interest |
| **Mixing Frequency (fmix)** | 5 MHz | Downmixing frequency |
| **FIR Taps** | 128 | Filter order |
| **Decimation Factor** | 8 | Post-filter downsampling |
| **Cutoff Frequency (Fc)** | 3 MHz | FIR filter cutoff |
| **Buffer Size (N)** | 80,000 samples | Digitizer acquisition window |
| **FFT Size** | 1000 | PSD FFT analysis window |
| **FFT Overlap** | 100 | Welch PSD overlap |

## Building RAPID

### Prerequisites

- **C++23 compliant compiler** (e.g., GCC 13+, Clang 16+)
- **CMake 3.20+**
- **CAEN Libraries**:
  - FELib v1.3.2
  - Dig2 v1.8.1
- **FFTW3** (Fast Fourier Transform library)
- **MIDAS** (optional, for frontend integration)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

#### Optional: Building without MIDAS

To build without MIDAS support (standalone DSP testing):

```bash
cmake -DNO_MIDAS=ON ..
cmake --build .
```

#### Specifying MIDAS Location

If MIDAS is installed in a non-standard location:

```bash
export MIDASSYS=/path/to/midas
cmake ..
```

## Usage

### As a MIDAS Frontend

RAPID integrates with MIDAS as a frontend:

```bash
mserver &                    # Start MIDAS server
fe -i midas &               # Start RAPID frontend
odbedit                     # Configure via ODB
```

Configuration is accessible via ODB at `/Equipment/DAQ DSP/Settings`:

```
/Equipment/DAQ DSP/Settings
  00/  (Digitizer raw waveforms)
    type = "waveform"
    sample_rate = 62500000
  01/  (Downmixed waveforms)
    type = "waveform"
    sample_rate = 62500000
  02/  (Filtered & decimated)
    type = "waveform"
    sample_rate = 7812500  (62.5 MHz / 8)
  03/  (Power Spectral Density)
    type = "psd"
    sample_rate = 7812500
```

### Configuring the DSP Pipeline

Edit stage parameters in `src/main_midas.cpp`:

```cpp
// Modify these constants before building:
static constexpr double Fs = 62.5e6;      // Sampling rate
static constexpr double f0 = 6e6;         // RF frequency
static constexpr double fmix = 5e6;       // Mix frequency
static constexpr std::size_t Decim = 8;   // Decimation factor
// ... and others

// Prescaling: 0 = disabled, N = save every N-th buffer
static constexpr uint32_t DIG_Prescale = 100;
static constexpr uint32_t PSD_Prescale = 1;
```

### Running Tests

Test programs are available in the `tests/` directory for individual components:

```bash
cd build
# Compile with test target instead (see CMakeLists.txt)
cmake -DTEST_BUILD=ON ..
./tests/mixer_test
./tests/filter_test
./tests/psd_test
```

## Output Format

RAPID generates MIDAS events with multiple banks per stage:

- **Ts_**: Timestamp (uint64_t, 1 entry)
- **Si_**: I-channel data (float, N entries)
- **Sq_**: Q-channel data (float, N entries) [if complex stage]

where `_` is the stage ID (0-3).

Example MIDAS bank layout for full pipeline:

```
Event contains 10+ banks:
  Ts0, Si0, Sq0  (Stage 0: raw I/Q from digitizer)
  Ts1, Si1, Sq1  (Stage 1: downmixed I/Q)
  Ts2, Si2, Sq2  (Stage 2: filtered, decimated I/Q)
  Ts3, Si3, Sq3  (Stage 3: power spectral density)
```

## Performance

RAPID is optimized for low-latency, real-time operation:

- **Lock-free Queues**: SPSC queues avoid kernel synchronization overhead
- **CPU Pinning**: Thread-to-core affinity reduces cache misses and context switching
- **Prefilled Buffer Pools**: Eliminates dynamic allocation in the hot path
- **Data Accumulation**: Optional windowing reduces MIDAS event rate while maintaining quality
- **Fast Math**: Compiled with `-O3 -ffast-math` for maximum throughput

Typical performance: ~62.5M samples/second through full 4-stage pipeline with real-time DSP.

## Project Structure

```
RAPID/
├── src/
│   ├── main_midas.cpp              # MIDAS frontend entry point
│   ├── daq-controller.h             # Main pipeline orchestrator
│   ├── caen-digitiser.h             # Hardware digitizer interface
│   ├── dsp-controller.h             # DSP stage management
│   ├── filter.h                     # FIR filtering implementation
│   ├── mixer.h                      # Frequency downmixing
│   ├── psd.h                        # Welch PSD estimator
│   ├── fft.h                        # FFT wrapper (FFTW)
│   ├── spsc-queue.h                 # Lock-free single-producer queue
│   ├── *-window.h                   # Window functions (Hamming, Hann, Kaiser)
│   └── *-traits.h                   # Type traits and compile-time configuration
├── tests/
│   ├── mixer_test.cpp               # Test downmixer component
│   ├── filter_test.cpp              # Test FIR filter
│   └── psd_test.cpp                 # Test PSD estimation
├── webserver/                       # Optional web interface components
├── housekeeping/                    # Utility scripts
├── CMakeLists.txt                   # Build configuration
└── LICENSE                          # GPLv3 license
```

## Dependencies

- **CAEN FELib**: For digitizer frontend library interface
- **CAEN Dig2**: For digitizer hardware communication
- **FFTW3**: For FFT computation
- **MIDAS**: Optional, for data acquisition framework
- **POSIX Threads**: For multi-threading and CPU affinity (Linux)

## Citation

If you use RAPID in your research, please cite it as follows:

```bibtex
@software{rapid2026,
  title={RAPID: Real-time Acquisition Pipeline for IQ Data},
  author={Millns, Matthew and Smith, Edward},
  year={2026},
  url={https://github.com/HALO-DM/RAPID},
  doi={https://doi.org/10.5281/zenodo.20272944}
}
```

## License

RAPID is licensed under the **GNU General Public License v3.0** (GPLv3). See the `LICENSE` file for full terms.

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes with clear messages
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a Pull Request with a description of changes

## Support & Acknowledgments

**Authors**: Matthew Millns, Edward Smith (University of Manchester)

**Organization**: HALO-DM (HALO Dark Matter Collaboration)

For questions or issues, please open a GitHub issue in this repository.

---

**Last Updated**: May 2026  
**Repository**: https://github.com/HALO-DM/RAPID
