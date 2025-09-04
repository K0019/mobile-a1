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
using System;
using System.Collections.Generic;
using System.Linq;
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

    public class GameObject
    {
        private readonly EntityHandle entityID;
        private Transform transComp;
        public ref Transform transform
        {
            get
            {
                InternalCalls.GetTransform(entityID, out transComp);
                return ref transComp;
            }
        }

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
            this.entityID = entityID;
        }
		/*****************************************************************//*!
        \brief
	        Sets the entity's tranform from c# side
        \param[in] t
            new transform of the entity
        \return
	        None
        *//******************************************************************/
		public void SetTransform(Transform t)
		{
			transComp = t;
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
            InternalCalls.DestroyEntity(obj.entityID);
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

                getter(entityID, component =>
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

                getter(entityID, component =>
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

			object obj = InternalCalls.GetScriptInstance(entityID, name);

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

            object obj = InternalCalls.GetChildScriptInstance(entityID, name);

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
                parentID = parent.Value.GetID();
            }
            GameObject obj = new GameObject(InternalCalls.InstanstiateGameObject(original.entityID, parentID));
            return obj;
        }

        public static GameObject InstantiatePrefab(Prefab prefabName, Transform? parent = null)
        {
            EntityHandle parentID = 0;
            if (parent.HasValue)
            {
                parentID = parent.Value.GetID();
            }
            GameObject obj = new GameObject(InternalCalls.InstantiatePrefab(prefabName, parentID));
            return obj;
        }
	}
}
