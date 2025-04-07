/******************************************************************************/
/*!
\file   Messaging.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/23/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
    This is the template definition source file for the messaging system which allows
    code to communicate with other systems without knowing about their existence.

    This was previously implemented by Chua Wen Shing Bryan but has been rewritten
    with the main purpose of allowing a more natural interface with the system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Messaging.h"

namespace Messaging {

    namespace Internal {

        template<typename ...Args>
        Subscriber::Subscriber(void(*callbackFunc)(Args...))
            : identifier{ reinterpret_cast<uint64_t>(callbackFunc) }
            , func{ GenerateIntermediateFunc(std::function<void(Args...)>{ callbackFunc }) }
        {
        }

        template<typename ...Args>
        Subscriber::Subscriber(uint64_t identifier, const std::function<void(Args...)>& callbackFunc)
            : identifier{ identifier }
            , func{ GenerateIntermediateFunc(callbackFunc) }
        {
        }

        template<typename ...Args>
        std::function<Subscriber::InvokeFuncSig> Subscriber::GenerateIntermediateFunc(const std::function<void(Args...)>& callbackFunc)
        {
            return [callbackFunc, indexSequence = std::make_index_sequence<sizeof...(Args)>{}](std::initializer_list<std::any> params) -> INVOKE_RESULT {
                // Check for incorrect number of parameters.
                if (params.size() != sizeof...(Args))
                    return INVOKE_RESULT::FAILURE_PARAM_COUNT_MISMATCH;
                try
                {
                    // Make an index sequence so we can index the initializer list as we unwrap it into the function parameters,
                    // then cast each each argument to the appropriate type and call the function alongside the additional parameters as needed by derived classes.
                    [&callbackFunc, params]<size_t ...Index>(std::index_sequence<Index...>) -> void {
                        callbackFunc(std::any_cast<std::reference_wrapper<std::add_const_t<std::remove_reference_t<Args>>>>(params.begin()[Index])...);
                    }(indexSequence);
                    return INVOKE_RESULT::SUCCESS;
                }
                // std::any_cast will throw this if a cast failed, meaning the params we received are incompatible with the function.
                catch (const std::bad_any_cast&)
                {
                    return INVOKE_RESULT::FAILURE_PARAM_TYPE_INCOMPATIBILITY;
                }
            };
        }

        template<typename ...Args>
        void EventsList::BroadcastAll(const std::string& eventName, const Args&... params)
        {
            BroadcastAll(eventName, { std::make_any<const decltype(std::cref(params))>(std::cref(params))... });
        }

    }

    template<typename ...Args>
    void Subscribe(const std::string& eventName, void(*func)(Args...))
    {
        ST<Internal::EventsList>::Get()->AddSubscriber(eventName, Internal::Subscriber{ func });
    }

    template<typename ...Args>
    void Unsubscribe(const std::string& eventName, void(*func)(Args...))
    {
        ST<Internal::EventsList>::Get()->RemoveSubscriber(eventName, reinterpret_cast<uint64_t>(func));
    }

    template<typename ...Args>
    void BroadcastAll(const std::string& eventName, const Args&... params)
    {
        ST<Internal::EventsList>::Get()->BroadcastAll(eventName, params...);
    }

}
