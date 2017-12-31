#include "stdafx.h"
#include <intrin.h>
#include <nmmintrin.h>
#include "Operator.h"
#include "ComparerSingle.cpp"
#include "Comparer.h"

#pragma unmanaged

template<CompareOperatorN cOp, BooleanOperatorN bOp, SigningN sign>
static void WhereN(unsigned __int16* set, int length, unsigned __int16 value, unsigned __int64* matchVector)
{
	int i = 0;
	unsigned __int64 result;

	// Load a mask to convert unsigned values for signed comparison
	__m256i unsignedToSigned = _mm256_set1_epi16(-32768);

	// Load copies of the value to compare against
	__m256i blockOfValue = _mm256_set1_epi16(value);
	if (sign == SigningN::Unsigned) blockOfValue = _mm256_sub_epi16(blockOfValue, unsignedToSigned);

	// Build a PEXT mask asking for every other bit (1010 = A)
	unsigned int everyOtherBit = 0xAAAAAAAA;

	// Compare 64-byte blocks and generate a 64-bit result while there's enough data
	int blockLength = length & ~63;
	for (; i < blockLength; i += 64)
	{
		// Load 64 2-byte values to compare
		__m256i block1 = _mm256_loadu_si256((__m256i*)(&set[i]));
		__m256i block2 = _mm256_loadu_si256((__m256i*)(&set[i + 16]));
		__m256i block3 = _mm256_loadu_si256((__m256i*)(&set[i + 32]));
		__m256i block4 = _mm256_loadu_si256((__m256i*)(&set[i + 48]));

		// Convert them to signed form, if needed
		if (sign == SigningN::Unsigned)
		{
			block1 = _mm256_sub_epi16(block1, unsignedToSigned);
			block2 = _mm256_sub_epi16(block2, unsignedToSigned);
			block3 = _mm256_sub_epi16(block3, unsignedToSigned);
			block4 = _mm256_sub_epi16(block4, unsignedToSigned);
		}

		// Compare them to the desired value, building a mask with 0xFFFF for matches and 0x0000 for non-matches
		__m256i matchMask1;
		__m256i matchMask2;
		__m256i matchMask3;
		__m256i matchMask4;

		switch (cOp)
		{
		case CompareOperatorN::GreaterThan:
		case CompareOperatorN::LessThanOrEqual:
			matchMask1 = _mm256_cmpgt_epi16(block1, blockOfValue);
			matchMask2 = _mm256_cmpgt_epi16(block2, blockOfValue);
			matchMask3 = _mm256_cmpgt_epi16(block3, blockOfValue);
			matchMask4 = _mm256_cmpgt_epi16(block4, blockOfValue);
			break;
		case CompareOperatorN::LessThan:
		case CompareOperatorN::GreaterThanOrEqual:
			matchMask1 = _mm256_cmpgt_epi16(blockOfValue, block1);
			matchMask2 = _mm256_cmpgt_epi16(blockOfValue, block2);
			matchMask3 = _mm256_cmpgt_epi16(blockOfValue, block3);
			matchMask4 = _mm256_cmpgt_epi16(blockOfValue, block4);
			break;
		case CompareOperatorN::Equal:
		case CompareOperatorN::NotEqual:
			matchMask1 = _mm256_cmpeq_epi16(block1, blockOfValue);
			matchMask2 = _mm256_cmpeq_epi16(block2, blockOfValue);
			matchMask3 = _mm256_cmpeq_epi16(block3, blockOfValue);
			matchMask4 = _mm256_cmpeq_epi16(block4, blockOfValue);
			break;
		}

		// Convert the masks into bits (one bit per byte, so still two duplicate bits per row matched)
		unsigned int matchBits1 = _mm256_movemask_epi8(matchMask1);
		unsigned int matchBits2 = _mm256_movemask_epi8(matchMask2);
		unsigned int matchBits3 = _mm256_movemask_epi8(matchMask3);
		unsigned int matchBits4 = _mm256_movemask_epi8(matchMask4);

		// Get every other bit (so it's one per row) and merge together pairs
		unsigned int matchBits2_1 = _pext_u32(matchBits2, everyOtherBit) << 16 | _pext_u32(matchBits1, everyOtherBit);
		unsigned int matchBits4_3 = _pext_u32(matchBits4, everyOtherBit) << 16 | _pext_u32(matchBits3, everyOtherBit);

		// Merge the result to get 64 bits for whether 64 rows matched
		result = ((unsigned __int64)matchBits4_3) << 32 | matchBits2_1;

		// Negate the result for operators we ran the opposites of
		if (cOp == CompareOperatorN::LessThanOrEqual || cOp == CompareOperatorN::GreaterThanOrEqual || cOp == CompareOperatorN::NotEqual)
		{
			result = ~result;
		}

		// Merge the result with the existing bit vector bits based on the boolean operator requested
		switch (bOp)
		{
		case BooleanOperatorN::And:
			matchVector[i >> 6] &= result;
			break;
		case BooleanOperatorN::Or:
			matchVector[i >> 6] |= result;
			break;
		}
	}

	// Match remaining values individually
	if (length & 63) 
	{
		if(sign == SigningN::Unsigned) 
			WhereSingle<cOp, unsigned __int16>(&set[i], length - i, value, bOp, &matchVector[i >> 6]);
		else 
			WhereSingle<cOp, __int16>((__int16*)&set[i], length - i, (__int16)value, bOp, &matchVector[i >> 6]);
	}
}

#pragma managed

namespace XForm
{
	namespace Native
	{
		void Comparer::Where(array<UInt16>^ left, Int32 index, Int32 length, Byte cOp, UInt16 right, Byte bOp, array<UInt64>^ vector, Int32 vectorIndex)
		{
			if (index < 0 || length < 0 || vectorIndex < 0) throw gcnew IndexOutOfRangeException();
			if (index + length > left->Length) throw gcnew IndexOutOfRangeException();
			if (vectorIndex + length > (vector->Length * 64)) throw gcnew IndexOutOfRangeException();
			if ((vectorIndex & 63) != 0) throw gcnew ArgumentException("Offset Where must run on a multiple of 64 offset.");

			pin_ptr<UInt16> pLeft = &left[index];
			pin_ptr<UInt64> pVector = &vector[vectorIndex >> 6];

			switch ((CompareOperatorN)cOp)
			{
			case CompareOperatorN::Equal:
				WhereN<CompareOperatorN::Equal, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			case CompareOperatorN::NotEqual:
				WhereN<CompareOperatorN::NotEqual, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			case CompareOperatorN::LessThan:
				WhereN<CompareOperatorN::LessThan, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			case CompareOperatorN::LessThanOrEqual:
				WhereN<CompareOperatorN::LessThanOrEqual, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			case CompareOperatorN::GreaterThan:
				WhereN<CompareOperatorN::GreaterThan, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			case CompareOperatorN::GreaterThanOrEqual:
				WhereN<CompareOperatorN::GreaterThanOrEqual, BooleanOperatorN::Or, SigningN::Unsigned>(pLeft, length, right, pVector);
				break;
			default:
				throw gcnew ArgumentException("cOp");
			}
		}

		void Comparer::Where(array<Int16>^ left, Int32 index, Int32 length, Byte cOp, Int16 right, Byte bOp, array<UInt64>^ vector, Int32 vectorIndex)
		{
			if (index < 0 || length < 0 || vectorIndex < 0) throw gcnew IndexOutOfRangeException();
			if (index + length > left->Length) throw gcnew IndexOutOfRangeException();
			if (vectorIndex + length >(vector->Length * 64)) throw gcnew IndexOutOfRangeException();
			if ((vectorIndex & 63) != 0) throw gcnew ArgumentException("Offset Where must run on a multiple of 64 offset.");

			pin_ptr<Int16> pLeft = &left[index];
			pin_ptr<UInt64> pVector = &vector[vectorIndex >> 6];

			switch ((CompareOperatorN)cOp)
			{
			case CompareOperatorN::Equal:
				WhereN<CompareOperatorN::Equal, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			case CompareOperatorN::NotEqual:
				WhereN<CompareOperatorN::NotEqual, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			case CompareOperatorN::LessThan:
				WhereN<CompareOperatorN::LessThan, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			case CompareOperatorN::LessThanOrEqual:
				WhereN<CompareOperatorN::LessThanOrEqual, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			case CompareOperatorN::GreaterThan:
				WhereN<CompareOperatorN::GreaterThan, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			case CompareOperatorN::GreaterThanOrEqual:
				WhereN<CompareOperatorN::GreaterThanOrEqual, BooleanOperatorN::Or, SigningN::Signed>((unsigned __int16*)pLeft, length, (unsigned __int16)right, pVector);
				break;
			default:
				throw gcnew ArgumentException("cOp");
			}
		}
	}
}
