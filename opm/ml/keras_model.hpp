#ifndef KERAS_MODEL_H_
#define KERAS_MODEL_H_

#include <algorithm>
#include <chrono>
#include <math.h>
#include <numeric>
#include <string>
#include <vector>

#include <opm/material/densead/Math.hpp>
#include <sstream>

// typedef double Scalar;

//typedef Opm::DenseAd::Evaluation<double, 3> Evaluation;
//typedef float Evaluation;
//typedef double Evaluation;
namespace Opm {

#define KASSERT(x, ...)                                                        \
    if (!(x)) {                                                                \
        printf("KASSERT: %s(%d): ", __FILE__, __LINE__);                       \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        return false;                                                          \
    }

#define KASSERT_EQ(x, y, eps)                                                  \
    if (fabs(x.value() - y.value()) > eps) {                                                   \
        printf("KASSERT: Expected %f, got %f\n", y.value(), x.value());                        \
        return false;                                                          \
    }

#ifdef DEBUG
#define KDEBUG(x, ...)                                                         \
    if (!(x)) {                                                                \
        printf("%s(%d): ", __FILE__, __LINE__);                                \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        exit(-1);                                                              \
    }
#else
#define KDEBUG(x, ...) ;
#endif

template<class Foo>
class Tensor {
  public:
    Tensor() {}

    Tensor(int i) { Resize(i); }

    Tensor(int i, int j) { Resize(i, j); }

    Tensor(int i, int j, int k) { Resize(i, j, k); }

    Tensor(int i, int j, int k, int l) { Resize(i, j, k, l); }

    void Resize(int i) {
        dims_ = {i};
        data_.resize(i);
    }

    void Resize(int i, int j) {
        dims_ = {i, j};
        data_.resize(i * j);
    }

    void Resize(int i, int j, int k) {
        dims_ = {i, j, k};
        data_.resize(i * j * k);
    }

    void Resize(int i, int j, int k, int l) {
        dims_ = {i, j, k, l};
        data_.resize(i * j * k * l);
    }

    inline void Flatten() {
        KDEBUG(dims_.size() > 0, "Invalid tensor");

        int elements = dims_[0];
        for (unsigned int i = 1; i < dims_.size(); i++) {
            elements *= dims_[i];
        }
        dims_ = {elements};
    }

    inline Foo& operator()(int i) {
        KDEBUG(dims_.size() == 1, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);

        return data_[i];
    }

    inline Foo& operator()(int i, int j) {
        KDEBUG(dims_.size() == 2, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);

        return data_[dims_[1] * i + j];
    }

    inline Foo operator()(int i, int j) const {
        KDEBUG(dims_.size() == 2, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);

        return data_[dims_[1] * i + j];
    }

    inline Foo& operator()(int i, int j, int k) {
        KDEBUG(dims_.size() == 3, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);

        return data_[dims_[2] * (dims_[1] * i + j) + k];
    }

    inline Foo& operator()(int i, int j, int k, int l) {
        KDEBUG(dims_.size() == 4, "Invalid indexing for tensor");
        KDEBUG(i < dims_[0] && i >= 0, "Invalid i: %d (max %d)", i, dims_[0]);
        KDEBUG(j < dims_[1] && j >= 0, "Invalid j: %d (max %d)", j, dims_[1]);
        KDEBUG(k < dims_[2] && k >= 0, "Invalid k: %d (max %d)", k, dims_[2]);
        KDEBUG(l < dims_[3] && l >= 0, "Invalid l: %d (max %d)", l, dims_[3]);

        return data_[dims_[3] * (dims_[2] * (dims_[1] * i + j) + k) + l];
    }

    inline void Fill(Foo value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    Tensor Unpack(int row) const {
        KASSERT(dims_.size() >= 2, "Invalid tensor");
        std::vector<int> pack_dims =
            std::vector<int>(dims_.begin() + 1, dims_.end());
        int pack_size = std::accumulate(pack_dims.begin(), pack_dims.end(), 0);

        typename std::vector<Foo>::const_iterator first =
            data_.begin() + (row * pack_size);
        typename std::vector<Foo>::const_iterator last =
            data_.begin() + (row + 1) * pack_size;

        Tensor x = Tensor();
        x.dims_ = pack_dims;
        x.data_ = std::vector<Foo>(first, last);

        return x;
    }

    Tensor Select(int row) const {
        Tensor x = Unpack(row);
        x.dims_.insert(x.dims_.begin(), 1);

        return x;
    }

    Tensor operator+(const Tensor& other) {
        KASSERT(dims_ == other.dims_,
                "Cannot add tensors with different dimensions");

        Tensor result;
        result.dims_ = dims_;
        result.data_.reserve(data_.size());

        std::transform(data_.begin(), data_.end(), other.data_.begin(),
                       std::back_inserter(result.data_),
                       [](Foo x, Foo y) { return x + y; });

        return result;
    }

    Tensor Multiply(const Tensor& other) {
        KASSERT(dims_ == other.dims_,
                "Cannot multiply elements with different dimensions");

        Tensor result;
        result.dims_ = dims_;
        result.data_.reserve(data_.size());

        std::transform(data_.begin(), data_.end(), other.data_.begin(),
                       std::back_inserter(result.data_),
                       [](Foo x, Foo y) { return x * y; });

        return result;
    }

    Tensor Dot(const Tensor& other) {
        KDEBUG(dims_.size() == 2, "Invalid tensor dimensions");
        KDEBUG(other.dims_.size() == 2, "Invalid tensor dimensions");
        KASSERT(dims_[1] == other.dims_[0],
                "Cannot multiply with different inner dimensions");

        Tensor tmp(dims_[0], other.dims_[1]);

        for (int i = 0; i < dims_[0]; i++) {
            for (int j = 0; j < other.dims_[1]; j++) {
                for (int k = 0; k < dims_[1]; k++) {
                    tmp(i, j) += (*this)(i, k) * other(k, j);
                }
            }
        }

        return tmp;
    }

    // void Print() {
    //     if (dims_.size() == 1) {
    //         printf("[ ");
    //         for (int i = 0; i < dims_[0]; i++) {
    //             printf("%f ", (*this)(i));
    //         }
    //         printf("]\n");
    //     } else if (dims_.size() == 2) {
    //         printf("[\n");
    //         for (int i = 0; i < dims_[0]; i++) {
    //             printf(" [ ");
    //             for (int j = 0; j < dims_[1]; j++) {
    //                 printf("%f ", (*this)(i, j));
    //             }
    //             printf("]\n");
    //         }
    //         printf("]\n");
    //     } else if (dims_.size() == 3) {
    //         printf("[\n");
    //         for (int i = 0; i < dims_[0]; i++) {
    //             printf(" [\n");
    //             for (int j = 0; j < dims_[1]; j++) {
    //                 printf("  [ ");
    //                 for (int k = 0; k < dims_[2]; k++) {
    //                     printf("%f ", (*this)(i, j, k));
    //                 }
    //                 printf("  ]\n");
    //             }
    //             printf(" ]\n");
    //         }
    //         printf("]\n");
    //     } else if (dims_.size() == 4) {
    //         printf("[\n");
    //         for (int i = 0; i < dims_[0]; i++) {
    //             printf(" [\n");
    //             for (int j = 0; j < dims_[1]; j++) {
    //                 printf("  [\n");
    //                 for (int k = 0; k < dims_[2]; k++) {
    //                     printf("   [");
    //                     for (int l = 0; l < dims_[3]; l++) {
    //                         printf("%f ", (*this)(i, j, k, l));
    //                     }
    //                     printf("]\n");
    //                 }
    //                 printf("  ]\n");
    //             }
    //             printf(" ]\n");
    //         }
    //         printf("]\n");
    //     }
    // }

    // void PrintShape() {
    //     printf("(");
    //     for (unsigned int i = 0; i < dims_.size(); i++) {
    //         printf("%d ", dims_[i]);
    //     }
    //     printf(")\n");
    // }

    std::vector<int> dims_;
    std::vector<Foo> data_;
};

template<class Evaluation>
class KerasLayer {
  public:
    KerasLayer() {}

    virtual ~KerasLayer() {}

    virtual bool LoadLayer(std::ifstream* file) = 0;

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out) = 0;
};

template<class Evaluation>
class KerasLayerActivation : public KerasLayer<Evaluation> {
  public:
    enum ActivationType {
        kLinear = 1,
        kRelu = 2,
        kSoftPlus = 3,
        kSigmoid = 4,
        kTanh = 5,
        kHardSigmoid = 6
    };

    KerasLayerActivation() : activation_type_(ActivationType::kLinear) {}

    virtual ~KerasLayerActivation() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    ActivationType activation_type_;
};

template<class Evaluation>
class KerasLayerScaling : public KerasLayer<Evaluation> {
  public:
    KerasLayerScaling(): data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

    virtual ~KerasLayerScaling() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;
    float data_min;
    float data_max;
    float feat_inf;
    float feat_sup;

    // float feat;
    // float feature_range;

    // KerasLayerActivation<Evaluation> activation_;
};

template<class Evaluation>
class KerasLayerUnScaling : public KerasLayer<Evaluation> {
  public:
    KerasLayerUnScaling(): data_min(1.0f), data_max(1.0f), feat_inf(1.0f), feat_sup(1.0f) {}

    virtual ~KerasLayerUnScaling() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;
    float data_min;
    float data_max;
    float feat_inf;
    float feat_sup;

    // KerasLayerActivation<Evaluation> activation_;
};


template<class Evaluation>
class KerasLayerDense : public KerasLayer<Evaluation> {
  public:
    KerasLayerDense() {}

    virtual ~KerasLayerDense() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;

    KerasLayerActivation<Evaluation> activation_;
};

template<class Evaluation>
class KerasLayerConvolution2d: public KerasLayer<Evaluation> {
  public:
    KerasLayerConvolution2d() {}

    virtual ~KerasLayerConvolution2d() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    Tensor<float> weights_;
    Tensor<float> biases_;

    KerasLayerActivation<Evaluation> activation_;
};

template<class Evaluation>
class KerasLayerFlatten : public KerasLayer<Evaluation> {
  public:
    KerasLayerFlatten() {}

    virtual ~KerasLayerFlatten() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
};

template<class Evaluation>
class KerasLayerElu : public KerasLayer<Evaluation> {
  public:
    KerasLayerElu() : alpha_(1.0f) {}

    virtual ~KerasLayerElu() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    float alpha_;
};

template<class Evaluation>
class KerasLayerMaxPooling2d : public KerasLayer<Evaluation> {
  public:
    KerasLayerMaxPooling2d() : pool_size_j_(0), pool_size_k_(0) {}

    virtual ~KerasLayerMaxPooling2d() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    unsigned int pool_size_j_;
    unsigned int pool_size_k_;
};

// template<class Evaluation>
// class KerasLayerLSTM : public KerasLayer<Evaluation> {
//   public:
//     KerasLayerLSTM() : return_sequences_(false) {}
//
//     virtual ~KerasLayerLSTM() {}
//
//     virtual bool LoadLayer(std::ifstream* file);
//
//     virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);
//
//   private:
//     bool Step(Tensor<Evaluation>* x, Tensor<Evaluation>* out, Tensor<Evaluation>* ht_1, Tensor<Evaluation>* ct_1);
//
//     Tensor<float> Wi_;
//     Tensor<float> Ui_;
//     Tensor<float> bi_;
//     Tensor<float> Wf_;
//     Tensor<float> Uf_;
//     Tensor<float> bf_;
//     Tensor<float> Wc_;
//     Tensor<float> Uc_;
//     Tensor<float> bc_;
//     Tensor<float> Wo_;
//     Tensor<float> Uo_;
//     Tensor<float> bo_;
//
//     KerasLayerActivation<Evaluation> innerActivation_;
//     KerasLayerActivation<Evaluation> activation_;
//     bool return_sequences_;
// };


template<class Evaluation>
class KerasLayerEmbedding : public KerasLayer<Evaluation> {
  public:
    KerasLayerEmbedding() {}

    virtual ~KerasLayerEmbedding() {}

    virtual bool LoadLayer(std::ifstream* file);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    Tensor<float> weights_;
};

template<class Evaluation>
class KerasModel {
  public:
    enum LayerType {
        // kConvolution2d = 2,
        kFlatten = 1,
        // kElu = 4,
        // kMaxPooling2D = 6,
        // kLSTM = 7,
        // kEmbedding = 8,
        kScaling = 2,
        kUnScaling = 3,
        kDense = 4,
        kActivation = 5
    };

    KerasModel() {}

    virtual ~KerasModel() {
        for (unsigned int i = 0; i < layers_.size(); i++) {
            delete layers_[i];
        }
    }

    virtual bool LoadModel(const std::string& filename);

    virtual bool Apply(Tensor<Evaluation>* in, Tensor<Evaluation>* out);

  private:
    std::vector<KerasLayer<Evaluation>*> layers_;
};

class KerasTimer {
  public:
    KerasTimer() {}

    void Start() { start_ = std::chrono::high_resolution_clock::now(); }

    float Stop() {
        std::chrono::time_point<std::chrono::high_resolution_clock> now =
            std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = now - start_;

        return diff.count();
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

} // namespace Opm

#endif // KERAS_MODEL_H_
