#pragma once
#include "GUIAsECS.h"

namespace editor {

	class InputConfig : public WindowBase<InputConfig, false>
	{
	public:
		InputConfig();

		virtual void DrawWindow() override;

	private:
		using VecOfInputSets = decltype(std::declval<Input>().Editor_GetInputSets());

		void CreateNewInputSet();
		void CreateNewAction();

		void DrawInputSetsColumn(VecOfInputSets& inputSets, SPtr<internal::InputSet>& selectedInputSet);
		const std::string* DrawActionsColumn(SPtr<internal::InputSet>& inputSet, SPtr<internal::InputActionBase>& action);
		void DrawInspector(SPtr<internal::InputActionBase>& action, const std::string* actionName);
		
		template <internal::INPUT_COMPOSITE_TYPE CompositeType>
		void DrawInspector_Action(SPtr<internal::InputAction<CompositeType>> action, const std::string* actionName);

	private:
		WPtr<internal::InputSet> selectedInputSetPtr;
		WPtr<internal::InputActionBase> selectedActionPtr;

	private:
		static const std::array<const char*, +internal::INPUT_COMPOSITE_TYPE::NUM_TYPES> compositeNames;

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

}


