 /*----------------------------------------------------------------------------
* Copyright � Microsoft Corp.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not 
* use this file except in compliance with the License.  You may obtain a copy 
* of the License at http://www.apache.org/licenses/LICENSE-2.0  
* 
* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
* WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
* MERCHANTABLITY OR NON-INFRINGEMENT. 
*
* See the Apache Version 2.0 License for specific language governing 
* permissions and limitations under the License.
*---------------------------------------------------------------------------
* 
* C++ AMP standard algorithm library.
*
* This file contains the additional utilities to support unit tests.
*---------------------------------------------------------------------------*/

#pragma once

#include <vector>
#include <algorithm>
#include <iostream>

#include <amp_algorithms.h>
#include <amp_stl_algorithms.h>

using namespace concurrency;
using namespace amp_algorithms;
using namespace amp_stl_algorithms;

//  Define these namespaces and types to pick up poorly specified namespaces and types in library code.
//  This makes the test code more like a real library client which may define conflicting namespaces.

namespace details { };
namespace _details { };
namespace direct3d { };
namespace graphics { };
namespace fast_math { };
namespace precise_math { };

class extent { };
class index { };
class array { };

//  Set USE_REF to use the REF accelerator for all tests. This is useful if tests fail on a particular machine as
//  failure may be due to a driver bug.

namespace testtools
{
    inline void set_default_accelerator(std::wstring test_name)
    {
#if (defined(USE_REF) || defined(_DEBUG))
        std::wstring dev_path = accelerator().device_path;
        bool set_ok = accelerator::set_default(accelerator::direct3d_ref) || 
            (dev_path == accelerator::direct3d_ref);

        if (!set_ok)
        {
            std::wstringstream str;
            str << "Unable to set default accelerator to REF. Using " << dev_path << "." << std::endl;
        }
#endif
        accelerator().get_default_view().flush();
    }

    //===============================================================================
    //  Helper functions to generate test data of random numbers.
    //===============================================================================

    template<typename T>
    inline int test_array_size()
    {
        int size;
#if _DEBUG
        size = 1023;
#else
        size = 1023 + 1029;
#endif
        if (std::is_same<T, int>::value)
            size *= 13;
        else if (std::is_same<T, float>::value)
            size *= 5;
        return size;
    }

    template <typename T>
    inline void generate_data(std::vector<T> &v)
    {
        srand(2012);    // Set random number seed so tests are reproducible.
        std::generate(begin(v), end(v), [=]{
            T v = (T)rand();
            return ((static_cast<int>(v) % 4) == 0) ? -v : v;
        });
    }

    template <>
    inline void generate_data(std::vector<unsigned int> &v)
    {
        srand(2012);    // Set random number seed so tests are reproducible.
        std::generate(begin(v), end(v), [=](){ return (unsigned int) rand(); });
    }

    //===============================================================================
    //  CPU based scan implementation for comparing results from AMP implementations.
    //===============================================================================

    template <typename InIt, typename OutIt>
    inline void scan_sequential_exclusive(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;
        int previous = T(0);
        auto result = T(0);

        std::transform((first), last, (dest_first), [=, &result, &previous](const T& v)
        {
            result = previous;
            previous += v;
            return result;
        });
    }

    template <typename InIt, typename OutIt>
    inline void scan_sequential_inclusive(InIt first, InIt last, OutIt dest_first)
    {
        typedef InIt::value_type T;
        auto result = T(0);

        std::transform(first, last, dest_first, [=, &result](const T& v)
        {
            result += v;
            return result;
        });
    }
    
    //===============================================================================
    //  Comparison.
    //===============================================================================

    // Helper function for floating-point comparison. It combines absolute and relative comparison techniques,
    // in order to check if two floating-point are close enough to be considered as equal.
    template<typename T>
    inline bool are_almost_equal(T v1, T v2, const T maxAbsoluteDiff, const T maxRelativeDiff)
    {
        // Return quickly if floating-point representations are exactly the same,
        // additionally guard against division by zero when, both v1 and v2 are equal to 0.0f
        if (v1 == v2) 
        {
            return true;
        }
        else if (fabs(v1 - v2) < maxAbsoluteDiff) // absolute comparison
        {
            return true;
        }

        T diff = 0.0f;

        if (fabs(v1) > fabs(v2))
        {
            diff = fabs(v1 - v2) / fabs(v1);
        }
        else
        {
            diff = fabs(v2 - v1) / fabs(v2);
        }

        return (diff < maxRelativeDiff); // relative comparison
    }

    // Compare two floats and return true if they are close to each other.
    inline bool compare(float v1, float v2, 
        const float maxAbsoluteDiff = 0.000005f,
        const float maxRelativeDiff = 0.001f)
    {
        return are_almost_equal(v1, v2, maxAbsoluteDiff, maxRelativeDiff);
    }

    template<typename T>
    inline bool compare(const T &v1, const T &v2)
    {
        // This function is constructed in a way that requires T
        // only to define operator< to check for equality

        if (v1 < v2)
        {
            return false;
        }
        if (v2 < v1)
        {
            return false;
        }
        return true;
    }

    template<typename StlFunc, typename AmpFunc, typename Data>
    void compare_binary_operator(StlFunc stl_func, AmpFunc amp_func, Data tests)
    {
        for (auto p : tests)
        {
           EXPECT_EQ(stl_func(p.first, p.second), amp_func(p.first, p.second));
        }
    }

    template<typename StlFunc, typename AmpFunc, typename Data>
    void compare_unary_operator(StlFunc stl_func, AmpFunc amp_func, Data tests)
    {
        for (auto p : tests)
        {
            EXPECT_EQ(stl_func(p), amp_func(p));
        }
    }

    // Compare array_view with other STL containers.
    template<typename T>
    size_t size(const array_view<T>& arr)
    {
        return arr.extent.size();
    }

    template<typename T>
    size_t size(const std::vector<T>& arr)
    {
        return arr.size();
    }

    template <typename T1, typename T2>
    bool are_equal(const T1& expected, const T2& actual, size_t expected_size = -1)
    {    
        const int output_range = 8;
        if (expected_size == -1)
        {
            expected_size = std::distance(begin(expected), end(expected));
            EXPECT_EQ(expected_size, std::distance(begin(actual), end(actual)));
        }
        if (expected_size == 0)
        {
            return true;
        }
        bool is_same = true;
        for (int i = 0; (i < static_cast<int>(expected_size) && is_same); ++i)
        {
            EXPECT_EQ(expected[i], actual[i]);
            if (expected[i] != actual[i])
            {
                is_same = false;
            }
        }
        return is_same;
    }

    //===============================================================================
    //  Stream output overloads for std::vector, array and array_view.
    //===============================================================================

    // Setting the container width in the output stream modifies the number of elements output from the containers
    // subsequently output in the stream. The following outputs the first 4 elements of data.
    //
    //      std::vector<int> data(12, 1);
    //      cout << container_width(4) << data;

    class container_width
    {
    public:
        explicit container_width(size_t width) : m_width(width) { }

    private:
        size_t m_width;

        template <class T, class Traits>
        inline friend std::basic_ostream<T, Traits>& operator <<
            (std::basic_ostream<T, Traits>& os, const container_width& container)
        {
            os.iword(_details::geti()) = long(container.m_width);
            return os;
        }
    };

    // TODO: These print two ',' as a delimiter not one. Fix.
    template<typename StrmType, typename Traits, typename T, int N>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const std::array<T, N>& vec)
    {
        std::copy(std::begin(vec), std::begin(vec) + std::min<size_t>(_details::get_width(os), vec.size()),
            std::ostream_iterator<T, Traits::char_type>(os, _details::get_delimiter<Traits::char_type>()));
        return os;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const std::vector<T>& vec)
    {
        std::copy(std::begin(vec), std::begin(vec) + std::min<size_t>(_details::get_width(os), vec.size()),
            std::ostream_iterator<T, Traits::char_type>(os, _details::get_delimiter<Traits::char_type>()));
        return os;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, concurrency::array<T, 1>& vec)
    {
        size_t i = std::min<size_t>(_details::get_width(os), vec.extent[0]);
        std::vector<const T> buffer(i);
        copy(vec.section(0, static_cast<int>(i)), std::begin(buffer));
        return os << buffer;
    }

    template<typename StrmType, typename Traits, typename T>
    std::basic_ostream<StrmType, Traits>& operator<< (std::basic_ostream<StrmType, Traits>& os, const concurrency::array_view<T, 1>& vec)
    {
        size_t i = std::min<size_t>(_details::get_width(os), vec.extent[0]);
        std::vector<T> buffer(i);
        copy(vec.section(0, static_cast<int>(i)), std::begin(buffer));
        return os << buffer;
    }

    namespace _details
    {
        inline int geti()
        {
            static int i = std::ios_base::xalloc();
            return i;
        }

        template <typename STREAM>
        inline size_t get_width(STREAM& os)
        {
            const size_t default_width = 4;
            size_t width = os.iword(geti());
            return (width == 0) ? default_width : width;
        }

        template <typename T>
        inline T* get_delimiter()
        {
            assert(false);
            return nullptr;
        }

        template <>
        inline char* get_delimiter()
        {
            static char delim(',');
            return &delim;
        }

        template <>
        inline wchar_t* get_delimiter()
        {
            static wchar_t delim(L',');
            return &delim;
        }
    } // namespace _details

    //===============================================================================
    //  Basic performance timing.
    //===============================================================================

    inline double elapsed_time(const LARGE_INTEGER& start, const LARGE_INTEGER& end)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return (double(end.QuadPart) - double(start.QuadPart)) * 1000.0 / double(freq.QuadPart);
    }

    template <typename Func>
    double time_func(accelerator_view& view, Func f)
    {
        //  Ensure that the C++ AMP runtime is initialized.
        accelerator::get_all();
        //  Ensure that the C++ AMP kernel has been JITed.
        f();
        view.wait();

        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start);
        f();
        view.wait();
        QueryPerformanceCounter(&end);

        return elapsed_time(start, end);
    }
}; // namespace test_tools

class testbase
{
protected:
    testbase()
    {
        testtools::set_default_accelerator(L"stl_algorithms_tests");
        accelerator().default_view.wait();
    }
};

template<int _Size = 13>
class stl_algorithms_testbase : public testbase
{
protected:
    std::array<int, _Size> input;
    array_view<int> input_av;
    std::array<int, _Size> output;
    array_view<int> output_av;
    std::array<int, _Size> expected;

    stl_algorithms_testbase() :
        input_av(concurrency::extent<1>(static_cast<int>(input.size())), input),
        output_av(concurrency::extent<1>(static_cast<int>(output.size())), output)
    {
        const std::array<int, _Size> input_data = { { 1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 10, 2 } };
        int i = 0;
        for (auto& v : input)
        {
            v = input_data[i++ % 13];
        }
        std::fill(begin(output), end(output), -1);
        std::fill(begin(expected), end(expected), -1);
    }

    static const int size = _Size;
};

template <typename T>
class greater_than
{
private:
    T m_value;
public:
    greater_than(const T& value = 0) : m_value(value) {}

    T operator()(const T &v) const restrict(cpu, amp)
    {
        return (v > m_value) ? 1 : 0;
    }
};
