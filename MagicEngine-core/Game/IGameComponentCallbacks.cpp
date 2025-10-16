/******************************************************************************/
/*!
\file   IGameComponentCallbacks.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a component callbacks interface that also provides a
  OnStart() interface that is only called when the simulation is running.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "IGameComponentCallbacks.h"

GameComponentCallbacksHandler::GameComponentCallbacksHandler()
{
	Messaging::Subscribe("OnEngineSimulationStart", OnEngineSimulationStart);
}

GameComponentCallbacksHandler::~GameComponentCallbacksHandler()
{
	Messaging::Unsubscribe("OnEngineSimulationStart", OnEngineSimulationStart);
}

void GameComponentCallbacksHandler::OnEngineSimulationStart()
{
	for (auto massCallback : ST<GameComponentCallbacksHandler>::Get()->retroactiveCallbacks)
		massCallback();
}
