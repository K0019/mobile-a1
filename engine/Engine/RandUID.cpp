/******************************************************************************/
/*!
\file   RandUID.cpp
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
  This is the source file that implements functions declared in RandUID.h interface file,
  that generate random UIDs and hashes for different variables.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "RandUID.h"

namespace util {

	//! The hasher class object that hashes objects.
	std::hash<std::string> strHasher;

	uint64_t Rand_UID()
	{
		constexpr uint16_t MASK12BITS{ 0xFFF };
		constexpr uint64_t MASK40BITS{ 0xFF'FFFF'FFFF };
		// We'll generate UIDs with 12bits serial number, 12bits random, 40bits timestamp
		static std::uniform_int_distribution<uint16_t> dis{ 0, MASK12BITS };
		static uint16_t callCount{ 0 };
		// Get timestamp, counter and random numbers
		int64_t timestamp{ std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch()).count() };
		++callCount;
		uint16_t randNum{dis(GetEngine()) };

		// Pack into 64 bits
		return (static_cast<uint64_t>(callCount & MASK12BITS) << 52) +
			   (static_cast<uint64_t>(randNum & MASK12BITS) << 40) +
			   (timestamp & MASK40BITS);
	}

	uint64_t GenHash(const std::string& s)
	{
		return strHasher(s);
	}

	size_t HashMatrix(const glm::mat4& mat)
	{
        size_t hash = 0;
        for(int i = 0; i < 16; ++i) {
            hash ^= std::hash<float>{}(mat[i / 4][i % 4]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }

}
