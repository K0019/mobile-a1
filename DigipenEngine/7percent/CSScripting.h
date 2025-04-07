/******************************************************************************/
/*!
\file   CSScripting.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
  This is the interface for the class that will be integrating C# scripting 
  into the game engine. The libraries used are from mono.

  Also contains declarations of the classes ScriptClass and ScriptInstance
  used in the scripting engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Physics.h"

// Forward Declare
class ScriptComponent;

using VariantType = property::data; // Add types as needed

namespace CSharpScripts {

	namespace internal {

		/*****************************************************************//*!
		\brief
			Stores a handle to a variable field within a ScriptInstance's C# script
			and provides an encapsulation and standardized interface to it.
		*//******************************************************************/
		class Field : public ISerializeableWithoutJsonObj
		{
		public:
			struct ValueOperations
			{
				void(*RetrieveValueFromScript)(VariantType* valuePtr, MonoClassField* field, MonoObject* instance);
				void(*SetValueOfScript)(const VariantType& value, MonoClassField* field, MonoObject* instance);
			};

			static const ValueOperations* GetValueOperation(int typeEnum);
			static bool IsValidField(MonoClassField* field);

		public:
			Field(MonoObject* instance, MonoClassField* field);

			void SetValue(MonoObject* instance, const VariantType& newValue);

			// TEMPORARY
			VariantType& GetValue();

		private:
			void RetrieveValueFromScript(MonoObject* instance);

		public:
			void Serialize(Serializer& writer) const override;
			void Deserialize(Deserializer& reader) override;

		private:
			static const std::unordered_map<int, ValueOperations> valueTypeToOperationsMap;

		private:
			MonoClassField* field;
			MonoType* type;
			int typeEnum;
			VariantType value;

			property_vtable()
		};

		/*****************************************************************//*!
		\brief
			Stores a handle to a GameObject field within a ScriptInstance's C# script.
		*//******************************************************************/
		class GameObjectField
		{
		public:

		private:
			

		};

	}

	/*****************************************************************//*!
	\brief
		Class used to store a mono class object
	*//******************************************************************/
	class ScriptClass
	{
	public:
		/*****************************************************************//*!
		\brief
			Default constructor of the class
		\return
			None
		*//******************************************************************/
		ScriptClass() = default;

		/*****************************************************************//*!
		\brief
			Non-default constructor of the class
		\param[in] classNameSpace
			Namespace of the class to be obtained from the assembly
		\param[in] className
			Name of the class to be obtained from the assembly
		\return
			None
		*//******************************************************************/
		ScriptClass(const std::string& classNameSpace, const std::string& className);

		/*****************************************************************//*!
		\brief
			Destructor of the class
		\return
			None
		*//******************************************************************/
		~ScriptClass();

		/*****************************************************************//*!
		\brief
			Returns the full name of the c# class.
			E.g; ScriptingEngine.ComponentTest
		\return
			string of the full name of the C# class
		*//******************************************************************/
		std::string GetFullName() const;

		/*****************************************************************//*!
		\brief
			Instantiates an instance object of the c# class
		\return
			Mono Object instance of the class
		*//******************************************************************/
		MonoObject* Instanstiate() const;

		/*****************************************************************//*!
		\brief
			Gets the method, if found, from the c# class
		\param[in] name
			Name of the method to get
		\param[in] paramCount
			Number of params the method requires
		\return
			Mono Method object of the c# class method
		*//******************************************************************/
		MonoMethod* GetMethod(const std::string& name, int paramCount);

		/*****************************************************************//*!
		\brief
			Invoked the method on the c# side.
		\param[in] instance
			Instance of the c# class object to call the method from
		\param[in] method
			The method object to call from the c# side
		\param[in] params
			The params the method requires to be called
		\return
			The same instance that was given to the function.
		*//******************************************************************/
		MonoObject* InvokeMethod(MonoObject* instance, MonoMethod* method, void** params = nullptr);

		/*****************************************************************//*!
		\brief
			Gets the MonoClass object stored in this class
		\return
			The mono class object
		*//******************************************************************/
		MonoClass* GetClass() const;

		/*****************************************************************//*!
		\brief
			Invokes the SetHandle method of the class instance.
		\param[in]
			params to pass into the method.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeSetHandle(MonoObject* instance, void** params = nullptr);

		/*****************************************************************//*!
		\brief
			Invokes the OnCreate method of the class instance.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeOnCreate(MonoObject* instance);

		/*****************************************************************//*!
		\brief
			Invokes the Awake method of the class instance.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeAwake(MonoObject* instance);

		/*****************************************************************//*!
		\brief
			Invokes the OnStart method of the class instance.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeOnStart(MonoObject* instance);

		/*****************************************************************//*!
		\brief
			Invokes the OnUpdate method of the class instance.
		\param[in]
			params to pass into the method.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeOnUpdate(MonoObject* instance, void** params = nullptr);

		/*****************************************************************//*!
		\brief
			Invokes the OnCollision method of the class instance.
		\param[in]
			params to pass into the method.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeOnCollision(MonoObject* instance, void** params = nullptr);

		/*****************************************************************//*!
		\brief
			Invokes the LateUpdate method of the class instance.
		\param[in]
			params to pass into the method.
		\return
			Updated MonoObject* of the class instance.
		*//******************************************************************/
		MonoObject* InvokeLateUpdate(MonoObject* instance, void** params = nullptr);

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the SetHandle method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool SetHandleExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the OnCreate method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool OnCreateExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the OnUpdate method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool OnUpdateExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the OnCollision method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool OnCollisionExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the OnStart method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool OnStartExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the Awake method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool AwakeExists() const;

		/*****************************************************************//*!
		\brief
			Checks to see if the class contains the LateHandle method.
		\return
			Bool if the method exists.
		*//******************************************************************/
		bool LateUpdateExists() const;
	private:
		std::string m_ClassNameSpace;
		std::string m_ClassName;

		MonoMethod* m_SetHandleMethod = nullptr;
		MonoMethod* m_OnCreateMethod = nullptr;
		MonoMethod* m_OnUpdateMethod = nullptr;
		MonoMethod* m_OnCollisionMethod = nullptr;

		MonoMethod* m_OnStartMethod = nullptr;
		MonoMethod* m_AwakeMethod = nullptr;
		MonoMethod* m_LateUpdateMethod = nullptr;

		MonoClass* m_MonoClass = nullptr;
	};

	/*****************************************************************//*!
	\brief
		Class used to store an instance of a Script class.
	*//******************************************************************/
	class ScriptInstance : public ISerializeable
	{
	private:
		using PublicVarsMapType = std::unordered_map<std::string, internal::Field>;

	public:
		/*****************************************************************//*!
		\brief
			Default constructor of the class
		\return
			None
		*//******************************************************************/
		ScriptInstance() = default;

		/*****************************************************************//*!
		\brief
			Non-Default constructor of the class
		\param[in]
			Script class to make an instance of
		\return
			None
		*//******************************************************************/
		ScriptInstance(const ScriptClass& sclass);

		/*****************************************************************//*!
		\brief
			Copy constructor
		\param other
			The script instance to copy.
		\return
			None
		*//******************************************************************/
		ScriptInstance(const ScriptInstance& other);

		/*****************************************************************//*!
		\brief
			Copy assignment
		\param other
			The script instance to copy.
		\return
			This
		*//******************************************************************/
		ScriptInstance& operator=(const ScriptInstance& other);

		/*****************************************************************//*!
		\brief
			Move operator
		\param other
			The script instance to move data from.
		\return
			This
		*//******************************************************************/
		ScriptInstance& operator=(ScriptInstance&& other) noexcept;

		/*****************************************************************//*!
		\brief
			Move constructor
		\param other
			The script instance to move.
		\return
			None
		*//******************************************************************/
		ScriptInstance(ScriptInstance&& other) noexcept;

		/*****************************************************************//*!
		\brief
			Destructor of the class
		\return
			None
		*//******************************************************************/
		~ScriptInstance();

		/*****************************************************************//*!
		\brief
			Gets the class of the instance.
		\return
			Script class object
		*//******************************************************************/
		ScriptClass& GetClass();

		/*****************************************************************//*!
		\brief
			Invokes the SetHandle method in the c# side of the class instance
			Sets the Entity Handle ID, acquired by using the Script Component 
			this instance is attached to to get the Entity ID,
			of the class to enable functionality with ECS.
		\param[in] comp
			The script component this object is attached to.
		\return
			None
		*//******************************************************************/
		void SetHandleInvoke(const ScriptComponent& comp);

		/*****************************************************************//*!
		\brief
			Invokes the Awake method of the class found in the c# side.
		\return
			None
		*//******************************************************************/
		void AwakeInvoke();

		/*****************************************************************//*!
		\brief
			Invokes the OnStart method of the class found in the c# side.
		\return
			None
		*//******************************************************************/
		void OnStartInvoke();

		/*****************************************************************//*!
		\brief
			Invokes the OnCreate method of the class found in the c# side.
		\return
			None
		*//******************************************************************/
		void OnCreateInvoke();

		/*****************************************************************//*!
		\brief
			Invokes the OnUpdate method of the class found in the c# side.
		\param[in] ts
			delta time
		\return
			None
		*//******************************************************************/
		void OnUpdateInvoke(float ts);

		/*****************************************************************//*!
		\brief
			Invokes the OnCollision method of the class found in the c# side.
		\param[in] coll
			collision data
		\return
			None
		*//******************************************************************/
		void OnCollisionInvoke(Physics::CollisionEventData& collisionData);

		/*****************************************************************//*!
		\brief
			Invokes the LateUpdate method of the class found in the c# side.
		\param[in] ts
			delta time
		\return
			None
		*//******************************************************************/
		void LateUpdateInvoke(float ts);

		/*****************************************************************//*!
		\brief
			Retrieves the public variables that can be found inside the c# class
			and stores them in an unordered map
		\return
			None
		*//******************************************************************/
		void RetrievePublicVariables();

		/*****************************************************************//*!
		\brief
			Returns the unordered map filled with the variables. Used for
			rendering inside the editor to show the variables inside the class
			instance.
		\return
			The unordered map of variables
		*//******************************************************************/
		PublicVarsMapType& GetPublicVars(); // TODO: Fix all conflicts here

		/*****************************************************************//*!
		\brief
			Returns the unordered map filled with the variables. Used for
			rendering inside the editor to show the variables inside the class
			instance.
		\return
			The unordered map of variables
		*//******************************************************************/
		const PublicVarsMapType& GetPublicVars() const;

		/*****************************************************************//*!
		\brief
			Returns Instance of the script class.
		\return
			MonoObject* that points to the class instance.
		*//******************************************************************/
		MonoObject* GetInstance() const;

		/*****************************************************************//*!
		\brief
			Returns the full name of the class (ClassNamespace.ClassName)
		\return
			string of the class' full name.
		*//******************************************************************/
		std::string GetClassFullName() const;

		/*****************************************************************//*!
		\brief
			Used in the ImGui editor drawing function when a user changes 
			the value of a variable. Sets the new value into the c# class instance
			then retrieves the variables again to update this unordered map
		\param[in] varName
			Name/Key of the variable to update in the unordered map
		\param[in] newValue
			New Value of the variable found in the unordered map
		\return
			None
		*//******************************************************************/
		void SetPublicVar(const std::string& varName,const VariantType& newValue);

	public:
		void Serialize(Serializer& writer) const override;
		void Deserialize(Deserializer& reader) override;

	private:
		ScriptClass* m_ScriptClass;

		MonoObject* m_Instance = nullptr;

		MonoGCHandle m_InstanceHandle;

		bool StartCall = false;

		PublicVarsMapType m_PublicVars;

		property_vtable()
	};

	/*****************************************************************//*!
	\brief
		Class used to integrate, initialize and handle mono library for
		c# scripting.
	*//******************************************************************/
	class CSScripting
	{
	public:
		/*****************************************************************//*!
		\brief
			Initializes the components of the scripting engine for the game engine.
		\return
			None
		*//******************************************************************/
		static void Init();
		/*****************************************************************//*!
		\brief
			Cleans up the scripting engine when game engine is exited
		\return
			None
		*//******************************************************************/
		static void Exit();

		/*****************************************************************//*!
		\brief
			Loads the Core assembly of the scripting engine
		\param[in] filepath
			Essentially a string that contains the path to the .dll for
			the core assembly
		\return
			None
		*//******************************************************************/
		static void LoadAssembly(const std::filesystem::path& filepath);

		/*****************************************************************//*!
		\brief
			Creates the .csproj file of the UserAssembly library.
		\return
			None
		*//******************************************************************/
		static void CreateUserProject();

		/*****************************************************************//*!
		\brief
			Compiles the .csproj file into the UserAssembly.dll
		\return
			None
		*//******************************************************************/
		static void CompileUserAssembly();

		/*****************************************************************//*!
		\brief
			Compiles the .csproj file into the UserAssembly.dll on a separate thread.
			This opens a popup that will only close once the operation is complete, during which time the user can not interact with the program.
		\param callback
			The function to call once the async process is complete
		*//******************************************************************/
		static void CompileUserAssemblyAsync(void(*callback)());

		/*****************************************************************//*!
		\brief
			Joins the compile user assembly thread.
			TODO: Should create a class that can track tasks like this, so main thread doesn't need to call this for every other class that implements this pattern.
		*//******************************************************************/
		static void CheckCompileUserAssemblyAsyncCompletion();

		/*****************************************************************//*!
		\brief
			Removes temporary files generated in the process of compiling the user assembly.
		*//******************************************************************/
		static void CleanUserAssemblyTempFiles();

		/*****************************************************************//*!
		\brief
			Loads the UserAssmble.dll into mono
		\return
			None
		*//******************************************************************/
		static void LoadUserAssembly(const std::filesystem::path& filepath);

		/*****************************************************************//*!
		\brief
			Just a function that tests if Mono is running properly.
			Contains c++ and c# calls to test functionality.
		\return
			None
		*//******************************************************************/
		static void MonoTest();

		/*****************************************************************//*!
		\brief
			Gets a class from the core assembly via name
		\param[in] cName
			Name of class to retrieve
		\return
			Reference to desired ScriptClass object
		*//******************************************************************/
		static ScriptClass& GetClassFromData(std::string cName);

		/*****************************************************************//*!
		\brief
			Gets the unordered map containing all object classes found in the 
			core assembly
		\return
			Unordered map of scriptclass objects
		*//******************************************************************/
		static std::unordered_map<std::string, ScriptClass>& GetCoreClassMap();

		/*****************************************************************//*!
		\brief
			Reloads boths the Core and User assembly to update the user scripts
			within the engine.
		\return
			None
		*//******************************************************************/
		static void ReloadAssembly();
	private:
		/*****************************************************************//*!
		\brief
			Initializes the mono library for the engine to use C# scripting on
			boot up.
		\return
			None
		*//******************************************************************/
		static void InitMono();

		/*****************************************************************//*!
		\brief
			Instantiates an object instance of a given Mono class
		\param[in] monoClass
			Class to instantiate an instance of
		\return
			Object instance of the param class
		*//******************************************************************/
		static MonoObject* InstanstiateClass(MonoClass* monoClass);

		//! Stores a handle to the thread running user assembly compilation, for async purposes.
		static std::future<int> compileUserAssemblyFuture;
		//! Tracks whether an async user compilation task is ongoing
		static bool isCompilingUserAssemblyAsync;
		//! The callback function to call once the async user compilation is complete
		static void(*compileUserAssemblyCallback)();

		friend class ScriptClass;
	};

}

property_begin(CSharpScripts::internal::Field)
{
}
property_vend_h(CSharpScripts::internal::Field)

property_begin(CSharpScripts::ScriptInstance)
{
}
property_vend_h(CSharpScripts::ScriptInstance)
