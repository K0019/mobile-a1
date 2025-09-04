/******************************************************************************/
/*!
\file   InternalCall.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/22/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This contains the declarations of the c# counterpart of the internal call
    functions. Their Definitions(and their function level documentation) are the same
    as the ones found in ScriptGlue.cpp
    These declarations are used to link the c++ functions to c# so that they may be 
    used in c# context.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using GlmSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    // This class contains calls from C++ to be used in C#
    internal static class InternalCalls
    {
        #region TestCalls
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CppNativeLog(string message, int value);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void CppNativeLog_Vector(ref vec3 param, out vec3 res);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CppNativeLog_VectorDot(ref vec3 param);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void MoveRight(EntityHandle entityHandle, float unitsMoved);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void ChangeColour(EntityHandle entityHandle, float dt);
        #endregion

        #region Time

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float GetDeltaTime();

        #endregion

        #region Input

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool GetCurrKey(int keyCode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyPressed(int keyCode);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyReleased(int keyCode);
        #endregion

        #region Transform

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransform(EntityHandle entityHandle, out Transform t);

        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //internal extern static void GetChildTransform(UInt64 entityHandle, out Transform t);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalPos(EntityHandle entityHandle, ref vec3 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalPos(EntityHandle entityHandle, out vec3 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldPos(EntityHandle entityHandle, ref vec3 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldPos(EntityHandle entityHandle, out vec3 pos);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalScale(EntityHandle entityHandle, ref vec3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalScale(EntityHandle entityHandle, out vec3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldScale(EntityHandle entityHandle, ref vec3 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldScale(EntityHandle entityHandle, out vec3 scale);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalRotate(EntityHandle entityHandle, vec3 rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalRotate(EntityHandle entityHandle, out vec3 rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldRotate(EntityHandle entityHandle, vec3 rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldRotate(EntityHandle entityHandle, out vec3 rot);
        #endregion

        #region Physics
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsComp(EntityHandle entityHandle, out Physics p);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsMass(EntityHandle entityHandle, out float mass);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsFriction(EntityHandle entityHandle, out float friction);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetPhysicsVelocity(EntityHandle entityHandle, ref vec3 velocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsVelocity(EntityHandle entityHandle, out vec3 velocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetPhysicsAngularVelocity(EntityHandle entityHandle, float angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsAngularVelocity(EntityHandle entityHandle, out float angularVelocity);
        #endregion

        #region Text
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetText(EntityHandle entityHandle, out Text t);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTextColor(EntityHandle entityHandle, ref vec4 c);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTextColor(EntityHandle entityHandle, out vec4 c);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTextString(EntityHandle entityHandle, string text);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static string GetTextString(EntityHandle entHandle);
        #endregion

        #region Logging
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log(string message, int level);
		#endregion

		#region Audio
		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void StartSingleSound(float volume, string name, bool loop);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
  //      internal extern static void StartSingleSoundWithPosition(float volume, string name, bool loop, vec2 position);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void StartGroupedSound(float volume, string name, bool loop);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void StartGroupedSoundWithPosition(float volume, string name, bool loop, vec2 position);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void StopSound(string name);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void StopAllSounds();

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void SetChannelGroup(string soundName, string channelName);

		//[MethodImplAttribute(MethodImplOptions.InternalCall)]
		//internal extern static void SetGroupVolume(float volume, string group);
		#endregion

        #region Entity
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern object GetScriptInstance(EntityHandle entityHandle, string scriptName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern object GetChildScriptInstance(EntityHandle entityHandle, string scriptName);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern EntityHandle FindEntity(string name);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void DestroyEntity(EntityHandle entityHandle);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern EntityHandle InstanstiateGameObject(EntityHandle originalEntityHandle, EntityHandle parentToAttach);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern EntityHandle InstantiatePrefab(string prefabName, EntityHandle parentToAttach);

        #endregion

        #region Camera
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern void SetCameraZoom(float newZoom);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern float GetCameraZoom();
        #endregion

        #region Animator
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void GetAnimatorComp(EntityHandle entityHandle, out Animator a);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void GetChildAnimatorComp(EntityHandle entityHandle, out Animator a);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimation(EntityHandle entityHandle, string name);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimationSpeed(EntityHandle entityHandle, float speed);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimationLooping(EntityHandle entityHandle, bool looping);
        #endregion

        #region GameManager
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool GetPlayerJumpEnhanced();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static bool GetStatusPause();

        #endregion

        #region Cheats
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SpawnEnemyWave();
        #endregion
    }

}
