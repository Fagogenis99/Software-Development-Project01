#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <cstdint>
#include <cstring>

struct Matrix {
    int n = 0;     // number of vectors
    int d = 0;     // dimension
    std::vector<float> a; // flattened: size = n*d

    float* row(int i)             { return a.data() + static_cast<size_t>(i) * d; }
    const float* row(int i) const { return a.data() + static_cast<size_t>(i) * d; }
};

// ---- utilities -------------------------------------------------------------

inline uint32_t read_u32_be(std::ifstream& in) {
    unsigned char b[4];
    if (!in.read(reinterpret_cast<char*>(b), 4)) throw std::runtime_error("Unexpected EOF");
    return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
}

inline void require(bool cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}

// ---- MNIST (.idx3-ubyte) → Big-Endian header + pixels ---------------------

// If normalize=true, pixels become [0,1]; else [0,255] as floats.
inline Matrix load_mnist_images(const std::string& path, bool normalize=false) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open MNIST file: " + path);

    const uint32_t magic = read_u32_be(in);
    const uint32_t n     = read_u32_be(in);
    const uint32_t rows  = read_u32_be(in);
    const uint32_t cols  = read_u32_be(in);

    require(magic == 0x00000803u, "MNIST: wrong magic (expected 2051)");
    require(rows > 0 && cols > 0, "MNIST: invalid image size");

    const uint64_t d64 = uint64_t(rows) * cols;
    require(d64 <= INT32_MAX, "MNIST: dimension too large");
    const int d = static_cast<int>(d64);

    Matrix M;
    M.n = static_cast<int>(n);
    M.d = d;
    M.a.resize(static_cast<size_t>(M.n) * M.d);

    std::vector<unsigned char> buf(d);
    for (int i = 0; i < M.n; ++i) {
        if (!in.read(reinterpret_cast<char*>(buf.data()), d))
            throw std::runtime_error("MNIST: unexpected EOF while reading pixels");
        if (normalize) {
            for (int j = 0; j < d; ++j) M.a[size_t(i)*d + j] = buf[j] / 255.0f;
        } else {
            for (int j = 0; j < d; ++j) M.a[size_t(i)*d + j] = static_cast<float>(buf[j]);
        }
    }
    return M;
}

// ---- SIFT (.fvecs) → Little-Endian blocks [int dim][dim floats] -----------

// Reads until EOF. Verifies all vectors share the same dimension.
inline Matrix load_fvecs(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open fvecs file: " + path);

    // Determine file size to pre-allocate roughly
    in.seekg(0, std::ios::end);
    std::streampos end = in.tellg();
    in.seekg(0, std::ios::beg);

    Matrix M;
    int common_d = -1;
    std::vector<float> data;
    data.reserve(static_cast<size_t>( (end / 4) )); // rough upper bound

    while (true) {
        int32_t d = 0;
        in.read(reinterpret_cast<char*>(&d), 4);
        if (!in) {
            // normal EOF (no partial record)
            break;
        }
        require(d > 0 && d <= 65536, "fvecs: invalid dimension");
        if (common_d == -1) common_d = d;
        require(d == common_d, "fvecs: mixed dimensions are not supported");

        const size_t off = data.size();
        data.resize(off + static_cast<size_t>(d));

        // read d floats
        in.read(reinterpret_cast<char*>(data.data() + off), sizeof(float) * static_cast<size_t>(d));
        require(static_cast<bool>(in), "fvecs: unexpected EOF inside a vector");
        ++M.n;
    }

    if (M.n == 0) throw std::runtime_error("fvecs: file contains zero vectors");
    M.d = common_d;
    M.a = std::move(data);
    require(static_cast<int>(M.a.size()) == M.n * M.d, "fvecs: size mismatch");
    return M;
}
