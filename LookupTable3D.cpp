/////////////////////////////////////////////////////////////////////////////////////////////
// This file is part of LookupTable <https://github.com/zjldemers/LookupTableND>.
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

#include "LookupTable3D.h"
#include <stdexcept>

using namespace zjld; // feel free to remove/rename as the license above allows
using std::string;
using std::vector;
using utils::Result;


bool LookupTable3D::IsValidSourceData(const TableDataSet& aData) const
{
	if (aData.size() != 4)
		return false; // must have 3 indep data and 1 dep data
	// Dep data must be last element and must be exactly size of indep data product
	// e.g. a 2x3x4 indep data set requires a dep data of size exactly 24
	return (aData[0].size() * aData[1].size() * aData[2].size() == aData[3].size()
		&& CheckMonotonicallyIncreasing(aData));
}



size_t LookupTable3D::LookupIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::LookupIndexAt({ aDim1Index, aDim2Index, aDim3Index });
}
bool LookupTable3D::QueryIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index,
	size_t* outIndex,
	std::string* outErrMsg) const
{
	return LookupTableND::QueryIndexAt({ aDim1Index,aDim2Index,aDim3Index }, 
										outIndex, outErrMsg);
}
Result<size_t> LookupTable3D::QueryIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::QueryIndexAt({ aDim1Index, aDim2Index, aDim3Index });
}





double LookupTable3D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::LookupByIndices({ aDim1Index, aDim2Index, aDim3Index });
}
bool LookupTable3D::QueryByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index,
	double* outValue,
	string* outErrMsg) const
{
	return LookupTableND::QueryByIndices({ aDim1Index,aDim2Index,aDim3Index },
											outValue, outErrMsg);
}
Result<double> LookupTable3D::QueryByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::QueryByIndices({ aDim1Index, aDim2Index, aDim3Index });
}



double LookupTable3D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value,
	const double& aDim3Value) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");

	// Retrieve the low index and percent progress data corresponding to the query and
	// store for later use during interpolation.
	size_t low0, low1, low2;
	double prc0, prc1, prc2;
	GetPositionInfo(0, aDim1Value, &low0, &prc0);
	GetPositionInfo(1, aDim2Value, &low1, &prc1);
	GetPositionInfo(2, aDim3Value, &low2, &prc2);

	// Get all surrounding values to interpolate between (e.g. if needing a value at
	// (1.2,2.7,0.3), get (1,2,0), (1,3,0), (2,2,0), (2,3,0), (1,2,1), (1,3,1), (2,2,1),
	// and (2,3,1) then interpolate with the percent progresses above).
	double LLL = LookupByIndices(low0, low1, low2);
	double LLH = LookupByIndices(low0, low1, low2 + 1);
	double LHL = LookupByIndices(low0, low1 + 1, low2);
	double LHH = LookupByIndices(low0, low1 + 1, low2 + 1);
	double HLL = LookupByIndices(low0 + 1, low1, low2);
	double HLH = LookupByIndices(low0 + 1, low1, low2 + 1);
	double HHL = LookupByIndices(low0 + 1, low1 + 1, low2);
	double HHH = LookupByIndices(low0 + 1, low1 + 1, low2 + 1);
	
	// Interpolate between the surrounding positions until finding the final value
	double LL = utils::Lerp(LLL, LLH, prc2);
	double LH = utils::Lerp(LHL, LHH, prc2);
	double HL = utils::Lerp(HLL, HLH, prc2);
	double HH = utils::Lerp(HHL, HHH, prc2);
	double L = utils::Lerp(LL, LH, prc1);
	double H = utils::Lerp(HL, HH, prc1);
	return utils::Lerp(L, H, prc0);
}
double LookupTable3D::LookupByValues(const vector<double>& aValueInputs) const
{
	if (aValueInputs.size() != 3)
		throw std::invalid_argument("Must provide one input per independent variable.");
	return LookupByValues(aValueInputs.at(0), aValueInputs.at(1), aValueInputs.at(2));
}
bool LookupTable3D::QueryByValues(const double& aDim1Value,
	const double& aDim2Value,
	const double& aDim3Value,
	double* outValue,
	string* outErrMsg) const
{
	if (nullptr == outValue || nullptr == outErrMsg)
		return false;
	try {
		*outValue = LookupByIndices(aDim1Value, aDim2Value, aDim3Value);
	}
	catch (std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
bool LookupTable3D::QueryByValues(const vector<double>& aValueInputs,
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
Result<double> LookupTable3D::QueryByValues(const double& aDim1Value,
	const double& aDim2Value,
	const double& aDim3Value) const
{
	try {
		return Result<double>(LookupByValues(aDim1Value, aDim2Value, aDim3Value));
	}
	catch (std::exception& e) {
		return e.what();
	}
}
Result<double> LookupTable3D::QueryByValues(const std::vector<double>& aValueInputs) const
{
	try {
		return Result<double>(LookupByValues(aValueInputs));
	}
	catch (std::exception& e) {
		return e.what();
	}
}