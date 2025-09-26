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

	template<unsigned int p>
	constexpr int PowInt(int x)
	{
		if constexpr (p == 0)
			return 1;
		if constexpr (p == 1)
			return x;

		int tmp = PowInt<p / 2>(x);
		if constexpr (p % 2)
			return x * tmp * tmp;
		else
			return tmp * tmp;
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

inline constexpr Vec2 Vec2::operator-() const
{
	return Vec2{ -x, -y };
}

inline constexpr float Vec2::Dot(const Vec2& other) const
{
	return glm::dot(static_cast<const glm::vec2&>(*this), static_cast<const glm::vec2&>(other));
}

inline float Vec2::Length() const
{
	return glm::length(static_cast<const glm::vec2&>(*this));
}

inline constexpr float Vec2::LengthSqr() const
{
	return x * x + y * y;
}

inline constexpr Vec2 Vec2::Normalized() const
{
	return glm::normalize(static_cast<const glm::vec2&>(*this));
}

GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec2, glm::vec2, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec2, glm::vec2, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec2, glm::vec2, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec2, glm::vec2, /)

#pragma endregion // Vec2

#pragma region Vec3

inline constexpr Vec3::Vec3(float scalar)
	: glm::vec3{ scalar }
{
}

inline constexpr Vec3::Vec3(float x, float y, float z)
	: glm::vec3{ x, y, z }
{
}

inline constexpr Vec3::Vec3(const Vec2& vec, float z)
	: glm::vec3{ vec.x, vec.y, z }
{
}

inline constexpr Vec3::Vec3(const Vec4& vec)
	: glm::vec3{ vec.x, vec.y, vec.z }
{
}

inline constexpr Vec3::Vec3(const glm::vec3& other)
	: glm::vec3{ other }
{
}

inline constexpr Vec3::Vec3(glm::vec3&& other)
	: glm::vec3{ std::move(other) }
{
}

inline Vec3& Vec3::operator=(const glm::vec3& other)
{
	static_cast<glm::vec3&>(*this) = other;
	return *this;
}

inline Vec3& Vec3::operator=(glm::vec3&& other)
{
	static_cast<glm::vec3&>(*this) = std::move(other);
	return *this;
}

inline constexpr Vec3 Vec3::operator-() const
{
	return Vec3{ -x, -y, -z };
}

inline constexpr float Vec3::Dot(const Vec3& other) const
{
	return glm::dot(static_cast<const glm::vec3&>(*this), static_cast<const glm::vec3&>(other));
}

inline float Vec3::Length() const
{
	return glm::length(static_cast<const glm::vec3&>(*this));
}

inline constexpr float Vec3::LengthSqr() const
{
	return x * x + y * y + z * z;
}

inline constexpr Vec3 Vec3::Normalized() const
{
	return glm::normalize(static_cast<const glm::vec3&>(*this));
}

GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, +=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, -=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, *=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, /=)

GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec3, glm::vec3, /)

#pragma endregion // Vec3

#pragma region Vec4

inline constexpr Vec4::Vec4(float x, float y, float z, float w)
	: glm::vec4{ x, y, z, w }
{
}

inline constexpr Vec4::Vec4(const Vec3& vec, float w)
	: glm::vec4{ vec.x, vec.y, vec.z, w }
{
}

inline constexpr Vec4::Vec4(const glm::vec4& other)
	: glm::vec4{ other }
{
}

inline constexpr Vec4::Vec4(glm::vec4&& other)
	: glm::vec4{ std::move(other) }
{
}

inline Vec4& Vec4::operator=(const glm::vec4& other)
{
	static_cast<glm::vec4&>(*this) = other;
	return *this;
}

inline Vec4& Vec4::operator=(glm::vec4&& other)
{
	static_cast<glm::vec4&>(*this) = std::move(other);
	return *this;
}

inline constexpr Vec4 Vec4::operator-() const
{
	return Vec4{ -x, -y, -z, -w };
}

inline constexpr float Vec4::Dot(const Vec4& other) const
{
	return glm::dot(static_cast<const glm::vec4&>(*this), static_cast<const glm::vec4&>(other));
}

inline float Vec4::Length() const
{
	return glm::length(static_cast<const glm::vec4&>(*this));
}

inline constexpr float Vec4::LengthSqr() const
{
	return x * x + y * y + z * z + w * w;
}

inline constexpr Vec4 Vec4::Normalized() const
{
	return glm::normalize(static_cast<const glm::vec4&>(*this));
}

GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, +=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, -=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, *=)
GENERATE_MEMBER_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, /=)

GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, +)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, -)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, *)
GENERATE_GLOBAL_OPERATOR_ADAPTER_IPP(Vec4, glm::vec4, /)

#pragma endregion // Vec4

#pragma region Mat4

inline constexpr Mat4::Mat4(
	float x0, float y0, float z0, float w0,
	float x1, float y1, float z1, float w1,
	float x2, float y2, float z2, float w2,
	float x3, float y3, float z3, float w3
)
	: glm::mat4{
		x0, y0, z0, w0,
		x1, y1, z1, w1,
		x2, y2, z2, w2,
		x3, y3, z3, w3
	}
{
}

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
	*this = glm::translate(Mat4::Identity(), position);
	if (rotation.y) // yaw
		*this = glm::rotate(*this, rotation.y, { 0.0f, 1.0f, 0.0f });
	if (rotation.x) // pitch
		*this = glm::rotate(*this, rotation.x, { 1.0f, 0.0f, 0.0f });
	if (rotation.z) // roll
		*this = glm::rotate(*this, rotation.z, { 0.0f, 0.0f, 1.0f });
	*this = glm::scale(*this, scale);
}

inline constexpr Mat4 Mat4::Identity()
{
	return glm::identity<glm::mat4>();
}

inline constexpr Mat4 Mat4::Inverse() const
{
	return glm::inverse(*this);
}

inline Vec4 operator*(const Mat4& mat, const Vec4& vec)
{
	return {
		mat[0][0] * vec.x + mat[1][0] * vec.y + mat[2][0] * vec.z + mat[3][0] * vec.w,
		mat[0][1] * vec.x + mat[1][1] * vec.y + mat[2][1] * vec.z + mat[3][1] * vec.w,
		mat[0][2] * vec.x + mat[1][2] * vec.y + mat[2][2] * vec.z + mat[3][2] * vec.w,
		mat[0][3] * vec.x + mat[1][3] * vec.y + mat[2][3] * vec.z + mat[3][3] * vec.w
	};
}

#pragma endregion // Mat4
