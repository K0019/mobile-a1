#pragma once

namespace util {

	void AssertEntityHandleValid(ecs::EntityHandle entity);

	template <typename CompType>
	CompType* AssertCompExistsOnValidEntityAndGet(ecs::EntityHandle entity);

	MonoString* StrToMonoStr(const std::string& str);

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
