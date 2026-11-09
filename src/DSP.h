#pragma once
#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

using cpx = std::complex<double>;

inline std::vector<cpx> naiveDFT(const std::vector<cpx>& x) {
    const size_t N = x.size();
    std::vector<cpx> X(N);

    for (size_t k = 0; k < N; ++k) {
        cpx sum = 0.0;
        for (size_t n = 0; n < N; ++n) {
            double angle = 2.0 * std::numbers::pi * k * n / N;
            sum += x[n] * cpx(std::cos(angle), -std::sin(angle));
        }
        X[k] = sum;
    }
    return X;
}

// Recursive radix-2 FFT. x.size() must be a power of 2.
inline std::vector<cpx> fft(const std::vector<cpx>& x) {
    const size_t N = x.size();
    if (N == 1) return x;

    std::vector<cpx> even(N / 2), odd(N / 2);
    for (size_t m = 0; m < N / 2; ++m) {
        even[m] = x[2 * m];
        odd[m]  = x[2 * m + 1];
    }

    std::vector<cpx> E = fft(even);
    std::vector<cpx> O = fft(odd);

    std::vector<cpx> X(N);
    for (size_t k = 0; k < N / 2; ++k) {
        double angle = -2.0 * std::numbers::pi * k / N;
        cpx t = cpx(std::cos(angle), std::sin(angle)) * O[k];
        X[k]         = E[k] + t;
        X[k + N / 2] = E[k] - t;
    }
    return X;
}

// Hann window of length N.
inline std::vector<double> makeHann(size_t N) {
    std::vector<double> w(N);
    for (size_t i = 0; i < N; ++i)
        w[i] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / N));
    return w;
}

// N windowed samples centered on `center`; samples outside the audio are zero.
inline std::vector<cpx> getFrame(const std::vector<double>& audio,
                                 long center, size_t N,
                                 const std::vector<double>& hann) {
    std::vector<cpx> buf(N);
    long start = center - (long)N / 2;
    for (size_t i = 0; i < N; ++i) {
        long idx = start + (long)i;
        double s = (idx >= 0 && idx < (long)audio.size()) ? audio[idx] : 0.0;
        buf[i] = s * hann[i];
    }
    return buf;
}