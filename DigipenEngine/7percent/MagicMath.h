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

// glm template functions will treat our classes as a scalars when used as a function parameter without explicit instantiation of
// those functions. These helper macros help to reduce A LOT of boilerplate code to work around this problem.
#define GENERATE_OPERATOR_ADAPTER_H(OurClass, OperatorIdentifier) \
template <typename T> \
OurClass& OperatorIdentifier(const T& other);
#define GENERATE_OPERATOR_ADAPTER_IPP(OurClass, GLMClass, OperatorIdentifier) \
template<typename T> \
inline OurClass& OurClass::OperatorIdentifier(const T& other) \
{ \
	static_cast<GLMClass&>(*this) += other; \
	return *this; \
} \
template<> \
inline OurClass& OurClass::OperatorIdentifier(const OurClass& other) \
{ \
	static_cast<GLMClass&>(*this) += static_cast<const GLMClass&>(other); \
	return *this; \
}

#pragma region Vec2

struct Vec2 : public glm::vec2
{
	Vec2() = default;
	constexpr Vec2(float x, float y);
	constexpr Vec2(const glm::vec2& other);
	constexpr Vec2(glm::vec2&& other);
	Vec2& operator=(const glm::vec2& other);
	Vec2& operator=(glm::vec2&& other);

	constexpr float Dot(const Vec2& other) const;
	constexpr float LengthSqr() const;
	constexpr Vec2 Normalized() const;

private: // Hide stuff we don't want the users to see
	using glm::vec2::length;
};

#pragma endregion // Vec2

#pragma region Vec3

struct Vec3 : public glm::vec3
{
	Vec3() = default;
	constexpr Vec3(float x, float y, float z);
	constexpr Vec3(const glm::vec3& other);
	constexpr Vec3(glm::vec3&& other);
	Vec3& operator=(const glm::vec3& other);
	Vec3& operator=(glm::vec3&& other);

	GENERATE_OPERATOR_ADAPTER_H(Vec3, operator+=)
	GENERATE_OPERATOR_ADAPTER_H(Vec3, operator-=)
	GENERATE_OPERATOR_ADAPTER_H(Vec3, operator*=)
	GENERATE_OPERATOR_ADAPTER_H(Vec3, operator/=)

	constexpr float Dot(const Vec3& other) const;
	constexpr Vec3 Normalized() const;

private: // Hide stuff we don't want the users to see
	using glm::vec3::length;
};

#pragma endregion // Vec3

#pragma region Vec4

struct Vec4 : public glm::vec4
{
	Vec4() = default;
	constexpr Vec4(float x, float y, float z, float w);
	constexpr Vec4(const Vec3& vec, float w);

private: // Hide stuff we don't want the users to see
	using glm::vec4::length;
};

#pragma endregion // Vec4

#pragma region Mat4

struct Mat4 : public glm::mat4
{
	Mat4() = default;
	constexpr Mat4(const glm::mat4& other);
	constexpr Mat4(glm::mat4&& other);

	void Set(const Vec3& position, const Vec3& scale, const Vec3& rotation);
	static constexpr Mat4 Identity();
	constexpr Mat4 Inverse() const;
};

Vec3 operator*(const Mat4& mat, const Vec3& vec);

#pragma endregion // Mat4

#include "MagicMath.ipp"

#undef GENERATE_OPERATOR_ADAPTER_H
#undef GENERATE_OPERATOR_ADAPTER_IPP
