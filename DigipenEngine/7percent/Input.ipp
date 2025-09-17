#pragma once
#include "Input.h"

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::BUTTON, bool> InputBinding<CompositeType>::GetValue() const
{
	return Get<bool>(hardwareValues_Button);
}

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_1D, float> InputBinding<CompositeType>::GetValue() const
{
	return Get<float>(hardwareValues_1D[0]) - Get<float>(hardwareValues_1D[1]);
}

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_2D, Vec2> InputBinding<CompositeType>::GetValue() const
{
	return Vec2{
		Get<float>(hardwareValues_2D[0]) - Get<float>(hardwareValues_2D[1]),
		Get<float>(hardwareValues_2D[2]) - Get<float>(hardwareValues_2D[3]),
	};
}

template<INPUT_COMPOSITE_TYPE CompositeType>
template<INPUT_COMPOSITE_TYPE NewCompositeType>
InputBinding<NewCompositeType> InputBinding<CompositeType>::ConvertToCompositeType() const
{
	InputBinding<NewCompositeType> newBinding{};
	// The bindings' members are all trivial and 8 bytes aligned.
	// We can directly copy the memory as is to the new destination.
	std::memcpy(&newBinding, this, std::min(sizeof(*this), sizeof(newBinding)));
	return newBinding;
}

template<INPUT_COMPOSITE_TYPE CompositeType>
template<typename ValueType>
ValueType InputBinding<CompositeType>::Get(const InputHardwareValueLink& hardwareValueLink)
{
	auto variantValue{ hardwareValueLink.ReadValue() };

	if constexpr (std::is_same_v<ValueType, bool>)
		return std::visit(util::VisitFunctions{
			[](bool value) -> bool { return value; },
			[](float value) -> bool { return value >= internal::DEADZONE; }
		}, variantValue);
	else if constexpr (std::is_same_v<ValueType, float>)
		return std::visit(util::VisitFunctions{
			[](bool value) -> float { return (value ? 1.0f : 0.0f); },
			[](float value) -> float { return value; }
		}, variantValue);
	else
	{
		// Unimplemented value type for the variant InputHardwareValue
		assert(false);
		return ValueType{};
	}
}

template<INPUT_COMPOSITE_TYPE CompositeType>
template<typename FuncType>
void InputBinding<CompositeType>::Editor_ForEachHardwareValueLink(FuncType func)
{
	if constexpr (CompositeType == INPUT_COMPOSITE_TYPE::BUTTON)
		func(hardwareValues_Button);
	else if constexpr (CompositeType == INPUT_COMPOSITE_TYPE::AXIS_1D)
		std::for_each(hardwareValues_1D.begin(), hardwareValues_1D.end(), func);
	else if constexpr (CompositeType == INPUT_COMPOSITE_TYPE::AXIS_2D)
		std::for_each(hardwareValues_2D.begin(), hardwareValues_2D.end(), func);
	else
		assert(false); // Unimplemented composite type
}

template<INPUT_COMPOSITE_TYPE CompositeType>
InputAction<CompositeType>::InputAction()
	: InputActionBase{ CompositeType }
{
}

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::BUTTON, bool> InputAction<CompositeType>::GetValue() const
{
	for (const auto& binding : bindings)
		if (binding.GetValue())
			return true;
	return false;
}

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_1D, float> InputAction<CompositeType>::GetValue() const
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

template <INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE T>
std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_2D, Vec2> InputAction<CompositeType>::GetValue() const
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

template<INPUT_COMPOSITE_TYPE CompositeType>
void InputAction<CompositeType>::AddBinding()
{
	bindings.emplace_back();
}

template<INPUT_COMPOSITE_TYPE CompositeType>
const InputBinding<CompositeType>* InputAction<CompositeType>::GetBinding(size_t index) const
{
	return (0 <= index && index < bindings.size()) ? &bindings[index] : nullptr;
}
template<INPUT_COMPOSITE_TYPE CompositeType>
InputBinding<CompositeType>* InputAction<CompositeType>::GetBinding(size_t index)
{
	return (0 <= index && index < bindings.size()) ? &bindings[index] : nullptr;
}

template<INPUT_COMPOSITE_TYPE CompositeType>
template <INPUT_COMPOSITE_TYPE NewCompositeType>
InputAction<NewCompositeType> InputAction<CompositeType>::ConvertToCompositeType() const
{
	InputAction<NewCompositeType> newAction{};
	newAction.INTERNAL_ConvertAndSetBindings(bindings);
	return newAction;
}

template<INPUT_COMPOSITE_TYPE CompositeType>
template<INPUT_COMPOSITE_TYPE OriginalCompositeType>
void InputAction<CompositeType>::INTERNAL_ConvertAndSetBindings(const std::vector<InputBinding<OriginalCompositeType>>& originalBindings)
{
	std::transform(originalBindings.begin(), originalBindings.end(), std::back_inserter(bindings), [](const auto& binding) -> auto {
		return binding.ConvertToCompositeType<CompositeType>();
	});
}

template<INPUT_COMPOSITE_TYPE CompositeType>
std::vector<InputBinding<CompositeType>>& InputAction<CompositeType>::Editor_GetBindings()
{
	return bindings;
}

template <INPUT_COMPOSITE_TYPE CompositeType>
SPtr<const InputAction<CompositeType>> Input::GetAction(const std::string& actionName) const
{
	for (const auto& inputSet : inputSets)
		if (auto actionBasePtr{ inputSet.second->GetAction(actionName) })
			if (auto actionPtr{ std::dynamic_pointer_cast<const InputAction<CompositeType>>(actionBasePtr) })
				return actionPtr;
	return nullptr;
}
