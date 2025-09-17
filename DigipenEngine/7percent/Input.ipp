#pragma once
#include "Input.h"

namespace internal {

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
				[](float value) -> bool { return value >= DEADZONE; }
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
	InputAction<CompositeType>::InputAction()
		: InputActionBase{ CompositeType }
	{
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
}
