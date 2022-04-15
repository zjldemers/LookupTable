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


#ifndef ZJLD_UTILS_H_
#define ZJLD_UTILS_H_

#include <string>

namespace utils
{

	template<typename T>
	class Result {
		T           _value;
		bool        _valid;
		std::string _errMsg;

	public:
		Result()
			: _value{ T() }
			, _valid{ false }
			, _errMsg{ "Uninitialized." }
		{}
		explicit Result(const T& aValue)
			: _value{ aValue }
			, _valid{ true }
			, _errMsg{ "" }
		{}
		Result(const std::string& aErrMsg)
			: _value{ T() }
			, _valid{ false }
			, _errMsg{ aErrMsg }
		{}
		Result(const char* aErrMsg)
			: Result(std::string(aErrMsg))
		{}
		Result(const T& aValue, const T& aHighBound, const T& aLowBound = T())
		{
			_value = aValue;
			if (_value < aLowBound || _value >= aHighBound)
				ErrorMessage("Out of bounds.");
			else
				Valid(true);
		}

		T Value() const { return _value; }
		void Value(const T& aValue) { _value = aValue; }
		
		bool Valid() const { return _valid; }
		void Valid(const bool& aValid) {
			_valid = aValid;
			if (_valid)
				_errMsg.clear();
		}

		const std::string& ErrorMessage() const { return _errMsg; }
		void ErrorMessage(const std::string& aErrMsg) {
			_errMsg = aErrMsg;
			if (!_errMsg.empty())
				_valid = false;
		}
	};


	// Determine if two doubles are close enough to be considered approximately equal
	static bool IsApproxEqual(const double A,
		const double B)
	{
		double diff = std::abs(A - B);	// get the absolute value of the difference
		double max = std::max(A, B);	// get the maximum value between the two

		// Calculate an epsilon value (very, very small) with a buffer for a "close enough" equality
		double eps = max * std::numeric_limits<double>::epsilon() * 5.0;
		return (diff <= eps); // If diff <= epsilon, they are approximately equal
	}

	// Standard linear interpolation - gets the value that would be found between aValA
	// and aValB if progressed linearly by aPercentProgress (e.g. aValA=2.0, aValB=3.5,
	// aPercentProgress=0.5 -> return 2.75).
	// Note: allows for extrapolation if aPercentProgress < 0.0 or aPercentProgress > 1.0
	static double Lerp(const double aValA,
		const double aValB,
		const double aPercentProgress)
	{
		return aValA + aPercentProgress * (aValB - aValA);
	}

	// Inverted linear interpolation - returns the percentage aValToCompare is between
	// aValA and aValB (e.g. aValA=2.0, aValB=3.5, aValToCompare=2.75 -> return 0.5)
	// Note: allows for extrapolation if aValToCompare < aValA or aValToCompare > aValB
	static double ILerp(const double aValA,
		const double aValB,
		const double aValToCompare)
	{
		if (IsApproxEqual(aValA, aValB)) {
			// Avoid division by zero. Values are essentially the same so returning anything from 
			// 0.0 or 1.0 will make little to no difference downstream.
			return 0.0;
		}
		return (aValToCompare - aValA) / (aValB - aValA);
	}


}

#endif // ZJLD_UTILS_H_
