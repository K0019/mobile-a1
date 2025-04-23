#include "MagicMath.h"

namespace math {

	constexpr float ToRadians(float degrees)
	{
		return degrees / 180.0f * PI_f;
	}

	constexpr float ToDegrees(float radians)
	{
		return radians * 180.0f / PI_f;
	}

	constexpr float PowSqr(float x)
	{
		return x * x;
	}

	template <typename T>
	constexpr T Wrap(T x, const T& min, const T& max)
	{
		if (min > max)
			return Wrap(x, max, min);
		while (x < min)
			x += (max - min);
		while (x >= max)
			x -= (max - min);
		return x;
	}

	template <std::floating_point T>
	constexpr T Wrap(T x, const T& min, const T& max)
	{
		if (min > max)
			return Wrap(x, max, min);
		return (x >= 0 ? max : min) + std::fmod(x, max - min);
	}

	template <std::floating_point T>
	constexpr T Sign(T x)
	{
		if (x < 0)
			return -1;
		else
			return 1;
	}

}

#pragma region Vec2

inline constexpr Vec2::Vec2(float x, float y)
	: glm::vec2{ x, y }
{
}

inline constexpr Vec2::Vec2(const glm::vec2& other)
	: glm::vec2{ other }
{
}

inline constexpr Vec2::Vec2(glm::vec2&& other)
	: glm::vec2{ std::move(other) }
{
}

inline Vec2& Vec2::operator=(const glm::vec2& other)
{
	static_cast<glm::vec2&>(*this) = other;
	return *this;
}

inline Vec2& Vec2::operator=(glm::vec2&& other)
{
	static_cast<glm::vec2&>(*this) = std::move(other);
	return *this;
}

inline constexpr float Vec2::Dot(const Vec2& other) const
{
	return glm::dot(static_cast<const glm::vec2&>(*this), static_cast<const glm::vec2&>(other));
}

inline constexpr float Vec2::LengthSqr() const
{
	return x * x + y * y;
}

inline constexpr Vec2 Vec2::Normalized() const
{
	return glm::normalize(static_cast<const glm::vec2&>(*this));
}

#pragma endregion // Vec2

#pragma region Vec3

inline constexpr Vec3::Vec3(float x, float y, float z)
	: glm::vec3{ x, y, z }
{
}

inline constexpr Vec3 Vec3::Normalized() const
{
	return glm::normalize(static_cast<const glm::vec3&>(*this));
}

GENERATE_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, operator+=)
GENERATE_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, operator-=)
GENERATE_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, operator*=)
GENERATE_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, operator/=)

#pragma endregion // Vec3

#pragma region Vec4

#pragma endregion // Vec4

#pragma region Mat4

inline constexpr Mat4::Mat4(const glm::mat4& other)
	: glm::mat4{ other }
{
}

inline constexpr Mat4::Mat4(glm::mat4&& other)
	: glm::mat4{ std::move(other) }
{
}

inline void Mat4::Set(const Vec3& position, const Vec3& scale, const Vec3& rotation)
{
	*this = Identity();
	glm::translate(*this, position);
	if (rotation.y) // yaw
		glm::rotate(*this, rotation.y, { 0.0f, 1.0f, 0.0f });
	if (rotation.x) // pitch
		glm::rotate(*this, rotation.x, { 1.0f, 0.0f, 0.0f });
	if (rotation.z) // roll
		glm::rotate(*this, rotation.z, { 0.0f, 0.0f, 1.0f });
	glm::scale(*this, scale);
}

inline constexpr Mat4 Mat4::Identity()
{
	return glm::identity<glm::mat4>();
}

inline constexpr Mat4 Mat4::Inverse() const
{
	return glm::inverse(*this);
}

inline Vec3 operator*(const Mat4& mat, const Vec3& vec)
{
	return {
		mat[0][0] * vec.x + mat[1][0] * vec.y + mat[2][0] * vec.z + mat[3][0],
		mat[0][1] * vec.x + mat[1][1] * vec.y + mat[2][1] * vec.z + mat[3][1],
		mat[0][2] * vec.x + mat[1][2] * vec.y + mat[2][2] * vec.z + mat[3][2]
	};
}

#pragma endregion // Mat4
