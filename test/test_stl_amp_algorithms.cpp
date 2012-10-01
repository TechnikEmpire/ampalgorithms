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
 * This file contains the test driver
 *---------------------------------------------------------------------------*/
#define NOMINMAX

#include <vector>
#include <algorithm>
#include <iostream>
#include <amp_stl_algorithms.h>

using namespace concurrency;
using namespace amp_stl_algorithms;

// TODO: replace this with a unit test framework

void test_for_each_no_return()
{
	std::vector<int> vec(1024);
	std::fill(vec.begin(), vec.end(), 2);
	array_view<const int> av(1024, vec);
	int sum = 0;
	array_view<int> av_sum(1, &sum);
	amp_stl_algorithms::for_each_no_return(begin(av), end(av), [av_sum] (int val) restrict(amp) {
		atomic_fetch_add(&av_sum(0), val);
	});
	av_sum.synchronize();
	assert(sum == 1024 * 2);
}

void test_find()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	auto iter = amp_stl_algorithms::find(begin(av), end(av), 3);
	int position = std::distance(begin(av), iter);
	assert(position == 1);

	iter = amp_stl_algorithms::find(begin(av), end(av), 17);
	assert(iter == end(av));
}

void test_none_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == true);
	bool r2 = amp_stl_algorithms::none_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == false);
}

void test_any_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == false);
	bool r2 = amp_stl_algorithms::any_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == true);
}

void test_all_of()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2 };
	static const int n = sizeof(numbers)/sizeof(numbers[0]);

	array_view<const int> av(extent<1>(n), numbers);
	bool r1 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>10; });
	assert(r1 == false);
	bool r2 = amp_stl_algorithms::all_of(begin(av), end(av), [] (int v) restrict(amp) -> bool { return v>5; });
	assert(r2 == false);
}

void test_count()
{
	static const int numbers[] = {1 , 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
	static const int n = sizeof(numbers)/sizeof(numbers[0]);
	array_view<const int> av(extent<1>(n), numbers);
	auto r1 = amp_stl_algorithms::count(begin(av), end(av), 2);
	assert(r1 == 5);
	auto r2 = amp_stl_algorithms::count(begin(av), end(av), 17);
	assert(r2 == 0);
}

void test_begin_end_array_view()
{
	std::vector<int> v1(6);
	array_view<int> a1(6, v1);
	auto iter1 = begin(a1);

	array_view<const int> ar1 = a1;
	auto iter2 = begin(ar1);

	auto iter3 = iter1++;
	auto iter4 = ++iter1;
	auto iter5 = iter3 + 7;
	bool res = iter3 < iter4;
	assert(res);
}

void test_random_access_iterator()
{
	std::vector<int> v1(16);
	array_view<int> a1(16, v1);

	// can be default constructed.
	array_view_iterator<int> iter1; 
	auto iter2 = array_view_iterator<double>();

	// can be copy constructed
	array_view_iterator<int> iter3 = begin(a1);
	array_view_iterator<int> iter4(iter3);
	array_view_iterator<int> iter5 = iter4;
	
	// assignment
	iter5 = iter3;

	// equality/inequality comparisons
	bool res = iter3 == iter5;
	assert(res);
	iter3++;
	res = iter3 != iter4;
	assert(res);

	// dereference
	*iter3 = 10;
	assert(a1[1] == 10);
	
	// offset derference operator;
	iter3[2] = 5;
	assert(a1[1 + 2] == 5);

	// increment, decrement, + , -, +=, -=
	auto iter6 = iter3;
	auto iter7 = iter3;
	iter6++;
	iter6 = iter6 + 1;
	iter7 += 2;
	assert(iter6 == iter7);
	--iter6;
	--iter6;
	iter7 = iter7 - 2;
	assert(iter6 == iter7);
	iter7 = iter7 - 1;
	iter6 -= 1;
	assert(iter6 == iter7);

	// <, >, <= >=
	iter6 = iter3;
	iter7 = iter3 + 1;
	assert(iter6 < iter7);
	assert(iter6 <= iter7);
	assert(iter7 > iter6);
	assert(iter7 >= iter6);

	// *i++
	iter6 = begin(a1);
	*iter6 = 3;
	assert(a1[0] == 3);
	int x1 = *iter6++;
	assert(x1 == 3);
	*iter6++ = 7;
	assert(a1[1] == 7);
}

void test_random_access_iterator_in_amp()
{

	std::vector<int> v1(16);
	array_view<int> a1(16, v1);
	std::vector<int> v2(16);
	array_view<int> result(16, v2);
	result.discard_data();
	parallel_for_each(extent<1>(1), [=] (index<1> idx) restrict(amp) {
		int id = 1;

		// can be default constructed.
		array_view_iterator<int> iter1; 
		auto iter2 = array_view_iterator<double>();

		// can be copy constructed
		array_view_iterator<int> iter3 = begin(a1);
		array_view_iterator<int> iter4(iter3);
		array_view_iterator<int> iter5 = iter4;
	
		// assignment
		iter5 = iter3;

		// equality/inequality comparisons
		bool res = iter3 == iter5;
		result[id++] = res;
		iter3++;
		res = iter3 != iter4;
		result[id++] = res;

		// dereference
		*iter3 = 10;
		result[id++] = (a1[1] == 10);
	
		// offset derference operator;
		iter3[2] = 5;
		result[id++] = (a1[1 + 2] == 5);

		// increment, decrement, + , -, +=, -=
		auto iter6 = iter3;
		auto iter7 = iter3;
		iter6++;
		iter6 = iter6 + 1;
		iter7 += 2;
		result[id++] = (iter6 == iter7);
		--iter6;
		--iter6;
		iter7 = iter7 - 2;
		result[id++] = (iter6 == iter7);
		iter7 = iter7 - 1;
		iter6 -= 1;
		result[id++] = (iter6 == iter7);

		// <, >, <= >=
		iter6 = iter3;
		iter7 = iter3 + 1;
		result[id++] = (iter6 < iter7);
		result[id++] = (iter6 <= iter7);
		result[id++] = (iter7 > iter6);
		result[id++] = (iter7 >= iter6);

		// *i++
		iter6 = begin(a1);
		*iter6 = 3;
		result[id++] = (a1[0] == 3);
		int x1 = *iter6++;
		result[id++] = (x1 == 3);
		*iter6++ = 7;
		result[id++] = (a1[1] == 7);
		result[0] = id - 1;
	});
	result.synchronize();
	assert(v2[0] <= (int)v2.size() - 1);
	for (int i = 0; i < v2[0]; i++)
	{
		assert(v2[1 + i] == 1);
	}
}

void test_generate()
{
	std::vector<int> vec(1024);

	// Generate using an array_view over the vector. Requires explicit synchronize.
	array_view<int> av(1024, vec);
	av.discard_data();

	amp_stl_algorithms::generate(begin(av), end(av), [] () restrict(amp) {
		return 7;
	});
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 7);
	});
}

void test_generate_n()
{
	std::vector<int> vec(1024);
	array_view<int> av(1024, vec);
	av.discard_data();

	amp_stl_algorithms::generate_n(begin(av), av.extent.size(), [] () restrict(amp) {
		return 616;
	});
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 616);
	});
}

void test_unary_transform()
{
	const int size = 1024;
	std::vector<int> vec_in(size);
	std::fill(begin(vec_in), end(vec_in), 7);
	array_view<const int> av_in(size, vec_in);

	std::vector<int> vec_out(size);
	array_view<int> av_out(size, vec_out);

	// Test "transform" by doubling the input elements

	amp_stl_algorithms::transform(begin(av_in), end(av_in), begin(av_out), [] (int x) restrict(amp) {
		return 2 * x;
	});
	av_out.synchronize();

	std::for_each(begin(vec_out), end(vec_out), [] (int element) {
		assert(element == 2*7);
	});
}

void test_binary_transform()
{
	const int size = 1024;

	std::vector<int> vec_in1(size);
	std::fill(begin(vec_in1), end(vec_in1), 343);
	array_view<const int> av_in1(size, vec_in1);

	std::vector<int> vec_in2(size);
	std::fill(begin(vec_in2), end(vec_in2), 323);
	array_view<const int> av_in2(size, vec_in2);

	std::vector<int> vec_out(size);
	array_view<int> av_out(size, vec_out);

	// Test "transform" by adding the two input elements

	amp_stl_algorithms::transform(begin(av_in1), end(av_in1), begin(av_in2), begin(av_out), [] (int x1, int x2) restrict(amp) {
		return x1 + x2;
	});
	av_out.synchronize();

	std::for_each(begin(vec_out), end(vec_out), [] (int element) {
		assert(element == 343 + 323);
	});
}

void test_fill()
{
	std::vector<int> vec(1024);

	// Fill using an array_view iterator
	array_view<int> av(1024, vec);
	av.discard_data();

	amp_stl_algorithms::fill(begin(av), end(av), 7);
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 7);
	});
}

void test_fill_n()
{
	std::vector<int> vec(1024);
	array_view<int> av(1024, vec);
	av.discard_data();

	amp_stl_algorithms::fill_n(begin(av), av.extent.size(), 616);
	av.synchronize();

	std::for_each(begin(vec), end(vec), [] (int element) {
		assert(element == 616);
	});
}

void test_reduce1()
{
	static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
	static const int n = sizeof(numbers)/sizeof(numbers[0]);
	array_view<const int> av(extent<1>(n), numbers);
	auto result = amp_stl_algorithms::reduce(begin(av), end(av), 0);
	assert(result == 66);
}

void test_reduce2()
{
	static const int numbers[] = {1, 3, 6, 3, 2, 2, 7, 8, 2, 9, 2, 19, 2};
	static const int n = sizeof(numbers)/sizeof(numbers[0]);
	array_view<const int> av(extent<1>(n), numbers);
	auto result = amp_stl_algorithms::reduce(begin(av), end(av), 0, [](int a, int b) restrict(cpu, amp) {
		return (a < b) ? b : a;
	});
	assert(result == 19);
}

int main()
{
	test_begin_end_array_view();
	test_random_access_iterator();
	test_random_access_iterator_in_amp();
	test_for_each_no_return();
	test_find();
	test_none_of();
	test_all_of();
	test_any_of();
	test_count();
	test_generate();
	test_generate_n();
	test_unary_transform();
	test_binary_transform();
	test_fill();
	test_fill_n();
	test_reduce1();
	test_reduce2();
}