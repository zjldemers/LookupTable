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

#include "LookupTableND.h"

using namespace zjld; // feel free to remove/rename as the license above allows
using std::string;
using std::vector;
using utils::Result;


// ==== Begin Section: Construction/Destruction (Public) ==== //
LookupTableND::LookupTableND()
{
	ResetData();
}

LookupTableND::LookupTableND(const TableDataSet& aFullDataSet)
{
	PopulateData(aFullDataSet);
}

LookupTableND::LookupTableND(const TableDataSet& aIndepDataSet,
	const TableData& aDepData)
{
	PopulateData(aIndepDataSet, aDepData);
}
// ==== End Section: Construction/Destruction (Public) ==== //




// ==== Begin Section: Data Population (Public) ==== //
void LookupTableND::ResetData()
{
	_indepData = {};
	_depData = {};
	_valid = false;
}

bool LookupTableND::IsValidSourceData(const TableDataSet& aFullDataSet) const
{
	// Valid only if the last element (dependent data) has a size equal to the size of each
	//   independent data array multiplied together (e.g. a 2x3x4 needs a size 24 dep data)
	if (aFullDataSet.size() < 3)
		return false; // 1D tables are not implemented (1 indep, 1 dep -> size=2)
	size_t reqSize = 1;
	for (size_t i = 0; i < aFullDataSet.size() - 1; i++) {
		reqSize *= aFullDataSet.at(i).size();
	}
	if (reqSize != aFullDataSet.back().size())
		return false; // dimensions must lineup
	return CheckMonotonicallyIncreasing(aFullDataSet);
}

bool LookupTableND::IsValidSourceData(const TableDataSet& aIndepDataSet,
	const TableData& aDepData) const
{
	if (aIndepDataSet.size() < 2)
		return false; // 1D tables are not implemented
	TableDataSet fullData = aIndepDataSet;
	fullData.push_back(aDepData);
	return IsValidSourceData(fullData);
}

bool LookupTableND::PopulateData(const TableDataSet& aFullDataSet)
{
	if (IsValidSourceData(aFullDataSet)) {
		_indepData = TableDataSet(aFullDataSet.begin(), aFullDataSet.end() - 1);
		_depData = aFullDataSet.back();
		_valid = true;
	}
	else {
		ResetData();
	}
	return _valid;
}

bool LookupTableND::PopulateData(const TableDataSet& aIndepDataSet,
	const TableData& aDepData)
{
	TableDataSet fullData = aIndepDataSet;
	fullData.push_back(aDepData);
	return PopulateData(fullData);
}
// ==== End Section: Data Population (Public) ==== //



// ==== Begin Section: Lookup Methods (Public) ==== //
bool LookupTableND::GetIndexAt(const vector<size_t>& aInputs,
	size_t* outIndex,
	string* outErrMsg) const
{
	if (!ValidInput(aInputs, outIndex, outErrMsg))
		return false;
	size_t idx = 0, prod = 1, tmpSize;
	for (size_t i = 0; i < aInputs.size(); i++) {
		tmpSize = _indepData.at(i).size();
		if (aInputs.at(i) >= tmpSize) {
			*outErrMsg = "Invalid input: one or more indices were out of bounds.";
			return false;
		}
		idx += aInputs.at(i) * prod;
		prod *= tmpSize;
	}
	// Index calculation follows the pattern: i + j*ni + k*nj*ni + l*nk*nj*ni + ...
	if (idx >= _depData.size()) { // double check just in case, but shouldn't ever happen
		*outErrMsg = "Calculated index out of bounds.";
		return false;
	}
	*outIndex = idx;
	return true;
}

Result<size_t> LookupTableND::GetIndexAt(const vector<size_t>& aInputs) const
{
	size_t idx;
	string err;
	if (GetIndexAt(aInputs, &idx, &err))
		return Result<size_t>(idx);
	return err;
}

bool LookupTableND::LookupByIndices(const vector<size_t>& aIndexInputs,
	double* outValue,
	string* outErrMsg) const
{
	size_t idx;
	if (!GetIndexAt(aIndexInputs, &idx, outErrMsg))
		return false;
	if (nullptr == outValue) {
		*outErrMsg = "Provided out parameter was a nullptr.";
		return false;
	}
	*outValue = _depData.at(idx);
	return true;
}

Result<double> LookupTableND::LookupByIndices(const vector<size_t>& aIndexInputs) const
{
	double val;
	string err;
	if (LookupByIndices(aIndexInputs, &val, &err))
		return Result<double>(val);
	return err;
}

bool LookupTableND::LookupByValues(const vector<double>& aValueInputs,
	double* outValue,
	string* outErrMsg) const
{
	if (!ValidInput(aValueInputs, outValue, outErrMsg))
		return false;

	const size_t kInSize = _indepData.size(); // shorthand
	vector<size_t> lowIdxs = vector<size_t>(kInSize);
	vector<double> prcPrgs = vector<double>(kInSize);
	size_t comboCount = 1; // number of value combinations required for interpolation later
	for (size_t i = 0; i < kInSize; i++) {
		if (!GetPositionInfo(i, aValueInputs.at(i), &lowIdxs.at(i), &prcPrgs.at(i), outErrMsg))
			return false;
		comboCount <<= 1; // number of combinations increases by power of two per input
	}

	vector<size_t> inps = vector<size_t>(lowIdxs);	  // inputs for each interpolation
	vector<bool> bits = vector<bool>(kInSize, false); // used to modify inps programatically
	vector<double> vals = vector<double>(comboCount); // will hold all interpolated values
	for (size_t i = 0; i < comboCount; i++) {
		if (!LookupByIndices(inps, &vals.at(i), outErrMsg))
			return false;

		// Vary inputs programmatically, following a binary counter flipping between the low
		//	index value found above, and the index immediately following that one
		// e.g. for 3 inputs: low,low,low; low,low,low+1; low,low+1,low; low,low+1,low+1; ...
		bool flipped = false;
		for (int k = kInSize - 1; k >= 0; k--) {
			if (!flipped) {
				bits.at(k) = !bits.at(k);
				flipped = bits.at(k);
			}
			inps.at(k) = lowIdxs.at(k) + static_cast<size_t>(bits.at(k));
		}
	}

	// Work down through the inputs above, interpolating their results together
	for (size_t i = 0; i < kInSize; i++) {
		for (size_t j = 1; j < comboCount; j += 2) {
			vals.at(j / 2) = utils::Lerp(vals.at(j - 1), vals.at(j),
										 prcPrgs.at(prcPrgs.size() - i - 1));
		}
		comboCount >>= 1;
		// This process interpolates all combinations of intermediate interpoloations of any
		// number of dimensions by condensing the vals vector from back to front until the 
		// final interpolated value is calculated and stored at vals[0]
	}
	*outValue = vals.front();
	return true;
}


Result<double> LookupTableND::LookupByValues(const vector<double>& aValueInputs) const
{
	double val;
	string err;
	if (LookupByValues(aValueInputs, &val, &err))
		return Result<double>(val);
	return err;
}
// ==== End Section: Lookup Methods (Public) ==== //



// ==== Begin Section: Metadata (Public) ==== //
bool LookupTableND::Valid() const 
{ 
	return _valid; 
};
size_t LookupTableND::Dimensions() const 
{ 
	return _indepData.size(); 
}
size_t LookupTableND::DepDataSize() const 
{ 
	return _depData.size(); 
};
size_t LookupTableND::IndepDataSize(const size_t& aDimension) const 
{
	if (aDimension < Dimensions())
		return _indepData.at(aDimension).size();
	return 0; // out of bounds request, no size "in that dimension"
}
// ==== End Section: Metadata (Public) ==== //



// ==== Begin Section: Position Helpers (Protected) ==== //
bool LookupTableND::GetPositionInfo(const size_t& aDimension,
	const double& aValue,
	size_t* outLowIdx,
	double* outPercProgress,
	string* outErrMsg) const
{
	if (!CheckOutParams({ outLowIdx, outPercProgress }, outErrMsg))
		return false;
	double pos;
	if (!GetApproxPos(aDimension, aValue, &pos, outErrMsg))
		return false;
	
	// Take approx position and find the index beneath it and the percent progress to the
	// next index.  For example, pos=1.3, low=1, perc=0.3.
	*outLowIdx = static_cast<size_t>(pos);
	*outPercProgress = pos - static_cast<double>(*outLowIdx);
	if (*outLowIdx > 0 && *outLowIdx == _indepData.at(aDimension).size() - 1) {
		// If value is found at last index, reduce by one since it's the "low" value so
		// that checking the "high" value later will not go out of bounds, and use a
		// percent progress of 100% (1.0) to show that it's actually at the next value
		(*outLowIdx)--;
		*outPercProgress += 1.0; // +=1.0 instead of =1.0 to allow for extrapolation
	}
	return true;
}

bool LookupTableND::GetApproxPos(const size_t& aDimension,
	const double& aValue,
	double* outApproxPosition,
	string* outErrMsg) const
{
	if (!CheckOutParam(outApproxPosition, outErrMsg))
		return false;
	if (aDimension >= _indepData.size()) {
		*outErrMsg = "Invalid input: invalid dimension.";
		return false;
	}
	const TableData& data = _indepData.at(aDimension);
	if (data.empty()) {
		*outErrMsg = "Data vector is empty.";
		return false;
	}
	if (aValue < data.front() || aValue > data.back()) {
		*outErrMsg = "Invalid input: value given is outside of data bounds.";
		return false;
	}

	// Use a binary search to find the value searched for
	// - NOTE: this is why the independent data must be monotonically increasing
	size_t l = 0, r = data.size(); // left/right ends
	size_t m = (l + r) / 2;        // mid-point
	while (l < m && m < r) {
		if (utils::IsApproxEqual(aValue, data.at(m))) {
			l = m;
			r = m;
			break; // found it
		}
		if (aValue < data.at(m))
			r = m;
		else
			l = m;
		m = (l + r) / 2;
	}
	if (l == r) // found exact match
		*outApproxPosition = static_cast<double>(l);
	else {      // Otherwise: interpolate
		double perc = utils::ILerp(data.at(l), data.at(r), aValue);
		*outApproxPosition = static_cast<double>(l) + perc;
	}
	return true;
}
// ==== End Section: Position Helpers (Protected) ==== //




// ==== Begin Section: Validity Helpers (Protected) ==== //
bool LookupTableND::CheckMonotonicallyIncreasing(const TableDataSet& aFullDataSet) const
{
	for (size_t i = 0; i < aFullDataSet.size() - 1; i++) {
		const TableData& data = aFullDataSet.at(i); // shorthand
		for (size_t j = 1; j < data.size(); j++) {
			if (data.at(j - 1) >= data.at(j))
				return false;
		}
	}
	return true;
}
bool LookupTableND::CheckTableValidity(std::string* outErrMsg) const
{
	if (nullptr == outErrMsg)
		return false;
	if (!_valid) {
		*outErrMsg = "Unable to operate on invalid table.";
		return false;
	}
	return true;
}

bool LookupTableND::CheckOutParams(std::initializer_list<void*> outParams,
	std::string* outErrMsg) const
{
	if (!CheckTableValidity(outErrMsg))
		return false;
	for (void* p : outParams) {
		if (nullptr == p) {
			*outErrMsg = "Provided nullptr as out parameter.";
			return false;
		}
	}
	return true;
}
bool LookupTableND::CheckOutParam(void* outParam,
	std::string* outErrMsg) const
{
	return CheckOutParams({ outParam }, outErrMsg);
}
// ==== End Section: Validity Helpers (Protected) ==== //
