/******************************************************************************/
/*!
\file   TypeID.cpp
\par    Project: 7percent
\par    Course: CSD1451
\par    Section A
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is a source file that implements functions declared in TypeID.h interface file
  for functions that return a human-readable string of a type.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "TypeID.h"

namespace util {

	std::string AddSpaceBeforeEachCapital(std::string str)
	{
		for (size_t index{ 1 }; index < str.size(); ++index)
		{
			if ('A' <= str[index] && str[index] <= 'Z')
				str.insert(index++, 1, ' ');
		}
		return str;
	}

}