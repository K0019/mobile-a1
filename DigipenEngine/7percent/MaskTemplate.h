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
#include <bit>
#include <bitset>
#include "Serializer.h"
#include "Console.h"
#ifdef IMGUI_ENABLED
#include "imgui.h"
#endif

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
		// std::bitset<MaskTemplate<ENUM_TYPE, EnableMatrix>::BitSize> MaskTemplate<ENUM_TYPE, EnableMatrix>::ProcessMatrix(const std::bitset<BitSize>& input)
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

#ifdef IMGUI_ENABLED
	/*****************************************************************//*!
	\brief
		Draws the bits of this mask to the current ImGui context.
	\param namesArr
		The array of string names corresponding to each bit.
	*//******************************************************************/
	void MaskEditorDraw(const char* const* namesArr);
#endif

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
	writer.StartObject(key);
	for (ENUM_TYPE i{ static_cast<ENUM_TYPE>(0) }; i < ENUM_TYPE::TOTAL; ++i)
	{
		writer.StartObject(enumNamesArr ? enumNamesArr[+i] : std::to_string(+i));
		for (ENUM_TYPE j{ i }; j < ENUM_TYPE::TOTAL; ++j)
			writer.Serialize(enumNamesArr ? enumNamesArr[+j] : std::to_string(+j), TestMatrix(i, j));
		writer.EndObject();
	}
	writer.EndObject();
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::DeserializeMatrix(Deserializer& reader, const std::string& key, const char* const* enumNamesArr) requires EnableMatrix
{
	if (!reader.PushAccess(key))
		return;
	bool val{};
	for (ENUM_TYPE i{ static_cast<ENUM_TYPE>(0) }; i < ENUM_TYPE::TOTAL; ++i)
	{
		if (!reader.PushAccess(enumNamesArr ? enumNamesArr[+i] : std::to_string(+i)))
			continue;
		for (ENUM_TYPE j{ i }; j < ENUM_TYPE::TOTAL; ++j)
			if (reader.DeserializeVar(enumNamesArr ? enumNamesArr[+j] : std::to_string(+j), &val))
				SetMatrix(i, j, val);
		reader.PopAccess();
	}
	reader.PopAccess();
}

#ifdef IMGUI_ENABLED

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskEditorDraw(const char* const* namesArr)
{
	bool b{};
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
	{
		b = TestMask(static_cast<ENUM_TYPE>(i));
		if (ImGui::Checkbox(namesArr[i], &b))
			SetMask(static_cast<ENUM_TYPE>(i), b);
	}
}
#endif

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskSerialize(Serializer& serializer, const std::string& identifier, const char* const* namesArr) const
{
	// TODO: Could consider writing an array instead.
	serializer.StartObject(identifier);
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
		serializer.Serialize(namesArr ? namesArr[i] : std::to_string(i), TestMask(static_cast<ENUM_TYPE>(i)));
	serializer.EndObject();
}

template<typename ENUM_TYPE, bool EnableMatrix>
void MaskTemplate<ENUM_TYPE, EnableMatrix>::MaskDeserialize(Deserializer& deserializer, const std::string& key, const char* const* namesArr)
{
	if (!deserializer.PushAccess(key))
		return;
	bool b{};
	for (int i{}; i < static_cast<int>(ENUM_TYPE::TOTAL); ++i)
	{
		std::string bitKey{ namesArr ? namesArr[i] : std::to_string(i) };
		if (!deserializer.DeserializeVar(bitKey, &b))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize mask " << key << "'s bit " << bitKey << "! Aborting further deserialization of mask.";
			break;
		}
		SetMask(static_cast<ENUM_TYPE>(i), b);
	}
	deserializer.PopAccess();
}

#pragma endregion // Definition
