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
using GlmSharp;
using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Runtime.InteropServices;

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
        protected EntityHandle e_ID { get; set; }
        private Transform transComp;
        /*****************************************************************//*!
        \brief
            Sets the e_ID handle to be stored in this class instance. e_ID
            being the unique id of the entity the script is attached to.
        \return
            None
        *//******************************************************************/
        public void SetHandle(EntityHandle handle)
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
        C# counterpart to the c++ TextComponent
    \return
        None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public struct Text : Component
    {
        private vec4 c;
        private string str;

        private UInt64 eid;

        public vec4 color
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
		private vec3 Velocity;
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

        public vec3 velocity
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
