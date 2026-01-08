#include "pch.h"
#ifdef GLFW
#include "Utilities/ScriptingUtil.h"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#endif

namespace util {
	void AssertEntityHandleValid([[maybe_unused]] ecs::EntityHandle entity)
	{
		// Some transform object on C# side is attempting to update itself based on an
		// entity that doesn't exist! The entity has probably been deleted but C# is
		// still keeping a reference to that entity.
		assert(ecs::IsEntityHandleValid(entity));
	}

#ifdef GLFW
	MonoString* StrToMonoStr(const std::string& str)
	{
		return mono_string_new(mono_domain_get(), str.c_str());
	}
#endif
}
