#pragma once
#include <glm/glm.hpp>

namespace math {

	static constexpr float PI_f{ 3.141592653589793f };

	/*!***********************************************************************
	\brief
		Converts degrees to radians.
	\param degrees
		The angle in degrees.
	\return
		The angle in radians.
	*************************************************************************/
	constexpr float ToRadians(float degrees);

	/*!***********************************************************************
	\brief
		Converts radians to degrees.
	\param radians
		The angle in radians.
	\return
		The angle in degrees.
	*************************************************************************/
	constexpr float ToDegrees(float radians);

	/*!***********************************************************************
	\brief
		Calculates the square of a number.
	\param x
		The number to be squared.
	\return
		The square of the number.
	*************************************************************************/
	constexpr float PowSqr(float x);

	/*!***********************************************************************
	\brief
		Wraps a value around a range.
	\tparam T
		The type of the value.
	\param x
		The value to be wrapped.
	\param min
		The minimum value in the range.
	\param max
		The maximum value in the range.
	\return
		The wrapped value.
	*************************************************************************/
	template <typename T>
	constexpr T Wrap(T x, const T& min, const T& max);

	/*!***********************************************************************
	\brief
		Wraps a floating point around a range.
	\tparam T
		The type of the floating point.
	\param x
		The value to be wrapped.
	\param min
		The minimum value in the range.
	\param max
		The maximum value in the range.
	\return
		The wrapped value.
	*************************************************************************/
	template <std::floating_point T>
	constexpr T Wrap(T x, const T& min, const T& max);

	/*!***********************************************************************
	\brief
		Returns 1 or -1 depending on the sign of a floating point.
	\tparam T
		The type of the floating point.
	\param x
		The value whose sign is to be tested.
	\return
		1 if positive, -1 if negative.
	*************************************************************************/
	template <std::floating_point T>
	constexpr T Sign(T x);

}

#pragma region Vec3

struct Vec3 : public glm::vec3
{
	Vec3() = default;
	Vec3(float x, float y, float z);
	Vec3(const glm::vec3& other);
	Vec3(glm::vec3&& other);
	Vec3& operator=(const glm::vec3& other);
	Vec3& operator=(glm::vec3&& other);

	constexpr float Dot(const Vec3& other) const;
};

#pragma endregion // Vec3

#pragma region Vec4

struct Vec4 : public glm::vec4
{
	Vec4() = default;
	Vec4(float x, float y, float z, float w);
	Vec4(const Vec3& vec, float w);
};

#pragma endregion // Vec4

#pragma region Mat4

struct Mat4 : public glm::mat4
{
	Mat4() = default;
	Mat4(const glm::mat4& other);
	Mat4(glm::mat4&& other);

	void Set(const Vec3& position, const Vec3& scale, const Vec3& rotation);
	static constexpr Mat4 Identity();
	constexpr Mat4 Inverse() const;
};

Vec3 operator*(const Mat4& mat, const Vec3& vec);

#pragma endregion // Mat4

#include "MagicMath.ipp"
