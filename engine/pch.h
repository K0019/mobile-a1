/******************************************************************************/
/*!
\file   pch.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   02/05/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is a pre-compiled header file to speed up compilation.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

#define NOMINMAX
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <deque>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <iterator>
#include <ranges>
#include <algorithm>
#include <functional>
#include <bitset>
#include <any>
#include <optional>
#include <random>
#include <utility>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <typeinfo>
#include <exception>
#include <stdexcept>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <type_traits>
#include <limits>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <math.h>

#include "Singleton.h"
#include "Logging.h"
#include "MagicMath.h"
#include "Transform.h"
#include "ECS.h"
#include "IEditorComponent.h"
#include "IHiddenComponent.h"
#include "Messaging.h"
#include "Scheduler.h"
#include "EntityEvents.h"
#include "GameTime.h"
#include "TypeID.h"
#include "Utilities.h"
#include "MacroTemplates.h"
#include "ScriptingUtil.h"
#include "RandUID.h"

#include "StateMachine.h"
