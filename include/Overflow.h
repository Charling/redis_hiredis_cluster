#pragma once
#include <limits.h>

//whyglinux

namespace BASE
{

	inline int is_overflow_add_for_unsigned_int(unsigned int a, unsigned int b)
	{
		return UINT_MAX - a < b;
	}

	inline int is_overflow_add_for_signed_int(int a, int b)
	{
		return a >= 0 ? INT_MAX - a < b : INT_MIN - a > b;
	}

	inline int is_overflow_multiply_for_unsigned_int(unsigned int a, unsigned int b)
	{
		return a == 0 ? 0 : UINT_MAX / a < b;
	}

	inline int is_overflow_multiply_for_signed_int(int a, int b)
	{
		return a == 0 ? 0 :
			a > 0 && b > 0 || a < 0 && b < 0 ? INT_MAX / a < b : INT_MIN / a > b;
	}

} // end namespace Base
