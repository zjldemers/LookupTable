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
#include <stdexcept>

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
size_t LookupTableND::LookupIndexAt(const std::vector<size_t>& aInputs) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");
	if (aInputs.size() != _indepData.size())
		throw std::invalid_argument("Must provide one input per independent variable.");
	size_t idx = 0, prod = 1, tmpSize;
	for (size_t i = 0; i < aInputs.size(); i++) {
		tmpSize = _indepData.at(i).size();
		if (aInputs.at(i) >= tmpSize)
			throw std::invalid_argument("Input " + std::to_string(i) + " of value " 
				+ std::to_string(aInputs.at(i)) + " out of bounds[0, " 
				+ std::to_string(tmpSize-1) + "].");
		idx += aInputs.at(i) * prod;
		prod *= tmpSize;
	}
	// Index calculation follows the pattern: i + j*ni + k*nj*ni + l*nk*nj*ni + ...
	if (idx >= _depData.size()) // double check just in case, but shouldn't ever happen
		throw std::exception("Calculated index out of bounds.");
	return idx;
}

bool LookupTableND::QueryIndexAt(const vector<size_t>& aInputs,
	size_t* outIndex,
	string* outErrMsg) const
{
	if(nullptr == outIndex || nullptr == outErrMsg)
		return false;
	try {
		*outIndex = LookupIndexAt(aInputs);
	}
	catch(std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
Result<size_t> LookupTableND::QueryIndexAt(const vector<size_t>& aInputs) const
{
	try {
		return Result<size_t>(LookupIndexAt(aInputs));
	}
	catch(std::exception& e) {
		return e.what();
	}
}


double LookupTableND::LookupByIndices(const vector<size_t>& aIndexInputs) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");
	return _depData.at(LookupIndexAt(aIndexInputs));
}
bool LookupTableND::QueryByIndices(const vector<size_t>& aIndexInputs,
	double* outValue,
	string* outErrMsg) const
{
	if(nullptr == outValue || nullptr == outErrMsg)
		return false;
	try {
		*outValue = LookupByIndices(aIndexInputs);
	}
	catch(std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
Result<double> LookupTableND::QueryByIndices(const vector<size_t>& aIndexInputs) const
{
	try {
		return Result<double>(LookupByIndices(aIndexInputs));
	}
	catch(std::exception& e) {
		return e.what();
	}
}


double LookupTableND::LookupByValues(const vector<double>& aValueInputs) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");

	const size_t kInSize = _indepData.size(); // shorthand
	vector<size_t> lowIdxs = vector<size_t>(kInSize);
	vector<double> prcPrgs = vector<double>(kInSize);
	size_t comboCount = 1; // number of value combinations required for interpolation later
	for (size_t i = 0; i < kInSize; i++) {
		GetPositionInfo(i, aValueInputs.at(i), &lowIdxs.at(i), &prcPrgs.at(i));
		comboCount <<= 1; // number of combinations increases by power of two per input
	}

	vector<size_t> inps = vector<size_t>(lowIdxs);	  // inputs for each interpolation
	vector<bool> bits = vector<bool>(kInSize, false); // used to modify inps programatically
	vector<double> vals = vector<double>(comboCount); // will hold all interpolated values
	for (size_t i = 0; i < comboCount; i++) {
		vals.at(i) = LookupByIndices(inps);

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
	return vals.front();
}
bool LookupTableND::QueryByValues(const vector<double>& aValueInputs,
	double* outValue,
	string* outErrMsg) const
{
	if (nullptr == outValue || nullptr == outErrMsg)
		return false;
	try {
		*outValue = LookupByValues(aValueInputs);
	}
	catch(std::exception& e) {
		*outErrMsg = e.what();
		return false;
	}
	return true;
}
Result<double> LookupTableND::QueryByValues(const vector<double>& aValueInputs) const
{
	try {
		return Result<double>(LookupByValues(aValueInputs));
	}
	catch(std::exception& e) {
		return e.what();
	}
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
	if (aDimension >= Dimensions())
		throw std::invalid_argument("Invalid dimension provided: " + std::to_string(aDimension));
	return _indepData.at(aDimension).size();
}
// ==== End Section: Metadata (Public) ==== //



// ==== Begin Section: Position Helpers (Protected) ==== //
void LookupTableND::GetPositionInfo(const size_t& aDimension,
	const double& aValue,
	size_t* outLowIdx,
	double* outPercProgress) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");
	if (nullptr == outLowIdx || nullptr == outPercProgress)
		throw std::invalid_argument("Null pointer provided as input.");
	
	double pos;
	GetApproxPos(aDimension, aValue, &pos);
	
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
}

void LookupTableND::GetApproxPos(const size_t& aDimension,
	const double& aValue,
	double* outApproxPosition) const
{
	if (!_valid)
		throw std::exception("Unable to operate on invalid table.");
	if (nullptr == outApproxPosition)
		throw std::invalid_argument("Null pointer provided as input.");
	if (aDimension >= _indepData.size())
		throw std::invalid_argument("Invalid dimension (" + std::to_string(aDimension) + ") provided to " + std::to_string(_indepData.size()) + "-dimensional table.");

	const TableData& data = _indepData.at(aDimension);
	if (data.empty())
		throw std::exception("Data vector is empty.");
	if (aValue < data.front() || aValue > data.back())
		throw std::invalid_argument("Value given is outside of data bounds. (Extrapolation not supported.)");

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
// ==== End Section: Validity Helpers (Protected) ==== //
