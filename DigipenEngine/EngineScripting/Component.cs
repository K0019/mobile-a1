/******************************************************************************/
/*!
\file   Component.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This file contains the Component Counterparts found in the C++ side percentmath.h
    More will be added as needed

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Net;

namespace EngineScripting
{
    /*****************************************************************//*!
    \brief
	    Parent class to all scripts that are attachable to Entities.
    \return
	    None
    *//******************************************************************/
    public class EID // <- May need to expand this class
    {
        protected UInt64 e_ID { get; set; }
        private Transform transComp;
        /*****************************************************************//*!
        \brief
            Sets the e_ID handle to be stored in this class instance. e_ID
            being the unique id of the entity the script is attached to.
        \return
            None
        *//******************************************************************/
        public void SetHandle(UInt64 handle)
        {
            e_ID = handle;
            InternalCalls.GetTransform(e_ID, out transComp);
        }
        public ref Transform transform
        {
            get
            {
                InternalCalls.GetTransform(e_ID, out transComp);
                return ref transComp;
            }
        }

        protected void SetTransform(Transform t)
        {
            transComp = t; 
        }

        protected GameObject gameObject
        {
            get
            {
                return new GameObject(e_ID);
            }
        }

    }

    public interface Component
    {
        // This is empty and is just a marker to contain all components
    }


    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ vector2 struct
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct RaycastHit
    {
        public float distance;
        public Vector2 point;
        public UInt64 entity;
	}
    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ vector2 struct
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float x;
        public float y;

        /*****************************************************************//*!
        \brief
            Non-default constructor for Vector2
        \param[in] _x
            value to init x var of Vector2
        \param[in] _y
            value to init y var of Vector2
        \return
            None
        *//******************************************************************/
        public Vector2(float _x = 0.0f, float _y = 0.0f)
        {
            x = _x;
            y = _y;
        }

        /*****************************************************************//*!
        \brief
            Addition operator overload for Vector2
        \param[in] a
            Left side Vector2 to add.
        \param[in] b
            Right side Vector2 to add.
        \return
            Vector2 containing added values
        *//******************************************************************/
        public static Vector2 operator +(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x + b.x, a.y + b.y);
        }

        /*****************************************************************//*!
        \brief
            Subtraction operator overload for Vector2
        \param[in] a
            Left side Vector2 to subtract from.
        \param[in] b
            Right side Vector2 to subtract.
        \return
            Vector2 containing subtracted values
        *//******************************************************************/
        public static Vector2 operator -(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x - b.x, a.y - b.y);
        }

        /// <summary>
        /// Matthew's code here
        /// </summary>
        
        //Unity mimicry
        public static readonly Vector2 zero = new Vector2(0.0f, 0.0f);
        public static readonly Vector2 one = new Vector2(1.0f, 1.0f);
        public static readonly Vector2 up = new Vector2(0.0f, 1.0f);
        public static readonly Vector2 down = new Vector2(0.0f, -1.0f);
        public static readonly Vector2 left = new Vector2(-1.0f, 0.0f);
        public static readonly Vector2 right = new Vector2(1.0f, 0.0f);
        public Vector2 normalized { get { return Normalize(this); } }
        public Vector2 perpendicular { get { return Perpendicular(this); } }
        public float magnitude { get { return Magnitude(this); } }
        public float sqrMagnitude { get { return MagnitudeSqr(this); } }

		/*****************************************************************//*!
        \brief
	        Scalar multiply function for Vector2
        \param[in] a
	        Vector2 to multiply by b.
        \param[in] b
	        The scalar to multiply a by.
        \return
	       Vector2 with x and y coordinates multiplied by b
        *//******************************************************************/
		public static Vector2 operator *(Vector2 a, float b)
        {
            return new Vector2(a.x * b, a.y * b);
        }
		/*****************************************************************//*!
        \brief
	        Scalar multiply function for Vector2 with order swapped
        \param[in] b
	        The scalar to multiply a by.
        \param[in] a
	        Vector2 to multiply by b.
        \return
	       Vector2 with x and y coordinates multiplied by b
        *//******************************************************************/
		public static Vector2 operator *(float b, Vector2 a)
        {
            return a * b;
        }
		/*****************************************************************//*!
        \brief
	        Scalar divide function for Vector2
        \param[in] a
	        Vector2 to divide by b.
        \param[in] b
	        The scalar to divide a by.
        \return
	       Vector2 with x and y coordinates divided by b
        *//******************************************************************/
		public static Vector2 operator /(Vector2 a, float b)
        {
            return new Vector2(a.x / b, a.y / b);
        }

		/*****************************************************************//*!
        \brief
            Magnitude function for Vector2
        \return
            Calculated magnitude
        *//******************************************************************/
		public static float Magnitude(Vector2 value)
        {
            // Cast to float
            return (float)Math.Sqrt(MagnitudeSqr(value));

            // We don't appear to have access to MathF at the moment, so @Marc I'll leave this up to you. The next line should work on .NET 2.0 
            // https://learn.microsoft.com/en-us/dotnet/api/system.mathf.sqrt?view=net-9.0
            //return MathF.Sqrt(MagnitudeSqr(value));
        }

        /*****************************************************************//*!
        \brief
            Square Magnitude function for Vector2
        \param[in] value
            Vector2 to calculate the squared magnitude of
        \return
            Square magnitude of value
        *//******************************************************************/
        public static float MagnitudeSqr(Vector2 value)
        {
            return (value.x * value.x) + (value.y * value.y);

            // Functionally the same but I can't add this in good conscience
            //return value.Dot(value);
        }

        /*****************************************************************//*!
        \brief
            Normalize function for Vector2
        \param[in] value
		    Vector2 to normalize.
        \return
            Vector2 after normalizing values or zero Vector2 if the length is 0
        *//******************************************************************/
        public static Vector2 Normalize(Vector2 value)
        {
			// Get Length
			float len = Magnitude(value);

            //Return normalized vector if length is more than 0
            if (len > 0.0f)
            {
                return new Vector2(value.x / len, value.y / len);
            }

            // Return zero vector
            return zero;
        }
        /*****************************************************************//*!
        \brief
            Normalize function for Vector2, normalizes self
        \return
            Vector2 after normalizing values
        *//******************************************************************/
        public Vector2 Normalize()
        {
            // Get Length
            float len = magnitude;

            // Divide self by length if it's more than 0
            // We don't need to change anything if the length is already 0
            if (len > 0.0f)
            {
                this /= len;
            }

            return this;
        }
        /*****************************************************************//*!
	    \brief
		    Dot product function for Vector2. Returns the result of a . b
	    \param[in] a
		    Left side Vector2 to dot.
	    \param[in] b
		    Right side Vector2 to dot.
	    \return
		    a . b
	    *//******************************************************************/
        public static float Dot(Vector2 a, Vector2 b)
        {
            return a.x * b.x + a.y * b.y;
        }
        /*****************************************************************//*!
	    \brief
		    Perpendicular function for Vector2. Rotates the input Vector2 by 90 degrees.
	    \param[in] value
		    Vector2 to rotate.
	    \return
		    Vwctor2 with equivalent value of (-y, x).
	    *//******************************************************************/
        public static Vector2 Perpendicular(Vector2 value)
        {
            return new Vector2(-value.y, value.x);
        }

        /*****************************************************************//*!
	    \brief
		    Dot product function for Vector2. Returns the result of this . b
	    \param[in] b
		    Right side Vector2 to dot.
	    \return
		    this . b
	    *//******************************************************************/
        public float Dot(Vector2 b)
        {
            return x * b.x + y * b.y;
        }


        /*****************************************************************//*!
	    \brief
		    Move Towards function for Vector2. Gets a position which is moved 
            from start to end by the provided distance, clamping at the end if
            exceeded.
	    \param[in] start
		    The start vector.
	    \param[in] end
		    The end vector.
	    \param[in] distance
		    The maximum distance to travel.
	    \return
		    start translated towards end by distance
	    *//******************************************************************/
        public static Vector2 MoveTowards(Vector2 start, Vector2 end, float distance)
        {
            // Get distance in x and y
            Vector2 distanceVector = end - start;
            // Lengthsquared first, so if the later conditions fail we won't have to do
            // the Sqrt
            float lengthsquared = distanceVector.sqrMagnitude;

            // Just return end if distance exceeds the length or the distanceVector length is 0
            if (lengthsquared <= 0.0f || (distance >= 0f && lengthsquared <= distance * distance))
                return end;
            // Just return start if distance is 0, since we aren't moving
            else if (distance <= 0.0f)
                return start;

            // Finally perform the Sqrt
            float length = (float)Math.Sqrt(lengthsquared);

            // Return a Vector2 which is the equivalent of start translated towards end by distance/length
            return new Vector2(start.x + distanceVector.x * distance / length, start.y + distanceVector.y * distance / length);
        }

		/*****************************************************************//*!
	    \brief
		    UNCLAMPED Lerp function for Vector2. Gets a position which is 
            linerly interpolated between start and end. This can exceed start and end.
	    \param[in] start
		    The start vector.
	    \param[in] end
		    The end vector.
	    \param[in] amount
		    The amount to interpolate by.
	    \return
		    start interpolated towards end by amount.
	    *//******************************************************************/
		public static Vector2 LerpUnclamped(Vector2 start, Vector2 end, float amount)
        {
            return start + (end - start) * amount;
        }

		/*****************************************************************//*!
	    \brief
		    CLAMPED Lerp function for Vector2. Gets a position which is 
            linerly interpolated between start and end. This will not exceed 
            start or end.
	    \param[in] start
		    The start vector.
	    \param[in] end
		    The end vector.
	    \param[in] amount
		    The amount to interpolate by.
	    \return
		    start interpolated towards end by amount.
	    *//******************************************************************/
		public static Vector2 Lerp(Vector2 start, Vector2 end, float amount)
        {
            // Clamp amount
            amount = Mathf.Clamp(amount,0.0f,1.0f);

            // Reuse LerpUnclamped with the clamped value
            return LerpUnclamped(start, end, amount);
        }

		/*****************************************************************//*!
	    \brief
		    Angle function for Vector2, returns the angle between 2 Vector2s
            in RADIANS.
	    \param[in] from
		    The start vector.
	    \param[in] to
		    The end vector.
	    \param[in] amount
		    The amount to interpolate by.
	    \return
		    start interpolated towards end by amount.
	    *//******************************************************************/
		//public static float Angle(Vector2 from, Vector2 to)
        //{
        //    float angle = AngleRadians(from, to);
        //    return angle * 180.0f/(float)Math.PI;
        //}

		/*****************************************************************//*!
	    \brief
		    Angle function for Vector2, returns the angle between 2 Vector2s
            in RADIANS.
	    \param[in] from
		    The start vector.
	    \param[in] to
		    The end vector.
	    \param[in] amount
		    The amount to interpolate by.
	    \return
		    start interpolated towards end by amount.
	    *//******************************************************************/
		//public static float AngleRadians(Vector2 from, Vector2 to)
        //{
        //    float magsquared = from.sqrMagnitude*to.sqrMagnitude;
        //
        //    // Return 0 degrees if either of the vectors has 0.0 length
        //    if(magsquared <= Mathf.Epsilon)
        //    {
        //        return 0.0f;
        //    }
        //
        //
        //    return angle;
        //}
    }

    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ vector3 struct
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float x, y, z;


        /*****************************************************************//*!
        \brief
            Non-default constructor for Vector3
        \param[in] _x
            value to init x var of Vector3
        \param[in] _y
            value to init y var of Vector3
        \param[in] _z
            value to init z var of Vector3
        \return
            None
        *//******************************************************************/
        public Vector3(float _x, float _y, float _z)
        {
            this.x = _x;
            this.y = _y;
            this.z = _z;
        }
    }

    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ vector4 struct
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4
    {
        public float x, y, z, w;


        /*****************************************************************//*!
        \brief
            Non-default constructor for Vector3
        \param[in] _x
            value to init x var of Vector4
        \param[in] _y
            value to init y var of Vector4
        \param[in] _z
            value to init z var of Vector4
        \param[in] _w
            value to init w var of Vector4
        \return
            None
        *//******************************************************************/
        public Vector4(float _x, float _y, float _z, float _w)
        {
            this.x = _x;
            this.y = _y;
            this.z = _z;
            this.w = _w;
        }
    }


    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ Matrix3x3 struct
    \return
        None
    *//******************************************************************/
    public struct Matrix3x3
    {
        float[,] m;
        /*****************************************************************//*!
        \brief
            Non-default constructor for Matrix3x3
        \param[in] values
            Array of floats to init m
        \return
            None
        *//******************************************************************/
        public Matrix3x3(float[,] values)
        {
            m = values;
        }

        // add other stuff here
    }

    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ Transform struct
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Transform : Component
    {
        private Vector2 _position;
        private Vector2 _localPosition;
        private Vector2 _scale;
        private Vector2 _localScale;
        private float _rotation;
        private float _localRotation;
        private float _zPosition;

        private readonly UInt64 eid;

        public UInt64 GetID() => eid;


        public Vector2 position
		{
            get
            {
                InternalCalls.GetTransformWorldPos(eid, out _position);
                return _position;
            }// Return the current world position
			set
			{
				_position = value; // Update the internal position
				InternalCalls.SetTransformWorldPos(eid, ref _position); // Update C++ data
			}
		}
		public Vector2 localPosition
		{
            get
            {
                InternalCalls.GetTransformLocalPos(eid, out _localPosition);
                return _localPosition;
            }// Return the current local position
			set
			{
				_localPosition = value; // Update the internal local position
				InternalCalls.SetTransformLocalPos(eid, ref _localPosition); // Update C++ data
			}
		}

        public Vector2 scale
        {
            get
            {
                InternalCalls.GetTransformWorldScale(eid, out _scale);
                return _scale;
            }
            set
            {
				_scale = value;
                InternalCalls.SetTransformWorldScale(eid, ref _scale);
            }
        }

        public Vector2 localScale
        {
            get
            {
                InternalCalls.GetTransformLocalScale(eid, out _localScale);
                return _localScale;
            }
            set
            {
				_localScale = value;
                InternalCalls.SetTransformLocalScale(eid, ref _localScale);
            }
        }

        public float rotation
        {
            get
            {
                InternalCalls.GetTransformWorldRotate(eid, out _rotation);
                return _rotation;
            }
            set
            {
                _rotation = value;
                InternalCalls.SetTransformWorldRotate(eid, _rotation);
            }
        }

        public float localRotation
        {
            get
            {
                InternalCalls.GetTransformWorldRotate(eid, out _localRotation);
                return _localRotation;
            }
            set
            {
				_localRotation = value;
                InternalCalls.SetTransformWorldRotate(eid, _localRotation);
            }
        }

        public float zPosition
        {
            get
            {
                InternalCalls.GetTransformZPos(eid, out _zPosition);
                return _zPosition;
            }
            set
            {
                _zPosition = value;
                InternalCalls.SetTransformZPos(eid, _zPosition);
            }
        }
        
    }

    /*****************************************************************//*!
    \brief
        C# counterpart to the c++ TextComponent
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Text : Component
    {
        private Vector4 c;
        private string str;

        private UInt64 eid;

        public Vector4 color
        {
            get => c;
            set
            {
                c = value;
                InternalCalls.SetTextColor(eid, ref c);
            }
        }

        public string text
        {
            get 
            {
                InternalCalls.GetTextString(eid);
                return str;
            }
            set
            {
                str = value;
                InternalCalls.SetTextString(eid, str);
            }
        }

    }

	[StructLayout(LayoutKind.Sequential)]
    public struct Physics : Component
    {
		private float Mass;
		//private float RestitutionCoeff;
		private float FrictionCoeff;
		private Vector2 Velocity;
		private float AngVelocity;

        private UInt64 eid;

        public float GetMass()
        {
            InternalCalls.GetPhysicsMass(eid, out Mass);
            return Mass;
        }

        //public float GetRestitution()
        //{
        //    return RestitutionCoeff;
        //}
        //public void SetRestitution(float restitution)
        //{
        //    RestitutionCoeff = restitution;
        //}

        public float GetFriction()
        {
            return FrictionCoeff;
        }

        public Vector2 velocity
        {
            get
            {
                InternalCalls.GetPhysicsVelocity(eid, out Velocity);
                return Velocity;
            }
            set
            {
                Velocity = value;
                InternalCalls.SetPhysicsVelocity(eid, ref Velocity);
            }
        }
        public float angularVelocity
        {
            get
            {
                InternalCalls.GetPhysicsAngularVelocity(eid, out AngVelocity);
                return AngVelocity;
            }
            set
            {
                AngVelocity = value;
                InternalCalls.SetPhysicsAngularVelocity(eid, AngVelocity);
            }
        }
	}
}
