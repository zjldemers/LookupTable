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

bool LookupTable3D::GetIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index,
	size_t* outIndex,
	std::string* outErrMsg) const
{
	return LookupTableND::GetIndexAt({ aDim1Index,aDim2Index,aDim3Index }, 
										outIndex, outErrMsg);
}

bool LookupTable3D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index,
	double* outValue,
	string* outErrMsg) const
{
	return LookupTableND::LookupByIndices({ aDim1Index,aDim2Index,aDim3Index }, 
											outValue, outErrMsg);
}

bool LookupTable3D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value,
	const double& aDim3Value,
	double* outValue,
	string* outErrMsg) const
{
	if (!CheckOutParam(outValue, outErrMsg))
		return false;

	// Retrieve the low index and percent progress data corresponding to the query and
	// store for later use during interpolation.
	size_t low0, low1, low2;
	double prc0, prc1, prc2;
	if (!GetPositionInfo(0, aDim1Value, &low0, &prc0, outErrMsg)
		|| !GetPositionInfo(1, aDim2Value, &low1, &prc1, outErrMsg)
		|| !GetPositionInfo(2, aDim3Value, &low2, &prc2, outErrMsg))
	{
		return false;
	}

	// Get all surrounding values to interpolate between (e.g. if needing a value at
	// (1.2,2.7,0.3), get (1,2,0), (1,3,0), (2,2,0), (2,3,0), (1,2,1), (1,3,1), (2,2,1),
	// and (2,3,1) then interpolate with the percent progresses above).
	double LLL, LLH, LHL, LHH, HLL, HLH, HHL, HHH;
	if (!LookupByIndices(low0, low1, low2, &LLL, outErrMsg)
		|| !LookupByIndices(low0, low1, low2 + 1, &LLH, outErrMsg)
		|| !LookupByIndices(low0, low1 + 1, low2, &LHL, outErrMsg)
		|| !LookupByIndices(low0, low1 + 1, low2 + 1, &LHH, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1, low2, &HLL, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1, low2 + 1, &HLH, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1 + 1, low2, &HHL, outErrMsg)
		|| !LookupByIndices(low0 + 1, low1 + 1, low2 + 1, &HHH, outErrMsg))
	{
		return false;
	}

	// Interpolate between the surrounding positions until finding the final value
	double LL = utils::Lerp(LLL, LLH, prc2);
	double LH = utils::Lerp(LHL, LHH, prc2);
	double HL = utils::Lerp(HLL, HLH, prc2);
	double HH = utils::Lerp(HHL, HHH, prc2);
	double L = utils::Lerp(LL, LH, prc1);
	double H = utils::Lerp(HL, HH, prc1);
	*outValue = utils::Lerp(L, H, prc0);
	return true;
}

bool LookupTable3D::LookupByValues(const vector<double>& aValueInputs,
	double* outValue,
	string* outErrMsg) const
{
	if (!ValidInput(aValueInputs, outValue, outErrMsg))
		return false;
	return LookupByValues(aValueInputs.at(0), aValueInputs.at(1), aValueInputs.at(2),
							outValue, outErrMsg);
}

Result<size_t> LookupTable3D::GetIndexAt(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::GetIndexAt({ aDim1Index, aDim2Index, aDim3Index });
}

Result<double> LookupTable3D::LookupByIndices(const size_t& aDim1Index,
	const size_t& aDim2Index,
	const size_t& aDim3Index) const
{
	return LookupTableND::LookupByIndices({ aDim1Index, aDim2Index, aDim3Index });
}

Result<double> LookupTable3D::LookupByValues(const double& aDim1Value,
	const double& aDim2Value,
	const double& aDim3Value) const
{
	double val;
	string err;
	if (LookupByValues(aDim1Value, aDim2Value, aDim3Value, &val, &err))
		return Result<double>(val);
	return err;
}

Result<double> LookupTable3D::LookupByValues(const std::vector<double>& aValueInputs) const
{
	double val;
	string err;
	if (!ValidInput(aValueInputs, &val, &err))
		return err;
	if (!LookupByValues(aValueInputs.at(0), aValueInputs.at(1), aValueInputs.at(2), &val, &err))
		return err;
	return Result<double>(val);
}