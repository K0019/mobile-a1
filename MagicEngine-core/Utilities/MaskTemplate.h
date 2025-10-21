/******************************************************************************/
/*!
\file   MaskTemplate.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface and source file for a bitset template with an easy interface
  for mask overlap testing.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Utilities/Serializer.h"

// Forward declaration
namespace gui {
	bool Checkbox(const char* label, bool* v);
}

// TODO: Provide a way to customize which masks collide with which masks

#pragma region Interface

/*****************************************************************//*!
\class MaskTemplate
\brief
	A template for a bitmask.
\tparam ENUM_TYPE
	The type of enum identifying each bit.
	This enum should contain values ALL and TOTAL.
*//******************************************************************/
template <typename ENUM_TYPE, bool EnableMatrix = false>
class MaskTemplate
{
private:
	//! The number of bits required to store each mask bit of the provided enum.
	static constexpr size_t BitSize{ static_cast<size_t>(ENUM_TYPE::TOTAL) };

	//! The underlying type of the enum
	using UnderlyingEnumType = std::underlying_type_t<ENUM_TYPE>;

public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	MaskTemplate();

	/*****************************************************************//*!
	\brief
		Constructor.
	\param startingBits
		Flags will be set to the corresponding bit within this variable.
	*//******************************************************************/
	MaskTemplate(int startingBits);

	/*****************************************************************//*!
	\brief
		Constructor setting only specified layers to true and the rest to false.
	\param activeLayers
		The layers to be set to true.
	*//******************************************************************/
	MaskTemplate(std::initializer_list<ENUM_TYPE> activeLayers);

	/*****************************************************************//*!
	\brief
		Sets a mask bit.
	\param mask
		The mask bit.
	\param setTrue
		Whether to set the mask bit to true or false.
	*//******************************************************************/
	void SetMask(ENUM_TYPE mask, bool setTrue);

	/*****************************************************************//*!
	\brief
		Sets a set of mask bits.
	\param maskPack
		The mask bits to be set.
	\param setTrue
		Whether to set the mask bits to true or false.
	*//******************************************************************/
	void SetMask(const std::initializer_list<ENUM_TYPE>& maskPack, bool setTrue);

	/*****************************************************************//*!
	\brief
		Sets all mask bits.
	\param maskPack
		The values of the mask bits.
	*//******************************************************************/
	void SetMask(const std::bitset<BitSize>& maskPack);

	/*****************************************************************//*!
	\brief
		Sets all mask bits.
	\param maskPack
		The values of the mask bits.
	*//******************************************************************/
	void SetMask(const MaskTemplate& maskPack);

	/*****************************************************************//*!
	\brief
		Tests a mask bit.
	\param mask
		The mask bit to test.
	\return
		True if the mask bit is set. False otherwise.
	*//******************************************************************/
	bool TestMask(ENUM_TYPE mask) const;

	/*****************************************************************//*!
	\brief
		Tests mask bits that are true within the mask.
	\param mask
		The mask bits to test.
	\return
		True if any bits are overlapping. False otherwise.
	*//******************************************************************/
	bool TestMask(const std::bitset<BitSize>& mask) const;

	/*****************************************************************//*!
	\brief
		Tests mask bits that are true within the mask.
	\param mask
		The mask bits to test.
	\return
		True if any bits are overlapping. False otherwise.
	*//******************************************************************/
	bool TestMask(const MaskTemplate& other) const;

	/*****************************************************************//*!
	\brief
		Tests mask bits that are true within the mask, ignoring the conversion matrix.
	\param mask
		The mask bits to test.
	\return
		True if any bits are overlapping. False otherwise.
	*//******************************************************************/
	bool TestMaskRaw(const std::bitset<BitSize>& mask) const;

	/*****************************************************************//*!
	\brief
		Tests mask bits that are true within the mask, ignoring the conversion matrix.
	\param mask
		The mask bits to test.
	\return
		True if any bits are overlapping. False otherwise.
	*//******************************************************************/
	bool TestMaskRaw(const MaskTemplate& other) const;

	/*****************************************************************//*!
	\brief
		Tests whether all bits are true within the mask.
	\return
		True if all bits are true. False otherwise.
	*//******************************************************************/
	bool TestMaskAll() const;

	/*****************************************************************//*!
	\brief
		Gets the first "on" layer starting from 0.
	\return
		The first "on" layer.
	*//******************************************************************/
	ENUM_TYPE GetFirst1BitFromRight() const;

	/*****************************************************************//*!
	\brief
		Executes a function per "on" layer.
	\tparam Callable
		A function type that accepts ENUM_TYPE as a parameter.
	\param func
		The function to call.
	*//******************************************************************/
	template <typename Callable>
		requires std::regular_invocable<Callable, ENUM_TYPE>
	void ExecutePerActiveBit(Callable func) const;

private:
	//! Matrix determining which bits collide with which bits
	using MatrixType = std::array<std::bitset<BitSize>, BitSize>;
	static MatrixType matrix;

	/*****************************************************************//*!
	\brief
		Converts an input mask to an output mask based on the matrix.
	\return
		The output layer mask.
	*//******************************************************************/
	static std::bitset<BitSize> ProcessMatrix(const std::bitset<BitSize>& input) requires EnableMatrix
		// This is driving me insane, why doesn't this signature work when defined outside??
		// template<typename ENUM_TYPE, bool EnableMatrix>
		// std::bitset<MaskTemplate<ENUM_TYPE, EnableMatrix>::BitSize> MaskTemplate<ENUM_TYPE, EnableMatrix>::ProcessMatrix(const std::bitset<BitSize>& input) requires EnableMatrix
	{
		std::bitset<MaskTemplate<ENUM_TYPE, EnableMatrix>::BitSize> result{};
		auto integer{ input.to_ulong() };
		for (int bitPos{ std::countr_zero(integer) }; bitPos < MaskTemplate<ENUM_TYPE, EnableMatrix>::BitSize; bitPos = std::countr_zero(integer))
		{
			result |= matrix[bitPos];
			integer &= ~(1 << bitPos);
		}
		return result;
	}

	/*****************************************************************//*!
	\brief
		Gets whether a majority of the entries in the matrix are true.
	\return
		True if the majority of entries in the matrix are true. False otherwise.
	*//******************************************************************/
	static bool GetMatrixIsMajorityTrue() requires EnableMatrix;

public:
	/*****************************************************************//*!
	\brief
		Tests whether an enum collides with another enum in the matrix.
	\param a
		An enum.
	\param b
		Another enum.
	\return
		True if the enum collides with the other enum. False otherwise.
	*//******************************************************************/
	  static bool TestMatrix(ENUM_TYPE a, ENUM_TYPE b) requires EnableMatrix;

	/*****************************************************************//*!
	\brief
		Sets whether an enum collides with another enum in the matrix.
	\param a
		An enum.
	\param b
		Another enum.
	\param setTrue
		Whether the enum collides with the other enum.
	*//******************************************************************/
	static void SetMatrix(ENUM_TYPE a, ENUM_TYPE b, bool setTrue) requires EnableMatrix;

	/*****************************************************************//*!
	\brief
		Serializes the matrix.
	\param enumNamesArr
		The array of names of each of the enums.
	*//******************************************************************/
	static void SerializeMatrix(Serializer& writer, const std::string& key, const char* const* enumNamesArr = nullptr) requires EnableMatrix;

	/*****************************************************************//*!
	\brief
		Serializes the matrix.
	\param enumNamesArr
		The array of names of each of the enums.
	*//******************************************************************/
	static void DeserializeMatrix(Deserializer& reader, const std::string& key, const char* const* enumNamesArr = nullptr) requires EnableMatrix;

	/*****************************************************************//*!
	\brief
		Draws the bits of this mask to the current ImGui context.
	\param namesArr
		The array of string names corresponding to each bit.
	*//******************************************************************/
	void MaskEditorDraw(const char* const* namesArr);

	/*****************************************************************//*!
	\brief
		Serializes this mask to file.
	\param serializer
		The serializer interface.
	\param key
		The name of this mask in the serialization.
	\param namesArr
		The array of string names corresponding to each bit. If nullptr, uses
		the underlying int value of the enum instead.
	*//******************************************************************/
	void MaskSerialize(Serializer& serializer, const std::string& key, const char* const* namesArr = nullptr) const;

	/*****************************************************************//*!
	\brief
		Deserializes this mask from file.
	\param deserializer
		The deserializer interface.
	\param key
		The name of this mask in the serialization.
	\param namesArr
		The array of string names corresponding to each bit. If nullptr, uses
		the underlying int value of the enum instead.
	*//******************************************************************/
	void MaskDeserialize(Deserializer& deserializer, const std::string& key, const char* const* namesArr = nullptr);

private:
	/*****************************************************************//*!
	\brief
		Gets whether a majority of the bits in the mask are true.
	\return
		True if the majority of bits in the mask are true. False otherwise.
	*//******************************************************************/
	bool GetMaskMajorityIsTrue() const;

	/*****************************************************************//*!
	\brief
		Helper function for deserialization to get the mask of the current
		array element.
	\param deserializer
		The deserializer. Its state should be at the array element.
	\param namesArr
		The array of the enum's names.
	\return
		The array element converted into a mask type. TOTAL if invalid.
	*//******************************************************************/
	static ENUM_TYPE GetMaskOfCurrentElement(Deserializer& deserializer, const char* const* namesArr);

private:
	//! The mask bits.
	std::bitset<BitSize> masks;

};

#pragma endregion // Interface

#pragma region Definition

template<typename ENUM_TYPE, bool EnableMatrix>
MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskTemplate()
{
	SetMask(ENUM_TYPE::ALL, true);
}

template<typename ENUM_TYPE, bool EnableMatrix>
inline MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskTemplate(int startingBits)
{
	for (int bitIndex = 0; bitIndex < static_cast<int>(BitSize); ++bitIndex)
	{
		SetMask(static_cast<ENUM_TYPE>(bitIndex), (startingBits >> bitIndex) & 0b1);
	}
}

template<typename ENUM_TYPE, bool EnableMatrix>
MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskTemplate(std::initializer_list<ENUM_TYPE> activeLayers)
{
	for (ENUM_TYPE layer : activeLayers)
		SetMask(layer, true);
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SetMask(ENUM_TYPE mask, bool setTrue)
{
	switch (mask)
	{
	case ENUM_TYPE::ALL:
		if (setTrue)
			masks.set();
		else
			masks.reset();
		break;
	default:
		masks.set(static_cast<size_t>(mask), setTrue);
	}
}
template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SetMask(const std::initializer_list<ENUM_TYPE>& maskPack, bool setTrue)
{
	for (ENUM_TYPE mask : maskPack)
		SetMask(mask, setTrue);
}
template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SetMask(const std::bitset<BitSize>& maskPack)
{
	masks = maskPack;
}
template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SetMask(const MaskTemplate& maskPack)
{
	SetMask(maskPack.masks);
}

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMask(ENUM_TYPE mask) const
{
	return masks.test(static_cast<size_t>(mask));
}
template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMask(const std::bitset<BitSize>& mask) const
{
	return ((EnableMatrix ? ProcessMatrix(masks) : masks) & mask).any();
}
template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMask(const MaskTemplate& other) const
{
	return TestMask(other.masks);
}

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMaskRaw(const std::bitset<BitSize>& mask) const
{
	return (masks & mask).any();
}
template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMaskRaw(const MaskTemplate& other) const
{
	return TestMaskRaw(other.masks);
}

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMaskAll() const
{
	return masks.all();
}

template<typename ENUM_TYPE, bool EnableMatrix>
ENUM_TYPE MaskTemplate<ENUM_TYPE, EnableMatrix>::GetFirst1BitFromRight() const
{
	return static_cast<ENUM_TYPE>(std::countr_zero(masks.to_ulong()));
}

template<typename ENUM_TYPE, bool EnableMatrix>
template <typename Callable>
	requires std::regular_invocable<Callable, ENUM_TYPE>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::ExecutePerActiveBit(Callable func) const
{
	MaskTemplate copy{ *this };
	for (ENUM_TYPE bit{ copy.GetFirst1BitFromRight() }; bit < ENUM_TYPE::TOTAL; copy.SetMask(bit, false), bit = copy.GetFirst1BitFromRight())
		func(bit);
}

// Initialize the matrix with a 1-1 mapping
template<typename ENUM_TYPE, bool EnableMatrix>
MaskTemplate<ENUM_TYPE, EnableMatrix>::MatrixType MaskTemplate<ENUM_TYPE, EnableMatrix>::matrix{
	[] <size_t BitSize>() constexpr -> MatrixType {
		MatrixType matrixArr{};
		for (MaskTemplate<ENUM_TYPE, EnableMatrix>::UnderlyingEnumType i{}; i < BitSize; ++i)
			matrixArr[i].set(i);
		return matrixArr;
	}.operator()<MaskTemplate<ENUM_TYPE, EnableMatrix>::BitSize>()
};

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::GetMatrixIsMajorityTrue() requires EnableMatrix
{
	// Keep a count of the imbalance. If positive, majority are true. If negative, majority are false.
	int count{};
	for (ENUM_TYPE i{ static_cast<ENUM_TYPE>(0) }; i < ENUM_TYPE::TOTAL; ++i)
		for (ENUM_TYPE j{ i }; j < ENUM_TYPE::TOTAL; ++j)
			(TestMatrix(i, j) ? ++count : --count);
	return count >= 0;
}

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::TestMatrix(ENUM_TYPE a, ENUM_TYPE b) requires EnableMatrix
{
	return matrix[+a].test(+b);
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SetMatrix(ENUM_TYPE a, ENUM_TYPE b, bool setTrue) requires EnableMatrix
{
	matrix[+a].set(+b, setTrue);
	matrix[+b].set(+a, setTrue);
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::SerializeMatrix(Serializer& writer, const std::string& key, const char* const* enumNamesArr) requires EnableMatrix
{
	// Optimize storage space by only storing the minority setting (if most bits are true, store only false. vice versa)
	const bool majorityTrue{ GetMatrixIsMajorityTrue() };

	// Write whether we've written false or true only
	writer.StartObject(key);
	writer.Serialize("_majorityTrue", majorityTrue);

	// Serialize all combinations
	for (ENUM_TYPE i{ static_cast<ENUM_TYPE>(0) }; i < ENUM_TYPE::TOTAL; ++i)
	{
		writer.StartArray(enumNamesArr ? enumNamesArr[+i] : std::to_string(+i));
		for (ENUM_TYPE j{ i }; j < ENUM_TYPE::TOTAL; ++j)
			if (TestMatrix(i, j) != majorityTrue)
				writer.Serialize("", enumNamesArr ? enumNamesArr[+j] : std::to_string(+j));
		writer.EndArray();
	}
	writer.EndObject();
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::DeserializeMatrix(Deserializer& reader, const std::string& key, const char* const* enumNamesArr) requires EnableMatrix
{
	if (!reader.PushAccess(key))
		return;

	bool majorityTrue{};
	reader.DeserializeVar("_majorityTrue", &majorityTrue);

	// Deserialize all combinations
	for (ENUM_TYPE i{ static_cast<ENUM_TYPE>(0) }; i < ENUM_TYPE::TOTAL; ++i)
	{
		// Set all fresh combinations involving this bit to the majority first
		for (ENUM_TYPE j{ static_cast<ENUM_TYPE>(i) }; j < ENUM_TYPE::TOTAL; ++j)
			SetMatrix(i, j, majorityTrue);

		// Flip the bits that were specified as exceptions.
		if (!reader.PushAccess(enumNamesArr ? enumNamesArr[+i] : std::to_string(+i)))
			continue;
		for (size_t index{}; reader.PushArrayElementAccess(index); ++index)
		{
			ENUM_TYPE mask{ GetMaskOfCurrentElement(reader, enumNamesArr) };
			if (mask != ENUM_TYPE::TOTAL) // If the mask type exists
				SetMatrix(i, mask, !majorityTrue);
			reader.PopAccess();
		}
		reader.PopAccess();
	}
	reader.PopAccess();
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskEditorDraw(const char* const* namesArr)
{
	bool b{};
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
	{
		b = TestMask(static_cast<ENUM_TYPE>(i));
		if (gui::Checkbox(namesArr[i], &b))
			SetMask(static_cast<ENUM_TYPE>(i), b);
	}
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskSerialize(Serializer& serializer, const std::string& identifier, const char* const* namesArr) const
{
	// Optimize storage space by only storing the minority setting (if most bits are true, store only false. vice versa)
	const bool majorityTrue{ GetMaskMajorityIsTrue() };
	serializer.StartObject(identifier);
	serializer.Serialize("_majorityTrue", majorityTrue);

	// Write the bits that are in the minority
	serializer.StartArray("_except");
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
		if (TestMask(static_cast<ENUM_TYPE>(i)) != majorityTrue)
			serializer.Serialize("", namesArr ? namesArr[i] : std::to_string(i));
	serializer.EndArray();

	serializer.EndObject();
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskDeserialize(Deserializer& deserializer, const std::string& key, const char* const* namesArr)
{
	if (!deserializer.PushAccess(key))
		return;

	// Set all the bits according to the majority first
	bool majorityTrue{};
	deserializer.DeserializeVar("_majorityTrue", &majorityTrue);
	(majorityTrue ? masks.set() : masks.reset());

	// Access the except array
	if (!deserializer.PushAccess("_except"))
	{
		deserializer.PopAccess();
		return;
	}

	// For each entry, flip the bit
	for (size_t i{}; deserializer.PushArrayElementAccess(i); ++i)
	{
		ENUM_TYPE mask{ GetMaskOfCurrentElement(deserializer, namesArr) };
		if (mask == ENUM_TYPE::TOTAL)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Deserializing mask " << key << ": Invalid mask encountered!";
			deserializer.PopAccess();
			break;
		}

		SetMask(mask, !majorityTrue);
		deserializer.PopAccess();
	}

	deserializer.PopAccess(); // array access
	deserializer.PopAccess(); // object access
}

template<typename ENUM_TYPE, bool EnableMatrix>
bool MaskTemplate<ENUM_TYPE, EnableMatrix>::GetMaskMajorityIsTrue() const
{
	// Keep a count of the imbalance. If positive, majority are true. If negative, majority are false.
	int count{};
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
		(TestMask(static_cast<ENUM_TYPE>(i)) ? ++count : --count);
	return count >= 0;
}

template<typename ENUM_TYPE, bool EnableMatrix>
ENUM_TYPE MaskTemplate<ENUM_TYPE, EnableMatrix>::GetMaskOfCurrentElement(Deserializer& deserializer, const char* const* namesArr)
{
	std::string name{};
	if (!deserializer.DeserializeVar("", &name))
		return ENUM_TYPE::TOTAL;

	// If an array was not specified, treat the name as a number of the index.
	if (!namesArr)
		try {
			return static_cast<ENUM_TYPE>(std::min(std::stoul(name), static_cast<unsigned long>(ENUM_TYPE::TOTAL)));
		} catch (const std::logic_error&) {
			return ENUM_TYPE::TOTAL;
		}

	// Search the array for the index of the name.
	return static_cast<ENUM_TYPE>(std::min(
		util::FindIndexOfElement(namesArr, namesArr + +ENUM_TYPE::TOTAL, name),
		static_cast<size_t>(ENUM_TYPE::TOTAL)
	));
}

#pragma endregion // Definition
