#pragma once
// Forward declaration
#ifdef GLFW
struct _MonoString;
#endif

namespace util {
	void AssertEntityHandleValid(ecs::EntityHandle entity);

	template <typename CompType>
	CompType* AssertCompExistsOnValidEntityAndGet(ecs::EntityHandle entity);
#ifdef GLFW
	_MonoString* StrToMonoStr(const std::string& str);
#endif

}

namespace util {

	template<typename CompType>
	CompType* AssertCompExistsOnValidEntityAndGet(ecs::EntityHandle entity)
	{
		AssertEntityHandleValid(entity);

		auto compHandle{ entity->GetComp<CompType>() };
		// The calling function expects this entity to have this component, but it doesn't exist.
		assert(compHandle);

		return compHandle;
	}

}

