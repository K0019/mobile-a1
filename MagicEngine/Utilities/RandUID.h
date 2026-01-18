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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifndef __ANDROID__
#include <boost/type_index.hpp>
#else
#include <typeinfo>
#include <cxxabi.h>  // For abi::__cxa_demangle
#include <cstdlib>   // For free()
#endif

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
		Generates a random 32bit number.
	\return
		The generated UID.
	*//******************************************************************/
	uint32_t Rand_UID_32();

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

	/*****************************************************************//*!
	\brief
		Returns a hash of an object that is consistent across platforms.
	*//******************************************************************/
	template <typename T>
	uint64_t ConsistentHash();

}

namespace util {

	template<typename T>
	uint64_t ConsistentHash()
	{
#ifndef __ANDROID__
		std::string prettyName{ boost::typeindex::type_id<T>().pretty_name() };
#ifdef GLFW
		// MSVC appends a class/struct/whatever to the start. Remove it to get a consistent name
		prettyName.erase(0, prettyName.find(' ') + 1);
#endif
#else
		// On Android (Clang), demangle the type name to match Desktop format
		// typeid(T).name() returns mangled names like "15SpriteComponent"
		// We need demangled names like "SpriteComponent" to match Desktop hashes
		int status = 0;
		char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
		std::string prettyName;
		if (status == 0 && demangled) {
			prettyName = demangled;
			free(demangled);
		} else {
			// Fallback to mangled name if demangling fails
			prettyName = typeid(T).name();
		}
#endif
		return GenHash(prettyName);
	}

}
