/*
Copyright (C) 2003-2015 CompatibL

This file is part of TapeScript, an open source library and tape encoding
standard for adjoint algorithmic differentiation (AAD), available from

    http://github.com/compatibl/tapescript (source)
    http://tapescript.org (documentation)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef cl_tape_examples_impl_performance_tests_hpp
#define cl_tape_examples_impl_performance_tests_hpp

#include "impl/performance_utils.hpp"

namespace cl
{
    struct plus_task_factory
    {
#if defined NDEBUG
        enum { repeat = 1000000 };
#else
        enum { repeat = 100 };
#endif
        inline adjoint_task<tape_value> get_array_task(size_t n)
        {
            std::mt19937 gen;
            gen.seed(0);
            adjoint_task<tape_value> array_task;
            array_task.size_ = n;
            array_task.X_ = { gen_vector<tape_value>(n, gen), gen_vector<tape_value>(n, gen) };
            array_task.dx_ = { gen_vector<tape_value>(n, gen), gen_vector<tape_value>(n, gen) };
            array_task.w_ = { gen_vector<tape_value>(n, gen) };
            array_task.repeat_ = repeat / n + 1;
            array_task.func_ = [](std::vector<tape_object> const& v)
            {
                return std::vector<tape_object>{ v[0] + v[1] };
            };
            return array_task;
        }

        inline adjoint_task<double> get_double_task(size_t n)
        {
            std::mt19937 gen;
            gen.seed(0);
            adjoint_task<double> double_task;
            double_task.size_ = n;
            double_task.X_ = gen_vector<std::vector<double>>(2 * n, gen);
            double_task.dx_ = gen_vector<std::vector<double>>(2 * n, gen);
            double_task.w_ = gen_vector<std::vector<double>>(n, gen);
            double_task.repeat_ = repeat / n + 1;
            double_task.func_ = [n](std::vector<tape_double> const& v)
            {
                auto middle = v.begin() + n;
                std::vector<tape_double> result(n);
                std::transform(v.begin(), middle, middle, result.begin(), std::plus<tape_double>());
                return result;
            };
            return double_task;
        }
    };


    inline void plus_performance(std::ostream& out_str = fake_stream())
    {
        out_str << "Plus:\n\n";
        compare_performance(plus_task_factory(), 10, out_str);
        performance_plot("Plus pots", plus_task_factory());
    }

    inline void performance_tests()
    {
        std::ofstream of("performance_tests_output.txt");
        cl::tape_serializer<tape_value> serializer(of);
        serializer.precision(3);

        plus_performance(serializer);
    }
}

#endif // cl_tape_examples_impl_performance_tests_hpp