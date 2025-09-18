/******************************************************************************/
/*!
\file   Mathf.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	This file contains the float math utility struct Mathf.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

// Once again, if Marc is able to allow us to use MathF we can swap Math lines with the commented lines that immediately follow them
namespace EngineScripting
{
	public struct Mathf
	{
		public static readonly float PI = (float)Math.PI;

		//public static readonly float PI = MathF.PI;

		public static readonly float Rad2Deg = PI / 180.0f;
		public static readonly float Deg2Rad = 180.0f / PI;
		public static readonly float Epsilon = float.Epsilon;

		#region Angles
		/*****************************************************************//*!
		\brief
			Sine function. Returns sin of input value in RADIANS.
		\param[in] f
			value to sin.
		\return
			Sin(f).
		*//******************************************************************/

		public static float Sin(float f)
		{
			return (float)Math.Sin(f);
			//return MathF.Sin(f);
		}
		/*****************************************************************//*!
		\brief
			Cosine function. Returns cos of input value in RADIANS.
		\param[in] f
			value to cos.
		\return
			Cos(f).
		*//******************************************************************/

		public static float Cos(float f)
		{
			return (float)Math.Cos(f);
			//return MathF.Cos(f);
		}

		/*****************************************************************//*!
		\brief
			Tangent function. Returns tan of input value in RADIANS.
		\param[in] f
			value to tan.
		\return
			Tan(f).
		*//******************************************************************/
		public static float Tan(float f)
		{
			return (float)Math.Tan(f);
			//return MathF.Tan(f);
		}
		/*****************************************************************//*!
		\brief
			Arc sine function. Returns asin of input value in RADIANS.
		\param[in] f
			value to asin.
		\return
			Asin(f).
		*//******************************************************************/
		public static float Asin(float f)
		{
			return (float)Math.Asin(f);
			//return MathF.Asin(f);
		}

		/*****************************************************************//*!
		\brief
			Arc cosine function. Returns acos of input value in RADIANS.
		\param[in] f
			value to acos.
		\return
			Acos(f).
		*//******************************************************************/
		public static float Acos(float f)
		{
			return (float)Math.Acos(f);
			//return MathF.Acos(f);
		}

		/*****************************************************************//*!
		\brief
			Arc tangent function. Returns atan of input value in RADIANS.
		\param[in] f
			value to atan.
		\return
			Atan(f).
		*//******************************************************************/
		public static float Atan(float f)
		{
			return (float)Math.Atan(f);
			//return MathF.Atan(f);
		}

		/*****************************************************************//*!
		\brief
			Arc tangent 2 function. Returns atan in RADIANS, of angle whose tangent is y/x.
		\param[in] y
			y value.
		\param[in] x
			x value.
		\return
			Tan(f).
		*//******************************************************************/
		public static float Atan2(float y, float x)
		{
			return (float)Math.Atan2(y, x);
			//return MathF.Atan2(y,x);
		}

		/*****************************************************************//*!
		\brief
			Delta Angle function. Returns the angle difference between two angles
			in DEGREES.
		\param[in] current
			Starting angle.
		\param[in] target
			Target angle.
		\return
			Difference in angle of current and target.
		*//******************************************************************/
		public static float DeltaAngleDegrees(float current, float target)
		{
			// Repeat here in case target - current is larger than 360 degrees
			float distance = Repeat(target - current, 360.0f);

			// If it's more than 180 degrees it'll be faster to go the other direction
			if (distance > 180.0f)
			{
				distance -= 360.0f;
			}
			return distance;
		}

		#endregion
		/*****************************************************************//*!
		\brief
			Sign function. Returns -1 if f is negative, 1 if positive or zero.
		\param[in] f
			Value to test.
		\return
			-1 or 1.
		*//******************************************************************/
		public static float Sign(float f)
		{
			return f < 0.0f ? -1.0f : 1.0f;
		}
		/*****************************************************************//*!
		\brief
			Square Root function.
		\param[in] f
			Value to square root.
		\return
			Sqrt(f).
		*//******************************************************************/
		public static float Sqrt(float f)
		{
			return (float)Math.Sqrt(f);
			//return MathF.Sqrt(f);
		}

		/*****************************************************************//*!
		\brief
			Absolute function. Returns f as a positive value.
		\param[in] f
			Value to absolute.
		\return
			Abs(f).
		*//******************************************************************/
		public static float Abs(float f)
		{
			// System.Math contains an Abs for floats
			return Math.Abs(f);
		}
		#region Min, Max, Floor, Ceil
		/*****************************************************************//*!
		\brief
			Min function. Returns lower of two values.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\return
			Lower of a or b.
		*//******************************************************************/
		public static float Min(float a, float b)
		{
			return (a < b) ? a : b;
		}
		/*****************************************************************//*!
		\brief
			Min function. Returns lower of two values.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\return
			Lower of a or b.
		*//******************************************************************/
		public static float Max(float a, float b)
		{
			return (a > b) ? a : b;
		}
		/*****************************************************************//*!
		\brief
			Floor function. Rounds f DOWN to the nearest integer.
		\param[in] f
			Value to floor.
		\return
			Floor(f).
		*//******************************************************************/
		public static float Floor(float f)
		{
			return (float)Math.Floor(f);
			//return MathF.Floor(f);
		}
		/*****************************************************************//*!
		\brief
			Ceil function. Rounds f UP to the nearest integer.
		\param[in] f
			Value to ceil.
		\return
			Ceil(f).
		*//******************************************************************/
		public static float Ceil(float f)
		{
			return (float)Math.Ceiling(f);
			//return MathF.Floor(f);
		}
		#endregion
		#region Clamp, Repeat, Lerp, MoveTowards
		/*****************************************************************//*!
		\brief
			Clamp function. Returns f with value clamped between lowerBound 
			and upperBound.
		\param[in] f
			Value to clamp.
		\param[in] lowerBound
			Lower bound to clamp.
		\param[in] upperBound
			Upper bound to clamp.
		\return
			f clamped between lowerBound and upperBound. lowerBound if it is 
			higher than upperBound.
		*//******************************************************************/
		public static float Clamp(float f, float lowerBound, float upperBound)
		{
			// Clamp to lower
			if (f < lowerBound)
			{
				return lowerBound;
			}
			// Clamp to upper
			else if (f > upperBound)
			{
				return upperBound;
			}

			return f;

			// Ternary hell version
			//return f < lowerBound ? lowerBound : (f > upperBound ? upperBound : f);
		}

		/*****************************************************************//*!
		\brief
			Repeat function. Returns f with value repeating from 0 to upperBound.
		\param[in] f
			Value to clamp.
		\param[in] upperBound
			Upper bound.
		\return
			f repeated between 0 and upperBound.
		*//******************************************************************/
		public static float Repeat(float f, float upperBound)
		{
			// Return the remainder of f / upperBound
			return f - (Floor(f / upperBound) * upperBound);

			// Pretty sure this should work too but I'm too sleepy to test
			//return f % upperBound;

			// Code from Unity's Mathf, Clamp seems redundant
			//return Clamp(t - Floor(t / length) * length, 0f, length);

		}

		/*****************************************************************//*!
		\brief
			UNCLAMPED Lerp function. Returns a float that is linearly interpolated
			between a and b.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\param[in] amount
			Amount to interpolate.
		\return
			Lerped value between a and b.
		*//******************************************************************/

		public static float LerpUnclamped(float a, float b, float amount)
		{
			return a + (b - a);
		}

		/*****************************************************************//*!
		\brief
			CLAMPED Lerp function. Returns a float that is linearly interpolated
			between a and b.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\param[in] amount
			Amount to interpolate.
		\return
			Lerped value between a and b.
		*//******************************************************************/
		public static float Lerp(float a, float b, float amount)
		{
			return LerpUnclamped(a, b, Clamp(amount, 0.0f, 1.0f));
		}

		/*****************************************************************//*!
		\brief
			Move Towards function. Returns a float that is translated from start 
			to end by the provided distance and is clamped between start and end.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\param[in] amount
			Amount to move by.
		\return
			Lerped value between start and end.
		*//******************************************************************/
		public static float MoveTowards(float start, float end, float distance)
		{
			// Just return end if distance is more than the difference
			if (Abs(end - start) <= distance)
			{
				return end;
			}
			// Just return end if distance is more than the difference
			else if (distance < 0.0f)
			{
				return start;
			}
			return start + Sign(end - start) * distance;
		}

		/*****************************************************************//*!
		\brief
			Move Towards function. Returns a float that is translated from start 
			to end by the provided distance and is clamped between start and end.
		\param[in] a
			First value.
		\param[in] b
			Second value.
		\param[in] amount
			Amount to interpolate.
		\return
			Lerped value between a and b.
		*//******************************************************************/
		public static float MoveTowardsAngle(float current, float target, float delta)
		{
			// Just return current if delta is less than 0
			if (delta < 0.0f)
			{
				return current;
			}

			float difference = DeltaAngleDegrees(current, target);

			// Return target if delta is more than the angle delta
			if (delta > -difference && difference < delta)
			{
				return target;
			}
			target = current + difference;
			return MoveTowards(current, target, delta);
		}

		#endregion
	}
}
