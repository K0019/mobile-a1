/******************************************************************************/
/*!
\file   Gameobject.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/11/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This file contains the GameObject class of the core assembly library.
    This class is the API for Entities on the C# side and allows scripts to
    accesss entity components and scripts attached to them.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using GlmSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
namespace EngineScripting
{
	using Prefab = System.String;

	public struct EntityHandle
	{
		private UInt64 entityID;

		public EntityHandle(UInt64 entityID)
		{
			this.entityID = entityID;
		}
		public static implicit operator EntityHandle(UInt64 entityID) => new EntityHandle(entityID);

		public bool IsNull => entityID == 0;
	}

	/*****************************************************************//*!
    \brief
        C# counterpart to the c++ Transform struct
    \return
        None
    *//******************************************************************/
	[StructLayout(LayoutKind.Sequential)]
	public struct Transform
	{
		private vec3 _position;
		private vec3 _localPosition;
		private vec3 _scale;
		private vec3 _localScale;
		private vec3 _rotation;
		private vec3 _localRotation;

		public EntityHandle EntityHandle { get; private set; }

		public Transform(EntityHandle eid)
		{
			_position = _localPosition = _scale = _localScale = _rotation = _localRotation = default;
			EntityHandle = eid;
			RefreshValues();
		}

		/*****************************************************************//*!
        \brief
            Re-fetches and synchronizes transform values from C++ side.
        *//******************************************************************/
		[DllImport("__Internal", EntryPoint = "CS_SetTransform")]
		private static extern void RefreshValues(ref Transform transform);
		public void RefreshValues() => RefreshValues(ref this);

		/*****************************************************************//*!
        \brief
            Updates C++ side with C# side values.
        *//******************************************************************/
		[DllImport("__Internal", EntryPoint = "CS_SetLocalPosition")]
		private static extern void SetLocalPosition(EntityHandle entity, vec3 position);
		[DllImport("__Internal", EntryPoint = "CS_SetWorldPosition")]
		private static extern void SetWorldPosition(EntityHandle entity, vec3 position);
		[DllImport("__Internal", EntryPoint = "CS_SetLocalRotation")]
		private static extern void SetLocalRotation(EntityHandle entity, vec3 rotation);
		[DllImport("__Internal", EntryPoint = "CS_SetWorldRotation")]
		private static extern void SetWorldRotation(EntityHandle entity, vec3 rotation);
		[DllImport("__Internal", EntryPoint = "CS_SetLocalScale")]
		private static extern void SetLocalScale(EntityHandle entity, vec3 scale);
		[DllImport("__Internal", EntryPoint = "CS_SetWorldScale")]
		private static extern void SetWorldScale(EntityHandle entity, vec3 scale);


		public vec3 position
		{
			get => _position;
			set
			{
				_position = value; // Update the internal position
				SetWorldPosition(EntityHandle, _position); // Update C++ data
			}
		}
		public vec3 localPosition
		{
			get => _localPosition;
			set
			{
				_localPosition = value;
				SetLocalPosition(EntityHandle, _localPosition);
			}
		}

		public vec3 scale
		{
			get => _scale;
			set
			{
				_scale = value;
				SetWorldScale(EntityHandle, _scale);
			}
		}

		public vec3 localScale
		{
			get => _localScale;
			set
			{
				_localScale = value;
				SetLocalScale(EntityHandle, _localScale);
			}
		}

		public vec3 rotation
		{
			get => _rotation;
			set
			{
				_rotation = value;
				SetWorldRotation(EntityHandle, _rotation);
			}
		}

		public vec3 localRotation
		{
			get => _localRotation;
			set
			{
				_localRotation = value;
				SetLocalRotation(EntityHandle, _localRotation);
			}
		}

		public vec3 forward
		{
			get
			{
				var R = GetRotationMatrix();
				return new vec3(R.m02, R.m12, R.m22); // third column
			}
			set => SetOrientationFromForward(value);
		}

		public vec3 right
		{
			get
			{
				var R = GetRotationMatrix();
				return new vec3(R.m00, R.m10, R.m20); // first column
			}
			set => SetOrientationFromRight(value);
		}

		public vec3 up
		{
			get
			{
				var R = GetRotationMatrix();
				return new vec3(R.m01, R.m11, R.m21); // second column
			}
			set => SetOrientationFromUp(value);
		}


		private mat3 GetRotationMatrix()
		{
			vec3 rot = rotation;
			float cx = Mathf.Cos(rot.x); // pitch
			float sx = Mathf.Sin(rot.x);
			float cy = Mathf.Cos(rot.y); // yaw
			float sy = Mathf.Sin(rot.y);
			float cz = Mathf.Cos(rot.z); // roll
			float sz = Mathf.Sin(rot.z);

			// Rotation order: Y * X * Z
			return new mat3(
				cy * cz + sy * sx * sz, cz * sy * sx - cy * sz, sy * cx,
				cx * sz, cx * cz, -sx,
				cy * sx * sz - cz * sy, cy * cz * sx + sy * sz, cy * cx
			);
		}

		private void SetOrientationFromForward(vec3 forward, vec3 worldUp = default)
		{
			if (worldUp == default)
				worldUp = new vec3(0, 1, 0);

			vec3 f = forward.Normalized;
			vec3 r = vec3.Cross(worldUp, f).Normalized;
			vec3 u = vec3.Cross(f, r); // re-orthogonalize up

			// Build rotation matrix
			mat3 R = new mat3(
				r.x, u.x, f.x,
				r.y, u.y, f.y,
				r.z, u.z, f.z
			);

			vec3 rot;
			// Euler angles
			rot.x = Mathf.Asin(-R.m21);           // Pitch                     
			rot.y = Mathf.Atan2(R.m20, R.m22);    // Yaw          
			rot.z = Mathf.Atan2(R.m01, R.m11);    // Roll

			rot = rot * Mathf.Rad2Deg;

			// Force set rotation
			rotation = rot;
		}

		void SetOrientationFromRight(vec3 right, vec3 worldUp = default)
		{
			if (worldUp == default)
				worldUp = new vec3(0, 1, 0);

			vec3 r = right.Normalized;
			vec3 u = vec3.Cross(worldUp, r).Normalized;
			vec3 f = vec3.Cross(r, u);

			mat3 R = new mat3(
				r.x, u.x, f.x,
				r.y, u.y, f.y,
				r.z, u.z, f.z
			);

			vec3 rot;

			rot.x = Mathf.Asin(-R.m21);           // Pitch
			rot.y = Mathf.Atan2(R.m20, R.m22);    // Yaw  
			rot.z = Mathf.Atan2(R.m01, R.m11);    // Roll 

			rot = rot * Mathf.Rad2Deg;

			// Force set rotation
			rotation = rot;
		}
		void SetOrientationFromUp(vec3 up, vec3 forwardHint = default)
		{
			if (forwardHint == default)
				forwardHint = new vec3(0, 0, 1);

			vec3 u = up.Normalized;
			vec3 f = vec3.Cross(forwardHint, u).Normalized;
			vec3 r = vec3.Cross(u, f);

			mat3 R = new mat3(
				r.x, u.x, f.x,
				r.y, u.y, f.y,
				r.z, u.z, f.z
			);

			vec3 rot;

			rot.x = Mathf.Asin(-R.m21);           // Pitch
			rot.y = Mathf.Atan2(R.m20, R.m22);    // Yaw  
			rot.z = Mathf.Atan2(R.m01, R.m11);    // Roll 

			rot = rot * Mathf.Rad2Deg;

			// Force set rotation
			rotation = rot;
		}

	}

	public class GameObject
	{
		public Transform transform;

		/*****************************************************************//*!
        \brief
	        Constructor for the class
        \param[in] entityID
            Saves the Entityhandle pointer as a UInt64
        \return
	        None
        *//******************************************************************/
		public GameObject(EntityHandle entityID)
		{
			transform = new Transform(entityID);
		}

		/*****************************************************************//*!
        \brief
	        Finds an entity through the use of their name component
        \param[in] name
	        Name of the gameobject to find
        \return
	        GameObject to be found
        *//******************************************************************/
		[DllImport("__Internal", EntryPoint = "CS_FindEntityByName")]
		private static extern EntityHandle FindEntity(string name);
		public static GameObject Find(string name)
		{
			EntityHandle handle = FindEntity(name);
			if (handle.IsNull)
				return null;
			return new GameObject(handle);
		}

		/*****************************************************************//*!
        \brief
	        Destroys a gameobject
        \param[in] obj
	        GameObject to destroy in the entity map
        \return
	        None
        *//******************************************************************/
		[DllImport("__Internal", EntryPoint = "CS_FindEntityByName")]
		private static extern void DeleteEntity(EntityHandle entity);
		public void Destroy(GameObject obj)
		{
			if (obj != null)
				DeleteEntity(obj.transform.EntityHandle);
		}

		/*****************************************************************//*!
        \brief
	        Dictionaries for the GetComponent<T>() to use
        *//******************************************************************/
		#region Dictionaries
		private readonly Dictionary<Type, Func<EntityHandle, IComponent>> componentGetters = new Dictionary<Type, Func<EntityHandle, IComponent>>()
		{
			{ typeof(Text), (EntityHandle entity) => Text.RetrieveComp(entity) },
		};

		[DllImport("__Internal", EntryPoint = "CS_GetScriptInstance")]
		private static extern IntPtr GetScriptObject(EntityHandle entity, string type);

		//private readonly Dictionary<Type, Action<EntityHandle, Action<IComponent>>> childComponentGetters = new Dictionary<Type, Action<EntityHandle, Action<IComponent>>>()
		//{
		//{ typeof(Transform), (eID, callback) =>
		//    {
		//        InternalCalls.GetChildTransform(eID, out var component);
		//        callback(component);
		//    }
		//},

		//{ typeof(Physics), (eID, callback) =>
		//    {
		//        InternalCalls.GetPhysicsComp(eID, out var component);
		//        callback(component);
		//    }
		//},
		//
		//{ typeof(Text), (eID, callback) =>
		//    {
		//        InternalCalls.GetText(eID, out var component);
		//        callback(component);
		//    }
		//},
		//
		//    { typeof(Animator), (eID, callback) =>
		//        {
		//            InternalCalls.GetChildAnimatorComp(eID, out var component);
		//            callback(component);
		//        }
		//    }
		//};
		#endregion
		/*****************************************************************//*!
        \brief
	        Template function to get access to an attached component of the entity
        \return
	        Specified component to get
        *//******************************************************************/
		public T GetComponent<T>() where T : IComponent
		{
			// For C++ components
			if (componentGetters.TryGetValue(typeof(T), out var getter))
				return (T)getter(transform.EntityHandle);

			// For custom C# components
			IntPtr scriptObjPtr = GetScriptObject(transform.EntityHandle, typeof(T).FullName);
			if (scriptObjPtr != IntPtr.Zero)
			{
				GCHandle gcHandle = GCHandle.FromIntPtr(scriptObjPtr);
				T scriptObj = (T)gcHandle.Target;
				gcHandle.Free();
				return scriptObj;
			}

			// Unable to find
			return default;
		}

		/*****************************************************************//*!
        \brief
	        Template function to get access to an attached component of the entity's
            children
        \return
	        Specified component to get
        *//******************************************************************/
		//public T GetComponentInChildren<T>() where T : struct, IComponent
		//      {
		//          T result = default;


		//          if (childComponentGetters.TryGetValue(typeof(T), out var getter))
		//          {

		//              getter(transform.EntityHandle, component =>
		//              {
		//                  result = (T)(object)component;
		//              });
		//          }

		//          return result;
		//      }

		/*****************************************************************//*!
        \brief
	        Creates a copy of a GameObject.
        \param[in] original
            The GameObject to instantiate a copy of.
        \param[in] parent
            Optional param to attach the copied GameObject to.
        \return
	        The copied GameObject
        *//******************************************************************/
		[DllImport("__Internal", EntryPoint = "CS_CloneEntity")]
		private static extern EntityHandle CloneEntity(EntityHandle entity, EntityHandle parent);
		public static GameObject Instanstiate(GameObject original, Transform? parent = null)
		{
			EntityHandle parentID = 0;
			if (parent.HasValue)
			{
				parentID = parent.Value.EntityHandle;
			}
			GameObject obj = new GameObject(CloneEntity(original.transform.EntityHandle, parentID));
			return obj;
		}

		[DllImport("__Internal", EntryPoint = "CS_SpawnPrefab")]
		private static extern EntityHandle SpawnPrefab(string name, EntityHandle parent);
		public static GameObject InstantiatePrefab(Prefab prefabName, Transform? parent = null)
		{
			EntityHandle parentID = 0;
			if (parent.HasValue)
			{
				parentID = parent.Value.EntityHandle;
			}
			GameObject obj = new GameObject(SpawnPrefab(prefabName, parentID));
			return obj;
		}
	}
}
