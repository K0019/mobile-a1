/******************************************************************************/
/*!
\file   RandUID.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (67%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Ryan Cheong (33%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
  This is the interface file for functions that generate random UIDs and hashes for
  different variables.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <cstdint>
#include <string>
#include <vulkan/glm/glm.hpp>
#include <vulkan/glm/gtc/matrix_transform.hpp>

namespace util {

	/*****************************************************************//*!
	\brief
		Generates a random 64bit number, which in theory should have low probability of collision.
	\return
		The generated UID.
	*//******************************************************************/
	uint64_t Rand_UID();

	/*****************************************************************//*!
	\brief
		Generates a hash from a string. Identical strings will have the same hash.
	\param s
		The string.
	\return
		The hash of the string.
	*//******************************************************************/
	uint64_t GenHash(const std::string& s);

	/*****************************************************************//*!
	\brief
		Generates a hash from a mat4. Identical mat4 will have the same hash.
	\param mat
		The mat4.
	\return
		The hash of the mat4.
	*//******************************************************************/
	size_t HashMatrix(const glm::mat4& mat);

}

