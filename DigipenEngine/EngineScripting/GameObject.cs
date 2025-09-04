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
    public struct Transform : Component
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
                SetWorldScale(EntityHandle, _rotation);
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
		public static GameObject Find(string name)
        {
            //internal call here
            EntityHandle handle = InternalCalls.FindEntity(name);
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
		public void Destroy(GameObject obj)
        {
            if (obj == null)
            {
                return;
            }
            InternalCalls.DestroyEntity(obj.transform.EntityHandle);
        }

		/*****************************************************************//*!
        \brief
	        Dictionaries for the GetComponent<T>() to use
        *//******************************************************************/
		#region Dictionaries
		private readonly Dictionary<Type, Action<EntityHandle, Action<Component>>> componentGetters = new Dictionary<Type, Action<EntityHandle, Action<Component>>>()
        {
            { typeof(Transform), (eID, callback) =>
                {
                    InternalCalls.GetTransform(eID, out var component);
                    callback(component);
                }
            },

            { typeof(Physics), (eID, callback) =>
                {
                    InternalCalls.GetPhysicsComp(eID, out var component);
                    callback(component);
                }
            },

            { typeof(Text), (eID, callback) =>
                {
                    InternalCalls.GetText(eID, out var component);
                    callback(component);
                }
            },

            { typeof(Animator), (eID, callback) =>
                {
                    InternalCalls.GetAnimatorComp(eID, out var component);
                    callback(component);
                }
            }
        };

        private readonly Dictionary<Type, Action<EntityHandle, Action<Component>>> childComponentGetters = new Dictionary<Type, Action<EntityHandle, Action<Component>>>()
        {
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
            { typeof(Animator), (eID, callback) =>
                {
                    InternalCalls.GetChildAnimatorComp(eID, out var component);
                    callback(component);
                }
            }
        };
		#endregion
		/*****************************************************************//*!
        \brief
	        Template function to get access to an attached component of the entity
        \return
	        Specified component to get
        *//******************************************************************/
		public T GetComponent<T>() where T : struct, Component
        {
            T result = default;


            if (componentGetters.TryGetValue(typeof(T), out var getter))
            {

                getter(transform.EntityHandle, component =>
                {
                    result = (T)(object)component;
                });
            }

            return result;
        }

		/*****************************************************************//*!
        \brief
	        Template function to get access to an attached component of the entity's
            children
        \return
	        Specified component to get
        *//******************************************************************/
		public T GetComponentInChildren<T>() where T : struct, Component
        {
            T result = default;


            if (childComponentGetters.TryGetValue(typeof(T), out var getter))
            {

                getter(transform.EntityHandle, component =>
                {
                    result = (T)(object)component;
                });
            }

            return result;
        }
		// FOR FINDING SCRIPTS
		/*****************************************************************//*!
        \brief
	        Template function to get access to a specified script attached to
            the entity.
        \return
	        The specified script class instance in memory
        *//******************************************************************/
		public T GetScript<T>() where T : class
		{
			string name = typeof(T).Name;

			object obj = InternalCalls.GetScriptInstance(transform.EntityHandle, name);

			return (T)obj;
		}
		/*****************************************************************//*!
        \brief
	        Template function to get access to a specified script attached to
            the entity's children.
        \return
	        The specified script class instance in memory
        *//******************************************************************/
		public T GetScriptInChildren<T>() where T : class
        {
            string name = typeof(T).Name;

            object obj = InternalCalls.GetChildScriptInstance(transform.EntityHandle, name);

            return (T)obj;
        }

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
		public static GameObject Instanstiate(GameObject original, Transform? parent = null)
        {
            EntityHandle parentID = 0;
            if (parent.HasValue)
            {
                parentID = parent.Value.EntityHandle;
            }
            GameObject obj = new GameObject(InternalCalls.InstanstiateGameObject(original.transform.EntityHandle, parentID));
            return obj;
        }

        public static GameObject InstantiatePrefab(Prefab prefabName, Transform? parent = null)
        {
            EntityHandle parentID = 0;
            if (parent.HasValue)
            {
                parentID = parent.Value.EntityHandle;
            }
            GameObject obj = new GameObject(InternalCalls.InstantiatePrefab(prefabName, parentID));
            return obj;
        }
	}
}
