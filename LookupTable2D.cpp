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

bool LookupTable2D::GetIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	size_t* outIndex,
	std::string* outErrMsg) const
{
	return LookupTableND::GetIndexAt({ aDim1Index,aDim2Index }, outIndex, outErrMsg);
}

bool LookupTable2D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	double* outValue,
	string* outErrMsg) const
{
	return LookupTableND::LookupByIndices({ aDim1Index, aDim2Index }, outValue, outErrMsg);
}

bool LookupTable2D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value,
	double* outValue,
	string* outErrMsg) const
{
	if (!CheckOutParam(outValue, outErrMsg))
		return false;

	// Retrieve the low index and percent progress data corresponding to the query and
	// store for later use during interpolation.
	size_t low0, low1;
	double prc0, prc1;
	if (!GetPositionInfo(0, aDim1Value, &low0, &prc0, outErrMsg)
     || !GetPositionInfo(1, aDim2Value, &low1, &prc1, outErrMsg))
	{
		return false;
	}

	// Get all surrounding values to interpolate between (e.g. if needing a value at
	// (1.2,2.7), get (1,2), (1,3), (2,2), and (2,3), then interpolate with the percent
	// progresses above).
	double LL, LH, HL, HH;
	if (!LookupByIndices(low0, low1, &LL, outErrMsg)
		|| !LookupByIndices(low0, low1 + 1, &LH, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1, &HL, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1 + 1, &HH, outErrMsg))
	{
		return false;
	}

	// Interpolate between the surrounding positions until finding the final value
	double L = utils::Lerp(LL, LH, prc1);
	double H = utils::Lerp(HL, HH, prc1);
	*outValue = utils::Lerp(L, H, prc0);
	return true;
}

bool LookupTable2D::LookupByValues(const vector<double>& aValueInputs,
	double* outValue,
	string* outErrMsg) const
{
	if (!ValidInput(aValueInputs, outValue, outErrMsg))
		return false;
	return LookupByValues(aValueInputs.at(0), aValueInputs.at(1), outValue, outErrMsg);
}

Result<size_t> LookupTable2D::GetIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::GetIndexAt({ aDim1Index, aDim2Index });
}

Result<double> LookupTable2D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index) const
{
	return LookupTableND::LookupByIndices({ aDim1Index, aDim2Index });
}

Result<double> LookupTable2D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value) const
{
	double val;
	string err;
	if (LookupByValues(aDim1Value, aDim2Value, &val, &err))
		return Result<double>(val);
	return err;
}

Result<double> LookupTable2D::LookupByValues(const std::vector<double>& aValueInputs) const
{
	double val;
	string err;
	if (!ValidInput(aValueInputs, &val, &err))
		return err;
	if(!LookupByValues(aValueInputs.at(0), aValueInputs.at(1), &val, &err))
		return err;
	return Result<double>(val);
}