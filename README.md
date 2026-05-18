# RAPID - Real-time Acquisition Pipeline for IQ Data

RAPID is a high-performance, real-time data acquisition and digital signal processing (DSP) pipeline designed for IQ (In-phase/Quadrature) data collection and analysis. It integrates hardware digitisers with optimised DSP stages to enable real-time processing of sampled signals.

## Overview

RAPID combines several key components:

- **Hardware Digitisation**: Interface with CAEN digitiser hardware for high-speed signal acquisition
- **Real-time DSP Pipeline**: Multi-stage processing including downmixing, filtering, and spectral analysis
- **MIDAS Integration**: Integration with the MIDAS data acquisition framework for flexible event handling and storage
- **Optimized Processing**: Lock-free queues and CPU affinity for low-latency performance

### Key Features

- **Real-time Processing**: Multi-threaded pipeline architecture with CPU pinning for maximum performance
- **Configurable DSP Stages**: Flexible signal processing pipeline including:
  - Downmixing (frequency translation)
  - Decimating FIR filtering
  - Welch Power Spectral Density (PSD) estimation
- **Complex Signal Support**: Full I/Q (complex) signal handling throughout the pipeline
- **Hardware Integration**: Support for CAEN FELib and Dig2 libraries for digitiser control
- **FFTW Integration**: Fast Fourier Transform for spectral analysis using industry-standard FFTW3
- **Prescaling & Accumulation**: Configurable data reduction with optional accumulation and normalisation

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

## Building RAPID

### Prerequisites

- **C++23 compliant compiler** (e.g., GCC 13+, Clang 16+)
- **CMake 3.20+**
- **CAEN Libraries**:
  - FELib v1.3.2
  - Dig2 v1.8.1
- **FFTW3** (Fast Fourier Transform library)
- **MIDAS** (optional, for frontend integration)

### Setup

- First, need to set all relavent environment variables. See `bin/setup.sh` for an example script. Run via:

```bash
chmod +x bin/setup.sh
source bin/setup.sh
```

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

### Configuring the DSP Pipeline

- Edit stage parameters in `src/main_midas.cpp`:

- Instantiate individual DSP stages and then wrap in DSP controllers. Use a producer frontend (e.g. CAEN digitiser) and DSP controllers to build a DAQ controller.
  
- Instantiate a pointer to the DAQ controller in `frontend_init()` and configure the ODB.
  
- Each stage in the DAQ controller has an entry in the MIDAS ODB (/Equipment/DAQ DSP/Settings). These settings provide webserver information, for example:

```
meta["01"]["type"] = "waveform";
    meta["01"]["title"] = "Downmixed Waveforms";
    meta["01"]["x_unit"] = "Sample";
    meta["01"]["y_unit"] = "V";
    meta["01"]["sample_rate"] = axion::Fs;
```

  - This adds stage `01` to the ODB.
  - `type` describes the type of data being showed. This can we either `waveform` of `fft`/`psd` (they are the same).
  - `x_unit` and `y_unit` describe the axis units that are displayed on the webserver.
  - `sample_rate` is the sampling rate of that signal.

### Running RAPID

- Once built, start RAPID using the `bin/start_midas.sh` script via:

```bash
chmod +x bin/start_midas.sh
source bin/start_midas.sh
```

- This runs the MIDAS webserver on port 8080 and the plotting webserver on port 8090.
  
- RAPID can be stopped using the `bin/start_midas.sh` script via:

```bash
chmod +x bin/start_midas.sh
source bin/start_midas.sh
```

### Running Tests

- Test programs are available in the `tests/` directory for individual components. 

- To test DSP stages (.cpp), edit the CMake accordingly.
  
- To test efficiency (.py), the script can run with no CMake altering.

## Output Format

RAPID generates MIDAS events with multiple banks per stage:

- **Ts_**: Timestamp (uint64_t, 1 entry)
- **Si_**: I-channel data (float, N entries)
- **Sq_**: Q-channel data (float, N entries) [if complex stage]

where `_` is the stage ID (0-3).

Example MIDAS bank layout for full pipeline:

```
Event contains 10+ banks:
  Ts00, Si00, Sq00  (Stage 0: raw I/Q from digitizer)
  Ts01, Si01, Sq01  (Stage 1: downmixed I/Q)
  Ts02, Si02, Sq02  (Stage 2: filtered, decimated I/Q)
  Ts03, Si03, Sq03  (Stage 3: power spectral density)
```

## Performance

RAPID is optimised for low-latency, real-time operation:

- **Lock-free Queues**: SPSC queues avoid kernel synchronization overhead
- **CPU Pinning**: Thread-to-core affinity reduces cache misses and context switching
- **Prefilled Buffer Pools**: Eliminates dynamic allocation in the hot path
- **Data Accumulation**: Optional windowing reduces MIDAS event rate while maintaining quality
- **Fast Math**: Compiled with `-O3 -ffast-math` for maximum throughput

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
├── webserver/                       # display web interface functionality
├── housekeeping/                    # stream additional slow control data from Postgress database
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
