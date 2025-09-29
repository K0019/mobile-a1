/******************************************************************************/
/*!
\file   Utilities.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (50%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Ryan Cheong (50%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
  This is the source file implementing various miscellaneous functions that could be
  useful in a wide variety of cases.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Utilities.h"
#include "Engine.h"

namespace util {
	std::mt19937& GetEngine() {
        static std::mt19937 mt{std::random_device{}()};
        return mt;
    }

	const std::string& EmptyStr()
	{
		static std::string empty{};
		return empty;
	}

	float RandomRangeFloat(float min, float max) {
				return RandomRange(min,max);
	}

	float PerlinNoise(float x, float y)
	{
		const Vec2 point{ x, y };

		// Some insane math to generate consistent gradient values at integer grid coordinates.
		constexpr auto RandGradient{ [](int x, int y) -> Vec2 {
			constexpr unsigned int w{ sizeof(unsigned int) * 8 };
			constexpr unsigned int s{ w / 2 };
			unsigned int a{ static_cast<unsigned int>(x) }, b{ static_cast<unsigned int>(y) };

			a *= 3284157443;

			b ^= (a << s) | (a >> (w - s));
			b *= 1911520717;

			a ^= (b << s) | (b >> (w - s));
			a *= 2048419325;

			float c{ a * (math::PI_f / ~(~0u >> 1)) };
			return { std::sinf(c), std::cosf(c) };
		} };

		// Helper func to compute dot of vector from point to corner with gradient vector.
		constexpr auto GridDotGradient{ [](const Vec2& point, int x, int y) -> float {
			return Vec2{ point.x - static_cast<float>(x), point.y - static_cast<float>(y) }.Dot(RandGradient(x, y));
		} };

		// Get the coordinates of the 4 corners of the grid square containing this point.
		int leftX{ static_cast<int>(point.x) }, rightX{ leftX + 1 },
			topY{ static_cast<int>(point.y) }, botY{ topY + 1 };
		// Get interpolation weights
		float tX{ point.x - static_cast<float>(leftX) }, tY{ point.y - static_cast<float>(topY) };

		// Interpolate horizontally at top and bottom corners.
		float interpTopX{ LerpCubic(GridDotGradient(point, leftX, topY), GridDotGradient(point, rightX, topY), tX) };
		float interpBotX{ LerpCubic(GridDotGradient(point, leftX, botY), GridDotGradient(point, rightX, botY), tX) };

		// Final result is the vertical interpolation between the horizontal interpolations.
		return LerpCubic(interpTopX, interpBotX, tY);
	}

	char ToLower(int c)
	{
		return static_cast<char>(std::tolower(c));
	}
	std::string ToLowerStr(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
		return str;
	}

	//bool IsPointInside(const Vec2& point, const Transform& transform)
	//{
	//	// Get transform properties
	//	Vec2 position = transform.GetWorldPosition();
	//	Vec2 scale = transform.GetWorldScale();
	//	float rotation = transform.GetWorldRotation();

	//	// Translate point to origin
	//	Vec2 translatedPoint = point - position;

	//	// Rotate point in the opposite direction
	//	float cosTheta = std::cos(-rotation);
	//	float sinTheta = std::sin(-rotation);
	//	Vec2 rotatedPoint;
	//	rotatedPoint.x = translatedPoint.x * cosTheta - translatedPoint.y * sinTheta;
	//	rotatedPoint.y = translatedPoint.x * sinTheta + translatedPoint.y * cosTheta;

	//	// Check if the rotated point is inside the non-rotated rectangle
	//	float halfWidth = scale.x / 2.0f;
	//	float halfHeight = scale.y / 2.0f;

	//	return (rotatedPoint.x >= -halfWidth && rotatedPoint.x <= halfWidth &&
	//		rotatedPoint.y >= -halfHeight && rotatedPoint.y <= halfHeight);
	//}

	Vec2 RotatePoint(const Vec2& point, const Vec2& center, float angle)
	{
		angle = glm::radians(angle);
		float cosTheta = std::cos(angle);
		float sinTheta = std::sin(angle);

		Vec2 translatedPoint = point - center;
		Vec2 rotatedPoint;
		rotatedPoint.x = translatedPoint.x * cosTheta - translatedPoint.y * sinTheta;
		rotatedPoint.y = translatedPoint.x * sinTheta + translatedPoint.y * cosTheta;

		return rotatedPoint + center;
	}

	//void DrawBoundingBox(const Transform& transform, const Vec3& color, float alpha)
	//{
	//	DrawBoundingBox(transform.GetWorldPosition(), transform.GetWorldScale(), color, transform.GetWorldRotation(), alpha);
	//}

	//void DrawBoundingBox(const Vec2& pos, const Vec2& scale, const Vec3& color, float rotation, float alpha)
	//{
	//	Vec2 halfscale = scale * 0.5f;
	//	Vec2 corners[4] = {
	//		Vec2(-halfscale.x, -halfscale.y),
	//		Vec2(halfscale.x, -halfscale.y),
	//		Vec2(halfscale.x, halfscale.y),
	//		Vec2(-halfscale.x, halfscale.y)
	//	};

	//	// Rotate and translate all corners
	//	for (int i = 0; i < 4; ++i)
	//	{
	//		corners[i] = RotatePoint(corners[i], Vec2{}, rotation) + pos;
	//	}

	//	Vec4 lineColor(color, alpha);

	//	// Draw all four sides of the bounding box
	//	for (int i = 0; i < 4; ++i)
	//	{
	//		int next = (i + 1) % 4;
	//		ST<Engine>::Get()->_vulkan->_renderer->AddLineInstance(corners[i], corners[next], lineColor);
	//	}
	//}

	void DrawLine([[maybe_unused]] const Vec2& start, [[maybe_unused]] const Vec2& end, [[maybe_unused]] const Vec3& color, [[maybe_unused]] float alpha)
	{
		//Vec4 lineColor(color, alpha);
		//ST<Engine>::Get()->_vulkan->_renderer->AddLineInstance(start, end, lineColor);
	}
}
