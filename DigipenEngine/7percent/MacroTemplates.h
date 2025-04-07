/******************************************************************************/
/*!
\file   MacroTemplates.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   12/05/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is a header only file that defines certain macros that generate various
  boilerplate but useful helper code.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <type_traits>

#define GENERATE_ENUM_CLASS_BITWISE_OPERATORS(EnumType) \
inline constexpr EnumType operator&(EnumType x, EnumType y) { \
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(x) & static_cast<std::underlying_type_t<EnumType>>(y)); \
} \
inline constexpr EnumType operator|(EnumType x, EnumType y) { \
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(x) | static_cast<std::underlying_type_t<EnumType>>(y)); \
}

#define GENERATE_ENUM_CLASS_ITERATION_OPERATORS(EnumType) \
inline constexpr EnumType& operator++(EnumType& x) { \
	return x = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(x) + 1); \
} \
inline constexpr bool operator<(EnumType x, EnumType y) { \
	return static_cast<std::underlying_type_t<EnumType>>(x) < static_cast<std::underlying_type_t<EnumType>>(y); \
}

#define GENERATE_ENUM_CLASS_ARITHMETIC_OPERATORS(EnumType) \
inline constexpr EnumType operator+(EnumType x, int offset) { \
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(x) + offset); \
}
