/******************************************************************************/
/*!
\file   Animator.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	Contains the API for the Animator component

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    public struct Animator : Component
    {
        UInt64 eid;

		/*****************************************************************//*!
		\brief
			Sets the current animation of the entity.
		\param[in] name
			Name of the animation.
        \return
            None.
		*//******************************************************************/
		public void SetAnimation(string name)
        {
            InternalCalls.SetAnimation(eid, name);
        }

		/*****************************************************************//*!
		\brief
			Sets the speed of the animation of the entity.
		\param[in] speed
			Speed of the animation
        \return
            None.
		*//******************************************************************/
		public void SetSpeed(float speed) 
        {
            InternalCalls.SetAnimationSpeed(eid, speed);
        }

		/*****************************************************************//*!
		\brief
			Sets the the looping flag of the animation
		\param[in] looping
			bool of if its looping or not.
		\return
			None.
		*//******************************************************************/
		public void SetLooping(bool looping) 
        {
           InternalCalls.SetAnimationLooping(eid, looping);
        }


    }
}
