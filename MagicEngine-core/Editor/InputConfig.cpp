#include "pch.h"
#include "Editor/InputConfig.h"

namespace editor {

#define X(name, str, type) str,
	const std::array<const char*, +INPUT_COMPOSITE_TYPE::NUM_TYPES> InputConfig::compositeNames{
		ENUM_INPUT_COMPOSITE_TYPE
	};
#undef X
	const std::array<const char*, (+INPUT_COMPOSITE_TYPE::NUM_TYPES - 1) * 2> InputConfig::hardwareValueLinkNames{
		"X Positive",
		"X Negative",
		"Y Positive",
		"Y Negative",
	};
#define X(name, str) str,
	const std::array<const char*, +INPUT_DEVICE_TYPE::NUM_DEVICES> InputConfig::hardwareDeviceNames{
		ENUM_INPUT_DEVICE_TYPE
	};
#undef X
#define X(name, glfw, str) std::make_pair(KEY::name, str),
	const std::unordered_map<KEY, const char*> InputConfig::keyIdentifierNames{
		ENUM_KEY
	};
#undef X
#define X(name, glfw, str) KEY::name,
	const std::array keyEnums{
		ENUM_KEY
	};
#undef X

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
		gui::SameLine();
		if (gui::Button newActionButton{ "New Binding" })
			CreateNewBinding();

		if (gui::Table table{ "", 4, true })
		{
			table.AddColumnHeader("Input Sets");
			table.AddColumnHeader("Actions");
			table.AddColumnHeader("Bindings");
			table.AddColumnHeader("Settings");
			table.SubmitColumnHeaders();

			DrawInputSetsColumn();
			table.NextColumn();
			const std::string* actionName{ DrawActionsColumn() };
			table.NextColumn();
			DrawBindingsColumn();
			table.NextColumn();
			DrawInspector(actionName);
		}
	}

	void InputConfig::CreateNewInputSet()
	{
		// Create an input set, adding a number to the back if it fails
		std::string newInputSetName{ "New Input Set" };
		if (!ST<MagicInput>::Get()->CreateInputSet(newInputSetName))
		{
			int i{};
			while (!ST<MagicInput>::Get()->CreateInputSet(newInputSetName + std::to_string(i)))
				++i;
			newInputSetName += std::to_string(i);
		}

		// Switch to it
		ST<MagicInput>::Get()->SwitchInputSet(newInputSetName);
		selectedInputSetPtr = ST<MagicInput>::Get()->Editor_GetCurrentInputSet();
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

	void InputConfig::CreateNewBinding()
	{
		auto currentAction{ selectedActionPtr.lock() };
		if (!currentAction)
			return;

		currentAction->AddBinding();
	}

	void InputConfig::DrawInputSetsColumn()
	{
		auto selectedInputSet{ selectedInputSetPtr.lock() };

		for (auto& [name, inputSet] : ST<MagicInput>::Get()->Editor_GetInputSets())
		{
			gui::SetID id{ name.get().c_str() };
			gui::Renamable::Wrap(name.get().c_str(),
				[this, &name, &inputSet, &selectedInputSet]() -> void { // Normal
					if (gui::Selectable(name.get().c_str(), inputSet.get() == selectedInputSet))
						selectedInputSetPtr = inputSet.get();
				}, [&name](const char* newName) -> void { // Rename
					// Should be safe to make changes because the vector we're iterating is an rvalue
					ST<MagicInput>::Get()->RenameInputSet(name, newName);
				}
			);
		}
	}

	const std::string* InputConfig::DrawActionsColumn()
	{
		auto selectedInputSet{ selectedInputSetPtr.lock() };
		if (!selectedInputSet)
			return nullptr;

		auto selectedAction{ selectedActionPtr.lock() };
		const std::string* selectedActionNamePtr{};

		for (auto& [name, action] : selectedInputSet->Editor_GetActions())
		{
			gui::SetID id{ name.get().c_str() };

			gui::Renamable::Wrap(name.get().c_str(),
				[this, &name, &action, &selectedAction, &selectedActionNamePtr]() -> void { // Normal
					bool isSelected{ action.get() == selectedAction };
					if (gui::Selectable(name.get().c_str(), isSelected))
					{
						// Select the action and deselect previous binding
						selectedActionPtr = selectedAction = action.get();
						selectedBindingIndex = std::numeric_limits<size_t>::max();
						isSelected = true;
					}

					if (isSelected)
						selectedActionNamePtr = &name.get();
				}, [&selectedInputSet, &name](const char* newName) -> void { // Renamed
					// Should be safe to make changes because the vector we're iterating is an rvalue
					selectedInputSet->RenameAction(name, newName);
				}
			);
		}

		return selectedActionNamePtr;
	}

	void InputConfig::DrawBindingsColumn()
	{
		auto action{ selectedActionPtr.lock() };
		if (!action)
			return;

		switch (action->GetCompositeType())
		{
#define X(name, str, type) \
		case INPUT_COMPOSITE_TYPE::name: \
			{ \
			auto& bindings{ std::static_pointer_cast<InputAction<type>>(action)->Editor_GetBindings() }; \
			for (size_t index{}; index < bindings.size(); ++index) \
				if (gui::Selectable(std::to_string(index).c_str(), index == selectedBindingIndex)) \
					selectedBindingIndex = index; \
			} \
			break;
			ENUM_INPUT_COMPOSITE_TYPE
#undef X
		default:
			break;
		}
	}

	void InputConfig::DrawInspector(const std::string* actionName)
	{
		if (auto action{ selectedActionPtr.lock() })
		{
			// Draw the binding inspector if action exists and a binding exists at our selected index
			switch (action->GetCompositeType())
			{
#define X(name, str, type) \
			case INPUT_COMPOSITE_TYPE::name: \
				if (auto binding{ std::static_pointer_cast<InputAction<type>>(action)->GetBinding(selectedBindingIndex) }) \
				{ \
					DrawInspector_Binding(*binding); \
					return; \
				} \
				break;
			ENUM_INPUT_COMPOSITE_TYPE
#undef X
			default:
				break;
			}

			// Otherwise draw the action inspector
			DrawInspector_Action(action, actionName);
		}
	}

	void InputConfig::DrawInspector_Action(SPtr<InputActionBase>& action, const std::string* actionName)
	{
		if (!action)
			return;

		switch (action->GetCompositeType())
		{
#define X(name, str, type) \
		case INPUT_COMPOSITE_TYPE::name: \
			DrawInspector_Action(std::static_pointer_cast<InputAction<type>>(action), actionName); \
			break;
		ENUM_INPUT_COMPOSITE_TYPE
#undef X
		default:
			break;
		}
	}

	void InputConfig::DrawInspector_HardwareValueLink(InputHardwareValueLink& hardwareValueLink, int index)
	{
		int deviceType{ +hardwareValueLink.GetDeviceType() };
		// TODO: Give a nicer hardware value link name to the user (button should not be called X Positive)
		if (gui::Combo deviceCombo{ hardwareValueLinkNames[index], hardwareDeviceNames, &deviceType})
			hardwareValueLink.SetDeviceType(static_cast<INPUT_DEVICE_TYPE>(deviceType));

		// TODO: Different keys depending on device type
		auto keyIdentifierNameIter{ keyIdentifierNames.find(static_cast<KEY>(hardwareValueLink.GetKeyIdentifier())) };
		const char* selectedKeyIdentifierName{ (keyIdentifierNameIter == keyIdentifierNames.end() ? nullptr : keyIdentifierNameIter->second) };
		if (gui::Combo keyCombo{ "Key", selectedKeyIdentifierName })
			for (KEY key : keyEnums)
				if (keyCombo.Selectable(keyIdentifierNames.at(key), +key == hardwareValueLink.GetKeyIdentifier()))
					hardwareValueLink.SetKeyIdentifier(+key);
	}

}
