#pragma once
#include "Input.h"

template <InputSupportedValueTypes ValueType>
ValueType InputBinding<ValueType>::GetValue() const
{
	if constexpr (std::is_same_v<ValueType, bool>)
		return Get<bool>(hardwareValues_Button);
	else if constexpr (std::is_same_v<ValueType, float>)
		return Get<float>(hardwareValues_1D[0]) - Get<float>(hardwareValues_1D[1]);
	else if constexpr (std::is_same_v<ValueType, Vec2>)
		return Vec2{
			Get<float>(hardwareValues_2D[0]) - Get<float>(hardwareValues_2D[1]),
			Get<float>(hardwareValues_2D[2]) - Get<float>(hardwareValues_2D[3]),
		};
	else
	{
		assert(false); // Unimplemented value type
		return ValueType{};
	}
}

template<InputSupportedValueTypes ValueType>
template<InputSupportedValueTypes NewValueType>
InputBinding<NewValueType> InputBinding<ValueType>::ConvertToValueType() const
{
	InputBinding<NewValueType> newBinding{};
	// The bindings' members are all trivial and 8 bytes aligned.
	// We can directly copy the memory as is to the new destination.
	std::memcpy(&newBinding, this, std::min(sizeof(*this), sizeof(newBinding)));
	return newBinding;
}

template<InputSupportedValueTypes ValueType>
template <typename DesiredValueType>
DesiredValueType InputBinding<ValueType>::Get(const InputHardwareValueLink& hardwareValueLink)
{
	auto variantValue{ hardwareValueLink.ReadValue() };

	if constexpr (std::is_same_v<DesiredValueType, bool>)
		return std::visit(util::VisitFunctions{
			[](bool value) -> bool { return value; },
			[](float value) -> bool { return value >= internal::DEADZONE; }
		}, variantValue);
	else if constexpr (std::is_same_v<DesiredValueType, float>)
		return std::visit(util::VisitFunctions{
			[](bool value) -> float { return (value ? 1.0f : 0.0f); },
			[](float value) -> float { return value; }
		}, variantValue);
	else
	{
		// Unimplemented value type for the variant InputHardwareValue
		assert(false);
		return DesiredValueType{};
	}
}

template<InputSupportedValueTypes ValueType>
void InputBinding<ValueType>::Serialize(Serializer& writer) const
{
	if constexpr (std::is_same_v<ValueType, bool>)
		writer.Serialize("hardwareLink", hardwareValues_Button);
	else if constexpr (std::is_same_v<ValueType, float>)
		writer.Serialize("hardwareLink", hardwareValues_1D);
	else if constexpr (std::is_same_v<ValueType, Vec2>)
		writer.Serialize("hardwareLink", hardwareValues_2D);
	else
		assert(false); // Unimplemented value type
}

template<InputSupportedValueTypes ValueType>
void InputBinding<ValueType>::Deserialize(Deserializer& reader)
{
	if constexpr (std::is_same_v<ValueType, bool>)
		reader.Deserialize("hardwareLink", &hardwareValues_Button);
	else if constexpr (std::is_same_v<ValueType, float>)
		reader.DeserializeVar("hardwareLink", &hardwareValues_1D);
	else if constexpr (std::is_same_v<ValueType, Vec2>)
		reader.DeserializeVar("hardwareLink", &hardwareValues_2D);
	else
		assert(false); // Unimplemented value type
}

template<InputSupportedValueTypes ValueType>
template<typename FuncType>
void InputBinding<ValueType>::Editor_ForEachHardwareValueLink(FuncType func)
{
	if constexpr (std::is_same_v<ValueType, bool>)
		func(hardwareValues_Button);
	else if constexpr (std::is_same_v<ValueType, float>)
		std::for_each(hardwareValues_1D.begin(), hardwareValues_1D.end(), func);
	else if constexpr (std::is_same_v<ValueType, Vec2>)
		std::for_each(hardwareValues_2D.begin(), hardwareValues_2D.end(), func);
	else
		assert(false); // Unimplemented value type
}

template<InputSupportedValueTypes ValueType>
InputAction<ValueType>::InputAction()
	: InputActionBase{
		std::is_same_v<ValueType, bool> ? INPUT_COMPOSITE_TYPE::BUTTON :
		std::is_same_v<ValueType, float> ? INPUT_COMPOSITE_TYPE::AXIS_1D :
		std::is_same_v<ValueType, Vec2> ? INPUT_COMPOSITE_TYPE::AXIS_2D :
		INPUT_COMPOSITE_TYPE::NUM_TYPES
	}
{
	assert(GetCompositeType() != INPUT_COMPOSITE_TYPE::NUM_TYPES); // Unimplemented value type (see above constructor initializer)
}

template <InputSupportedValueTypes ValueType>
ValueType InputAction<ValueType>::GetValue() const
{
	if constexpr (std::is_same_v<ValueType, bool>)
	{
		for (const auto& binding : bindings)
			if (binding.GetValue())
				return true;
		return false;
	}
	else if constexpr (std::is_same_v<ValueType, float>)
	{
		float finalReading{};
		for (const auto& binding : bindings)
		{
			float bindingReading{ binding.GetValue() };
			if (std::fabsf(bindingReading) > finalReading)
				finalReading = bindingReading;
		}
		return finalReading;
	}
	else if constexpr (std::is_same_v<ValueType, Vec2>)
	{
		Vec2 finalReading{};
		for (const auto& binding : bindings)
		{
			Vec2 bindingReading{ binding.GetValue() };
			if (std::fabsf(bindingReading.x) > finalReading.x)
				finalReading.x = bindingReading.x;
			if (std::fabsf(bindingReading.y) > finalReading.y)
				finalReading.y = bindingReading.y;
		}
		return finalReading;
	}
	else
	{
		assert(false); // Unimplemented value type
		return ValueType{};
	}
}

template<InputSupportedValueTypes ValueType>
void InputAction<ValueType>::AddBinding()
{
	bindings.emplace_back();
}

template<InputSupportedValueTypes ValueType>
const InputBinding<ValueType>* InputAction<ValueType>::GetBinding(size_t index) const
{
	return (0 <= index && index < bindings.size()) ? &bindings[index] : nullptr;
}
template<InputSupportedValueTypes ValueType>
InputBinding<ValueType>* InputAction<ValueType>::GetBinding(size_t index)
{
	return (0 <= index && index < bindings.size()) ? &bindings[index] : nullptr;
}

template<InputSupportedValueTypes ValueType>
template <InputSupportedValueTypes NewValueType>
InputAction<NewValueType> InputAction<ValueType>::ConvertToValueType() const
{
	InputAction<NewValueType> newAction{};
	newAction.INTERNAL_ConvertAndSetBindings(bindings);
	return newAction;
}

template<InputSupportedValueTypes ValueType>
template <InputSupportedValueTypes OriginalValueType>
void InputAction<ValueType>::INTERNAL_ConvertAndSetBindings(const std::vector<InputBinding<OriginalValueType>>& originalBindings)
{
	std::transform(originalBindings.begin(), originalBindings.end(), std::back_inserter(bindings), [](const auto& binding) -> auto {
		return binding.ConvertToValueType<ValueType>();
	});
}

template<InputSupportedValueTypes ValueType>
void InputAction<ValueType>::Serialize(Serializer& writer) const
{
	// For above InputSet to identify what type of InputAction to create
	writer.Serialize("compositeType", +GetCompositeType());
	writer.Serialize("bindings", bindings);
}

template<InputSupportedValueTypes ValueType>
void InputAction<ValueType>::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("bindings", &bindings);
}

template<InputSupportedValueTypes ValueType>
std::vector<InputBinding<ValueType>>& InputAction<ValueType>::Editor_GetBindings()
{
	return bindings;
}

template <InputSupportedValueTypes ValueType>
SPtr<const InputAction<ValueType>> Input::GetAction(const std::string& actionName) const
{
	for (const auto& inputSet : inputSets)
		if (auto actionBasePtr{ inputSet.second->GetAction(actionName) })
			if (auto actionPtr{ std::dynamic_pointer_cast<const InputAction<ValueType>>(actionBasePtr) })
				return actionPtr;
	return nullptr;
}
