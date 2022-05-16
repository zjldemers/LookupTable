/////////////////////////////////////////////////////////////////////////////////////////////
// This file is part of LookupTable <https://github.com/zjldemers/LookupTable>.
// 
// MIT License
// Copyright (c) 2022 Zachary J. L. Demers
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "LookupTable2D.h"
#include <stdexcept>

using namespace zjld; // feel free to remove/rename as the license above allows
using std::string;
using std::vector;
using utils::Result;


bool LookupTable2D::IsValidSourceData(const TableDataSet& aFullDataSet) const
{
	if (aFullDataSet.size() != 3)
		return false; // must have 2 indep data and 1 dep data
	// Dep data must be last element and must be exactly size of indep data product
	// e.g. a 2x3 indep data set requires a dep data of size exactly 6
	return (aFullDataSet[0].size() * aFullDataSet[1].size() == aFullDataSet[2].size()
		&& CheckMonotonicallyIncreasing(aFullDataSet));
}


size_t LookupTable2D::LookupIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::LookupIndexAt({ aDim1Index, aDim2Index });
}
bool LookupTable2D::QueryIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	size_t* outIndex,
	std::string* outErrMsg) const
{
	return LookupTableND::QueryIndexAt({ aDim1Index,aDim2Index }, outIndex, outErrMsg);
}
Result<size_t> LookupTable2D::QueryIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::QueryIndexAt({ aDim1Index, aDim2Index });
}




double LookupTable2D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::LookupByIndices({ aDim1Index, aDim2Index });
}
bool LookupTable2D::QueryByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	double* outValue,
	string* outErrMsg) const
{
	return LookupTableND::QueryByIndices({ aDim1Index, aDim2Index }, outValue, outErrMsg);
}
Result<double> LookupTable2D::QueryByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::QueryByIndices({ aDim1Index, aDim2Index });
}




double LookupTable2D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");

	// Retrieve the low index and percent progress data corresponding to the query and
	// store for later use during interpolation.
	size_t low0, low1;
	double prc0, prc1;
	GetPositionInfo(0, aDim1Value, &low0, &prc0);
	GetPositionInfo(1, aDim2Value, &low1, &prc1);

	// Get all surrounding values to interpolate between (e.g. if needing a value at
	// (1.2,2.7), get (1,2), (1,3), (2,2), and (2,3), then interpolate with the percent
	// progresses above).
	double LL = LookupByIndices(low0, low1);
	double LH = LookupByIndices(low0, low1 + 1);
	double HL = LookupByIndices(low0 + 1, low1);
	double HH = LookupByIndices(low0 + 1, low1 + 1);

	// Interpolate between the surrounding positions until finding the final value
	double L = utils::Lerp(LL, LH, prc1);
	double H = utils::Lerp(HL, HH, prc1);
	return utils::Lerp(L, H, prc0);
}
double LookupTable2D::LookupByValues(const std::vector<double>& aValueInputs) const
{
	if (aValueInputs.size() != 2)
		throw std::invalid_argument("Must provide one input per independent variable.");
	return LookupByValues(aValueInputs.at(0), aValueInputs.at(1));
}
bool LookupTable2D::QueryByValues(const double& aDim1Value,
	const double& aDim2Value,
	double* outValue,
	string* outErrMsg) const
{
	if (nullptr == outValue || nullptr == outErrMsg)
		return false;
	try {
		*outValue = LookupByIndices(aDim1Value, aDim2Value);
	}
	catch (std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
bool LookupTable2D::QueryByValues(const vector<double>& aValueInputs,
	double* outValue,
	string* outErrMsg) const
{
	if (nullptr == outValue || nullptr == outErrMsg)
		return false;
	try {
		*outValue = LookupByValues(aValueInputs);
	}
	catch (std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
Result<double> LookupTable2D::QueryByValues(const double& aDim1Value,
	const double& aDim2Value) const
{
	try {
		return Result<double>(LookupByValues(aDim1Value, aDim2Value));
	}
	catch (std::exception& e) {
		return e.what();
	}
}
Result<double> LookupTable2D::QueryByValues(const std::vector<double>& aValueInputs) const
{
	try {
		return Result<double>(LookupByValues(aValueInputs));
	}
	catch (std::exception& e) {
		return e.what();
	}
}