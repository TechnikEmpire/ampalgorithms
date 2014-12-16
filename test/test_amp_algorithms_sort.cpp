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
* This file contains unit tests.
*---------------------------------------------------------------------------*/

// TODO: Rename this file to test_amp_algorithms_radix_sort

#include "stdafx.h"

#include <amp_algorithms.h>
#include "testtools.h"

using namespace amp_algorithms;
using namespace testtools;

static const int test_tile_size = 256;

class amp_algorithms_radix_sort_tests : public testbase, public ::testing::Test {};

//----------------------------------------------------------------------------
//  Type conversion and radix calculation tests
//----------------------------------------------------------------------------

typedef ::testing::Types<int, unsigned int, float> radix_sort_supported_types;

enum parameter
{
    index = 0,
    value = 1,
    expected = 2
};

const std::tuple<unsigned, int, int> details_radix_key_value_width_2_data[] = {
    std::make_tuple(0, 3, 3),       // 000010 => ----10
    std::make_tuple(0, 1, 1),       // 000001 => ----01
    std::make_tuple(1, 3, 0),       // 000011 => --00--
    std::make_tuple(1, 13, 3),      // 001101 => --11--
    std::make_tuple(2, 45, 2),      // 101101 => 10----
};

class details_radix_key_value_width_2_tests : public ::testing::TestWithParam < std::tuple<unsigned, int, int> > {};

TEST_P(details_radix_key_value_width_2_tests, test)
{
    //  0 0000  0  0        8 1000  2  0
    //  1 0001  0  1        9 1001  2  1
    //  2 0010  0  2       10 1010  2  2
    //  3 0011  0  3       11 1011  2  3
    //  4 0100  1  0       12 1100  3  0
    //  5 0101  1  1       13 1101  3  1
    //  6 0110  1  2       14 1110  3  2
    //  7 0111  1  3       15 1111  3  3

    auto t = GetParam();
    int result = amp_algorithms::_details::radix_key_value<int, 2>(std::get<parameter::value>(t), std::get<parameter::index>(t));
    ASSERT_EQ(std::get<parameter::expected>(t), result);

}

INSTANTIATE_TEST_CASE_P(amp_algorithms_radix_sort_tests, details_radix_key_value_width_2_tests, ::testing::ValuesIn(details_radix_key_value_width_2_data));

const std::tuple<unsigned, int, int> details_radix_key_value_width_4_data[] = {
    std::make_tuple(0, 0x09, 9),    // 00001010
    std::make_tuple(1, 0x03, 0),    // 00001011
    std::make_tuple(1, 0x10, 1),    // 00010000
    std::make_tuple(1, 0xAD, 10),   // 10101101
};

class details_radix_key_value_width_4_tests : public ::testing::TestWithParam < std::tuple<unsigned, int, int> > {};

TEST_P(details_radix_key_value_width_4_tests, test)
{
    auto t = GetParam();
    int result = amp_algorithms::_details::radix_key_value<int, 4>(std::get<parameter::value>(t), std::get<parameter::index>(t));
    ASSERT_EQ(std::get<parameter::expected>(t), result);
}

INSTANTIATE_TEST_CASE_P(amp_algorithms_radix_sort_tests, details_radix_key_value_width_4_tests, ::testing::ValuesIn(details_radix_key_value_width_4_data));

const std::tuple<float, unsigned> details_convert_to_from_uint_data[] = { 
    std::make_tuple(1.0f, 3212836864),
    std::make_tuple(1.314f, 3215470887),
    std::make_tuple(0.0f, 2147483648),
    std::make_tuple(4.0f, 3229614080),
    std::make_tuple(-4.5674f, 1064163291)
};

class details_convert_to_uint_tests : public ::testing::TestWithParam < std::tuple<float, unsigned> > {};

TEST_P(details_convert_to_uint_tests, test)
{
    float val = std::get<0>(GetParam());
    unsigned result = amp_algorithms::_details::convert_to_uint(val);
    ASSERT_EQ(std::get<1>(GetParam()), result);
}

INSTANTIATE_TEST_CASE_P(amp_algorithms_radix_sort_tests, details_convert_to_uint_tests, ::testing::ValuesIn(details_convert_to_from_uint_data));

class details_convert_from_uint_tests : public ::testing::TestWithParam < std::tuple<float, unsigned> > {};

TEST_P(details_convert_from_uint_tests, test)
{
    unsigned val = std::get<1>(GetParam());
    float result = amp_algorithms::_details::convert_from_uint<float>(val);
    ASSERT_EQ(std::get<0>(GetParam()), result);
}

INSTANTIATE_TEST_CASE_P(amp_algorithms_radix_sort_tests, details_convert_from_uint_tests, ::testing::ValuesIn(details_convert_to_from_uint_data));

template <typename T>
class details_convert_to_from_uint_tests : public ::testing::Test { };

TYPED_TEST_CASE_P(details_convert_to_from_uint_tests);

TYPED_TEST_P(details_convert_to_from_uint_tests, test) 
{
    for (auto v : details_convert_to_from_uint_data)
    {
        TypeParam val = static_cast<TypeParam>(std::get<0>(v));
        unsigned result_int = amp_algorithms::_details::convert_to_uint(val);
        TypeParam result = amp_algorithms::_details::convert_from_uint<TypeParam>(result_int);
        EXPECT_EQ(val, result);
    }
}

REGISTER_TYPED_TEST_CASE_P(details_convert_to_from_uint_tests, test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_algorithms_radix_sort_tests, details_convert_to_from_uint_tests, radix_sort_supported_types);

//----------------------------------------------------------------------------
// Internal implementation steps tests
//----------------------------------------------------------------------------

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_0_tile_4_bin_width_2_data_16)
{
    std::array<unsigned, 16> input =     { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,   4, 12,  9,  8 };
    // Key 0 values, 2 bit key:            3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
    std::array<unsigned, 16> expected  = { 1,  2,  6,  3,    0, 13, 10, 11,    5, 10, 14, 15,   4, 12,  8,  9 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<4> compute_domain = input_av.get_extent().tile<4>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<4> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[4];

        tile_data[idx] = input_av[gidx];
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<int, 4, 2>(tile_data, input_av.extent.size(), tidx, 0);

        tidx.barrier.wait();
        input_av[gidx] = tile_data[idx];
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_0_tile_32_bin_width_2_data_16)
{
    std::array<unsigned, 16> input =     { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,   4, 12,  9,  8 };
    // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
    std::array<unsigned, 16> expected  = { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,  14,  3, 11, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<32> compute_domain = input_av.get_extent().tile<32>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<32> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[32];

        tile_data[idx] = input_av[gidx];
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<int, 32, 2>(tile_data, input_av.extent.size(), tidx, 0);

        tidx.barrier.wait();
        input_av[gidx] = tile_data[idx];
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_0_tile_32_bin_width_2_data_32)
{
    std::array<unsigned long, 32> input = { 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0 };
    // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
    std::array<unsigned long, 32> expected = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3 };
    array_view<unsigned long> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<32> compute_domain = input_av.extent.tile<32>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<32> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static unsigned long tile_data[32];

        if (gidx < input_av.extent[0])
        {
            tile_data[idx] = input_av[gidx];
        }
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<unsigned long, 32, 2>(tile_data, input_av.extent.size(), tidx, 0);

        tidx.barrier.wait();
        if (gidx < input_av.extent[0])
        {
            input_av[gidx] = tile_data[idx];
        }
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_0_tile_32_bin_width_4_data_32)
{
    std::array<unsigned long, 32> input = { 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0 };
    // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
    std::array<unsigned long, 32> expected = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3 };
    array_view<unsigned long> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<32> compute_domain = input_av.extent.tile<32>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<32> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static unsigned long tile_data[32];

        if (gidx < input_av.extent[0])
        {
            tile_data[idx] = input_av[gidx];
        }
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<unsigned long, 32, 2>(tile_data, input_av.extent.size(), tidx, 0);

        tidx.barrier.wait();
        if (gidx < input_av.extent[0])
        {
            input_av[gidx] = tile_data[idx];
        }
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_0_tile_256_bin_width_2_data_16)
{
    std::array<unsigned, 16> input = { 3, 2, 1, 6, 10, 11, 13, 0, 15, 10, 5, 14, 4, 12, 9, 8 };
    // rdx                                 3   2   1   2     2   3   1   0     3   2   1   2    0   0   1   0
    std::array<unsigned, 16> expected = { 0, 4, 12, 8, 1, 13, 5, 9, 2, 6, 10, 10, 14, 3, 11, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<256> compute_domain = input_av.extent.tile<256>().pad();

    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<256> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[256];

        tile_data[idx] = input_av[gidx];
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<int, 256, 2>(tile_data, input_av.extent.size(), tidx, 0);

        tidx.barrier.wait();
        input_av[gidx] = tile_data[idx];
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_tile_by_key_with_index_1_tile_4_bin_width_2_data_16)
{
    std::array<unsigned, 16> input = { 1, 2, 6, 3, 0, 13, 10, 11, 5, 10, 14, 15, 4, 12, 8, 9 };
    // Key 1 values, 2 bit key:            0   0   1   0    0   3   2   2    1   2   3   3    1   3   2   2
    std::array<unsigned, 16> expected = { 1, 2, 3, 6, 0, 10, 11, 13, 5, 10, 14, 15, 4, 8, 9, 12 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);

    concurrency::tiled_extent<4> compute_domain = input_av.get_extent().tile<4>().pad();
    concurrency::parallel_for_each(compute_domain, [=](concurrency::tiled_index<4> tidx) restrict(amp)
    {
        const int gidx = tidx.global[0];
        const int idx = tidx.local[0];
        tile_static int tile_data[4];
        tile_data[idx] = input_av[gidx];
        tidx.barrier.wait();

        amp_algorithms::_details::radix_sort_tile_by_key<int, 4, 2>(tile_data, 16, tidx, 1);

        tidx.barrier.wait();
        input_av[gidx] = tile_data[idx];
    });

    input_av.synchronize();
    ASSERT_TRUE(are_equal(expected, input_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_by_key_with_index_0_tile_4_data_16)
{
    // gidx                                                    0   1   2   3     4   5   6   7     8   9  10  11    12  13  14  15
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    // rdx =                                                   3,  2,  1,  2,    2,  3,  1,  0,    3,  2,  1,  2,    0,  0,  1,  0

    std::array<unsigned, 16> per_tile_rdx_histograms =       { 0,  1,  2,  1,    1,  1,  1,  1,    0,  1,  2,  1,    3,  1,  0,  0 };
    std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  0,  1,  3,    0,  1,  2,  3,    0,  0,  1,  3,    0,  3,  4,  4 };

    std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 0,  1,  0,  3,    1,  1,  1,  1,    2,  1,  2,  0,    1,  1,  1,  0 };
    std::array<unsigned, 16> tile_histogram_segscan =        { 0,  0,  1,  1,    0,  1,  2,  3,    0,  2,  3,  5,    0,  1,  2,  3 };
    std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,    0,  1,  2,  1,    1,  2,  3,  2,    1,  3,  5,  3 };
                                                                     
    std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                              0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                              0,0,0,0,0,0,0,0,0,0,0,0 };
                                                                     
    std::array<unsigned, 16> sorted_per_tile =               { 1,  2,  6,  3,    0, 13, 10, 11,    5, 10, 14, 15,    4, 12,  8,  9 };
    // rdx =                                                   1,  2,  2,  3,    0,  1,  2,  3,    1,  2,  2,  3,    0,  0,  0,  1
    std::array<unsigned, 16> dest_gidx =                     { 4,  8,  9, 13,    0,  5, 10, 14,    6, 11, 12, 15,    1,  2,  3,  7 }; 
                                                                     
    std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };

    // dest_gidx = idx - per_tile_rdx_offsets[tlx][rd x_0] + per_tile_rdx_offsets[tlx][rdx_0] + tile_rdx_offsets[(rdx * tile_count) + tlx] + global_rdx_offsets[rdx_0]
    //                                             
    //                                                         1 => 0 - 0  + 0 +  4 = 4
    //                                                         2 => 1 - 1  + 0 +  8 = 8 
    //                                                         2 => 2 - 1  + 0 +  8 = 9
    //                                                         3 => 3 - 3  + 0 + 13 = 13
    //                                                                         
    //                                                                           0 => 0 - 0  + 0 +  0 = 0
    //                                                                           1 => 1 - 1  + 1 +  4 = 5
    //                                                                           2 => 2 - 2  + 2 +  8 = 10
    //                                                                           3 => 3 - 3  + 1 + 13 = 14
    //                                                                         
    //                                                                                             1 => 0 - 0  + 2 +  4 = 6
    //                                                                                             2 => 1 - 1  + 3 +  8 = 11
    //                                                                                             2 => 2 - 1  + 3 +  8 = 12
    //                                                                                             3 => 3 - 3  + 2 + 13 = 15

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 404);

    amp_algorithms::_details::radix_sort_by_key<unsigned, 4, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_0, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_by_key_with_index_0_tile_8_data_16)
{
    // gidx                                                    0   1   2   3   4   5   6   7     8   9  10  11  12  13  14  15
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6, 10, 11, 13,  0,   15, 10,  5, 14,  4, 12,  9,  8 };
    // rdx =                                                   3,  2,  1,  2,  2,  3,  1,  0,    3,  2,  1,  2,  0,  0,  1,  0

    std::array<unsigned, 16> per_tile_rdx_histograms =       { 1,  2,  3,  2,        0,0,0,0,    3,  2,  2,  1,        0,0,0,0 };
    std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  1,  3,  6,        0,0,0,0,    0,  3,  5,  7,        0,0,0,0 };

    std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 1,  3,    2,  2,    3,  2,    2,  1,            0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> tile_histogram_segscan =        { 0,  1,    0,  2,    0,  3,    0,  2,            0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,        0,0,0,0,    1,  2,  3,  2,        0,0,0,0 };

    std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> dest_gidx =                     { 4,  8,  9, 13,    0,  5, 10, 14,    6, 11, 12, 15,    1,  2,  3,  7 }; 
                                                                  
    std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,  1, 13,  5,  9,    2,  6, 10, 10, 14,  3, 11, 15 };
                                                           
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);

    amp_algorithms::_details::radix_sort_by_key<unsigned, 8, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_0, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_by_key_with_index_0_tile_32_data_16)
{
    // gidx                                                    0   1   2   3   4   5   6   7     8   9  10  11  12  13  14  15
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6, 10, 11, 13,  0,   15, 10,  5, 14,  4, 12,  9,  8 };
    // rdx =                                                   3,  2,  1,  2,  2,  3,  1,  0,    3,  2,  1,  2,  0,  0,  1,  0

    std::array<unsigned, 16> per_tile_rdx_histograms =       { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> per_tile_rdx_offsets =          { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };

    std::array<unsigned, 16> per_tile_rdx_histograms_tp =    { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> tile_histogram_segscan =        { 0,  0,  0,  0,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> tile_rdx_offsets =              { 0,  0,  0,  0,                          0,0,0,0,0,0,0,0,0,0,0,0 };

    std::array<unsigned, 16> global_histogram =              { 4,  4,  5,  3,                          0,0,0,0,0,0,0,0,0,0,0,0 };
    std::array<unsigned, 16> global_rdx_offsets =            { 0,  4,  8, 13,                          0,0,0,0,0,0,0,0,0,0,0,0 };

    std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output); 
    amp_algorithms::fill(output_av, 0);

    amp_algorithms::_details::radix_sort_by_key<unsigned, 32, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 0);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_0, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_by_key_with_index_1_tile_4_data_16)
{
    // gidx                                                    0   1   2   3     4   5   6   7     8   9  10  11    12  13  14  15
    std::array<unsigned, 16> input =                         { 0,  4, 12,  8,   1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
    // rdx                                                     0   0   1   0    0   3   2   2     1   2   3   3    1   3   2   2

    std::array<unsigned, 16> sorted_by_key_1 =               { 0,  1,  2,  3,   4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };

    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 404);

    amp_algorithms::_details::radix_sort_by_key<unsigned, 4, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 1);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_1, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_by_key_with_index_1_tile_8_data_16)
{
    std::array<unsigned, 16> input =                         { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
    std::array<unsigned, 16> sorted_by_key_1 =               { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);

    amp_algorithms::_details::radix_sort_by_key<unsigned, 8, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av, 1);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_1, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_4_data_16)
{
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 404);
            
    amp_algorithms::_details::radix_sort<unsigned, 4, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_4_float_data_16)
{
    std::array<float, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<float, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
    array_view<float> input_av(static_cast<int>(input.size()), input);
    std::array<float, 16> output;
    array_view<float> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 404);
            
    amp_algorithms::_details::radix_sort<float, 4, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_8_data_16)
{
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);
            
    amp_algorithms::_details::radix_sort<unsigned, 8, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_32_data_16)
{
    std::array<unsigned, 16> input =                         { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<unsigned, 16> sorted =                        { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };
    array_view<unsigned> input_av(static_cast<int>(input.size()), input);
    std::array<unsigned, 16> output;
    array_view<unsigned> output_av(static_cast<int>(output.size()), output);
    amp_algorithms::fill(output_av, 0);
            
    amp_algorithms::_details::radix_sort<unsigned, 32, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_4_data_1024)
{
    std::vector<int> input(1024, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<int, 1> input_av(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<int> output(input.size(), 0);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    amp_algorithms::_details::radix_sort<int, 4, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_8_data_1024)
{
    std::vector<int> input(1024, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<int, 1> input_av(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<int> output(input.size(), 0);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    amp_algorithms::_details::radix_sort<int, 8, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_16_data_1024)
{
    std::vector<int> input(1024, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<int, 1> input_av(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<int> output(input.size(), 0);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    amp_algorithms::_details::radix_sort<int, 16, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, details_radix_sort_with_tile_32_data_1024)
{
    std::vector<int> input(1024, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<int, 1> input_av(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<int> output(input.size(), 0);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    amp_algorithms::_details::radix_sort<int, 32, 2>(amp_algorithms::_details::auto_select_target(), input_av, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

//----------------------------------------------------------------------------
// Public API tests
//----------------------------------------------------------------------------

TEST_F(amp_algorithms_radix_sort_tests, radix_sort_with_data_16)
{
    std::array<int, 16> input =                              { 3,  2,  1,  6,   10, 11, 13,  0,   15, 10,  5, 14,    4, 12,  9,  8 };
    std::array<unsigned, 16> sorted_by_key_0 =               { 0,  4, 12,  8,    1, 13,  5,  9,    2,  6, 10, 10,   14,  3, 11, 15 };
    std::array<int, 16> sorted_by_key_1 =                    { 0,  1,  2,  3,    4,  5,  6,  8,    9, 10, 10, 11,   12, 13, 14, 15 };

    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> output(input.size(), 404);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    radix_sort(input_vw, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(sorted_by_key_1, output_av));
}

TEST_F(amp_algorithms_radix_sort_tests, radix_sort_with_data_64)
{
    std::vector<int> input(64, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<int, 1> input_vw(static_cast<int>(input.size()), input);
    std::vector<int> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<int> output(input.size(), 404);
    array_view<int> output_av(static_cast<int>(output.size()), output);

    radix_sort(input_vw, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

template <typename T>
class radix_sort_with_float_data_1283 : public ::testing::Test { };

TYPED_TEST_CASE_P(radix_sort_with_float_data_1283);

TYPED_TEST_P(radix_sort_with_float_data_1283, test)
{
    std::vector<TypeParam> input(1283, 1);
#if _MSC_VER < 1800
    std::iota(input.rbegin(), input.rend(), 0);
#else
    std::iota(rbegin(input), rend(input), 0);
#endif
    concurrency::array_view<TypeParam, 1> input_vw(static_cast<TypeParam>(input.size()), input);
    std::vector<TypeParam> expected(input.size());
    std::iota(begin(expected), end(expected), 0);
    std::vector<TypeParam> output(input.size(), 404);
    array_view<TypeParam> output_av(static_cast<TypeParam>(output.size()), output);

    radix_sort(input_vw, output_av);

    output_av.synchronize();
    ASSERT_TRUE(are_equal(expected, output_av));
}

REGISTER_TYPED_TEST_CASE_P(radix_sort_with_float_data_1283, test);
INSTANTIATE_TYPED_TEST_CASE_P(amp_algorithms_radix_sort_tests, radix_sort_with_float_data_1283, radix_sort_supported_types);

// TODO: Finish make_array_view, assuming we really need it.

template< typename ConstRandomAccessIterator >
void make_array_view( ConstRandomAccessIterator first, ConstRandomAccessIterator last )
{
    return array_view(std::distance(first, last), first);
}
