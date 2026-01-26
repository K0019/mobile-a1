#include "Utilities/MagicMath.h"

namespace math {

	namespace internal {
		constexpr void ConstexprMemCpy(void* dst, const void* src, size_t size)
		{
			for (std::size_t i{}; i < size; ++i)
				reinterpret_cast<uint8_t*>(dst)[i] = reinterpret_cast<const uint8_t*>(src)[i];
		}
		constexpr float __int_as_float(int32_t a) { float r; ConstexprMemCpy(&r, &a, sizeof(r)); return r; }
		constexpr int32_t __float_as_int(float a) { int32_t r; ConstexprMemCpy(&r, &a, sizeof(r)); return r; }
	}

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

	inline float FastLog(float x)
	{
		// https://stackoverflow.com/questions/39821367/very-fast-approximate-logarithm-natural-log-function-in-c
		float m, r, s, t, i, f;
		int32_t e;

		e = (internal::__float_as_int(x) - 0x3f2aaaab) & 0xff800000;
		m = internal::__int_as_float(internal::__float_as_int(x) - e);
		i = (float)e * 1.19209290e-7f; // 0x1.0p-23
		/* m in [2/3, 4/3] */
		f = m - 1.0f;
		s = f * f;
		/* Compute log1p(f) for f in [-1/3, 1/3] */
		r = std::fmaf(0.230836749f, f, -0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
		t = std::fmaf(0.331826031f, f, -0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
		r = std::fmaf(r, s, t);
		r = std::fmaf(r, s, f);
		r = std::fmaf(i, 0.693147182f, r); // 0x1.62e430p-1 // log(2) 
		return r;
	}

	inline Mat4 EulerAnglesToRotationMatrix(const Vec3& angles)
	{
		return Mat4{ glm::yawPitchRoll(glm::radians(angles.y), glm::radians(angles.x), glm::radians(angles.z)) };
	}

	inline Vec3 EulerAnglesToVector(float pitch, float yaw)
	{
		pitch = glm::radians(pitch);
		yaw = glm::radians(yaw);
		float pitchCos{ std::cosf(pitch) }, pitchSin{ std::sinf(pitch) };
		float yawCos{ std::cosf(yaw) }, yawSin{ std::sinf(yaw) };
		return Vec3{
			pitchCos * yawSin,
			pitchSin,
			pitchCos * yawCos
		};
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
	template<std::floating_point T>
	constexpr bool NearZero(T x)
	{
		return std::fabs(x) <= std::numeric_limits<T>::epsilon();
	}
	template <std::floating_point T>
	constexpr T Abs(T x)
	{
		if (x < 0)
			return -x;
		else
			return x;
	}

	constexpr float MoveTowards(float current, float target, float delta)
	{
		if (Abs(target - current) <= delta)
		{
			return target;
		}

		return current + Sign(target - current) * delta;
	}

	constexpr float RepeatAngle(float angle)
	{
		// "Wraps" the value until it's within 0.0f to 360.0f
		while (angle >= 360.0f) angle -= 360.0f;
		while (angle < 0.0f) angle += 360.0f;
		return angle;
	}

	constexpr float AngleDifference(float a, float b)
	{
		float difference = RepeatAngle(b - a);

		// Makes the angle difference negative if it's greater than 180 degrees
		if (difference > 180.0f)
		{
			difference -= 360.0f;
		}

		return difference;
	}

	constexpr float MoveTowardsAngle(float current, float target, float delta)
	{
		// We don't do anything if delta isn't +ve
		if (delta < 0.0f)
			return current;

		float difference = AngleDifference(current, target);
		if (Abs(difference) <= delta)
		{
			return target;
		}

		return MoveTowards(current, current + difference, delta);
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
	: glm::vec2{ std::forward<glm::vec2>(other) }
{
}

inline constexpr Vec2::Vec2(Vec3&& other)
	: glm::vec2{ other.x, other.y }
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

inline Vec3::operator Vec2()
{
	return Vec2{ x, y };
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

inline Vec4::operator Vec2()
{
	return Vec2{ x, y };
}

inline Vec4::operator Vec3()
{
	return Vec3{ x, y, z };
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
	*this = Mat4::Identity();
	Mat4 matScale{ glm::scale(*this, scale) };
	Mat4 matRotate{ glm::yawPitchRoll(glm::radians(rotation.y), glm::radians(rotation.x), glm::radians(rotation.z)) };
	Mat4 matTranslate{ glm::translate(*this, position) };
	*this = matTranslate * matRotate * matScale;
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
