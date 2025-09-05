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
        Note: Unused for now since current marshalling implementation requires structs instead of classes.
        Unsure if this can be fixed/workaround.
    \return
	    None
    *//******************************************************************/
    [StructLayout(LayoutKind.Sequential)]
    public class ComponentBase : IComponent // <- May need to expand this class
    {
        protected EntityHandle e_ID { get; private set; }

        // For Scripting framework to set this component's entity when the component's class gets duplicated
        private void SetHandle(EntityHandle entityHandle) => e_ID = entityHandle;


        [DllImport("__Internal", EntryPoint = "CS_GetTransform")]
        private static extern Transform GetTransform(EntityHandle entity);
        public Transform transform => GetTransform(e_ID);

        public GameObject gameObject => new GameObject(e_ID);

    }

    public interface IComponent
    {
    }

	[StructLayout(LayoutKind.Sequential)]
    public struct Physics : IComponent
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
