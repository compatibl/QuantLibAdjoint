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

#ifndef cl_tape_examples_impl_regression_examples_hpp
#define cl_tape_examples_impl_regression_examples_hpp

#define CL_BASE_SERIALIZER_OPEN
#include <cl/tape/tape.hpp>
#include "linear_regression.hpp"
#include "regression_examples_impl.hpp"

namespace cl
{
    struct initial_data
    {
        // Control production of tape output to serializer.
        bool flag_serializer = false;

        // Input linear regression parameters.
        // n: number of points {x_i, y_i}.
        // x_i are x_0 = 0, x_1 = 1, ..., x_n = n.
        // y_i are y_i = a + b * x_i + exp(-1 * c * x_i).
#if defined NDEBUG
        size_t n = 5000;
#else
        size_t n = 2000;
#endif
        double a = 1.0;
        double b = 2.0;
        double c = 3.0;

        // Tolerance for analytical vs. adjoint derivative check.
        double eps = 1e-10;
    };

    // Example of linear regression differentiation with respect to parameters of input distribution using optimized tape.
    inline void linear_regression_with_params_optimized_example(const initial_data& data, std::ostream& out_str = std::cout)
    {
        // Control production of tape output to serializer.
        bool flag_serializer = data.flag_serializer;

        // Input linear regression parameters.
        // n: number of points {x_i, y_i}.
        // x_i are x_0 = 0, x_1 = 1, ..., x_n = n.
        // y_i are y_i = a + b * x_i + exp(-1 * c * x_i).
        size_t n = data.n;
        double a = data.a;
        double b = data.b;
        double c = data.c;

        // Tolerance for analytical vs. adjoint derivative check.
        double eps = data.eps;

        out_str << "Linear regression with parameters (optimized tape):\n" << std::endl;

        // Input values initialization.
        out_str << "Input vector size: n = " << n << std::endl;
        LinearRegression lin_regr(n, a, b, c);
        lin_regr.Calculate();
        cl::tape_value a_ref = a;
        cl::tape_value b_ref = b;
        cl::tape_value c_ref = c;
        std::vector<cl::tape_object> X = { a_ref, b_ref, c_ref };
        if (flag_serializer)
            out_str << "Input vector: " << X << "\n";

        std::clock_t start_time = std::clock();
        // Declare the X vector as independent and start a tape recording.
        cl::Independent(X);

        // Output calculations.
        cl::tape_object& par_a = X[0];
        cl::tape_object& par_b = X[1];
        cl::tape_object& par_c = X[2];
        // Obtain x_i values.
        cl::tape_object x = lin_regr.GetInputX();
        // Calculate corresponding y_i values.
        cl::tape_object y = par_a + x * par_b + exp(-1 * par_c * x);
        // Start linear regression calculation: calculate mean values.
        cl::tape_object x_mean = 1.0 / n * cl::tapescript::sum_vec(x);
        cl::tape_object y_mean = 1.0 / n * cl::tapescript::sum_vec(y);
        // Variance times n: n * Var[x]
        cl::tape_object var_x_n = cl::tapescript::sum_vec((x - x_mean) * (x - x_mean));
        // Covariance times n: n * Cov[x, y]
        cl::tape_object cov_xy_n = cl::tapescript::sum_vec((x - x_mean) * (y - y_mean));
        // Linear regression coefficients.
        cl::tape_object beta = cov_xy_n / var_x_n;
        cl::tape_object alpha = y_mean - beta * x_mean;
        // Estimation for y_i.
        cl::tape_object y_estimate = alpha + beta * x;
        // Output vector.
        std::vector<cl::tape_object> Y = { alpha, beta, y_estimate };
        //out_str << "Output vector: " << Y << "\n\n";

        if (flag_serializer)
            out_str << "Ininial Forward(0) sweep...\n\n";
        // Declare a tape function and stop the tape recording.
        cl::tape_function<cl::tape_value> f(X, Y);
        std::clock_t stop_time = std::clock();
        out_str << "Tape memory (bytes): " << f.Memory() << std::endl;
        out_str << "Tape creation took (ms): " << (stop_time - start_time) / (double)(CLOCKS_PER_SEC) * 1000 << '\n';

        // Forward sweep calculations.
        cl::tape_array d_ref_array;
        d_ref_array.resize(3);

        // Derivative calculation time.
        std::clock_t calc_time = 0;

        // Derivatives with respect to a.
        std::vector<cl::tape_value> dx = { 1, 0, 0 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        std::vector<cl::tape_value> forw;
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dA(lin_regr, forw, eps);

        // Derivatives with respect to b.
        dx = { 0, 1, 0 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dB(lin_regr, forw, eps);

        // Derivatives with respect to c.
        dx = { 0, 0, 1 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dC(lin_regr, forw, eps);

        out_str << "Forward calculation took (ms): " << calc_time / (double)(CLOCKS_PER_SEC) * 1000 << '\n';
        out_str << "All derivatives calculated successfully." << std::endl;
        out_str << std::endl;
    }

    // Example of linear regression differentiation with respect to parameters of input distribution using non-optimized tape.
    inline void linear_regression_with_params_nonoptimized_example(const initial_data& data, std::ostream& out_str = std::cout)
    {
        // Control production of tape output to serializer.
        bool flag_serializer = data.flag_serializer;

        // Input linear regression parameters.
        // n: number of points {x_i, y_i}.
        // x_i are x_0 = 0, x_1 = 1, ..., x_n = n.
        // y_i are y_i = a + b * x_i + exp(-1 * c * x_i).
        size_t n = data.n;
        double a = data.a;
        double b = data.b;
        double c = data.c;

        // Tolerance for analytical vs. adjoint derivative check.
        double eps = data.eps;

        out_str << "Linear regression with parameters (non-optimized tape):\n" << std::endl;

        // Input values initialization.
        out_str << "Input vector size: n = " << n << std::endl;
        LinearRegression lin_regr(n, a, b, c);
        lin_regr.Calculate();
        cl::tape_double_vector X = { a, b, c };
        if (flag_serializer)
            out_str << "Input vector: " << X << "\n";

        std::clock_t start_time = std::clock();
        // Declare the X vector as independent and start a tape recording.
        cl::Independent(X);

        // Output calculations.
        cl::tape_double_vector x(n);
        // Obtain x_i values.
        cl::tape_array x_ref = lin_regr.GetInputX();
        for (int i = 0; i < n; i++)
            x[i] = x_ref[i];
        // Calculate corresponding y_i values.
        cl::tape_double_vector y(n);
        cl::tape_double& par_a = X[0];
        cl::tape_double& par_b = X[1];
        cl::tape_double& par_c = X[2];
        for (int i = 0; i < n; i++)
            y[i] = par_a + x[i] * par_b + std::exp(-1 * par_c * x[i]);
        // Start linear regression calculation: calculate mean values.
        cl::tape_double x_mean = 0.0;
        for (int i = 0; i < n; i++)
            x_mean += x[i];
        x_mean /= n;
        cl::tape_double y_mean = 0.0;
        for (int i = 0; i < n; i++)
            y_mean += y[i];
        y_mean /= n;
        // Variance times n: n * Var[x]
        cl::tape_double var_x_n = 0.0;
        for (int i = 0; i < n; i++)
            var_x_n += (x[i] - x_mean) * (x[i] - x_mean);
        // Covariance times n: n * Cov[x, y]
        cl::tape_double cov_xy_n = 0.0;
        for (int i = 0; i < n; i++)
            cov_xy_n += (x[i] - x_mean) * (y[i] - y_mean);
        // Linear regression coefficients.
        cl::tape_double beta = cov_xy_n / var_x_n;
        cl::tape_double alpha = y_mean - beta * x_mean;
        // Estimation for y_i.
        cl::tape_double_vector y_estimate(n);
        for (int i = 0; i < n; i++)
            y_estimate[i] = alpha + beta * x[i];
        // Output vector.
        cl::tape_double_vector Y(n + 2);
        Y[0] = alpha;
        Y[1] = beta;
        for (int i = 0; i < n; i++)
            Y[2 + i] = y_estimate[i];
        if (flag_serializer)
            out_str << "Output vector: " << Y << "\n\n";

        if (flag_serializer)
            out_str << "Ininial Forward(0) sweep...\n\n";
        // Declare a tape function and stop the tape recording.
        cl::tape_function<double> f(X, Y);
        std::clock_t stop_time = std::clock();
        out_str << "Tape memory (bytes): " << f.Memory() << std::endl;
        out_str << "Tape creation took (ms): " << (stop_time - start_time) / (double)(CLOCKS_PER_SEC) * 1000 << '\n';

        // Derivative calculation time.
        std::clock_t calc_time = 0;

        // Forward sweep calculations.
        std::vector<double> dx(3);

        // Derivatives with respect to a.
        dx = { 1, 0, 0 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        std::vector<double> forw;
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dA(lin_regr, forw, eps);

        // Derivatives with respect to b.
        dx = { 0, 1, 0 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dB(lin_regr, forw, eps);

        // Derivatives with respect to c.
        dx = { 0, 0, 1 };
        if (flag_serializer)
            out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
        start_time = std::clock();
        if (flag_serializer)
            forw = f.Forward(1, dx, out_str);
        else
            forw = f.Forward(1, dx);
        stop_time = std::clock();
        calc_time += stop_time - start_time;
        cl::check_derivatives_dC(lin_regr, forw, eps);

        out_str << "Forward calculation took (ms): " << calc_time / (double)(CLOCKS_PER_SEC) * 1000 << '\n';
        out_str << "All derivatives calculated successfully." << std::endl;
        out_str << std::endl;
    }

    // Example of linear regression differentiation with respect to input points using optimized tape.
    inline void linear_regression_with_points_optimized_example(const initial_data& data, std::ostream& out_str = std::cout)
    {
        // Control production of tape output to serializer.
        bool flag_serializer = data.flag_serializer;

        // Input linear regression parameters.
        // n: number of points {x_i, y_i}.
        // x_i are x_0 = 0, x_1 = 1, ..., x_n = n.
        // y_i are y_i = a + b * x_i + exp(-1 * c * x_i).
        size_t n = data.n;
        double a = data.a;
        double b = data.b;
        double c = data.c;

        // Tolerance for analytical vs. adjoint derivative check.
        double eps = data.eps;

        out_str << "Linear regression with points (optimized tape):\n" << std::endl;

        // Input values initialization.
        out_str << "Input vector size: n = " << n << std::endl;
        LinearRegression lin_regr(n, a, b, c);
        lin_regr.Calculate();
        cl::tape_array x_ref_array = lin_regr.GetInputX();
        cl::tape_array y_ref_array = lin_regr.GetInputY();
        cl::tape_value x_ref(x_ref_array), y_ref(y_ref_array);
        std::vector<cl::tape_object> X = { x_ref, y_ref };
        if (flag_serializer)
            out_str << "Input vector: " << X << "\n";

        std::clock_t start_time = std::clock();
        // Declare the X vector as independent and start a tape recording.
        cl::Independent(X);

        // Output calculations.
        cl::tape_object& x = X[0];
        cl::tape_object& y = X[1];
        // Start linear regression calculation: calculate mean values.
        cl::tape_object x_mean = 1.0 / n * cl::tapescript::sum_vec(x);
        cl::tape_object y_mean = 1.0 / n * cl::tapescript::sum_vec(y);
        // Variance times n: n * Var[x]
        cl::tape_object var_x_n = cl::tapescript::sum_vec((x - x_mean) * (x - x_mean));
        // Covariance times n: n * Cov[x, y]
        cl::tape_object cov_xy_n = cl::tapescript::sum_vec((x - x_mean) * (y - y_mean));
        // Linear regression coefficients.
        cl::tape_object beta = cov_xy_n / var_x_n;
        cl::tape_object alpha = y_mean - beta * x_mean;
        // Estimation for y_i.
        cl::tape_object y_estimate = alpha + beta * x;
        // Output vector.
        std::vector<cl::tape_object> Y = { alpha, beta, y_estimate };
        if (flag_serializer)
            out_str << "Output vector: " << Y << "\n\n";

        if (flag_serializer)
            out_str << "Ininial Forward(0) sweep...\n\n";
        // Declare a tape function and stop the tape recording.
        cl::tape_function<cl::tape_value> f(X, Y);
        std::clock_t stop_time = std::clock();
        out_str << "Tape memory (bytes): " << f.Memory() << std::endl;
        out_str << "Tape creation took (ms): " << (stop_time - start_time) / (double)(CLOCKS_PER_SEC)* 1000 << '\n';

        // Forward sweep calculations.
        cl::tape_array dx_ref_array, dy_ref_array;
        dx_ref_array.resize(n);
        dy_ref_array.resize(n);

        // Derivative calculation time.
        std::clock_t calc_time = 0;

        // Derivatives with respect to x_i.
        for (int j = 0; j < n; j++)
            dy_ref_array[j] = 0;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
                dx_ref_array[j] = 0;
            dx_ref_array[i] = 1;

            cl::tape_value dx_ref(dx_ref_array), dy_ref(dy_ref_array);
            std::vector<cl::tape_value> dx = { dx_ref, dy_ref };
            if (flag_serializer)
                out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
            start_time = std::clock();
            std::vector<cl::tape_value> forw;
            if (flag_serializer)
                forw = f.Forward(1, dx, out_str);
            else
                forw = f.Forward(1, dx);
            stop_time = std::clock();
            calc_time += stop_time - start_time;
            cl::check_derivatives_dX(lin_regr, forw, i, eps);
        }

        // Derivatives with respect to y_i.
        for (int j = 0; j < n; j++)
            dx_ref_array[j] = 0;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
                dy_ref_array[j] = 0;
            dy_ref_array[i] = 1;

            cl::tape_value dx_ref(dx_ref_array), dy_ref(dy_ref_array);
            std::vector<cl::tape_value> dx = { dx_ref, dy_ref };
            if (flag_serializer)
                out_str << "Forward(1, dx) sweep for dx = " << dx << "..." << std::endl;
            start_time = std::clock();
            std::vector<cl::tape_value> forw;
            if (flag_serializer)
                forw = f.Forward(1, dx, out_str);
            else
                forw = f.Forward(1, dx);
            stop_time = std::clock();
            calc_time += stop_time - start_time;
            cl::check_derivatives_dY(lin_regr, forw, i, eps);
        }

        out_str << "Forward calculation took (ms): " << calc_time / (double)(CLOCKS_PER_SEC)* 1000 << '\n';
        out_str << "All derivatives calculated successfully." << std::endl;
        out_str << std::endl;
    }

    // Example of linear regression differentiation with respect to input points using non-optimized tape.
    inline void linear_regression_with_points_nonoptimized_example(const initial_data& data, std::ostream& out_str = std::cout)
    {
        // Control production of tape output to serializer.
        bool flag_serializer = data.flag_serializer;

        // Input linear regression parameters.
        // n: number of points {x_i, y_i}.
        // x_i are x_0 = 0, x_1 = 1, ..., x_n = n.
        // y_i are y_i = a + b * x_i + exp(-1 * c * x_i).
        size_t n = data.n;
        double a = data.a;
        double b = data.b;
        double c = data.c;

        // Tolerance for analytical vs. adjoint derivative check.
        double eps = data.eps;

        out_str << "Linear regression with points (non-optimized tape):\n" << std::endl;

        // Input values initialization.
        out_str << "Input vector size: n = " << n << std::endl;
        LinearRegression lin_regr(n, a, b, c);
        lin_regr.Calculate();
        cl::tape_array x_ref_array = lin_regr.GetInputX();
        cl::tape_array y_ref_array = lin_regr.GetInputY();
        cl::tape_double_vector X(2 * n);
        for (int i = 0; i < n; i++)
            X[i] = x_ref_array[i];
        for (int i = 0; i < n; i++)
            X[n + i] = y_ref_array[i];
        if (flag_serializer)
            out_str << "Input vector: " << X << "\n";

        std::clock_t start_time = std::clock();
        // Declare the X vector as independent and start a tape recording.
        cl::Independent(X);

        // Output calculations.
        // Start linear regression calculation: calculate mean values.
        cl::tape_double x_mean = 0.0;
        for (int i = 0; i < n; i++)
            x_mean += X[i];
        x_mean /= n;
        cl::tape_double y_mean = 0.0;
        for (int i = 0; i < n; i++)
            y_mean += X[n + i];
        y_mean /= n;
        // Variance times n: n * Var[x]
        cl::tape_double var_x_n = 0.0;
        for (int i = 0; i < n; i++)
            var_x_n += (X[i] - x_mean) * (X[i] - x_mean);
        // Covariance times n: n * Cov[x, y]
        cl::tape_double cov_xy_n = 0.0;
        for (int i = 0; i < n; i++)
            cov_xy_n += (X[i] - x_mean) * (X[n + i] - y_mean);
        // Linear regression coefficients.
        cl::tape_double beta = cov_xy_n / var_x_n;
        cl::tape_double alpha = y_mean - beta * x_mean;
        // Estimation for y_i.
        cl::tape_double_vector y_estimate(n);
        for (int i = 0; i < n; i++)
            y_estimate[i] = alpha + beta * X[i];
        // Output vector.
        cl::tape_double_vector Y(n + 2);
        Y[0] = alpha;
        Y[1] = beta;
        for (int i = 0; i < n; i++)
            Y[2 + i] = y_estimate[i];
        if (flag_serializer)
            out_str << "Output vector: " << Y << "\n\n";

        if (data.flag_serializer)
            out_str << "Ininial Forward(0) sweep...\n\n";
        // Declare a tape function and stop the tape recording.
        cl::tape_function<double> f(X, Y);
        std::clock_t stop_time = std::clock();
        out_str << "Tape memory (bytes): " << f.Memory() << std::endl;
        out_str << "Tape creation took (ms): " << (stop_time - start_time) / (double)(CLOCKS_PER_SEC)* 1000 << '\n';

        // Derivative calculation time.
        std::clock_t calc_time = 0;

        // Forward sweep calculations.
        std::vector<double> dx(2 * n);

        // Derivatives with respect to x_i.
        for (int j = 0; j < n; j++)
            dx[n + j] = 0;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
                dx[j] = 0;
            dx[i] = 1;
            start_time = std::clock();
            std::vector<double> forw;
            if (flag_serializer)
                forw = f.Forward(1, dx, out_str);
            else
                forw = f.Forward(1, dx);
            stop_time = std::clock();
            calc_time += stop_time - start_time;
            cl::check_derivatives_dX(lin_regr, forw, i, eps);
        }

        // Derivatives with respect to y_i.
        for (int j = 0; j < n; j++)
            dx[j] = 0;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
                dx[n + j] = 0;
            dx[n + i] = 1;
            start_time = std::clock();
            std::vector<double> forw;
            if (flag_serializer)
                forw = f.Forward(1, dx, out_str);
            else
                forw = f.Forward(1, dx);
            stop_time = std::clock();
            calc_time += stop_time - start_time;
            cl::check_derivatives_dY(lin_regr, forw, i, eps);
        }

        out_str << "Forward calculation took (ms): " << calc_time / (double)(CLOCKS_PER_SEC)* 1000 << '\n';
        out_str << "All derivatives calculated successfully." << std::endl;
        out_str << std::endl;
    }

    inline void regression_examples()
    {
        std::ofstream of("regression_output.txt");
        cl::tape_serializer<cl::tape_value> serializer(of);
        serializer.precision(3);

        // Input data parameters.
        initial_data data;

        linear_regression_with_params_optimized_example(data, serializer);
        linear_regression_with_params_nonoptimized_example(data, serializer);
        linear_regression_with_points_optimized_example(data, serializer);
        linear_regression_with_points_nonoptimized_example(data, serializer);
    }
}

#endif // cl_tape_examples_impl_regression_examples_hpp
