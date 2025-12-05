/*
  Copyright (c) 2016 Robert W. Rose
  Copyright (c) 2018 Paul Maevskikh
  Copyright (c) 2024 NORCE

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  Note: This file is based on kerasify/keras_model.cc
*/

#include <opm/ml/ml_model.hpp>

namespace Opm
{

namespace ML
{
    void NNTimer::start()
    {
        start_ = std::chrono::high_resolution_clock::now();
    }

    float NNTimer::stop()
    {
        using namespace std::chrono;
        return duration<float, std::milli>(high_resolution_clock::now() - start_).count();
    }

    template class NNModel<float>;
    template class NNModel<Opm::DenseAd::Evaluation<float, 3>>;
    template class NNModel<Opm::DenseAd::Evaluation<float, 2>>;
    template class NNModel<Opm::DenseAd::Evaluation<float, 1>>;

    template class NNModel<double>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 6>>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 5>>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 4>>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 3>>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 2>>;
    template class NNModel<Opm::DenseAd::Evaluation<double, 1>>;

} // namespace ML

} // namespace Opm
