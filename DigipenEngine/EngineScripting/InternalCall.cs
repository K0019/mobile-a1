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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
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
        internal extern static void CppNativeLog_Vector(ref Vector3 param, out Vector3 res);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float CppNativeLog_VectorDot(ref Vector3 param);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void MoveRight(UInt64 entityHandle, float unitsMoved);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void ChangeColour(UInt64 entityHandle, float dt);
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
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetMouseWorldPos(out Vector2 pos);
        #endregion

        #region Transform

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransform(UInt64 entityHandle, out Transform t);

        //[MethodImplAttribute(MethodImplOptions.InternalCall)]
        //internal extern static void GetChildTransform(UInt64 entityHandle, out Transform t);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalPos(UInt64 entityHandle, ref Vector2 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalPos(UInt64 entityHandle, out Vector2 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldPos(UInt64 entityHandle, ref Vector2 pos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldPos(UInt64 entityHandle, out Vector2 pos);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalScale(UInt64 entityHandle, ref Vector2 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalScale(UInt64 entityHandle, out Vector2 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldScale(UInt64 entityHandle, ref Vector2 scale);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldScale(UInt64 entityHandle, out Vector2 scale);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformLocalRotate(UInt64 entityHandle, float rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformLocalRotate(UInt64 entityHandle, out float rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformWorldRotate(UInt64 entityHandle, float rot);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformWorldRotate(UInt64 entityHandle, out float rot);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTransformZPos(UInt64 entityHandle, float zPos);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTransformZPos(UInt64 entityHandle, out float zPos);
        #endregion

        #region Physics
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsComp(UInt64 entityHandle, out Physics p);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsMass(UInt64 entityHandle, out float mass);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsFriction(UInt64 entityHandle, out float friction);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetPhysicsVelocity(UInt64 entityHandle, ref Vector2 velocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsVelocity(UInt64 entityHandle, out Vector2 velocity);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetPhysicsAngularVelocity(UInt64 entityHandle, float angularVelocity);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetPhysicsAngularVelocity(UInt64 entityHandle, out float angularVelocity);
        #endregion

        #region Text
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetText(UInt64 entityHandle, out Text t);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTextColor(UInt64 entityHandle, ref Vector4 c);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void GetTextColor(UInt64 entityHandle, out Vector4 c);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void SetTextString(UInt64 entityHandle, string text);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static string GetTextString(UInt64 entHandle);
        #endregion

        #region Logging
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Log(string message, int level);
		#endregion

		#region Audio
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void StartSingleSound(float volume, string name, bool loop);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void StartSingleSoundWithPosition(float volume, string name, bool loop, Vector2 position);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void StartGroupedSound(float volume, string name, bool loop);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void StartGroupedSoundWithPosition(float volume, string name, bool loop, Vector2 position);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void StopSound(string name);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void StopAllSounds();

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void SetChannelGroup(string soundName, string channelName);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void SetGroupVolume(float volume, string group);
		#endregion

		#region RayCast
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Raycast(Vector2 origin, Vector2 direction, out RaycastHit hit);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Raycast(Vector2 origin, Vector2 direction, int layerMask, out RaycastHit hit);
        #endregion

        #region Entity
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern object GetScriptInstance(UInt64 entityHandle, string scriptName);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern object GetChildScriptInstance(UInt64 entityHandle, string scriptName);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern UInt64 FindEntity(string name);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void DestroyEntity(UInt64 entityHandle);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern UInt64 InstanstiateGameObject(UInt64 originalEntityHandle, UInt64 parentToAttach);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern UInt64 InstantiatePrefab(string prefabName, UInt64 parentToAttach);

        #endregion

        #region Camera
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern void SetCameraZoom(float newZoom);

		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal static extern float GetCameraZoom();
        #endregion

        #region Animator
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void GetAnimatorComp(UInt64 entityHandle, out Animator a);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void GetChildAnimatorComp(UInt64 entityHandle, out Animator a);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimation(UInt64 entityHandle, string name);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimationSpeed(UInt64 entityHandle, float speed);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetAnimationLooping(UInt64 entityHandle, bool looping);
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
