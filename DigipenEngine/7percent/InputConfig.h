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
		void DrawActionsColumn(SPtr<internal::InputSet>& inputSet, SPtr<internal::InputActionBase>& action);
		void DrawInspector(SPtr<internal::InputActionBase>& action);
		
		template <internal::INPUT_COMPOSITE_TYPE CompositeType>
		void DrawInspector_Action(SPtr<internal::InputAction<CompositeType>>& action);

	private:
		WPtr<internal::InputSet> selectedInputSetPtr;
		WPtr<internal::InputActionBase> selectedActionPtr;

	};

	template<internal::INPUT_COMPOSITE_TYPE CompositeType>
	void InputConfig::DrawInspector_Action(SPtr<internal::InputAction<CompositeType>>& action)
	{
	}

}


