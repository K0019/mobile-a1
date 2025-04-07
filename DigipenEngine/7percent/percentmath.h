/******************************************************************************/
/*!
\file   PercentMath.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Marc Alviz Evangelista (95%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\author Matthew Chan Shao jie (5%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
Math Library for 7percent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <cmath>
#include <limits>
#include <ostream>
#include <vulkan/glm/glm.hpp>

// Forward declarations
struct Vector2;
Vector2 operator*(float, Vector2);

namespace math {

	static constexpr float PI_f{ 3.141592653589793f };

	/*!***********************************************************************
	\brief
	Converts degrees to radians.

	\param[in] degrees
	The angle in degrees.

	\return
	The angle in radians.
	*************************************************************************/
	static constexpr float ToRadians(float degrees)
	{
		return degrees / 180.0f * PI_f;
	}

	/*!***********************************************************************
	\brief
	Converts radians to degrees.

	\param[in] radians
	The angle in radians.

	\return
	The angle in degrees.
	*************************************************************************/
	static constexpr float ToDegrees(float radians)
	{
		return radians * 180.0f / PI_f;
	}

	/*!***********************************************************************
	\brief
	Calculates the square of a number.

	\param[in] x
	The number to be squared.

	\return
	The square of the number.
	*************************************************************************/
	static constexpr float PowSqr(float x)
	{
		return x * x;
	}

	/*!***********************************************************************
	\brief
	Clamps a number within a range.

	\param[in] x
	The number to be clamped.

	\param[in] min
	The minimum amount.

	\param[in] max
	The maximum amount.

	\return
	The clamped number.
	*************************************************************************/
	template <typename T>
	static constexpr T Clamp(T x, T min, T max)
	{
		if (x < min)
			return min;
		else if (x > max)
			return max;
		else
			return x;
	}

	/*!***********************************************************************
	\brief
	Repeats a number through a range.

	\param[in] x
	The number to be clamped.

	\param[in] min
	The minimum amount.

	\param[in] max
	The maximum amount (exclusive).

	\return
	The repeated number.
	*************************************************************************/
	template <typename T>
	static constexpr T Repeat(T x, T min, T max)
	{
		if (max <= min)
			return min;
		while (x < min)
			x += (max - min);
		while (x >= max)
			x -= (max - min);
		return x;
	}

	/*!***********************************************************************
	\brief
	Returns -1 if the input value is negative, 1 if positive.

	\param[in] x
	The number to be tested.

	\return
	The sign of the number.
	*************************************************************************/
	template <typename T>
	static constexpr T Sign(T x)
	{
		if (x < 0.0f)
			return -1.0f;
		else
			return 1.0f;
	}

	/*!***********************************************************************
	\brief
	Returns the lower of 2 values.

	\param[in] x
	The first value.

	\param[in] y
	The second value.

	\return
	The lower value.
	*************************************************************************/
	template <typename T>
	static constexpr T Min(T x, T y)
	{
		return x < y ? x : y;
	}

	/*!***********************************************************************
	\brief
	Returns the higher of 2 values.

	\param[in] x
	The first value.

	\param[in] y
	The second value.

	\return
	The higher value.
	*************************************************************************/
	template <typename T>
	static constexpr T Max(T x, T y)
	{
		return x > y ? x : y;
	}

	/*!***********************************************************************
	\brief
	Returns the higher of 2 values.

	\param[in] x
	The first value.

	\param[in] y
	The second value.

	\return
	The higher value.
	*************************************************************************/
	template <typename T>
	static constexpr T MoveTowards(T x, T y, T delta)
	{
		// Just return end if distance is more than the difference
		if (std::fabs(y - x) <= delta)
		{
			return y;
		}
		// Just return end if distance is more than the difference
		else if (delta < 0.0f)
		{
			return x;
		}
		return x + Sign(y - x) * delta;
	}

}

#pragma region Vector2

    /*!***********************************************************************
    \brief
    2D Vector struct.
    *************************************************************************/
struct Vector2 {
	float x, y;

	/*!***********************************************************************
    \brief
    Default constructor for Vector2. Inits x and y to 0.0f
    *************************************************************************/
	Vector2() : x{ 0.0f }, y{ 0.0f }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector2. Inits x and y with param values.

    \param[in] _x
    The x-coordinate value.

	\param[in] _y
    The y-coordinate value.
    *************************************************************************/
	Vector2(float _x, float _y) : x{ _x }, y{ _y }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector2. Inits x and y with the values from glm::vec2 param.

    \param[in] vec
	The glm::vec2 to initialize from.
    *************************************************************************/
	Vector2(const glm::vec2& vec) : x{ vec.x }, y{ vec.y }
	{
	}


    /*!***********************************************************************
    \brief
    Constructor for Vector2. Inits x and y with the same scalar param

    \param[in] scalar
	The scalar value to initialize x and y with.
    *************************************************************************/
	explicit Vector2(float scalar) : x{ scalar }, y{ scalar }
	{
	}

    /*!***********************************************************************
    \brief
    Conversion operator to glm::vec2.

    \return
	The glm::vec2 representation of the Vector2.
    *************************************************************************/
	operator glm::vec2() const
	{
		return { x, y };
	}

    /*!***********************************************************************
    \brief
    Adds another Vector2 to this Vector2.

    \param[in] other
    The Vector2 to add.

    \return
    A reference to this Vector2 after the addition.
    *************************************************************************/
	Vector2& operator+=(const Vector2& other)
	{
		this->x += other.x;
		this->y += other.y;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Subtracts another Vector2 from this Vector2.

    \param[in] other
    The Vector2 to subtract.

    \return
    A reference to this Vector2 after the subtraction.
    *************************************************************************/
	Vector2& operator -=(const Vector2& other)
	{
		this->x -= other.x;
		this->y -= other.y;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Unary negation operator.

    \return
    The negated Vector2.
    *************************************************************************/
	Vector2 operator -() const
	{
		return { -x, -y };
	}

    /*!***********************************************************************
    \brief
    Divides this Vector2 by a scalar value.

    \param[in] value
    The scalar value to divide by.

    \return
    A reference to this Vector2 after the division.
    *************************************************************************/
	Vector2& operator /=(const float& value)
	{
		this->x /= value;
		this->y /= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Divides this Vector2 by another Vector2 component-wise.

    \param[in] other
    The Vector2 to divide by.

    \return
    The resulting Vector2 after the division.
    *************************************************************************/
	Vector2 operator/(const Vector2& other) const {
		return { x / other.x, y / other.y };
	}

    /*!***********************************************************************
    \brief
    Multiplies this Vector2 by a scalar value.

    \param[in] value
    The scalar value to multiply by.

    \return
    A reference to this Vector2 after the multiplication.
    *************************************************************************/
	Vector2& operator *=(const float& value)
	{
		this->x *= value;
		this->y *= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Equality comparison with another Vector2.

    \param[in] other
    The Vector2 to compare with.

    \return
    True if the Vector2s are equal, and false if not.
    *************************************************************************/
	bool operator==(const Vector2& other) const 
	{
		return std::fabs(x - other.x) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(y - other.y) <= std::numeric_limits<float>::epsilon();
	}

    /*!***********************************************************************
    \brief
    Inequality comparison with another Vector2.

    \param[in] other
    The Vector2 to compare with.

    \return
    True if the Vector2s are not equal, and false if they are.
    *************************************************************************/
	bool operator!=(const Vector2& other) const 
	{
		return !(*this == other);
	}

    /*!***********************************************************************
    \brief
    Rotates this Vector2 by a given angle in radians.

    \param[in] radians
    The angle in radians to rotate by.

    \return
    The rotated Vector2.
    *************************************************************************/
	Vector2 Rotate(const float radians) const
	{
		const float cos = std::cos(radians);
		const float sin = std::sin(radians);
		return { x * cos - y * sin, x * sin + y * cos };
	}

    /*!***********************************************************************
    \brief
    Rotates this Vector2 by 90 degrees.

    \return
    The rotated Vector2.
    *************************************************************************/
	Vector2 Rotate90() const
	{
		return { -y, x };
	}

	/*!***********************************************************************
	\brief
	Creates a Vector2 from a given angle in radians. Anti-clockwise, 0 radians points right.

	\param[in] radians
	The angle in radians.

	\return
	The Vector2.
	*************************************************************************/
	static Vector2 FromAngle(float radians)
	{
		return { std::cosf(radians), std::sinf(radians) };
	}

    /*!***********************************************************************
    \brief
    Normalizes this Vector2.

    \return
    The normalized Vector2.
    *************************************************************************/
	Vector2 Normalize() const
	{
		const float len = Length();
		float px = this->x;
		float py = this->y;
		if (len > std::numeric_limits<float>::epsilon())
		{
			const float inv_len = 1.0f / len;
			px *= inv_len;
			py *= inv_len;
		}
		return { px, py };
	}

    /*!***********************************************************************
    \brief
    Calculates the length/magnitude of this Vector2.

    \return
    The length/magnitude of this Vector2.
    *************************************************************************/
	float Length() const
	{
		return sqrt(x * x + y * y);
	}

    /*!***********************************************************************
    \brief
    Calculates the squared length of this Vector2.

    \return
    The squared length of this Vector2.
    *************************************************************************/
	constexpr float LengthSquared() const
	{
		return (x * x) + (y * y);
	}

    /*!***********************************************************************
    \brief
    Calculates the dot product of this Vector2 with another Vector2.

    \param[in] other
    The other Vector2.

    \return
    The dot product of the two Vector2s.
    *************************************************************************/
	constexpr float Dot(const Vector2& other) const
	{
		return x * other.x + y * other.y;
	}

    /*!***********************************************************************
    \brief
    Calculates the dot product of this Vector2 with another Vector2.

    \param[in] other
    The other Vector2.

    \return
    The cross product of the two Vector2s.
    *************************************************************************/
	Vector2 Cross(const Vector2& other) const
	{
		return { y * other.x - x * other.y, x * other.y - y * other.x };
	}

    /*!***********************************************************************
    \brief
    Calculates the cross product of this Vector2 with another Vector2 as a scalar.

    \param[in] other
    The other Vector2.

    \return
    The cross product of the two Vector2s as a scalar.
    *************************************************************************/
	constexpr float CrossScalar(const Vector2& other) const
	{
		return x * other.y - y * other.x;
	}

	/*!***********************************************************************
	\brief
	Calculates the projection of this Vector2 onto another Vector2.

	\param[in] other
	The other Vector2.

	\return
	The projection of this Vector2 onto the other Vector2.
	*************************************************************************/
	Vector2 Proj(const Vector2& other) const
	{
		return Dot(other) * other;
	}

    /*!***********************************************************************
    \brief
    Multiplies this Vector2 by another Vector2 component-wise.

    \param[in] other
    The other Vector2.

    \return
    The resulting Vector2 after the multiplication.
    *************************************************************************/
	Vector2 operator*(const Vector2& other) const
	{
		return { x * other.x, y * other.y };
	}
};

    /*!***********************************************************************
    \brief
    Calculates the dot product of two Vector2s.

    \param[in] a
    The first Vector2.

	\param[in] b
    The second Vector2.

    \return
    The dot product of the two Vector2s.
    *************************************************************************/
inline float Dot(const Vector2& a, const Vector2& b)
{
	return a.x * b.x + a.y * b.y;
}

    /*!***********************************************************************
    \brief
    Adds two Vector2s.

    \param[in] a
    The first Vector2.

	\param[in] b
    The second Vector2.

    \return
    The sum of the two Vector2s.
    *************************************************************************/
inline Vector2 operator+(Vector2 a, const Vector2& b) 
{
	return a += b;
}

    /*!***********************************************************************
    \brief
    Subtracts one Vector2 from another Vector2.

    \param[in] a
    The first Vector2.

	\param[in] b
    The second Vector2.

    \return
    The difference between the two Vector2s.
    *************************************************************************/
inline Vector2 operator-(Vector2 a, const Vector2& b) 
{
	return a -= b;
}

    /*!***********************************************************************
    \brief
    Divides a Vector2 by a scalar value.

    \param[in] a
    The Vector2 to divide.

	\param[in] value
    The scalar value to divide by.

    \return
    The resulting Vector2 after the division.
    *************************************************************************/
inline Vector2 operator/(Vector2 a, float value) 
{
	return a /= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a Vector2 by a scalar value.

    \param[in] a
    The Vector2 to multiply.

	\param[in] value
    The scalar value to multiply by.

    \return
    The resulting Vector2 after the multiplication.
    *************************************************************************/
inline Vector2 operator*(Vector2 a, float value) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a scalar value by a Vector2.

	\param[in] value
    The scalar value to multiply.

    \param[in] a
    The Vector2 to multiply by.

    \return
    The resulting Vector2 after the multiplication.
    *************************************************************************/
inline Vector2 operator*(float value, Vector2 a) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Calculates the cross product of this Vector2 with another Vector2.

    \param[in] a
    The Vector2 to multiply.

	\param[in] b
    The second Vector2.

    \return
    The cross product of the two Vector2s as a scalar.
    *************************************************************************/
inline float Cross(const Vector2& a, const Vector2& b) 
{
	return a.x * b.y - a.y * b.x;
}

    /*!***********************************************************************
    \brief
    Calculates the cross product of a Vector2 and a scalar.

    \param[in] v
    The Vector2.

	\param[in] a
    The scalar value.

    \return
    The cross product of the Vector2 and the scalar.
    *************************************************************************/
inline Vector2 Cross(const Vector2& v, float a) 
{
	return { a * v.y, -a * v.x };
}

    /*!***********************************************************************
    \brief
    Calculates the squared distance between two Vector2s.

    \param[in] a
    The first Vector2.

	\param[in] b
    The second Vector2.

    \return
    The squared distance between the two Vector2s.
    *************************************************************************/
	inline float DistSqr(const Vector2& a, const Vector2& b) 
	{
		return (a - b).LengthSquared();
	}

    /*!***********************************************************************
    \brief
    Calculates the absolute values of the elements of a Vector2.

    \param[in] v
    The Vector2.

    \return
    The Vector2 with absolute values.
    *************************************************************************/
	inline Vector2 Abs(const Vector2& v) 
	{
		return { std::fabs(v.x), std::fabs(v.y) };
	}

    /*!***********************************************************************
    \brief
    Output stream operator for printing a Vector2.

    \param[out] os
    The output stream.

    \param[in] point
    The Vector2 to print.

    \return
    The output stream after printing the Vector2.
    *************************************************************************/
inline std::ostream& operator<<(std::ostream& os, const Vector2& point) 
{
	return os << "Vector2(" << point.x << "," << point.y << ")";
}

#pragma endregion

#pragma region Vector3

    /*!***********************************************************************
    \brief
    3D Vector struct
    *************************************************************************/
struct Vector3 {
	float x, y, z;

    /*!***********************************************************************
    \brief
    Default constructor for Vector3. Inits x, y, and z to 0.0f
    *************************************************************************/
	Vector3() : x{ 0.0f }, y{ 0.0f }, z{ 0.0f }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector3. Inits x, y and z with param values.

    \param[in] _x
    The x-coordinate value.

	\param[in] _y
    The y-coordinate value.

	\param[in] _z
    The z-coordinate value.
    *************************************************************************/
	Vector3(float _x, float _y, float _z) : x{ _x }, y{ _y }, z{ _z }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector3. Inits x and y with the values Vector2 param.
	Sets z with param value, has default value of 0.0f.

    \param[in] vec2
	The Vector2 to initialize x and y from.

	\param[in] _z
	The z-coordinate value.
    *************************************************************************/
	Vector3(const Vector2& vec2, float _z = 0.0f) : x{ vec2.x }, y{ vec2.y }, z{ _z }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector3. Inits x, y and z with the values from glm::vec3 param.

    \param[in] vec
	The glm::vec3 to initialize from.
    *************************************************************************/
	Vector3(const glm::vec3& vec) : x{ vec.x }, y{ vec.y }, z{ vec.z }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector3. Inits x, y and z with the same scalar param

    \param[in] scalar
	The scalar value to initialize x, y and z with.
    *************************************************************************/
	explicit Vector3(float scalar) : x{ scalar }, y{ scalar }, z{ scalar }
	{
	}

    /*!***********************************************************************
    \brief
    Conversion operator to glm::vec3.

    \return
	The glm::vec3 representation of the Vector3.
    *************************************************************************/
	operator glm::vec3() const
	{
		return { x, y, z };
	}

    /*!***********************************************************************
    \brief
    Adds another Vector3 to this Vector3.

    \param[in] other
	The Vector3 to add.

	\return
	A reference to this Vector3 after the addition.
    *************************************************************************/
	Vector3& operator+=(const Vector3& other) 
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Subtracts another Vector3 from this Vector3.

    \param[in] other
	The Vector3 to subtract.

	\return
	A reference to this Vector3 after the subtraction.
    *************************************************************************/
	Vector3& operator-=(const Vector3& other) 
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Divides this Vector3 by a scalar value.

    \param[in] value
	The scalar value to divide by.

	\return
	A reference to this Vector3 after the division.
    *************************************************************************/
	Vector3& operator/=(const float value) 
	{
		x /= value;
		y /= value;
		z /= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Multiplies this Vector3 by a scalar value.

    \param[in] value
	The scalar value to multiply by.

	\return
	A reference to this Vector3 after the multiplication.
    *************************************************************************/
	Vector3& operator*=(const float value) 
	{
		x *= value;
		y *= value;
		z *= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Unary negation operator.

	\return
	The negated Vector3.
    *************************************************************************/
	Vector3 operator-() const 
	{
		return { -x, -y, -z };
	}


    /*!***********************************************************************
    \brief
    Equality comparison with another Vector3.

    \param[in] other
	The Vector3 to compare with.

	\return
	True if the Vector3s are equal, and false if not.
    *************************************************************************/
	bool operator==(const Vector3& other) const 
	{
		return std::fabs(x - other.x) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(y - other.y) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(z - other.z) <= std::numeric_limits<float>::epsilon();
	}

    /*!***********************************************************************
    \brief
    Inequality comparison with another Vector3.

    \param[in] other
	The Vector3 to compare with.

	\return
	True if the Vector3s are not equal, and false if they are.
    *************************************************************************/
	bool operator!=(const Vector3& other) const 
	{
		return !(*this == other);
	}

    /*!***********************************************************************
    \brief
    Normalizes this Vector3.

	\return
	The normalized Vector3.
    *************************************************************************/
	Vector3 Normalize() const
	{
		const float len = Length();
		float px = this->x;
		float py = this->y;
		float pz = this->z;
		if (len > std::numeric_limits<float>::epsilon())
		{
			const float inv_len = 1.0f / len;
			px *= inv_len;
			py *= inv_len;
			pz *= inv_len;
		}
		return { px, py, pz };
	}

    /*!***********************************************************************
    \brief
    Calculates the length/magnitude of this Vector3.

	\return
	The length/magnitude of this Vector3.
    *************************************************************************/
	float Length() const
	{
		return sqrt(x * x + y * y + z * z);
	}

    /*!***********************************************************************
    \brief
    Calculates the squared length of this Vector3.

	\return
	The squared length of this Vector3.
    *************************************************************************/
	float LengthSquared() const
	{
		return (x * x) + (y * y) + (z * z);
	}

    /*!***********************************************************************
    \brief
    Calculates the dot product of this Vector3 with another Vector3.

	\param[in] other
	The other Vector3.

	\return
	The dot product of the two Vector3s.
    *************************************************************************/
	float Dot(const Vector3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

    /*!***********************************************************************
    \brief
    Calculates the cross product of this Vector3 with another Vector3.

	\param[in] other
	The other Vector3.

	\return
	The cross product of the two Vector3s.
    *************************************************************************/
	Vector3 Cross(const Vector3& other) const
	{
		return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x };
	}
};

    /*!***********************************************************************
    \brief
    Adds two Vector3s.

	\param[in] a
	The first Vector3.

	\param[in] b
	The second Vector3.

	\return
	The sum of the two Vector3s.
    *************************************************************************/
inline Vector3 operator+(Vector3 a, const Vector3& b) 
{
	return a += b;
}

    /*!***********************************************************************
    \brief
    Subtracts one Vector3 from another Vector3.

	\param[in] a
	The first Vector3.

	\param[in] b
	The second Vector3.

	\return
	The difference between the two Vector3s.
    *************************************************************************/
inline Vector3 operator-(Vector3 a, const Vector3& b) 
{
	return a -= b;
}

    /*!***********************************************************************
    \brief
    Divides a Vector3 by a scalar value.

	\param[in] a
	The Vector3 to divide.

	\param[in] value
	The scalar value to divide by.

	\return
	The resulting Vector3 after the division.
    *************************************************************************/
inline Vector3 operator/(Vector3 a, float value) 
{
	return a /= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a Vector3 by a scalar value.

	\param[in] a
	The Vector3 to multiply.

	\param[in] value
	The scalar value to divide by.

	\return
	The resulting Vector3 after the multiplication.
    *************************************************************************/
inline Vector3 operator*(Vector3 a, float value) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a scalar value by a Vector3.

	\param[in] value
	The scalar value to multiply.

	\param[in] a
	The Vector3 to multiply by.

	\return
	The resulting Vector3 after the multiplication.
    *************************************************************************/
inline Vector3 operator*(float value, Vector3 a) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Calculates the dot product of two Vector3s.

	\param[in] a
	The first Vector3.

	\param[in] b
	The second Vector3.

	\return
	The dot product of the two Vector3s.
    *************************************************************************/
inline float Dot(const Vector3& a, const Vector3& b) 
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

    /*!***********************************************************************
    \brief
    Calculates the cross product of two Vector3s.

	\param[in] a
	The first Vector3.

	\param[in] b
	The second Vector3.

	\return
	The cross product of the two Vector3s.
    *************************************************************************/
inline Vector3 Cross(const Vector3& a, const Vector3& b) 
{
	return a.Cross(b);
}

    /*!***********************************************************************
    \brief
    Output stream operator for printing a Vector3.

	\param[out] os
	The output stream.

	\param[in] vec
	The Vector3 to print.

	\return
	The output stream after printing the Vector3.
    *************************************************************************/
inline std::ostream& operator<<(std::ostream& os, const Vector3& vec) 
{
	return os << "Vector3(" << vec.x << "," << vec.y << "," << vec.z << ")";
}

#pragma endregion

#pragma region Vector4

    /*!***********************************************************************
    \brief
    4D Vector struct.
    *************************************************************************/
struct Vector4 {
	float x, y, z, w;

    /*!***********************************************************************
    \brief
    Default constructor for Vector4. Inits x, y, z, and w to 0.0f.
    *************************************************************************/
	Vector4() : x{ 0.0f }, y{ 0.0f }, z{ 0.0f }, w{ 0.0f }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector4. Inits x, y, z, and w with param values.

	\param[in] _x
	The x-coordinate value.

	\param[in] _y
	The y-coordinate value.

	\param[in] _z
	The z-coordinate value.

	\param[in] _w
	The w-coordinate value.
    *************************************************************************/
	Vector4(float _x, float _y, float _z, float _w) : x{ _x }, y{ _y }, z{ _z }, w{ _w }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector4. Inits x, y and z with the values Vector3 param.
	Sets w with param value, has default value of 0.0f.

    \param[in] vec3
	The Vector3 to initialize x, y and z from.

	\param[in] _w
	The w-coordinate value.
    *************************************************************************/
	Vector4(const Vector3& vec3, float _w = 0.0f) : x{ vec3.x }, y{ vec3.y }, z{ vec3.z }, w{ _w }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector4. Inits x, y, z, and w with the values from the glm::vec4 param.

    \param[in] vec
	The glm::vec4 to initialize from.
    *************************************************************************/
	Vector4(const glm::vec4& vec) : x{ vec.x }, y{ vec.y }, z{ vec.z }, w{ vec.w }
	{
	}

    /*!***********************************************************************
    \brief
    Constructor for Vector4. Inits x, y, z and w with the same scalar param

    \param[in] scalar
	The scalar value to initialize x, y, z, and w with.
    *************************************************************************/
	explicit Vector4(float scalar) : x{ scalar }, y{ scalar }, z{ scalar }, w{ scalar }
	{
	}

    /*!***********************************************************************
    \brief
    Conversion operator to glm::vec4.

    \param[in] scalar
	The glm::vec4 representation of the Vector4.
    *************************************************************************/
	operator glm::vec4() const
	{
		return { x, y, z, w };
	}

    /*!***********************************************************************
    \brief
    Adds another Vector4 to this Vector4.

    \param[in] other
	The Vector4 to add.

	\return
	A reference to this Vector4 after the addition.
    *************************************************************************/
	Vector4& operator+=(const Vector4& other) 
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Subtracts another Vector4 from this Vector4.

    \param[in] other
	The Vector4 to subtract.

	\return
	A reference to this Vector4 after the subtraction.
    *************************************************************************/
	Vector4& operator-=(const Vector4& other) 
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Divides this Vector4 by a scalar value.

    \param[in] value
	The scalar value to divide by.

	\return
	A reference to this Vector4 after the division.
    *************************************************************************/
	Vector4& operator/=(const float value) 
	{
		x /= value;
		y /= value;
		z /= value;
		w /= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Multiplies this Vector4 by a scalar value.

    \param[in] value
	The scalar value to multiply by.

	\return
	A reference to this Vector4 after the multiplication.
    *************************************************************************/
	Vector4& operator*=(const float value) 
	{
		x *= value;
		y *= value;
		z *= value;
		w *= value;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Unary negation operator.

	\return
	The negated Vector4.
    *************************************************************************/
	Vector4 operator-() const 
	{
		return { -x, -y, -z, -w };
	}

   /*!***********************************************************************
    \brief
    Equality comparison with another Vector4.

    \param[in] other
	The Vector4 to compare with.

	\return
	True if the Vector4s are equal, and false if not.
    *************************************************************************/
	bool operator==(const Vector4& other) const 
	{
		return std::fabs(x - other.x) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(y - other.y) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(z - other.z) <= std::numeric_limits<float>::epsilon() &&
			std::fabs(w - other.w) <= std::numeric_limits<float>::epsilon();
	}

    /*!***********************************************************************
    \brief
    Inequality comparison with another Vector4.

    \param[in] other
	The Vector4 to compare with.

	\return
	True if the Vector4s are not equal, and false if they are.
    *************************************************************************/
	bool operator!=(const Vector4& other) const 
	{
		return !(*this == other);
	}

    /*!***********************************************************************
    \brief
    Normalize the vector (make its length 1) but keep direction.

	\return
	The normalized Vector4.
    *************************************************************************/
	Vector4 Normalize() const
	{
		const float len = Length();
		float px = this->x;
		float py = this->y;
		float pz = this->z;
		float pw = this->w;
		if (len > std::numeric_limits<float>::epsilon())
		{
			const float inv_len = 1.0f / len;
			px *= inv_len;
			py *= inv_len;
			pz *= inv_len;
			pw *= inv_len;
		}
		return { px, py, pz, pw };
	}

    /*!***********************************************************************
    \brief
    Calculate the length/magnitude of the vector.

	\return
	The length/magnitude of the vector.
    *************************************************************************/
	float Length() const
	{
		return sqrt(x * x + y * y + z * z + w * w);
	}

    /*!***********************************************************************
    \brief
    Calculate the squared length of the vector.

	\return
	The squared length of the vector.
    *************************************************************************/
	float LengthSquared() const
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

    /*!***********************************************************************
    \brief
    Calculate the dot product of this vector with another vector.

	\param[in] other
	The other Vector4.

	\return
	The dot product of the two Vector4s.
    *************************************************************************/
	float Dot(const Vector4& other) const
	{
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

};

    /*!***********************************************************************
    \brief
    Adds two Vector4s.

	\param[in] a
	The first Vector4.

	\param[in] b
	The second Vector4.

	\return
	The sum of the two Vector4s.
    *************************************************************************/
inline Vector4 operator+(Vector4 a, const Vector4& b) 
{
	return a += b;
}

    /*!***********************************************************************
    \brief
    Subtracts one Vector4 from another Vector4.

	\param[in] a
	The first Vector4.

	\param[in] b
	The second Vector4.

	\return
	The difference between the two Vector4s.
    *************************************************************************/
inline Vector4 operator-(Vector4 a, const Vector4& b) 
{
	return a -= b;
}

    /*!***********************************************************************
    \brief
    Divides a Vector4 by a scalar value.

	\param[in] a
	The Vector4 to divide.

	\param[in] value
	The scalar value to divide by.

	\return
	The resulting Vector4 after the division.
    *************************************************************************/
inline Vector4 operator/(Vector4 a, float value) 
{
	return a /= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a Vector4 by a scalar value.

	\param[in] a
	The Vector4 to multiply.

	\param[in] value
	The scalar value to multiply by.

	\return
	The resulting Vector4 after the multiplication.
    *************************************************************************/
inline Vector4 operator*(Vector4 a, float value) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Multiplies a scalar value by a Vector4.

	\param[in] value
	The scalar value to multiply.

	\param[in] a
	The Vector4 to multiply by.

	\return
	The resulting Vector4 after the multiplication.
    *************************************************************************/
inline Vector4 operator*(float value, Vector4 a) 
{
	return a *= value;
}

    /*!***********************************************************************
    \brief
    Calculates the dot product of two Vector4s.

	\param[in] a
	The first Vector4.

	\param[in] b
	The second Vector4.

	\return
	The dot product of the two Vector4s.
    *************************************************************************/
inline float Dot(const Vector4& a, const Vector4& b) 
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

    /*!***********************************************************************
    \brief
    Output stream operator overload for printing vectors.

	\param[out] os
	The first Vector4.

	\param[in] v
	The Vector4 to print.

	\return
	The output stream after printing the Vector4.
    *************************************************************************/
inline std::ostream& operator<<(std::ostream& os, const Vector4& v)
{
	return os << "Vector4(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
}


#pragma endregion

#pragma region Matrix3x3

    /*!***********************************************************************
    \brief
    3x3 Matrix struct.
    *************************************************************************/
struct Matrix3x3 {
	float m[3][3];

    /*!***********************************************************************
    \brief
    Constructor to initialize the matrix with param values.

	\param[in] a11
	The value at row 1, column 1.
	
	\param[in] a12
	The value at row 1, column 2.
	
	\param[in] a13
	The value at row 1, column 3.
	
	\param[in] a21
	The value at row 2, column 1.
	
	\param[in] a22
	The value at row 2, column 2.
	
	\param[in] a23
	The value at row 2, column 3.
	
	\param[in] a31
	The value at row 3, column 1.
	
	\param[in] a32
	The value at row 3, column 2.

	\param[in] a33
	The value at row 3, column 3.
    *************************************************************************/
	Matrix3x3(float a11, float a12, float a13,
		float a21, float a22, float a23,
		float a31, float a32, float a33)
	{
		m[0][0] = a11; m[0][1] = a12; m[0][2] = a13;
		m[1][0] = a21; m[1][1] = a22; m[1][2] = a23;
		m[2][0] = a31; m[2][1] = a32; m[2][2] = a33;
	}

    /*!***********************************************************************
    \brief
    Default constructor for Matrix3x3. Inits the matrix as the identity matrix.
    *************************************************************************/
	Matrix3x3()
	{
		*this = Identity();
	}

    /*!***********************************************************************
    \brief
    Accesses the element at the specified row and column in the matrix for
	manipulation.	

	\param[in] row
	The row index.

	\param[in] col
	The column index.

	\return
	A reference to the element at the specified position
    *************************************************************************/
	float& operator()(int row, int col) 
	{
		return m[row][col];
	}

    /*!***********************************************************************
    \brief
    Accesses the element at the specified row and column in the matrix for
	reading only.	

	\param[in] row
	The row index.

	\param[in] col
	The column index.

	\return
	A const reference to the element at the specified position
    *************************************************************************/
	const float& operator()(int row, int col) const 
	{
		return m[row][col];
	}

    /*!***********************************************************************
    \brief
    Accesses the specified row in the matrix for manipulation.	

	\param[in] row
	The row index.

	\return
	A pointer to the specified row.
    *************************************************************************/
	float* operator[](int row) 
	{
		return m[row];
	}

    /*!***********************************************************************
    \brief
    Accesses the specified row in the matrix for reading only.	

	\param[in] row
	The row index.

	\return
	A const pointer to the specified row.
    *************************************************************************/
	const float* operator[](int row) const 
	{
		return m[row];
	}

	/*!***********************************************************************
    \brief
    Identity matrix.

	\return
	The identity matrix.
    *************************************************************************/
	static Matrix3x3 Identity()
	{
		return {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
		};
	}

	/*!***********************************************************************
    \brief
    Zero matrix.

	\return
	The zero matrix.
    *************************************************************************/
	static Matrix3x3 Zero()
	{
		return {
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f
		};
	}

    /*!***********************************************************************
    \brief
    Sets the matrix to a translation matrix.

	\param[in] pos
	The translation vector.

	\return
	A reference to the modified matrix.
    *************************************************************************/
	Matrix3x3& SetToTranslate(const Vector2& pos)
	{
		*this = Identity();
		m[0][2] = pos.x;
		m[1][2] = pos.y;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Sets the matrix to a rotation matrix around z-axis.

	\param[in] degrees
	The rotation angle in degrees.

	\return
	A reference to the modified matrix.
    *************************************************************************/
	Matrix3x3& SetToRotate(float degrees)
	{
		*this = Identity();
		float sinVal = std::sinf(math::ToRadians(degrees));
		float cosVal = std::cosf(math::ToRadians(degrees));
		m[0][0] = m[1][1] = cosVal;
		m[0][1] = -sinVal;
		m[1][0] = sinVal;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Sets the matrix to a scale matrix.

	\param[in] scale
	The scale vector.

	\return
	A reference to the modified matrix.
    *************************************************************************/
	Matrix3x3& SetToScale(const Vector2& scale)
	{
		*this = Identity();
		m[0][0] = scale.x;
		m[1][1] = scale.y;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Sets the matrix to a transformation matrix.
	
	\param[in] pos
	The translation vector.
	
	\param[in] scale
	The scale vector.

	\param[in] degrees
	The rotation angle in degrees.

	\return
	A reference to the modified matrix.
    *************************************************************************/
	Matrix3x3& SetToTransform(const Vector2& pos, const Vector2& scale, float degrees)
	{
		float sinVal = std::sinf(math::ToRadians(degrees));
		float cosVal = std::cosf(math::ToRadians(degrees));
		m[0][0] = cosVal * scale.x;
		m[0][1] = -sinVal * scale.y;
		m[0][2] = pos.x;

		m[1][0] = sinVal * scale.x;
		m[1][1] = cosVal * scale.y;
		m[1][2] = pos.y;

		m[2][0] = m[2][1] = 0.0f;
		m[2][2] = 1.0f;
		return *this;
	}

    /*!***********************************************************************
    \brief
    Adds another Matrix3x3 to this Matrix3x3.
	
	\param[in] other
	The Matrix3x3 to add.

	\return
	A reference to this Matrix3x3 after the addition.
    *************************************************************************/
    Matrix3x3& operator +=(const Matrix3x3& other)
    {
    	for (int i = 0; i < 3; ++i)
		{
    		for (int j = 0; j < 3; ++j)
    		{
				m[i][j] += other.m[i][j];
			}
		}
    	return *this;
    }

    /*!***********************************************************************
    \brief
    Subtracts another Matrix3x3 from this Matrix3x3.
	
	\param[in] other
	The Matrix3x3 to subtract.

	\return
	A reference to this Matrix3x3 after the subtraction.
    *************************************************************************/
    Matrix3x3& operator -=(const Matrix3x3& other)
    {
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				m[i][j] -= other.m[i][j];
			}
		}
		return *this;
    }

    /*!***********************************************************************
    \brief
    Multiplies this Matrix3x3 by a scalar value.
	
	\param[in] other
	The scalar value to multiply by.

	\return
	A reference to this Matrix3x3 after the multiplication.
    *************************************************************************/
    Matrix3x3& operator *=(float scalar)
    {
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				m[i][j] *= scalar;
			}
		}
		return *this;
    }

    /*!***********************************************************************
    \brief
    Divides this Matrix3x3 by a scalar value.
	
	\param[in] scalar
	The scalar value to divide by.

	\return
	A reference to this Matrix3x3 after the division.
    *************************************************************************/
    Matrix3x3& operator /=(float scalar)
    {
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				m[i][j] /= scalar;
			}
		}
		return *this;
    }

    /*!***********************************************************************
    \brief
    Multiplies this Matrix3x3 by another Matrix3x3.
	
	\param[in] other
	The Matrix3x3 to multiply by.

	\return
	A reference to this Matrix3x3 after the multiplication.
    *************************************************************************/
    Matrix3x3& operator *=(const Matrix3x3& other)
    {
    	Matrix3x3 result = Zero(); // Initialize result as Zero matrix
		for (int i = 0; i < 3; ++i) 
		{
			for (int j = 0; j < 3; ++j) 
			{
				for (int k = 0; k < 3; ++k) 
				{
					result.m[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}
		*this = result; // Update this matrix to the result
		return *this;
    }

    /*!***********************************************************************
    \brief
    Multiplies this Matrix3x3 by a Vector2.
	
	\param[in] other
	The Vector2 to multiply by.

	\param[in] z
	TThe z-coordinate value, has default value of 1.0f.

	\return
	The resulting Vector2 after the multiplication.
    *************************************************************************/
    Vector2 MultVec(const Vector2& other, float z = 1.0f)
    {
		return { m[0][0] * other.x + m[0][1] * other.y + m[0][2] * z,
		m[1][0] * other.x + m[1][1] * other.y + m[1][2] * z };
    }

    /*!***********************************************************************
    \brief
    Calculates the transpose of this Matrix3x3.

	\return
	The transposed Matrix3x3.
    *************************************************************************/
    Matrix3x3 Transpose() const
    {
		return {
		m[0][0], m[1][0], m[2][0],
		m[0][1], m[1][1], m[2][1],
		m[0][2], m[1][2], m[2][2]
		};
    }

    /*!***********************************************************************
    \brief
    Calculates the determinant of this Matrix3x3.

	\return
	The determinant of this Matrix3x3.
    *************************************************************************/
    float Determinant() const
    {
		return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
		m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
		m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }

    /*!***********************************************************************
    \brief
    Calculates the inverse of this Matrix3x3.

	\return
	The inverse of this Matrix3x3.
    *************************************************************************/
    Matrix3x3 Inverse() const
    {
		float det = Determinant();
		if (std::fabs(det) < std::numeric_limits<float>::epsilon()) 
		{
			// Handle the case where the matrix is singular
			return {}; // Return an identity matrix or handle error appropriately
    	}

		float invDet = 1.0f / det;
		Matrix3x3 result;

		result.m[0][0] = invDet * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
		result.m[0][1] = invDet * (m[0][2] * m[2][1] - m[0][1] * m[2][2]);
		result.m[0][2] = invDet * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);

		result.m[1][0] = invDet * (m[1][2] * m[2][0] - m[1][0] * m[2][2]);
		result.m[1][1] = invDet * (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
		result.m[1][2] = invDet * (m[0][2] * m[1][0] - m[0][0] * m[1][2]);

		result.m[2][0] = invDet * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
		result.m[2][1] = invDet * (m[0][1] * m[2][0] - m[0][0] * m[2][1]);
		result.m[2][2] = invDet * (m[0][0] * m[1][1] - m[0][1] * m[1][0]);

		return result;
    }

    /*!***********************************************************************
    \brief
    Equality comparison with another Matrix3x3.

	\param[in] other
	The Matrix3x3 to compare with.

	\return
	True if the Matrix3x3s are equal, false if they are not.
    *************************************************************************/
    bool operator ==(const Matrix3x3& other) const
    {
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				if (fabs(m[i][j] - other.m[i][j]) > std::numeric_limits<float>::epsilon()) // Tolerance for floating-point comparison
				{
					return false;
				}
			}
		}
		return true;
    }

    /*!***********************************************************************
    \brief
    Inequality comparison with another Matrix3x3.

	\param[in] other
	The Matrix3x3 to compare with.

	\return
	True if the Matrix3x3s are not equal, false if they are.
    *************************************************************************/
    bool operator !=(const Matrix3x3& other) const
    {
    	return !(*this == other);
    }
};

    /*!***********************************************************************
    \brief
    Matrix addition.

	\param[in] lhs
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix addition.
    *************************************************************************/
inline Matrix3x3 operator +(Matrix3x3 lhs, const Matrix3x3& rhs)
{
    lhs += rhs;
    return lhs;
}

    /*!***********************************************************************
    \brief
    Matrix subtraction.

	\param[in] lhs
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix subtraction.
    *************************************************************************/
inline Matrix3x3 operator -(Matrix3x3 lhs, const Matrix3x3& rhs)
{
    lhs -= rhs;
    return lhs;
}

    /*!***********************************************************************
    \brief
    Matrix scalar multiplication.

	\param[in] mat
	The matrix.
	
	\param[in] scalar
	The scalar value.

	\return
	The result of matrix scalar multiplication.
    *************************************************************************/
inline Matrix3x3 operator *(Matrix3x3 mat, float scalar)
{
    mat *= scalar;
    return mat;
}

    /*!***********************************************************************
    \brief
    Scalar matrix multiplication.
	
	\param[in] scalar
	The scalar value.

	\param[in] mat
	The matrix.

	\return
	The result of scalar matrix multiplication.
    *************************************************************************/
inline Matrix3x3 operator *(float scalar, Matrix3x3 mat)
{
    mat *= scalar;
    return mat;
}

    /*!***********************************************************************
    \brief
    Matrix scalar division.

	\param[in] mat
	The matrix.
	
	\param[in] scalar
	The scalar value.

	\return
	The result of matrix scalar division.
    *************************************************************************/
inline Matrix3x3 operator /(Matrix3x3 mat, float scalar)
{
    mat /= scalar;
    return mat;
}

    /*!***********************************************************************
    \brief
    Matrix matrix multiplication.

	\param[in] lhs
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix matrix mulitplication.
    *************************************************************************/
inline Matrix3x3 operator *(Matrix3x3 lhs, const Matrix3x3& rhs)
{
    lhs *= rhs;
    return lhs;
}

    /*!***********************************************************************
    \brief
    Matrix vector multiplication.

	\param[in] lhs
	The matrix.
	
	\param[in] rhs
	The vector2.

	\return
	The result of matrix vector mulitplication.
    *************************************************************************/
inline Vector2 operator*(const Matrix3x3& lhs, const Vector2& rhs)
{
    return { lhs[0][0] * rhs.x + lhs[0][1] * rhs.y + lhs[0][2],
             lhs[1][0] * rhs.x + lhs[1][1] * rhs.y + lhs[1][2] };
}

    /*!***********************************************************************
    \brief
    Output stream operator overload for printing matrices.

	\param[out] os
	The output stream.
	
	\param[in] mat
	The matrix to print.

	\return
	The output stream after printing the matrix.
    *************************************************************************/
inline std::ostream& operator<<(std::ostream& os, const Matrix3x3& mat)
{
    for (int i = 0; i < 3; ++i)
    {
        os << "| ";
        for (int j = 0; j < 3; ++j)
        {
            os << mat.m[i][j] << " ";
        }
        os << "|\n";
    }
    return os;
}

#pragma endregion

#pragma region Matrix4x4

    /*!***********************************************************************
    \brief
    4x4 Matrix struct.
    *************************************************************************/
struct Matrix4x4 {
	float m[4][4];

	/*!***********************************************************************
    \brief
    Constructor to initialize the matrix with param values.

	\param[in] a11
	The value at row 1, column 1.
	
	\param[in] a12
	The value at row 1, column 2.
	
	\param[in] a13
	The value at row 1, column 3.
	
	\param[in] a14
	The value at row 1, column 4.

	\param[in] a21
	The value at row 2, column 1.
	
	\param[in] a22
	The value at row 2, column 2.
	
	\param[in] a23
	The value at row 2, column 3.
		
	\param[in] a24
	The value at row 2, column 4.
	
	\param[in] a31
	The value at row 3, column 1.
	
	\param[in] a32
	The value at row 3, column 2.

	\param[in] a33
	The value at row 3, column 3.
	
	\param[in] a34
	The value at row 3, column 4.
		
	\param[in] a41
	The value at row 4, column 1.
	
	\param[in] a42
	The value at row 4, column 2.

	\param[in] a43
	The value at row 4, column 3.
	
	\param[in] a44
	The value at row 4, column 4.
    *************************************************************************/
	Matrix4x4(float a11, float a12, float a13, float a14,
		float a21, float a22, float a23, float a24,
		float a31, float a32, float a33, float a34,
		float a41, float a42, float a43, float a44) 
	{
		m[0][0] = a11; m[0][1] = a12; m[0][2] = a13; m[0][3] = a14;
		m[1][0] = a21; m[1][1] = a22; m[1][2] = a23; m[1][3] = a24;
		m[2][0] = a31; m[2][1] = a32; m[2][2] = a33; m[2][3] = a34;
		m[3][0] = a41; m[3][1] = a42; m[3][2] = a43; m[3][3] = a44;
	}

    /*!***********************************************************************
    \brief
    Default constructor for Matrix4x4. Inits the matrix as the identity matrix.
    *************************************************************************/
	Matrix4x4() 
	{
		*this = Identity();
	}

    /*!***********************************************************************
    \brief
    Constructor to initialize from a Matrix3x3. with default values for row
	and col 4.

	\param[in] mat3x3
	Matrix3x3 to init values in the 3x3 in the matrix.

	\param[in] a14
	The value at row 4, column 1 with default value of 0.0f.
	
	\param[in] a24
	The value at row 4, column 2 with default value of 0.0f.

	\param[in] a34
	The value at row 4, column 3 with default value of 0.0f.
	
	\param[in] a44
	The value at row 4, column 4 with default value of 1.0f.
    *************************************************************************/
	Matrix4x4(const Matrix3x3& mat3x3, float a14 = 0.0f, float a24 = 0.0f, float a34 = 0.0f, float a44 = 1.0f) 
	{
		m[0][0] = mat3x3.m[0][0]; m[0][1] = mat3x3.m[0][1]; m[0][2] = mat3x3.m[0][2]; m[0][3] = a14;
		m[1][0] = mat3x3.m[1][0]; m[1][1] = mat3x3.m[1][1]; m[1][2] = mat3x3.m[1][2]; m[1][3] = a24;
		m[2][0] = mat3x3.m[2][0]; m[2][1] = mat3x3.m[2][1]; m[2][2] = mat3x3.m[2][2]; m[2][3] = a34;
		m[3][0] = 0.0f;            m[3][1] = 0.0f;            m[3][2] = 0.0f;            m[3][3] = a44;
	}

    /*!***********************************************************************
    \brief
    Accesses the matrix elements for manipulation.

    \param[in] row
    The row index.
	
    \param[in] col
    The column index.

    \return
	A reference to the matrix element at the specified row and column.
    *************************************************************************/
    float& operator()(int row, int col) 
	{
    	return m[row][col];
    }

    /*!***********************************************************************
    \brief
    Accesses the matrix elements for reading only.

    \param[in] row
    The row index.
	
    \param[in] col
    The column index.

    \return
	A const reference to the matrix element at the specified row and column.
    *************************************************************************/
    const float& operator()(int row, int col) const 
	{
    	return m[row][col];
    }

    /*!***********************************************************************
    \brief
    Creates an identity matrix.

    \return
	The identity matrix.
    *************************************************************************/
    static Matrix4x4 Identity() 
	{
		return {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		};
    }

    /*!***********************************************************************
    \brief
    Creates a zero matrix.

    \return
	The zero matrix.
    *************************************************************************/
    static Matrix4x4 Zero() 
	{
		return {
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f
		};
    }

   	/*!***********************************************************************
    \brief
    Sets the matrix to a translation matrix.

    \param[in] pos
    The translation vector.

    \return
	A reference to the modified matrix.
    *************************************************************************/
    Matrix4x4& SetToTranslate(const Vector3& pos)
    {
		*this = Identity();
		m[0][3] = pos.x;
		m[1][3] = pos.y;
		m[2][3] = pos.z;
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Adds another Matrix4x4 to this Matrix4x4.

    \param[in] other
    The Matrix4x4 to add.

    \return
	A reference to this Matrix4x4 after the addition.
    *************************************************************************/
    Matrix4x4& operator +=(const Matrix4x4& other) 
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] += other.m[i][j];
			}	
		}
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Subtracts another Matrix4x4 from this Matrix4x4.

    \param[in] other
    The Matrix4x4 to subtract.

    \return
	A reference to this Matrix4x4 after the subtraction.
    *************************************************************************/
    Matrix4x4& operator -=(const Matrix4x4& other) 
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] -= other.m[i][j];
			}
		}
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Multiplies this Matrix4x4 by a scalar value.

    \param[in] scalar
    The scalar value to multiply by.

    \return
	A reference to this Matrix4x4 after the multiplication.
    *************************************************************************/
    Matrix4x4& operator *=(float scalar) 
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] *= scalar;
			}
		}
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Divides this Matrix4x4 by a scalar value.

    \param[in] scalar
    The scalar value to divide by.

    \return
	A reference to this Matrix4x4 after the division.
    *************************************************************************/
    Matrix4x4& operator /=(float scalar) 
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] /= scalar;	
			}
		}
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Multiplies this Matrix4x4 by another Matrix4x4.

    \param[in] other
    The Matrix4x4 to multiply by.

    \return
	A reference to this Matrix4x4 after the multiplication.
    *************************************************************************/
    Matrix4x4& operator *=(const Matrix4x4& other) 
	{
		Matrix4x4 result = Zero(); // Initialize result as Zero matrix
		for (int i = 0; i < 4; ++i) 
		{
			for (int j = 0; j < 4; ++j) 
			{
				for (int k = 0; k < 4; ++k) 
				{
					result.m[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}
		*this = result; // Update this matrix to the result
		return *this;
    }

   	/*!***********************************************************************
    \brief
    Calculates the transpose of this Matrix4x4.

    \return
	The transposed Matrix4x4.
    *************************************************************************/
    Matrix4x4 Transpose() const 
	{
		return {
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]
		};
    }

   	/*!***********************************************************************
    \brief
    Calculates the Determinant of this Matrix4x4.

    \return
	The determinant of this Matrix4x4.
    *************************************************************************/
    float Determinant() const 
	{
		float det = 0.0f;
		for (int i = 0; i < 4; ++i) 
		{
			Matrix3x3 subMatrix;
			for (int j = 0; j < 3; ++j) 
			{
				for (int k = 0; k < 3; ++k) 
				{
					subMatrix.m[j][k] = m[j + 1][(k >= i) ? (k + 1) : k];
				}
			}
			float subDet = subMatrix.Determinant();
			det += ((i % 2 == 0) ? 1.0f : -1.0f) * m[0][i] * subDet;
		}
		return det;
    }

   	/*!***********************************************************************
    \brief
    Calculates the Inverse of this Matrix4x4.

    \return
	The inverse of this Matrix4x4.
    *************************************************************************/
	Matrix4x4 Inverse() const 
	{
		float det = Determinant();
		if (std::fabs(det) < std::numeric_limits<float>::epsilon()) 
		{
			// Handle the case where the matrix is singular
			return {}; // Return an identity matrix or handle error appropriately
		}

		float invDet = 1.0f / det;

		Matrix4x4 result;
		//to actually check if this is correct at a later date, I don't think this is ever going to be called. 
		result.m[0][0] = invDet * (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
			m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
			m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]));

		result.m[0][1] = invDet * -(m[0][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
			m[0][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
			m[0][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]));

		result.m[0][2] = invDet * (m[0][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
			m[0][2] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) +
			m[0][3] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]));

		result.m[0][3] = invDet * -(m[0][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
			m[0][2] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) +
			m[0][3] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]));

		result.m[1][0] = invDet * -(m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
			m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
			m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]));

		result.m[1][1] = invDet * (m[0][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
			m[0][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
			m[0][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]));

		result.m[1][2] = invDet * -(m[0][0] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
			m[0][2] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
			m[0][3] * (m[1][0] * m[3][2] - m[1][2] * m[3][0]));

		result.m[1][3] = invDet * (m[0][0] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
			m[0][2] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
			m[0][3] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]));

		result.m[2][0] = invDet * (m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
			m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
			m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));

		result.m[2][1] = invDet * -(m[0][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
			m[0][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
			m[0][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));

		result.m[2][2] = invDet * (m[0][0] * (m[1][1] * m[3][3] - m[1][3] * m[3][1]) -
			m[0][1] * (m[1][0] * m[3][3] - m[1][3] * m[3][0]) +
			m[0][3] * (m[1][0] * m[3][1] - m[1][1] * m[3][0]));

		result.m[2][3] = invDet * -(m[0][0] * (m[1][1] * m[2][3] - m[1][3] * m[2][1]) -
			m[0][1] * (m[1][0] * m[2][3] - m[1][3] * m[2][0]) +
			m[0][3] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));

		result.m[3][0] = invDet * -(m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
			m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
			m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));

		result.m[3][1] = invDet * (m[0][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
			m[0][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
			m[0][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));

		result.m[3][2] = invDet * -(m[0][0] * (m[1][1] * m[3][2] - m[1][2] * m[3][1]) -
			m[0][1] * (m[1][0] * m[3][2] - m[1][2] * m[3][0]) +
			m[0][2] * (m[1][0] * m[3][1] - m[1][1] * m[3][0]));

		result.m[3][3] = invDet * (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
			m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
			m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));

		return result;
	}

	/*!***********************************************************************
    \brief
    Equality comparison with another Matrix4x4.

	\param[in] other
	The Matrix4x4 to compare with.

	\return
	True if the Matrix4x4s are equal, false if they are not.
    *************************************************************************/
    bool operator ==(const Matrix4x4& other) const 
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (std::fabs(m[i][j] - other.m[i][j]) > std::numeric_limits<float>::epsilon()) // Tolerance for floating-point comparison
				{
					return false;
				}
			}
		}
		return true;
    }

	/*!***********************************************************************
    \brief
    Inequality comparison with another Matrix4x4.

	\param[in] other
	The Matrix4x4 to compare with.

	\return
	True if the Matrix4x4s are not equal, false if they are.
    *************************************************************************/
    bool operator !=(const Matrix4x4& other) const 
	{
    	return !(*this == other);
    }

};

	/*!***********************************************************************
    \brief
    Matrix addition.

	\param[in] lhs
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix addition.
    *************************************************************************/
inline Matrix4x4 operator +(Matrix4x4 lhs, const Matrix4x4& rhs) 
{
	lhs += rhs;
	return lhs;
}

	/*!***********************************************************************
    \brief
    Matrix subtraction.

	\param[in] lhs
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix subtraction.
    *************************************************************************/
inline Matrix4x4 operator -(Matrix4x4 lhs, const Matrix4x4& rhs) 
{
	lhs -= rhs;
	return lhs;
}

	/*!***********************************************************************
    \brief
    Matrix scalar multiplication.

	\param[in] mat
	The matrix.
	
	\param[in] scalar
	The scalar value.

	\return
	The result of matrix scalar multiplication.
    *************************************************************************/
inline Matrix4x4 operator *(Matrix4x4 mat, float scalar) 
{
	mat *= scalar;
	return mat;
}

	/*!***********************************************************************
    \brief
    Scalar matrix multiplication.
	
	\param[in] scalar
	The scalar value.

	\param[in] mat
	The matrix.

	\return
	The result of scalar matrix multiplication.
    *************************************************************************/
inline Matrix4x4 operator *(float scalar, Matrix4x4 mat) 
{
	mat *= scalar;
	return mat;
}

	/*!***********************************************************************
    \brief
    Matrix scalar division.

	\param[in] mat
	The matrix.
	
	\param[in] scalar
	The scalar value.

	\return
	The result of matrix scalar division.
    *************************************************************************/
inline Matrix4x4 operator /(Matrix4x4 mat, float scalar) 
{
	mat /= scalar;
	return mat;
}

	/*!***********************************************************************
    \brief
    Matrix matrtix multiplication.

	\param[in] mat
	The left-hand side matrix.
	
	\param[in] rhs
	The right-hand side matrix.

	\return
	The result of matrix matrix multiplication.
    *************************************************************************/
inline Matrix4x4 operator *(Matrix4x4 lhs, const Matrix4x4& rhs) 
{
	lhs *= rhs;
	return lhs;
}

	/*!***********************************************************************
    \brief
    Overload of the output stream operator for printing a Matrix4x4.

	\param[out] os
	The output stream.
	
	\param[in] mat
	The Matrix4x4 to print.

	\return
	The output stream after printing the Matrix4x4.
    *************************************************************************/
inline std::ostream& operator<<(std::ostream& os, const Matrix4x4& mat) 
{
	for (int i = 0; i < 4; ++i) 
	{
		os << "| ";
		for (int j = 0; j < 4; ++j) 
		{
			os << mat.m[i][j] << " ";
		}
		os << "|\n";
	}
	return os;
}

#pragma endregion
