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

		void DrawInputSetsColumn(VecOfInputSets& inputSets, SPtr<internal::InputSet>& selectedInputSet);

	private:
		WPtr<internal::InputSet> selectedInputSetPtr;

	};

}


