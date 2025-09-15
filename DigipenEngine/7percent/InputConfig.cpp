#include "pch.h"
#include "InputConfig.h"

namespace editor {

	InputConfig::InputConfig()
		: WindowBase{ "Input Configuration", gui::Vec2{ 600, 600 } }
	{
	}

	void InputConfig::DrawWindow()
	{
		if (gui::Button newInputSetButton{ "New Input Set" })
			CreateNewInputSet();

		if (gui::Table table{ "", 3, true })
		{
			table.AddColumnHeader("Input Set");
			table.AddColumnHeader("Actions");
			table.AddColumnHeader("Settings");
			table.SubmitColumnHeaders();

			VecOfInputSets inputSets{ ST<Input>::Get()->Editor_GetInputSets() };
			SPtr<internal::InputSet> selectedInputSet{ selectedInputSetPtr.lock() };

			DrawInputSetsColumn(inputSets, selectedInputSet);

		}
	}

	void InputConfig::CreateNewInputSet()
	{
		// Create an input set, adding a number to the back if it fails
		std::string newInputSetName{ "New Input Set" };
		if (!ST<Input>::Get()->CreateInputSet(newInputSetName))
		{
			int i{};
			while (!ST<Input>::Get()->CreateInputSet(newInputSetName + std::to_string(i)))
				++i;
			newInputSetName += std::to_string(i);
		}

		// Switch to it
		ST<Input>::Get()->SwitchInputSet(newInputSetName);
	}

	void InputConfig::DrawInputSetsColumn(VecOfInputSets& inputSets, SPtr<internal::InputSet>& selectedInputSet)
	{
		for (auto& [name, inputSet] : inputSets)
		{
			gui::SetID id{ name.get().c_str() };
			if (gui::Selectable(name.get().c_str(), inputSet.get() == selectedInputSet))
				selectedInputSetPtr = inputSet.get();
		}
	}

}
