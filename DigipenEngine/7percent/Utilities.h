/******************************************************************************/
/*!
\file   Utilities.h
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
	This is an interface file for various miscellaneous functions that could be
	useful in a wide variety of cases.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <type_traits>
#include <cmath>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>

// Forward declares
struct Vector2;
struct Vector3;
class Transform;

#pragma region Internal

namespace util {
	namespace internal {

		// No definition because we're only using this for templating the received function.
		template <typename ReturnType, typename ...Args>
		ReturnType FunctionTypeSplitter(ReturnType(*)(Args...));

		// For default arguments in ToSortedVectorOfRefs()
		template <typename T, typename U>
		constexpr auto DefaultBinaryPairPred = [](const std::pair<T, U>& a, const std::pair<T, U>& b) -> bool { return a.first < b.first; };
		template <typename T, typename U>
		constexpr auto DefaultMonoPairSelectPred = [](const std::pair<T, U>&) -> bool { return true; };

	}
}

#pragma endregion // Internal

#pragma region Interface

namespace util {

	/*****************************************************************//*!
	\brief
		Gets the return type of a function.
	*//******************************************************************/
	template <auto Func>
	using ReturnType_t = decltype(internal::FunctionTypeSplitter(Func));

	/*****************************************************************//*!
	\struct IsPair
	\brief
		Type trait that identifies whether a type is a pair.
	*//******************************************************************/
	template <typename T>
	struct IsPair : std::false_type {
	};
	template <typename T1, typename T2>
	struct IsPair<std::pair<T1, T2>> : std::true_type {
	};
	template <typename T>
	constexpr bool IsPair_v = IsPair<T>::value;

	/*****************************************************************//*!
	\struct IsMap
	\brief
		Type trait that identifies whether a type is a map.
	*//******************************************************************/
	template <typename T>
	struct IsMap : std::false_type {
	};
	template <typename Key, typename Value, typename Pr, typename Alloc>
	struct IsMap<std::map<Key, Value, Pr, Alloc>> : std::true_type {
	};
	template <typename Key, typename Value, typename Hasher, typename Keyeq, typename Alloc>
	struct IsMap<std::unordered_map<Key, Value, Hasher, Keyeq, Alloc>> : std::true_type {
	};
	template <typename T>
	constexpr bool IsMap_v = IsMap<T>::value;

	/*****************************************************************//*!
	\struct IsMapIterator
	\brief
		Type trait that identifies whether a type is an iterator of a map
		(value type is a pair).
	*//******************************************************************/
	template <typename T, typename = void>
	struct IsMapIterator : std::false_type {
	};
	template <typename T>
	struct IsMapIterator<T, typename std::enable_if_t<IsPair_v<typename T::value_type>>> : std::true_type {
	};
	template <typename T>
	constexpr bool IsMapIterator_v = IsMapIterator<T>::value;

	/*****************************************************************//*!
	\concept ConvertibleToCArray
	\brief
		Determines whether a container's data is stored as a C array.
		Requires that the container has methods size() and data().
	*//******************************************************************/
	template <typename ContType>
	concept ConvertibleToCArray = requires(ContType container)
	{
		std::size(container);
		std::data(container);
	};

	/*****************************************************************//*!
	\brief
		Holds a static reference to a random engine.
	*//******************************************************************/
	std::mt19937& GetEngine();

	/*****************************************************************//*!
	\brief
		Generates a random number within a range. (works with implicit conversion)
	\tparam T
		The type of value to generate.
	\param minInclusive
		The minimum value.
	\param maxExclusive
		One past the maximum value. (if integer) or the maximum value (if float)
	\return
		The randomly generated value.
	*//******************************************************************/
	template <typename T>
	T RandomRange(T minInclusive, T maxExclusive);

	/*****************************************************************//*!
	\brief
		Generates a random float number within a range. (depreciated) Calls the templated function under the hood, so just use that instead.
	\param min
		The minimum value.
	\param max
		The maximum value.
	\return
		The randomly generated value.
	*//******************************************************************/
	float RandomRangeFloat(float min, float max);

	/*****************************************************************//*!
	\brief
		Samples a value from a perlin noise texture.
	\param x
		The x coordinate of a point in the texture.
	\param y
		The y coordinate of a point in the texture.
	\return
		The sampled value.
	*//******************************************************************/
	float PerlinNoise(float x, float y);

	/*****************************************************************//*!
	\brief
		Converts an uppercase character to lower case.
		Specifically for use with algorithms and strings to avoid conversion warnings.
	\param c
		The character.
	\return
		The character in uppercase.
	*//******************************************************************/
	char ToLower(int c);

	/*****************************************************************//*!
	\brief
		Lerps by a cubic function.
	\tparam T
		The type of value to lerp.
	\param from
		The value at ratio=0
	\param to
		The value at ratio=1
	\param ratio
		The lerp amount being 0 to 1.
	\return
		The result.
	*//******************************************************************/
	template <typename T>
	T LerpCubic(T from, T to, float ratio);

	/*****************************************************************//*!
	\brief
		Classic linear lerp.
	\tparam T
		The type of value to lerp.
	\param from
		The value at ratio=0
	\param to
		The value at ratio=1
	\param ratio
		The lerp amount being 0 to 1.
	\return
		The result.
	*//******************************************************************/
	template <typename T>
	T Lerp(T from, T to, float ratio);

	/*****************************************************************//*!
	\brief
		Lerp over time using a curve y = 1 - e^(-lerpFactor*dt)
	\tparam T
		The type of value to lerp.
	\param from
		The value at ratio=0
	\param to
		The value at ratio=1
	\param lerpFactor
		The speed of the lerp.
	\param dt
		The delta time this update.
	\return
		The result.
	*//******************************************************************/
	template <typename T>
	T Lerp(T from, T to, float lerpFactor, float dt);

	/*****************************************************************//*!
	\brief
		Transforms an unordered_map into a sorted vector, with the option of selecting only
		specific elements to include within the vector. The elements within the vector
		reference the original unordered_map's elements, so the lifetime of the
		vector is the same as the unordered_map.
	\tparam T
		The key type of the unordered_map.
	\tparam U
		The value type of the unordered_map.
	\tparam SortPred
		The type of the predicate for sorting.
	\tparam SelectPred
		The type of the predicate for selecting which elements to include within the vector.
	\param map
		The unordered_map.
	\param sortPred
		The predicate which determines the sorting.
	\param selectPred
		The predicate which determines which elements are included within the vector.
	\return
		The sorted vector.
	*//******************************************************************/
	template <typename T, typename U, typename SortPred = decltype(internal::DefaultBinaryPairPred<T, U>), typename SelectPred = decltype(internal::DefaultMonoPairSelectPred<T, U>)>
		requires std::predicate<SortPred, std::pair<T, U>, std::pair<T, U>>&& std::predicate<SelectPred, std::pair<T, U>>
	std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> ToSortedVectorOfRefs(
		const std::unordered_map<T, U>& map,
		SortPred sortPred = internal::DefaultBinaryPairPred<T, U>,
		SelectPred selectPred = internal::DefaultMonoPairSelectPred<T, U>
	);

	/*****************************************************************//*!
	\brief
		Transforms an unordered_map into a sorted vector, with the option of selecting only
		specific elements to include within the vector. The elements within the vector
		reference the original unordered_map's elements, so the lifetime of the
		vector is the same as the unordered_map.

		NOTE: Compiler needs help to deduce T and U in order to use this function.

	\tparam T
		The key type of the unordered_map.
	\tparam U
		The value type of the unordered_map.
	\tparam SortPred
		The type of the predicate for sorting.
	\tparam SelectPred
		The type of the predicate for selecting which elements to include within the vector.
	\param begin
		An iterator to the start of an unordered_map.
	\param end
		An iterator to 1 past the end of an unordered_map.
	\param sortPred
		The predicate which determines the sorting.
	\param selectPred
		The predicate which determines which elements are included within the vector.
	\return
		The sorted vector.
	*//******************************************************************/
	template <typename T, typename U, typename SortPred = decltype(internal::DefaultBinaryPairPred<T, U>), typename SelectPred = decltype(internal::DefaultMonoPairSelectPred<T, U>)>
		requires std::predicate<SortPred, std::pair<T, U>, std::pair<T, U>>&& std::predicate<SelectPred, std::pair<T, U>>
	std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> ToSortedVectorOfRefs(
		typename std::unordered_map<T, U>::const_iterator begin,
		const typename std::unordered_map<T, U>::const_iterator& end,
		SortPred sortPred = internal::DefaultBinaryPairPred<T, U>,
		SelectPred selectPred = internal::DefaultMonoPairSelectPred<T, U>
	);

	/*****************************************************************//*!
	\brief
		Tests if a point is inside an entity's square transform.
	\param point
		The point.
	\param transform
		The transform.
	\return
		True if the point lies within the specified transform. False otherwise.
	*//******************************************************************/
	bool IsPointInside(const Vector2& point, const Transform& transform);

	/*****************************************************************//*!
	\brief
		Rotates a point about another point.
	\param point
		The point.
	\param center
		The point to rotate about.
	\param angle
		The amount to rotate by.
	\return
		The new point's position.
	*//******************************************************************/
	Vector2 RotatePoint(const Vector2& point, const Vector2& center, float angle);

	/*****************************************************************//*!
	\brief
		Draws a box about an entity to the screen.
	\param transform
		The entity's transform.
	\param color
		The color to draw the box's border.
	\param alpha
		The opacity of the box.
	*//******************************************************************/
	void DrawBoundingBox(const Transform& transform, const Vector3& color, float alpha = 1.0f);

	/*****************************************************************//*!
	\brief
		Draws a box with the specified parameters.
	\param pos
		The center position.
	\param scale
		The scale of the box.
	\param color
		The color to draw the box's border.
	\param rotation
		The rotation of the box.
	\param alpha
		The opacity of the box.
	*//******************************************************************/
	void DrawBoundingBox(const Vector2& pos, const Vector2& scale, const Vector3& color, float rotation = 0.0f, float alpha = 1.0f);

	/*****************************************************************//*!
	\brief
		Draws a line.
	\param start
		The start world position.
	\param end
		The end world position.
	\param color
		The color of the line.
	\param alpha
		The opacity of the line.
	*//******************************************************************/
	void DrawLine(const Vector2& start, const Vector2& end, const Vector3& color, float alpha);
}

/*****************************************************************//*!
\brief
	Put behind an enum class variable to convert it to its underlying type.
\tparam T
	The enum class type.
\param e
	The enum class variable.
\return
	The value of the variable as the underlying type of the enum class.
\code{.cpp}
	ENUM_CLASS_TYPE x; // underlying type is int
	f(+x); // which takes an int param
\endcode
*//******************************************************************/
template <typename T>
constexpr std::enable_if_t<std::is_enum_v<T>, std::underlying_type_t<T>> operator+(T e);

#pragma endregion // Interface

#pragma region Definition

namespace util {
	//overloaded function to ensure that any integral types work together (int, size_t)
	template <typename T1, typename T2>
	auto RandomRange(T1 minInclusive, T2 maxExclusive) {
    using CommonType = std::common_type_t<T1, T2>;
    return RandomRange<CommonType>(static_cast<CommonType>(minInclusive), 
                                  static_cast<CommonType>(maxExclusive));
	}

template<typename T>
T RandomRange(T minInclusive, T maxExclusive) {
    static_assert(std::is_arithmetic_v<T>, "RandomRange requires arithmetic type");
    assert(minInclusive <= maxExclusive && "minInclusive must be less than or equal to maxExclusive");
    if constexpr(std::is_integral_v<T>) {
        // Handle the equal case specially
        if (minInclusive == maxExclusive) {
            return minInclusive;
        }
        std::uniform_int_distribution<T> dist(minInclusive, maxExclusive - 1);
        return dist(GetEngine());
    }
    else if constexpr(std::is_floating_point_v<T>) {
        // Handle the equal case specially
        if (minInclusive == maxExclusive) {
            return minInclusive;
        }
        std::uniform_real_distribution<T> dist(minInclusive, maxExclusive);
        return dist(GetEngine());
    }
    else {
        static_assert(std::is_arithmetic_v<T>, "RandomRange requires arithmetic type");
        return T{}; // won't compile anyway
    }
}
	template<typename T>
	T LerpCubic(T from, T to, float ratio)
	{
		return (to - from) * (3.0f - ratio * 2.0f) * ratio * ratio + from;
	}
	template<typename T>
	T Lerp(T from, T to, float ratio)
	{
		return from * (1.0f - ratio) + to * ratio;
	}

	template<typename T>
	T Lerp(T from, T to, float lerpFactor, float dt)
	{
		return Lerp(from, to, 1.0f - std::powf(2.718281828459045f, -lerpFactor * dt));
	}

	template<typename T, typename U, typename SortPred, typename SelectPred>
		requires std::predicate<SortPred, std::pair<T, U>, std::pair<T, U>>&& std::predicate<SelectPred, std::pair<T, U>>
	std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> ToSortedVectorOfRefs(const std::unordered_map<T, U>& map, SortPred sortPred, SelectPred selectPred)
	{
		std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> vec;
		vec.reserve(map.size());
		for(const auto& entry : map)
			if(selectPred(entry))
				vec.emplace_back(std::ref(entry.first), std::ref(entry.second));
		std::sort(vec.begin(), vec.end(), sortPred);
		return vec;
	}

	template<typename T, typename U, typename SortPred, typename SelectPred>
		requires std::predicate<SortPred, std::pair<T, U>, std::pair<T, U>>&& std::predicate<SelectPred, std::pair<T, U>>
	std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> ToSortedVectorOfRefs(typename std::unordered_map<T, U>::const_iterator begin, const typename std::unordered_map<T, U>::const_iterator& end, SortPred sortPred, SelectPred selectPred)
	{
		std::vector<std::pair<std::reference_wrapper<const T>, std::reference_wrapper<const U>>> vec;
		for(; begin != end; ++begin)
			if(selectPred(*begin))
				vec.emplace_back(std::ref(begin->first), std::ref(begin->second));
		std::sort(vec.begin(), vec.end(), sortPred);
		return vec;
	}

}

template<typename T>
constexpr std::enable_if_t<std::is_enum_v<T>, std::underlying_type_t<T>> operator+(T e)
{
	return static_cast<std::underlying_type_t<T>>(e);
}

#pragma endregion // Definition
