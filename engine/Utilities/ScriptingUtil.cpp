#include "pch.h"
#include "ScriptingUtil.h"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"

namespace util {

	void AssertEntityHandleValid(ecs::EntityHandle entity)
	{
		// Some transform object on C# side is attempting to update itself based on an
		// entity that doesn't exist! The entity has probably been deleted but C# is
		// still keeping a reference to that entity.
		assert(ecs::IsEntityHandleValid(entity));
	}

	MonoString* StrToMonoStr(const std::string& str)
	{
		return mono_string_new(mono_domain_get(), str.c_str());
	}

}
