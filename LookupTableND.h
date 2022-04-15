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


#ifndef _ZJLD_LOOKUP_TABLE_ND_H_
#define _ZJLD_LOOKUP_TABLE_ND_H_

#include <string>
#include <vector>
#include "utils.h"


namespace zjld // feel free to remove/rename as the license above allows
{
	
	typedef std::vector<double>    TableData;
	typedef std::vector<TableData> TableDataSet;


	// This class implements the basics of a lookup table with 2 to N-dimensions.  Data
	// structures with only 1 dimension are left for more trivial implementations.
	// Linear interpolation is used between data points, while extrapolation is currently
	// not implemented but rather returns an error if attempting to access a table's data
	// outside of its defined limits.  With this, it also requires the independent data
	// to be monotonically increasing (common in many tables, but not all).
	// NOTE: If implementing a 2D or 3D table, this is slightly less efficient than derived
	// classes LookupTable2D and LookupTable3D, respectively, due to use of loops, etc.
	class LookupTableND
	{
	protected:
		TableDataSet _indepData; // vector of vectors of independent variable data
		TableData _depData;		 // vector of dependent variable data
		bool _valid;			 // current validity status of the table

	public:

	// ==== Begin Section: Construction/Destruction (Public) ==== //
		/* - Provided a valid data set, these will initialize the table appropriately and
		* set _valid (accessed with Valid()) to true
		* - Provided no arguments or an invalid dataset (e.g. mismatched dimensions, less
		* than 2 dimensions, etc.), these will set _valid to false
		* - Note: a valid data set must have 2 or more independent data vectors at its
		* front and 1 dependent data vector at the end whose length is equal to the product
		* of the independent data vectors 
		* (e.g. indep[0].size=2, indep[1].size=3 -> dep.size=6)
		*/
		LookupTableND();
		LookupTableND(const TableDataSet& aFullDataSet);
		LookupTableND(const TableDataSet& aIndepDataSet,
			const TableData& aDepData);
		virtual ~LookupTableND(){}
	// ==== End Section: Construction/Destruction (Public) ==== //


	// ==== Begin Section: Data Population (Public) ==== //
		/* These check if a given data set is valid to supply the source for this table,
		* return true if true, falase otherwise.
		*/
		virtual bool IsValidSourceData(const TableDataSet& aFullDataSet) const;
		bool IsValidSourceData(const TableDataSet& aIndepDataSet,
			const TableData& aDepData) const;

		/* These attempt to populate the table with the given data.  If unable to, the table
		* is reset using ResetData, resulting in the Valid flag being false.
		*/
		bool PopulateData(const TableDataSet& aFullDataSet);
		bool PopulateData(const TableDataSet& aIndepDataSet,
			const TableData& aDepData);

		/* This empties all data in the table and sets the Valid flag to be false.
		*/
		void ResetData();
	// ==== End Section: Data Population (Public) ==== //

		
	// ==== Begin Section: Lookup Methods (Public) ==== //
		/* 
		* Note: Each of these methods have two useages.  One uses out parameters to store
		* the result or error message, returning true if successful or false if not.  The
		* other utilizes the utils::Result<T> class to contain the result in one place.
		* If lookups will be performed frequently, the out variable method is recommended
		* as it will be more efficient (requiring less construction/destruction overhead),
		* but if it is only called on occasionally, either option should work well and are
		* therefore both provided to appease the various preferences we all tend to have.
		*/
		
		/* These find the index in the dependent data vector that corresponds to the given
		* independent data indicies - provided all prerequisites (valid table, within table
		* bounds, etc.) are met.
		*/
		bool GetIndexAt(const std::vector<size_t>& aInputs,
			size_t* outIndex,
			std::string* outErrMsg) const;
		utils::Result<size_t> GetIndexAt(const std::vector<size_t>& aInputs) const;

		/* - These return the exact value stored at the respective index (using
		* GetIndexAt) in the dependent data vector.
		*/
		bool LookupByIndices(const std::vector<size_t>& aIndexInputs,
			double* outValue,
			std::string* outErrMsg) const;
		utils::Result<double> LookupByIndices(const std::vector<size_t>& aIndexInputs) const;

		/* These return the value stored in the dependent data vector that corresponds to 
		* the given values (if an exact match, it will be as exact as a double can be) or an
		* approximated value of what would be found there based off of simple linear 
		* interpolation between the closest points.
		*/
		virtual bool LookupByValues(const std::vector<double>& aValueInputs,
			double* outValue,
			std::string* outErrMsg) const;
		virtual utils::Result<double> LookupByValues(const std::vector<double>& aValueInputs) const;
	// ==== End Section: Lookup Methods (Public) ==== //


	// ==== Begin Section: Metadata (Public) ==== //
		bool Valid() const;
		size_t Dimensions() const;  // _indepData.size (vector of vectors)
		size_t DepDataSize() const; // _depData.size
		size_t IndepDataSize(const size_t& aDimension) const; // _indepData[aDimension].size
	// ==== End Section: Metadata (Public) ==== //


	protected:
		
	// ==== Begin Section: Position Helpers (Protected) ==== //
		/* This returns true or false based on success of the operation, with the error
		* message out variable containing a descriptive reason as to why it failed.
		* The other out varibles are the index of the element in the depenedent data vector 
		* that is the closest value to the value given while being less than it (e.g lower 
		* bound), and the "percent progress" to the next index (see example below).
		* For example: if _depData = {1.2, 3.4,  5.6, 7.8}
		* - 4.3 -> aValue (between indices  ^  &  ^ [1] and [2])
		* - outLowIdx -> 1 (_depData[1]=3.4)
		* - outPercProgress -> 0.40909... (about 41% of the way from 3.4 to 5.6)
		*/
		bool GetPositionInfo(const size_t& aDimension,
			const double& aValue,
			size_t* outLowIdx,
			double* outPercProgress,
			std::string* outErrMsg) const;

		/* This returns the floating point "index" to the location in the dependent data
		* vector where the given value would be located, using simple linear interpolation.
		*/
		bool GetApproxPos(const size_t& aDimension,
			const double& aValue,
			double* outApproxPosition,
			std::string* outErrMsg) const;
	// ==== End Section: Position Helpers (Protected) ==== //


	// ==== Begin Section: Validity Helpers (Protected) ==== //
		/* Returns false if any independent data vectors are NOT monotonically increasing
		* (required for searches, interpolations, etc.), or true otherwise.
		*/
		bool CheckMonotonicallyIncreasing(const TableDataSet& aFullDataSet) const;

		/* Returns false if _valid == false or if outErrMsg is a nullptr.
		*/
		bool CheckTableValidity(std::string* outErrMsg) const;

		/* Returns false if CheckTableValidity is false or any of the given out parameters
		* are nullptrs.
		*/
		bool CheckOutParams(std::initializer_list<void*> outParams,
			std::string* outErrMsg) const;
		bool CheckOutParam(void* outParam,
			std::string* outErrMsg) const;

		/* Checks validity of inputs, used mainly within lookup methods.
		*/
		template<typename T1>
		bool ValidInput(std::vector<T1> aInputs,
			void* outParam,
			std::string* outErrMsg) const
		{
			if (!CheckOutParam(outParam, outErrMsg))
				return false;
			if (aInputs.size() != _indepData.size()) {
				*outErrMsg = "Invalid input: must provide one input per independent variable.";
				return false;
			}
			return true;
		}
	// ==== End Section: Validity Helpers (Protected) ==== //

	};
}


#endif // LOOKUP_TABLE_ND_H_