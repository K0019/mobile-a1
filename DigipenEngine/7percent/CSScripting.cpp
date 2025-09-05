/******************************************************************************/
/*!
\file   CSScripting.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
  This is the source file that implements the functions of the C# scripting 
  intergration class for the engine. Has functionality now, will be adding more
  to it as time goes on.

  Also contains definitions of the classes ScriptClass and ScriptInstance
  used in the scripting engine.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "CSScripting.h"
#include "GameSettings.h"
#include "Popup.h"
#include "Engine.h"
#include "ScriptGlue.h"
#include "ScriptComponent.h"
#include "EntityUID.h"

namespace CSharpScripts
{
	// Static declares
	std::future<int> CSScripting::compileUserAssemblyFuture;
	bool CSScripting::isCompilingUserAssemblyAsync;
	void(*CSScripting::compileUserAssemblyCallback)();

	namespace internal {
		template <typename T>
		auto DefaultRetrieveValuesFromScript = [](VariantType* value, MonoClassField* field, MonoObject* instance) -> void {
			T tempVal{};
			mono_field_get_value(instance, field, &tempVal);
			*value = tempVal;
		};
		template <typename T>
		auto DefaultSetValueOfScript = [](const VariantType& value, MonoClassField* field, MonoObject* instance) -> void {
			T val{ std::get<T>(value) };
			mono_field_set_value(instance, field, &val);
		};
		
		auto DefaultRetrieveValuesFromScriptVector = [](VariantType* value, MonoClassField* field, MonoObject* instance) -> void {
			// CONSOLE_LOG_EXPLICIT("This is the get", LogLevel::LEVEL_DEBUG);
			//const char* fieldName = mono_field_get_name(field);
			// CONSOLE_LOG_EXPLICIT("fieldName = " + std::string(fieldName), LogLevel::LEVEL_DEBUG);

			// Get the type of the field (spawnLocation1 in the script)
			MonoType* fieldType = mono_field_get_type(field);

			// Get the MonoClass for the Vec3 type (this is already specialized, so we can assume it's Vec3)
			MonoClass* vectorClass = mono_type_get_class(fieldType);

			const char* className = mono_class_get_name(vectorClass);

			// CONSOLE_LOG_EXPLICIT("Class Name: " + std::string(className), LogLevel::LEVEL_DEBUG);


			if (std::strcmp(className, "Vec3") == 0)
			{
				// CONSOLE_LOG_EXPLICIT(std::string(fieldName) + " is a Vec3!", LogLevel::LEVEL_DEBUG);

				MonoClassField* xField = mono_class_get_field_from_name(vectorClass, "x");
				MonoClassField* yField = mono_class_get_field_from_name(vectorClass, "y");
				MonoClassField* zField = mono_class_get_field_from_name(vectorClass, "z");

				float xVal = 0.0f, yVal = 0.0f, zVal = 0.0f;

				// Ensure we have the correct MonoObject for the instance
				if (xField && yField && zField) {
					// Retrieve the values of each component
					mono_field_get_value(instance, xField, &xVal);
					mono_field_get_value(instance, yField, &yVal);
					mono_field_get_value(instance, zField, &zVal);

					//CONSOLE_LOG_EXPLICIT(std::string(fieldName) + " Values: ", LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" X: " + std::to_string(xVal), LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" Y: " + std::to_string(yVal), LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" Z: " + std::to_string(zVal), LogLevel::LEVEL_DEBUG);

					// Make sure we are updating the value correctly
					*value = Vec3{ xVal, yVal, zVal };
				}
				else {
					CONSOLE_LOG_EXPLICIT("Failed to find fields x, y, z in Vec3 struct!", LogLevel::LEVEL_ERROR);
				}
			}

			};

		auto DefaultSetValueOfScriptVector = [](const VariantType& value, MonoClassField* field, MonoObject* instance) -> void {
			// CONSOLE_LOG_EXPLICIT("This is the set", LogLevel::LEVEL_DEBUG);
			//const char* fieldName = mono_field_get_name(field);
			// CONSOLE_LOG_EXPLICIT("fieldName = " + std::string(fieldName), LogLevel::LEVEL_DEBUG);

			// Get the type of the field (spawnLocation1 in the script)
			MonoType* fieldType = mono_field_get_type(field);

			// Get the MonoClass for the Vec3 type (this is already specialized, so we can assume it's Vec3)
			MonoClass* vectorClass = mono_type_get_class(fieldType);

			const char* className = mono_class_get_name(vectorClass);

			// CONSOLE_LOG_EXPLICIT("Class Name: " + std::string(className), LogLevel::LEVEL_DEBUG);

			// Check if it's a Vec3 struct
			if (std::strcmp(className, "Vec3") == 0)
			{
				// CONSOLE_LOG_EXPLICIT(std::string(fieldName) + " is a Vec3!", LogLevel::LEVEL_DEBUG);

				// Retrieve the individual components "x", "y", "z" of the Vec3 struct
				MonoClassField* xField = mono_class_get_field_from_name(vectorClass, "x");
				MonoClassField* yField = mono_class_get_field_from_name(vectorClass, "y");
				MonoClassField* zField = mono_class_get_field_from_name(vectorClass, "z");

				// Assuming value is a Vec3 (which should be a tuple or a struct with x, y, z)
				const auto& vector = std::get<Vec3>(value);

				// Set the values of each component (x, y, z)
				float xVal = vector.x;
				float yVal = vector.y;
				float zVal = vector.z;

				if (xField && yField && zField) {
					// Set the values of each component
					mono_field_set_value(instance, xField, &xVal);
					mono_field_set_value(instance, yField, &yVal);
					mono_field_set_value(instance, zField, &zVal);

					//CONSOLE_LOG_EXPLICIT(std::string(fieldName) + " Values Set: ", LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" X: " + std::to_string(xVal), LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" Y: " + std::to_string(yVal), LogLevel::LEVEL_DEBUG);
					//CONSOLE_LOG_EXPLICIT(" Z: " + std::to_string(zVal), LogLevel::LEVEL_DEBUG);
				}
				else {
					CONSOLE_LOG_EXPLICIT("Failed to find fields x, y, z in Vec3 struct!", LogLevel::LEVEL_ERROR);
				}
			}
			};

		const std::unordered_map<int, Field::ValueOperations> Field::valueTypeToOperationsMap{
			{ MONO_TYPE_I4, { DefaultRetrieveValuesFromScript<int>, DefaultSetValueOfScript<int> } },
			{ MONO_TYPE_BOOLEAN, { DefaultRetrieveValuesFromScript<bool>, DefaultSetValueOfScript<bool> } },
			{ MONO_TYPE_R4, { DefaultRetrieveValuesFromScript<float>, DefaultSetValueOfScript<float> } },
			{ MONO_TYPE_R8, { DefaultRetrieveValuesFromScript<double>, DefaultSetValueOfScript<double> } },
			{ MONO_TYPE_U8, { DefaultRetrieveValuesFromScript<size_t>, DefaultSetValueOfScript<size_t> } },
			{ MONO_TYPE_VALUETYPE, { DefaultRetrieveValuesFromScriptVector, DefaultSetValueOfScriptVector}},
			{ MONO_TYPE_STRING, {
				// RetrieveValuesFromScript
				[](auto value, auto field, auto instance) -> void {
					MonoString* monoStringValue;
					mono_field_get_value(instance, field, &monoStringValue);
					*value = (monoStringValue ? std::string{ mono_string_to_utf8(monoStringValue) } : std::string{});
				},
				// SetValueOfScript
				[](auto value, auto field, auto instance) -> void {
					MonoString* monoString = mono_string_new(mono_domain_get(), std::get<std::string>(value).c_str());
					mono_field_set_value(instance, field, monoString);
				}
			} }
		};
	}


	/*****************************************************************//*!
	\brief
		Struct to contain necessary data for engine scripting during
		runtime
	*//******************************************************************/
	struct SCScriptData
	{
		MonoDomain* s_RootDomain = nullptr;
		MonoDomain* s_AppDomain = nullptr;	// This is the .dll library used for C# scripting

		MonoAssembly* s_CoreAssembly = nullptr; // General functionality
		MonoAssembly* s_UserAssembly = nullptr; // Custom user Scripts
		MonoImage* s_CoreAssemblyImage = nullptr;
		MonoImage* s_UserAssemblyImage = nullptr;

		ScriptClass EntityClass;

		std::unordered_map<std::string, ScriptClass> s_CoreClasses;
	};
	static SCScriptData* ScriptEngineData;

	namespace Utils {
		/*****************************************************************//*!
		\brief
			Reads bytes from a file and returns the data inside as a char*
		\param[in] filepath
			path to the file to read the bytes of
		\param[in, out] outSize
			pointer to container of the file size
		\return
			Data inside the file as a char*
		*//******************************************************************/
		char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize)
		{
			std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

			if (!stream)
			{
				// Failed to open the file
				return nullptr;
			}

			std::streampos end = stream.tellg();
			stream.seekg(0, std::ios::beg);
			uint32_t size = static_cast<uint32_t>(end - stream.tellg());

			if (size == 0)
			{
				// File is empty
				return nullptr;
			}

			char* buffer = new char[size];
			stream.read((char*)buffer, size);
			stream.close();

			*outSize = size;
			return buffer;
		}
		/*****************************************************************//*!
		\brief
			Loads the .dll file and creates an Assembly Object out of it
		\param[in] assemblyPath
			path to the .dll file
		\return
			Returns the Assembly object created from the .dll file
		*//******************************************************************/
		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath)
		{
			CONSOLE_LOG_EXPLICIT("Loading CSharp Assembly!", LogLevel::LEVEL_INFO);
			uint32_t fileSize = 0;
			char* fileData = ReadBytes(assemblyPath, &fileSize);

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Failed to load CSharp Assembly! msg: " << mono_image_strerror(status);
				return nullptr;
			}

			std::string p = assemblyPath.string();
			MonoAssembly* assembly = mono_assembly_load_from_full(image, p.c_str(), &status, 0);
			mono_image_close(image);

			// Don't forget to free the file data
			delete[] fileData;

			CONSOLE_LOG_EXPLICIT("CSharp Assembly loaded successfully!", LogLevel::LEVEL_INFO);
			return assembly;
		}

		/*****************************************************************//*!
		\brief
			Prints the c# classes found inside the Assembly
		\param[in] assembly
			Assembly object to print the c# classes found inside of it.
		\return
			None
		*//******************************************************************/
		static void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

				// Example: Engine.InternalCalls
				std::string msg = nameSpace;
				msg += ".";
				msg += name;
				CONSOLE_LOG_EXPLICIT(msg, LEVEL_DEBUG);
			}
		}		

		/*****************************************************************//*!
		\brief
			Loads all the classes found in the assembly into an unordered map.
			Used for making class instances later.
		\param[in] assembly
			Assembly object to load classes from.
		\return
			None
		*//******************************************************************/
		static void LoadUserAssemblyClasses(MonoAssembly* assembly)
		{
			CONSOLE_LOG(LEVEL_DEBUG) << "Loading custom script classes...";

			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			MonoClass* scriptBaseClass = mono_class_from_name(ScriptEngineData->s_CoreAssemblyImage, "EngineScripting", "ComponentBase");

			for (int32_t i = 0; i < numTypes; ++i)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);
				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

				MonoClass* mClass = mono_class_from_name(image, nameSpace, name);
				bool isScript = mono_class_is_subclass_of(mClass, scriptBaseClass, false);

				if (!isScript || mClass == scriptBaseClass)
					continue;

				// This is a custom script class. Register it.
				std::string fullName = nameSpace;
				fullName += ".";
				fullName += name;
				ScriptEngineData->s_CoreClasses[fullName] = ScriptClass{ nameSpace, name };

				CONSOLE_LOG(LEVEL_DEBUG) << "Custom script: " << fullName;
			}

			CONSOLE_LOG(LEVEL_DEBUG) << "Load custom script classes done";
		}

		/*****************************************************************//*!
		\brief
			Gets a reference to a loaded Script class object inside the 
			unordered map.
		\param[in] className
			Name of the class to get.
		\return
			Returns a scriptclass object reference found inside the map
		*//******************************************************************/
		static ScriptClass& RetriveClassByName(std::string className)
		{
			return ScriptEngineData->s_CoreClasses[className];
		}

		/*****************************************************************//*!
		\brief
			Gets unordered map of loaded Script classed found inside of the 
			core assembly.
		\return
			Returns a reference of the unordered map of scriptclasses
		*//******************************************************************/
		static std::unordered_map<std::string, ScriptClass>& GetCoreClasses()
		{
			return ScriptEngineData->s_CoreClasses;
		}
	}

	namespace internal {

		const Field::ValueOperations* Field::GetValueOperation(int typeEnum)
		{
			auto operationIter{ valueTypeToOperationsMap.find(typeEnum) };
			if (operationIter == valueTypeToOperationsMap.end())
				return nullptr;
			return &operationIter->second;
		}

		bool Field::IsValidField(MonoClassField* field)
		{
			return GetValueOperation(mono_type_get_type(mono_field_get_type(field))) != nullptr;
		}

		Field::Field(MonoObject* instance, MonoClassField* field)
			: field{ field }
			, type{ mono_field_get_type(field) }
			, typeEnum{ mono_type_get_type(type) }
			, value{}
		{
			assert(type); // Kendrick - I've not seen this value be null, but just in case since there was previously an if check here

			// Load default value from the script
			RetrieveValueFromScript(instance);
		}

		void Field::SetValue(MonoObject* instance, const VariantType& newValue)
		{
			if (value.index() != newValue.index())
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Failed to set a script field's value; Types don't match!";
				return;
			}
			value = newValue;
			
			// Set the value in the C# script. Value operation should always exist if the variant type exists (in theory I think).
			GetValueOperation(typeEnum)->SetValueOfScript(value, field, instance);
		}

		VariantType& Field::GetValue()
		{
			return value;
		}

		void Field::RetrieveValueFromScript(MonoObject* instance)
		{
			if (auto operation{ GetValueOperation(typeEnum) })
				operation->RetrieveValueFromScript(&value, field, instance);
			else
				CONSOLE_LOG(LEVEL_ERROR) << "ScriptInstance::RetrieveValueFromScript() - Unknown enum type";
		}

		void Field::Serialize(Serializer& writer) const
		{
			writer.Serialize("", value);
		}

		void Field::Deserialize(Deserializer& reader)
		{
			reader.Deserialize("", &value);
		}

	}

	void CSScripting::Init()
	{
		ScriptEngineData = new SCScriptData();
		ScriptEngineData->s_CoreClasses.clear();
		InitMono();
		LoadAssembly(ST<Filepaths>::Get()->engineScriptingDll);
		//Utils::LoadAssemblyClasses(ScriptEngineData->s_CoreAssembly); // <<< This one can remove later since no core classes will be exposed to attach to entities
		// Can check if .csproj exists. if exists then dont bother calling this function
		// for future notice - Marc

#ifndef IMGUI_ENABLED
		if (!std::filesystem::exists(ST<Filepaths>::Get()->userAssemblyDll))
		{
#endif
		CreateUserProject();
		CompileUserAssembly();
#ifndef IMGUI_ENABLED
		}
#endif

		LoadUserAssembly(ST<Filepaths>::Get()->userAssemblyDll);
		Utils::LoadUserAssemblyClasses(ScriptEngineData->s_UserAssembly);
		ScriptGlue::RegisterFunctions();
		for (auto& pair : ScriptEngineData->s_CoreClasses)
		{
			CONSOLE_LOG_EXPLICIT(pair.second.GetFullName(), LogLevel::LEVEL_DEBUG);
		}
		//MonoTest();
	}

	// Test the internal calls and other stuff here
	void CSScripting::MonoTest()
	{
		CONSOLE_LOG_EXPLICIT("~~~~~~~~~~~~ MONO TEST START ~~~~~~~~~~~~", LogLevel::LEVEL_DEBUG);
		CONSOLE_LOG_EXPLICIT("Calling C# functions inside C++", LogLevel::LEVEL_DEBUG);

		// Get assembly and class 
		//MonoImage* assemblyImage = mono_assembly_get_image(ScriptEngineData->s_AppAssembly);
		//ScriptEngineData->EntityClass = ScriptClass("", "CSharpTesting");

		ScriptInstance in(Utils::RetriveClassByName(".CSharpTesting"));
		in.OnCreateInvoke();
		// Create instance of class object and init
		CONSOLE_LOG_EXPLICIT("Creating Instance of a class object", LogLevel::LEVEL_DEBUG);
		//MonoObject* instance = ScriptEngineData->EntityClass.Instanstiate();
		MonoObject* instance = Utils::RetriveClassByName(".CSharpTesting").Instanstiate();
		ScriptEngineData->EntityClass = Utils::RetriveClassByName(".CSharpTesting");


		CONSOLE_LOG_EXPLICIT("Function with NO params Test", LogLevel::LEVEL_DEBUG);
		MonoMethod* printMsgFunc = ScriptEngineData->EntityClass.GetMethod("NoParamFunc", 0);
		ScriptEngineData->EntityClass.InvokeMethod(instance, printMsgFunc, nullptr);
		//MonoObject* exception = nullptr;
		//mono_runtime_invoke(printMsgFunc, instance, nullptr, &exception);

		CONSOLE_LOG_EXPLICIT("Function with ONE params Test", LogLevel::LEVEL_DEBUG);
		MonoMethod* printValFunc = ScriptEngineData->EntityClass.GetMethod("OneValueParamFunc", 1);

		int value = 5;
		void* param = &value;

		ScriptEngineData->EntityClass.InvokeMethod(instance, printValFunc, &param);

		//mono_runtime_invoke(printValFunc, instance, &param, &exception);

		CONSOLE_LOG_EXPLICIT("Function with TWO params Test", LogLevel::LEVEL_DEBUG);
		MonoMethod* printTwoValFunc = ScriptEngineData->EntityClass.GetMethod("TwoValueParamFunc", 2);

		int value2 = 400;
		void* params[2] = {
			&value,
			&value2
		};

		ScriptEngineData->EntityClass.InvokeMethod(instance, printTwoValFunc, params);

		//mono_runtime_invoke(printTwoValFunc, instance, params, &exception);

		CONSOLE_LOG_EXPLICIT("Function with String Param Test", LogLevel::LEVEL_DEBUG);
		// Need to use MonoString for any string related stuff
		MonoString* monoStr = mono_string_new(ScriptEngineData->s_AppDomain, "Hello World from C++!");
		MonoMethod* printCustomMessageFunc = ScriptEngineData->EntityClass.GetMethod("PrintStringMessageFunc", 1);

		void* strParam = monoStr;
		//mono_runtime_invoke(printCustomMessageFunc, instance, &strParam, nullptr);
		ScriptEngineData->EntityClass.InvokeMethod(instance, printCustomMessageFunc, &strParam);


		CONSOLE_LOG_EXPLICIT("~~~~~~~~~~~~  MONO TEST END  ~~~~~~~~~~~~", LogLevel::LEVEL_DEBUG);
	}

	ScriptClass& CSScripting::GetClassFromData(std::string cName)
	{
		return Utils::RetriveClassByName(cName);
	}

	std::unordered_map<std::string, ScriptClass>& CSScripting::GetCoreClassMap()
	{
		return Utils::GetCoreClasses();
	}

	void CSScripting::ReloadAssembly()
	{
		CONSOLE_LOG_EXPLICIT("~~~~~~~~~~ RELOADING USER SCRIPTS! ~~~~~~~~~~", LogLevel::LEVEL_DEBUG);
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(ScriptEngineData->s_AppDomain);
		ScriptEngineData->s_CoreClasses.clear();
		LoadAssembly(ST<Filepaths>::Get()->engineScriptingDll);
		//Utils::LoadAssemblyClasses(ScriptEngineData->s_CoreAssembly);

		CreateUserProject();
		CompileUserAssemblyAsync([]() -> void {
			LoadUserAssembly(ST<Filepaths>::Get()->userAssemblyDll);
			Utils::LoadUserAssemblyClasses(ScriptEngineData->s_UserAssembly);
			ScriptGlue::RegisterFunctions();

			// reload the ecs scripts
			// 
			for (auto ite = ecs::GetCompsBegin<ScriptComponent>(); ite != ecs::GetCompsEnd<ScriptComponent>(); ++ite)
			{
				ite.GetComp()->SaveVariables();
				ite.GetComp()->RemoveAllScripts();
			}

			// After reloading all the scripts load the variables into them
			for (auto ite = ecs::GetCompsBegin<ScriptComponent>(); ite != ecs::GetCompsEnd<ScriptComponent>(); ++ite)
			{
				ite.GetComp()->ReattachAllScripts();
				ite.GetComp()->LoadVariables();
			}
		});
	}

	void CSScripting::Exit()
	{
		// unload root domain
		mono_domain_set(mono_get_root_domain(), false);

		mono_domain_unload(ScriptEngineData->s_AppDomain);
		ScriptEngineData->s_AppDomain = nullptr;
		
		mono_jit_cleanup(ScriptEngineData->s_RootDomain);
		ScriptEngineData->s_RootDomain = nullptr;

		ScriptEngineData->s_CoreClasses.clear();
	
		delete ScriptEngineData;
	}

	void CSScripting::InitMono()
	{
		mono_set_assemblies_path("mono");
		MonoDomain* rootDomain = mono_jit_init("MyScriptRuntime");
		
		if (rootDomain == nullptr)
		{
			// Maybe log some error here
			CONSOLE_LOG_EXPLICIT("Root Domain failed to init!", LogLevel::LEVEL_ERROR);
			return;
		}
		else
		{
			CONSOLE_LOG_EXPLICIT(".NET Path Found!", LogLevel::LEVEL_INFO);
		}

		// Store the root domain pointer
		ScriptEngineData->s_RootDomain = rootDomain;

	}

	MonoObject* CSScripting::InstanstiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(ScriptEngineData->s_AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	void CSScripting::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Store the app domain
		char appName[] = "MyAppDomain";
		ScriptEngineData->s_AppDomain = mono_domain_create_appdomain(appName, nullptr);
		mono_domain_set(ScriptEngineData->s_AppDomain, true);

		// Load the Core Assembly
		ScriptEngineData->s_CoreAssembly = Utils::LoadMonoAssembly(filepath);
		ScriptEngineData->s_CoreAssemblyImage = mono_assembly_get_image(ScriptEngineData->s_CoreAssembly);


		// Check the types inside the Assembly(aka the methods to link C# and Cpp functions)
		Utils::PrintAssemblyTypes(ScriptEngineData->s_CoreAssembly);
	}

	void CSScripting::CreateUserProject()
	{
		std::ofstream csprojFile(ST<Filepaths>::Get()->csproj);
		if (!csprojFile.is_open())
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Failed to create/overwrite scripting .csproj file";
			return;
		}

		auto engineDll{ std::filesystem::absolute(ST<Filepaths>::Get()->engineScriptingDll) };
		auto csproj{ std::filesystem::absolute(ST<Filepaths>::Get()->csproj) };
		auto glmSharpDll{ std::filesystem::absolute(ST<Filepaths>::Get()->glmSharpDll) };

		csprojFile <<
R"(<Project Sdk="Microsoft.NET.Sdk">
	<PropertyGroup>
		<OutputType>Library</OutputType>
		<TargetFramework>net472</TargetFramework>
		<Platforms>x64</Platforms>
	</PropertyGroup>
	<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
		<OutputPath>Temp\bin\Debug\</OutputPath>
		<PlatformTarget>x64</PlatformTarget>
	</PropertyGroup>
	<PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
		<OutputPath>Temp\bin\Release\</OutputPath>
		<PlatformTarget>x64</PlatformTarget>
	</PropertyGroup>
	<ItemGroup>
		<Reference Include="EngineScripting">
			<HintPath>)"
			// On this line we call parent_path(), otherwise relative() will think the .csproj is a directory and not a file, thus adding an extra ../
			<< std::filesystem::relative(engineDll, csproj.parent_path()).string() <<
			R"(</HintPath>
		</Reference>
		<Reference Include="GlmSharp">
			<HintPath>)"
			// Same as above
			<< std::filesystem::relative(glmSharpDll, csproj.parent_path()).string() <<
			R"(</HintPath>
		</Reference>
	</ItemGroup>
</Project>)";

		csprojFile.close();
	}

	bool CSScripting::CompileUserAssembly()
	{
		// Helper function to detect if the main thread is still alive (so if we're multithreaded, we avoid any operation that requires the main thread)
		auto IsMainThreadAlive{ []() -> bool { return ST<Engine>::IsInitialized() && !ST<Engine>::Get()->IsShuttingDown(); } };


		// Create a read/write pipe for the compilation to output to
		HANDLE hReadPipe, hWritePipe;
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE; // Allow the handle to be inherited
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
			CONSOLE_LOG(LEVEL_ERROR) << "User Assembly Compilation: Failed to create pipe! Aborting.";
			return false;
		}

		// Setup the cmd command
		std::string csprojPathAsStr{ "\"" + ST<Filepaths>::Get()->csproj + "\"" };
		std::wstring csprojPath{ csprojPathAsStr.begin(), csprojPathAsStr.end() }; // Note that this only works with non-multibyte chars
		std::wstring buildCmd;
#ifdef _DEBUG
		buildCmd = L"dotnet build -c Debug " + csprojPath;
#else
		buildCmd = L"dotnet build -c Release " + csprojPath;
#endif

		// Execute command on a child process
		STARTUPINFOW si{};
		si.cb = sizeof(STARTUPINFOW);
		si.hStdOutput = hWritePipe;  // Redirect child process's stdout to pipe
		si.hStdError = hWritePipe;   // Optionally redirect stderr as well
		si.dwFlags = STARTF_USESTDHANDLES;
		PROCESS_INFORMATION pi{};
		if (!CreateProcessW(NULL, const_cast<LPWSTR>(buildCmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			CONSOLE_LOG(LEVEL_ERROR) << "User Assembly Compilation: Failed to execute build command! Aborting.";
			CloseHandle(hReadPipe);
			CloseHandle(hWritePipe);
			return false;
		}

		// Close the write end of the pipe, as the process is writing to it
		CloseHandle(hWritePipe);

		// Read the output from the pipe
		char buffer[4096];
		DWORD dwRead;
		while (true) {
			BOOL bSuccess = ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL);
			if (!bSuccess || dwRead == 0)
				break;
			// Null-terminate the string. -1 is to remove the \n
			buffer[dwRead - 1] = '\0';

			// If the main program is shutting down, there's no need to continue processing the compilation output
			if (!IsMainThreadAlive())
				break;
			CONSOLE_LOG(LEVEL_DEBUG) << buffer;
		}

		// Wait for the child process to finish
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Check if the compilation succeeded
		DWORD exitCode{};
		GetExitCodeProcess(pi.hProcess, &exitCode);
		bool compilationSucceeded{ exitCode == 0 };
		if (!compilationSucceeded && IsMainThreadAlive())
			CONSOLE_LOG(LEVEL_ERROR) << "User Assembly Compilation: Compilation failed! Please check debug output for the compilation log.";

		// Close handles
		CloseHandle(hReadPipe);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// If compilation failed, there's no new assembly to overwrite.
		// Otherwise, if we're on the async thread, the user might've closed the program while we were compiling.
		// In either case, just stop doing any further work.
		if (!(compilationSucceeded && IsMainThreadAlive()))
		{
			CleanUserAssemblyTempFiles();
			return false;
		}

		try
		{
			std::filesystem::path sourcePath;
#ifdef _DEBUG  // If in Debug mode
			sourcePath = ST<Filepaths>::Get()->scriptsWorkingDir + "/bin/Debug/net472/UserAssembly.dll";
#else  // If in Release mode
			sourcePath = ST<Filepaths>::Get()->scriptsWorkingDir + "/bin/Release/net472/UserAssembly.dll";
#endif
			std::filesystem::path destinationPath = ST<Filepaths>::Get()->userAssemblyDll;
			if (std::filesystem::exists(sourcePath))
			{
				std::filesystem::create_directories(destinationPath.parent_path());
				std::filesystem::copy_file(sourcePath, destinationPath,
					std::filesystem::copy_options::overwrite_existing);
			}
			else
				CONSOLE_LOG(LEVEL_ERROR) << "User Assembly Compilation: Assembly file does not exist: " << sourcePath.string() << " | Current Path: " << std::filesystem::current_path().string();
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "User Assembly Compilation: FileSystem error: " << e.what();
		}

		CleanUserAssemblyTempFiles();
		CONSOLE_LOG(LEVEL_INFO) << "User Assembly Compilation: Successful";
	}

	void CSScripting::CompileUserAssemblyAsync(void(*callback)())
	{
		if (isCompilingUserAssemblyAsync)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Tried to async compile user assembly while the operation is already ongoing!";
			return;
		}
		isCompilingUserAssemblyAsync = true;

		ST<Popup>::Get()->Open("Reloading Scripts...", "Reloading scripts...\nPlease wait a moment!", gui::PopupWindow::FLAG::NO_CLOSE_BUTTON);
		compileUserAssemblyCallback = callback;

		// Launch thread
		compileUserAssemblyFuture = std::async(std::launch::async, []() {
			CompileUserAssembly();
			return 0;
		});
	}

	void CSScripting::CheckCompileUserAssemblyAsyncCompletion()
	{
		if (!isCompilingUserAssemblyAsync)
			return;

		if (compileUserAssemblyFuture.wait_for(std::chrono::milliseconds::zero()) != std::future_status::ready)
			return;

		// Operation is complete
		compileUserAssemblyFuture.get();
		isCompilingUserAssemblyAsync = false;
		ST<Popup>::Get()->Close();

		if (compileUserAssemblyCallback)
			compileUserAssemblyCallback();
	}

	void CSScripting::CleanUserAssemblyTempFiles()
	{
		Filepaths& filepaths{ *ST<Filepaths>::Get() };
		std::filesystem::remove_all(filepaths.scriptsWorkingDir + "/bin");
		std::filesystem::remove_all(filepaths.scriptsWorkingDir + "/obj");
		//std::filesystem::remove(filepaths.csproj);
	}

	void CSScripting::LoadUserAssembly(const std::filesystem::path& filepath)
	{
		ScriptEngineData->s_UserAssembly = Utils::LoadMonoAssembly(filepath);
		ScriptEngineData->s_UserAssemblyImage = mono_assembly_get_image(ScriptEngineData->s_UserAssembly);
	}

	ScriptClass::ScriptClass(const std::string& classNameSpace, const std::string& className)
		: m_ClassNameSpace(classNameSpace)
		, m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(ScriptEngineData->s_CoreAssemblyImage, classNameSpace.c_str(), className.c_str());
		if (m_MonoClass == nullptr)
		{
			m_MonoClass = mono_class_from_name(ScriptEngineData->s_UserAssemblyImage, classNameSpace.c_str(), className.c_str());
		}
		m_SetHandleMethod = GetMethod("SetHandle", 1);
		m_OnCreateMethod = GetMethod("OnCreate", 0);
		m_OnUpdateMethod = GetMethod("OnUpdate", 1);
		m_OnCollisionMethod = GetMethod("OnCollision", 1);
		
		m_OnStartMethod = GetMethod("OnStart", 0);
		m_AwakeMethod = GetMethod("Awake", 0);
		m_LateUpdateMethod = GetMethod("LateUpdate", 1);
	}

	ScriptClass::~ScriptClass()
	{
		m_SetHandleMethod = nullptr;
		m_OnCreateMethod  = nullptr;
		m_OnUpdateMethod  = nullptr;
		m_OnCollisionMethod  = nullptr;
		m_OnStartMethod = nullptr;
		m_AwakeMethod = nullptr;
		m_MonoClass = nullptr;
		m_LateUpdateMethod = nullptr;
	}

	std::string ScriptClass::GetFullName() const
	{
		return std::string(m_ClassNameSpace + "." + m_ClassName);
	}

	MonoObject* ScriptClass::Instanstiate() const
	{
		if (!m_MonoClass)
			return nullptr;
		return CSScripting::InstanstiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int paramCount)
	{
		MonoMethod* method = mono_class_get_method_from_name(m_MonoClass, name.c_str(), paramCount);

		if (!method)
		{
			MonoClass* parent = mono_class_get_parent(m_MonoClass);
			while (parent)
			{
				method = mono_class_get_method_from_name(parent, name.c_str(), paramCount);
				if (method)
				{
					break;
				}
				parent = mono_class_get_parent(parent);
			}
		}

		return method;
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		return mono_runtime_invoke(method, instance, params, nullptr);
	}

	MonoClass* ScriptClass::GetClass() const
	{
		return m_MonoClass;
	}

	MonoObject* ScriptClass::InvokeSetHandle(MonoObject* instance, void** params)
	{
		return mono_runtime_invoke(m_SetHandleMethod, instance, params, nullptr);
	};

	MonoObject* ScriptClass::InvokeOnCreate(MonoObject* instance)
	{
		return mono_runtime_invoke(m_OnCreateMethod, instance, nullptr, nullptr);
	}

	MonoObject* ScriptClass::InvokeOnUpdate(MonoObject* instance, void** params)
	{
		return mono_runtime_invoke(m_OnUpdateMethod, instance, params, nullptr);
	}

	MonoObject* ScriptClass::InvokeOnCollision(MonoObject* instance, void** params)
	{
		return mono_runtime_invoke(m_OnCollisionMethod, instance, params, nullptr);
	}

	MonoObject* ScriptClass::InvokeLateUpdate(MonoObject* instance, void** params)
	{
		return mono_runtime_invoke(m_LateUpdateMethod , instance, params, nullptr);
	}

	MonoObject* ScriptClass::InvokeOnStart(MonoObject* instance)
	{
		return mono_runtime_invoke(m_OnStartMethod, instance, nullptr, nullptr);
	}

	MonoObject* ScriptClass::InvokeAwake(MonoObject* instance)
	{
		return mono_runtime_invoke(m_AwakeMethod, instance, nullptr, nullptr);
	}

	bool ScriptClass::SetHandleExists() const
	{
		return m_SetHandleMethod != nullptr;
	}

	bool ScriptClass::OnCreateExists() const
	{
		return m_OnCreateMethod != nullptr;
	}

	bool ScriptClass::OnUpdateExists() const
	{
		return m_OnUpdateMethod != nullptr;
	}

	bool ScriptClass::OnCollisionExists() const
	{
		return m_OnCollisionMethod != nullptr;
	}

	bool ScriptClass::OnStartExists() const
	{
		return m_OnStartMethod != nullptr;
	}

	bool ScriptClass::AwakeExists() const
	{
		return m_AwakeMethod != nullptr;
	}

	bool ScriptClass::LateUpdateExists() const
	{
		return m_LateUpdateMethod != nullptr;
	}

	ScriptInstance::ScriptInstance(const ScriptClass& sclass)
		: m_ScriptClass(new ScriptClass(sclass))
	{
		m_Instance = m_ScriptClass->Instanstiate();
		m_InstanceHandle = mono_gchandle_new_v2(m_Instance, true);
	}

	ScriptInstance::ScriptInstance(const ScriptInstance& other)
		: m_ScriptClass{ new ScriptClass{ *other.m_ScriptClass } }
		, m_Instance{ other.m_Instance }
		, m_InstanceHandle{ mono_gchandle_new_v2(m_Instance, true) }
		, m_PublicVars{ other.m_PublicVars }
	{
	}

	ScriptInstance& ScriptInstance::operator=(const ScriptInstance& other)
	{
		m_ScriptClass = other.m_ScriptClass;
		m_Instance = other.m_Instance;
		m_InstanceHandle = mono_gchandle_new_v2(m_Instance, true);
		m_PublicVars = other.m_PublicVars;
		return *this;
	}

	ScriptInstance& ScriptInstance::operator=(ScriptInstance&& other) noexcept {
		if (this != &other) {
			// Clean up existing resources
			if (m_InstanceHandle) {
				mono_gchandle_free_v2(m_InstanceHandle);
			}
			if (m_Instance) {
				// Optionally clean up the MonoObject here
				// mono_object_free(m_instance);  // Hypothetical cleanup
			}

			// Transfer ownership of Mono resources
			m_Instance = other.m_Instance;
			m_InstanceHandle = other.m_InstanceHandle;
			m_ScriptClass = other.m_ScriptClass; // Just copy the pointer, no ownership change
			m_PublicVars = std::move(other.m_PublicVars);

			// Nullify the moved-from object
			other.m_Instance = nullptr;
			other.m_InstanceHandle = 0;
			other.m_ScriptClass = nullptr; // No need to delete m_scriptClass

			
		}
		return *this;
	}

	ScriptInstance::ScriptInstance(ScriptInstance&& other) noexcept
		: m_ScriptClass{ std::move(other.m_ScriptClass) }
		, m_Instance{ other.m_Instance }
		, m_InstanceHandle{ other.m_InstanceHandle }
		, m_PublicVars{ std::move(other.m_PublicVars) }
	{
		other.m_ScriptClass = nullptr;
		other.m_Instance = nullptr;
		other.m_InstanceHandle = 0;
	}

	ScriptInstance::~ScriptInstance()
	{
		if (m_InstanceHandle != 0) {
			mono_gchandle_free_v2(m_InstanceHandle); // This frees the GC handle
			m_InstanceHandle = 0; // Ensure that we don't accidentally use it later
		}
		CONSOLE_LOG_EXPLICIT("ScriptInstance destructor!", LogLevel::LEVEL_DEBUG);
		m_PublicVars.clear();
		delete m_ScriptClass;
	}

	ScriptClass& ScriptInstance::GetClass()
	{
		return *m_ScriptClass;
	}

	void ScriptInstance::SetHandleInvoke(const ScriptComponent& comp)
	{
		if (!m_ScriptClass->SetHandleExists())
		{
			return;
		}
		uint64_t handle = reinterpret_cast<uint64_t>(ecs::GetEntity(&comp));

		void* param = &handle;
		m_ScriptClass->InvokeSetHandle(m_Instance, &param);
	}

	void ScriptInstance::AwakeInvoke()
	{
		if (!m_ScriptClass->AwakeExists())
		{
			return;
		}
		m_ScriptClass->InvokeAwake(m_Instance);
	}

	void ScriptInstance::OnStartInvoke()
	{
		if (!m_ScriptClass->OnStartExists())
		{
			return;
		}
		m_ScriptClass->InvokeOnStart(m_Instance);
	}

	void ScriptInstance::OnCreateInvoke()
	{
		if (!m_ScriptClass->OnCreateExists())
		{
			return;
		}

		m_ScriptClass->InvokeOnCreate(m_Instance);
	}

	void ScriptInstance::OnUpdateInvoke(float ts)
	{
		if (!m_ScriptClass->OnUpdateExists())
		{
			return;
		}
		void* param = &ts;
		m_ScriptClass->InvokeOnUpdate(m_Instance, &param);

	}

	void ScriptInstance::LateUpdateInvoke(float ts)
	{
		if (!m_ScriptClass->LateUpdateExists())
		{
			return;
		}
		void* param = &ts;
		m_ScriptClass->InvokeLateUpdate(m_Instance, &param);
	}

	void ScriptInstance::RetrievePublicVariables()
	{
		// For saving the value of public variables
		auto SaveField{ [m_Instance = m_Instance, &m_PublicVars = m_PublicVars]<typename T>(MonoClassField* field, const char* fieldName) -> void {
			T value{};
			mono_field_get_value(m_Instance, field, &value);
			m_PublicVars.try_emplace(fieldName, field, value);
		} };

		void* iter = nullptr; // Iterator for the field list

		for (MonoClassField* field{ mono_class_get_fields(m_ScriptClass->GetClass(), &iter) };
			field;
			field = mono_class_get_fields(m_ScriptClass->GetClass(), &iter))
		{
			const char* fieldName = mono_field_get_name(field);

			// TODO: Fix this in a future project!
			//   Because the scene files currently include public variables, we're forced to deserialize public variables, which means we're forced to
			//   retrieve and save them. In a future project, we should uncomment the lines below to not save public variables.
			// Only process public flags (1 == private, 6 == public)
			/*if (mono_field_get_flags(field) == 1)
				continue;*/

			// Get the field type
			MonoType* fieldType = mono_field_get_type(field);
			int fieldtag = mono_type_get_type(fieldType);
			if (fieldtag == MONO_TYPE_CLASS)
			{
				// auto classType{ mono_type_get_class(fieldType) };
				//CONSOLE_LOG(LEVEL_DEBUG) << "Scripting found class \"" << mono_class_get_name(classType) << "\" as a public local variable.";
			}
			
			if (internal::Field::IsValidField(field))
				m_PublicVars.try_emplace(fieldName, m_Instance, field);
		}
	}

	std::unordered_map<std::string, internal::Field>& ScriptInstance::GetPublicVars()
	{
		return m_PublicVars;
	}

	const std::unordered_map<std::string, internal::Field>& ScriptInstance::GetPublicVars() const
	{
		return m_PublicVars;
	}

	MonoObject* ScriptInstance::GetInstance() const
	{
		return m_Instance;
	}

	std::string ScriptInstance::GetClassFullName() const
	{
		return m_ScriptClass->GetFullName();
	}

	void ScriptInstance::SetPublicVar(const std::string& varName, const VariantType& newValue)
	{
		auto fieldIter{ m_PublicVars.find(varName) };
		if (fieldIter == m_PublicVars.end())
		{
			CONSOLE_LOG(LEVEL_WARNING) << "ScriptInstance::SetPublicVar() - Field \"" << varName << "\" not found!";
			return;
		}

		fieldIter->second.SetValue(m_Instance, newValue);
	}

	void ScriptInstance::Serialize(Serializer& writer) const
	{
		writer.Serialize("name", GetClassFullName());//Highly likely the name is not needed here
		writer.Serialize("vars", m_PublicVars);
	}
	void ScriptInstance::Deserialize(Deserializer& reader)
	{
		std::string name{};
		reader.DeserializeVar("name", &name);
		// TODO: Cleanup this probably unnecessary move
		*this = CSharpScripts::ScriptInstance(CSharpScripts::CSScripting::GetClassFromData(name));
		RetrievePublicVariables();
		
		// Deserialize the public variables. Using custom operation to avoid default construction of Field class in the default implementation.
		reader.DeserializeVar("vars", &m_PublicVars, [](Deserializer& reader, decltype(m_PublicVars)* publicVarsMapPtr) -> void {
			PublicVarsMapType::key_type key{};
			reader.DeserializeVar("key", &key);
			
			// Find the corresponding field and tell it to deserialize the value according to its type.
			auto fieldIter{ publicVarsMapPtr->find(key) };
			if (fieldIter == publicVarsMapPtr->end())
				CONSOLE_LOG(LEVEL_WARNING) << "ScriptInstance deserialization failed to find public variable with key \"" << key << "\"!";
			else
			{
				if (!reader.PushAccess("value"))
					return;
				fieldIter->second.Deserialize(reader);
				reader.PopAccess();
			}
		});
	}
}
