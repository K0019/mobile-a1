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
		gui::SameLine();
		if (gui::Button newActionButton{ "New Action" })
			CreateNewAction();

		if (gui::Table table{ "", 3, true })
		{
			table.AddColumnHeader("Input Set");
			table.AddColumnHeader("Actions");
			table.AddColumnHeader("Settings");
			table.SubmitColumnHeaders();

			VecOfInputSets inputSets{ ST<Input>::Get()->Editor_GetInputSets() };
			auto selectedInputSet{ selectedInputSetPtr.lock() };
			auto selectedAction{ selectedActionPtr.lock() };

			DrawInputSetsColumn(inputSets, selectedInputSet);
			table.NextColumn();
			DrawActionsColumn(selectedInputSet, selectedAction);
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
		selectedInputSetPtr = ST<Input>::Get()->Editor_GetCurrentInputSet();
	}

	void InputConfig::CreateNewAction()
	{
		auto currentInputSet{ selectedInputSetPtr.lock() };
		if (!currentInputSet)
			return;

		std::string newActionName{ "New Action" };
		if (!currentInputSet->CreateNewAction(newActionName))
		{
			int i{};
			while (!currentInputSet->CreateNewAction(newActionName + std::to_string(i)))
				++i;
			newActionName += std::to_string(i);
		}
	}

	void InputConfig::DrawInputSetsColumn(VecOfInputSets& inputSets, SPtr<internal::InputSet>& selectedInputSet)
	{
		for (auto& [name, inputSet] : inputSets)
		{
			gui::SetID id{ name.get().c_str() };
			if (gui::Selectable(name.get().c_str(), inputSet.get() == selectedInputSet))
				selectedInputSetPtr = selectedInputSet = inputSet.get();
		}
	}

	void InputConfig::DrawActionsColumn(SPtr<internal::InputSet>& selectedInputSet, SPtr<internal::InputActionBase>& selectedAction)
	{
		if (!selectedInputSet)
			return;

		for (auto& [name, action] : selectedInputSet->Editor_GetActions())
		{
			gui::SetID id{ name.get().c_str() };
			if (gui::Selectable(name.get().c_str(), action.get() == selectedAction))
				selectedActionPtr = selectedAction = action.get();
		}
	}

}
