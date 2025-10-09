/******************************************************************************/
/*!
\file   TypeID.h
\par    Project: 7percent
\par    Course: CSD1451
\par    Section A
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface and implementation file for functions that return a human-readable
  string of a type.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <string>

namespace util {

	/*****************************************************************//*!
	\brief
		Basically typeid(T).name(), but returns just the type's name, excluding extra stuff
		like class and namespace scopes.
	\tparam T
		The type.
	\return
		The class type's name as a string.
	*//******************************************************************/
	template <typename T>
	std::string GetNiceTypeName()
	{
		std::string typeName{ typeid(T).name() };
		size_t cutoffIndex{ typeName.find_last_of(" :") };
		return typeName.substr(cutoffIndex ? cutoffIndex + 1 : 0);
	}

	/*****************************************************************//*!
	\brief
		Adds a space before every capital letter in a string.
	\param str
		The string to add spaces into.
	\return
		A string with a space inserted before every capital letter.
	*//******************************************************************/
	std::string AddSpaceBeforeEachCapital(std::string str);

	/*****************************************************************//*!
	\brief
		Calls GetNiceTypeName(), then adds a space before every capital letter
	\tparam T
		The type.
	\return
		The class type's name as a string string with a space inserted before every capital letter.
	*//******************************************************************/
	template <typename T>
	std::string GetNiceSpacedTypeName()
	{
		return AddSpaceBeforeEachCapital(GetNiceTypeName<T>());
	}

}
