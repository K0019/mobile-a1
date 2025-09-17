#pragma once
#include "GUIAsECS.h"

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
		
		void DrawInspector_Action(SPtr<internal::InputActionBase>& action, const std::string* actionName);
		template <internal::INPUT_COMPOSITE_TYPE CompositeType>
		void DrawInspector_Action(SPtr<internal::InputAction<CompositeType>> action, const std::string* actionName);

		template <internal::INPUT_COMPOSITE_TYPE CompositeType>
		void DrawInspector_Binding(internal::InputBinding<CompositeType>& binding);
		void DrawInspector_HardwareValueLink(internal::InputHardwareValueLink& hardwareValueLink, int index);

	private:
		WPtr<internal::InputSet> selectedInputSetPtr;
		WPtr<internal::InputActionBase> selectedActionPtr;
		size_t selectedBindingIndex{};

	private:
		static const std::array<const char*, +internal::INPUT_COMPOSITE_TYPE::NUM_TYPES> compositeNames;
		static const std::array<const char*, (+internal::INPUT_COMPOSITE_TYPE::NUM_TYPES - 1) * 2> hardwareValueLinkNames;
		static const std::array<const char*, +internal::INPUT_DEVICE_TYPE::NUM_DEVICES> hardwareDeviceNames;
		static const std::unordered_map<KEY, const char*> keyIdentifierNames;

	};

	template<internal::INPUT_COMPOSITE_TYPE CompositeType>
	void InputConfig::DrawInspector_Action(SPtr<internal::InputAction<CompositeType>> action, const std::string* actionName)
	{
		internal::INPUT_COMPOSITE_TYPE selectedCompositeType{};
		if (gui::Combo compositeTypeDropdown{ "Composite Type", compositeNames, compositeNames[+action->GetCompositeType()], reinterpret_cast<int*>(&selectedCompositeType) })
		{
			auto inputSet{ selectedInputSetPtr.lock() };
			switch (selectedCompositeType)
			{
#define X(name, str) \
			case internal::INPUT_COMPOSITE_TYPE::name: \
			{ \
				auto newAction{ std::make_shared<internal::InputAction<internal::INPUT_COMPOSITE_TYPE::name>>(action->ConvertToCompositeType<internal::INPUT_COMPOSITE_TYPE::name>()) }; \
				selectedActionPtr = newAction; \
				inputSet->SetAction(*actionName, std::move(newAction)); \
				return; \
			}
			ENUM_INPUT_COMPOSITE_TYPE
#undef X
			}
		}
	}

	template<internal::INPUT_COMPOSITE_TYPE CompositeType>
	void InputConfig::DrawInspector_Binding(internal::InputBinding<CompositeType>& binding)
	{
		int index{};
		binding.Editor_ForEachHardwareValueLink([this, &index](internal::InputHardwareValueLink& hardwareValueLink) -> void {
			gui::Separator();
			DrawInspector_HardwareValueLink(hardwareValueLink, index++);
		});
	}

}


