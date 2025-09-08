#pragma once
#include <type_traits>
#include <concepts>
#include <glm/glm.hpp>

struct Vec2;
struct Vec3;
struct Vec4;

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
#define GENERATE_MEMBER_OPERATOR_ADAPTER_H(OurClass, GLMClass, OperatorIdentifier) \
template <typename T> \
	requires std::derived_from<T, GLMClass> \
constexpr OurClass& operator##OperatorIdentifier(const T& other); \
constexpr OurClass& operator##OperatorIdentifier(std::floating_point auto other);
#define GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(OurClass, GLMClass, OperatorIdentifier) \
template <typename T> \
	requires std::derived_from<T, GLMClass> \
inline constexpr OurClass& OurClass::operator##OperatorIdentifier(const T& other) \
{ \
	static_cast<GLMClass&>(*this) OperatorIdentifier static_cast<const GLMClass&>(other); \
	return *this; \
} \
inline constexpr OurClass& OurClass::operator##OperatorIdentifier(std::floating_point auto other) \
{ \
	static_cast<GLMClass&>(*this) OperatorIdentifier other; \
	return *this; \
}

#define GENERATE_GLOBAL_OPERATOR_ADAPTER_H(OurClass, GLMClass, OperatorIdentifier) \
template <typename T, typename U> \
	requires std::derived_from<T, GLMClass> && std::derived_from<U, GLMClass> \
constexpr OurClass operator##OperatorIdentifier(const T& left, const U& right); \
constexpr OurClass operator##OperatorIdentifier(const OurClass& left, std::floating_point auto right); \
constexpr OurClass operator##OperatorIdentifier(std::floating_point auto left, const OurClass& right);
#define GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(OurClass, GLMClass, OperatorIdentifier) \
template <typename T, typename U> \
	requires std::derived_from<T, GLMClass> && std::derived_from<U, GLMClass> \
inline constexpr OurClass operator##OperatorIdentifier(const T& left, const U& right) \
{ \
	return OurClass{ static_cast<const GLMClass&>(left) OperatorIdentifier static_cast<const GLMClass&>(right) }; \
} \
inline constexpr OurClass operator##OperatorIdentifier(const OurClass& left, std::floating_point auto right) \
{ \
	return OurClass{ static_cast<const GLMClass&>(left) OperatorIdentifier right }; \
} \
inline constexpr OurClass operator##OperatorIdentifier(std::floating_point auto left, const OurClass& right) \
{ \
	return OurClass{ left OperatorIdentifier static_cast<const GLMClass&>(right) }; \
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

	constexpr Vec2 operator-() const;
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec2, glm::vec2, +=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec2, glm::vec2, -=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec2, glm::vec2, *=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec2, glm::vec2, /=)

	constexpr float Dot(const Vec2& other) const;
	float Length() const;
	constexpr float LengthSqr() const;
	constexpr Vec2 Normalized() const;

private: // Hide stuff we don't want the users to see
	using glm::vec2::length;
};
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec2, glm::vec2, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec2, glm::vec2, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec2, glm::vec2, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec2, glm::vec2, /)

#pragma endregion // Vec2

#pragma region Vec3

struct Vec3 : public glm::vec3
{
	Vec3() = default;
	constexpr Vec3(float x, float y, float z);
	constexpr Vec3(const Vec2& vec, float z);
	explicit constexpr Vec3(const Vec4& vec);
	constexpr Vec3(const glm::vec3& other);
	constexpr Vec3(glm::vec3&& other);
	Vec3& operator=(const glm::vec3& other);
	Vec3& operator=(glm::vec3&& other);
	// Disallow implicit truncation to smaller vectors.
	operator Vec2() = delete;
	operator glm::vec2() = delete;

	constexpr Vec3 operator-() const;
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec3, glm::vec3, +=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec3, glm::vec3, -=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec3, glm::vec3, *=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec3, glm::vec3, /=)

	constexpr float Dot(const Vec3& other) const;
	float Length() const;
	constexpr float LengthSqr() const;
	constexpr Vec3 Normalized() const;

private: // Hide stuff we don't want the users to see
	using glm::vec3::length;
};
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec3, glm::vec3, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec3, glm::vec3, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec3, glm::vec3, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec3, glm::vec3, /)

#pragma endregion // Vec3

#pragma region Vec4

struct Vec4 : public glm::vec4
{
	Vec4() = default;
	constexpr Vec4(float x, float y, float z, float w);
	constexpr Vec4(const Vec3& vec, float w);
	constexpr Vec4(const glm::vec4& other);
	constexpr Vec4(glm::vec4&& other);
	Vec4& operator=(const glm::vec4& other);
	Vec4& operator=(glm::vec4&& other);
	// Disallow implicit truncation to smaller vectors.
	operator Vec2() = delete;
	operator glm::vec2() = delete;
	operator Vec3() = delete;
	operator glm::vec3() = delete;

	constexpr Vec4 operator-() const;
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec4, glm::vec4, +=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec4, glm::vec4, -=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec4, glm::vec4, *=)
	GENERATE_MEMBER_OPERATOR_ADAPTER_H(Vec4, glm::vec4, /=)

	constexpr float Dot(const Vec4& other) const;
	float Length() const;
	constexpr float LengthSqr() const;
	constexpr Vec4 Normalized() const;

private: // Hide stuff we don't want the users to see
	using glm::vec4::length;
};
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec4, glm::vec4, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec4, glm::vec4, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec4, glm::vec4, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_H(Vec4, glm::vec4, /)

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

Vec4 operator*(const Mat4& mat, const Vec4& vec);

#pragma endregion // Mat4

#include "MagicMath.ipp"

#undef GENERATE_MEMBER_OPERATOR_ADAPTER_H
#undef GENERATE_MEMBER_OPERATOR_ADAPTER_IPP
#undef GENERATE_GLOBAL_OPERATOR_ADAPTER_H
#undef GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP
