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

#ifndef _ZJLD_LOOKUP_TABLE_2D_H_
#define _ZJLD_LOOKUP_TABLE_2D_H_

#include "LookupTableND.h"

namespace zjld
{
	// This class is a specific implementation of the LookupTableND class, restricting the
	// dimensionality to exactly 2 dimensions (two independent data vectors along with one
	// corresponding/resultant dependent data vector.
	// This should operate slightly more efficiently for the 2D use-case than the more
	// flexible ND variation by reducing for loop usage since sizes are known.
	// Since this is just a subset implementation of the broader ND table, most methods use
	// the base class' implementation while the few below provide restricted options for the
	// 2-dimensional case.  With that in mind, view that class for more documentation.
	class LookupTable2D : public LookupTableND {
	public:
		using LookupTableND::LookupTableND; // use all base constructors as-is

		bool IsValidSourceData(const TableDataSet& aFullDataSet) const override;

		bool GetIndexAt(const size_t& aDim1Index,
			const size_t& aDim2Index,
			size_t* outIndex,
			std::string* outErrMsg) const;
		bool LookupByIndices(const size_t& aDim1Index,
			const size_t& aDim2Index,
			double* outValue,
			std::string* outErrMsg) const;
		bool LookupByValues(const double& aDim1Value,
			const double& aDim2Value,
			double* outValue,
			std::string* outErrMsg) const;
		bool LookupByValues(const std::vector<double>& aValueInputs,
			double* outValue,
			std::string* outErrMsg) const override;

		utils::Result<size_t> GetIndexAt(const size_t& aDim1Index,
			const size_t& aDim2Index) const;
		utils::Result<double> LookupByIndices(const size_t& aDim1Index,
			const size_t& aDim2Index) const;
		utils::Result<double> LookupByValues(const double& aDim1Value,
			const double& aDim2Value) const;
		utils::Result<double> LookupByValues(const std::vector<double>& aValueInputs) const override;
	};
}

#endif // _ZJLD_LOOKUP_TABLE_2D_H_