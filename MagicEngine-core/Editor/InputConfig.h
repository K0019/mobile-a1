#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Engine/Input.h"

namespace editor {

	class InputConfig : public WindowBase<InputConfig, false>
	{
	public:
		InputConfig();

		virtual void DrawWindow() override;

	private:
		void CreateNewInputSet();
		void CreateNewAction();
		void CreateNewBinding();

		void DrawInputSetsColumn();
		const std::string* DrawActionsColumn();
		void DrawBindingsColumn();
		void DrawInspector(const std::string* actionName);
		
		void DrawInspector_Action(SPtr<InputActionBase>& action, const std::string* actionName);
		template <InputSupportedValueTypes ValueType>
		void DrawInspector_Action(SPtr<InputAction<ValueType>> action, const std::string* actionName);

		template <InputSupportedValueTypes ValueType>
		void DrawInspector_Binding(InputBinding<ValueType>& binding);
		void DrawInspector_HardwareValueLink(InputHardwareValueLink& hardwareValueLink, int index);

	private:
		WPtr<InputSet> selectedInputSetPtr;
		WPtr<InputActionBase> selectedActionPtr;
		size_t selectedBindingIndex{};

	private:
		static const std::array<const char*, +INPUT_COMPOSITE_TYPE::NUM_TYPES> compositeNames;
		static const std::array<const char*, (+INPUT_COMPOSITE_TYPE::NUM_TYPES - 1) * 2> hardwareValueLinkNames;
		static const std::array<const char*, +INPUT_DEVICE_TYPE::NUM_DEVICES> hardwareDeviceNames;
		static const std::unordered_map<KEY, const char*> keyIdentifierNames;

	};

	template <InputSupportedValueTypes ValueType>
	void InputConfig::DrawInspector_Action(SPtr<InputAction<ValueType>> action, const std::string* actionName)
	{
		INPUT_COMPOSITE_TYPE selectedCompositeType{};
		if (gui::Combo compositeTypeDropdown{ "Composite Type", compositeNames, compositeNames[+action->GetCompositeType()], reinterpret_cast<int*>(&selectedCompositeType) })
		{
			auto inputSet{ selectedInputSetPtr.lock() };
			switch (selectedCompositeType)
			{
#define X(name, str, type) \
			case INPUT_COMPOSITE_TYPE::name: \
			{ \
				auto newAction{ std::make_shared<InputAction<type>>(action->template ConvertToValueType<type>()) }; \
				selectedActionPtr = newAction; \
				inputSet->SetAction(*actionName, std::move(newAction)); \
				return; \
			}
			ENUM_INPUT_COMPOSITE_TYPE
#undef X
			default:
				break;
			}
		}
	}

	template <InputSupportedValueTypes ValueType>
	void InputConfig::DrawInspector_Binding(InputBinding<ValueType>& binding)
	{
		int index{};
		binding.Editor_ForEachHardwareValueLink([this, &index](InputHardwareValueLink& hardwareValueLink) -> void {
			gui::Separator();
			gui::SetID id{ index };
			DrawInspector_HardwareValueLink(hardwareValueLink, index++);
		});
	}

}


