/******************************************************************************/
/*!
\file   Singleton.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface and implementation file for template class that
  creates singleton objects of arbituary types.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <memory>
#include <mutex>

/*****************************************************************//*!
\class ST
\brief
	The singleton class, that stores returns the same object instance no matter where it is accessed from.
\tparam T
	The type of the class.
*//******************************************************************/
template <typename T>
class ST
{
public:
	/*****************************************************************//*!
	\brief
		Gets the singleton class instance.
	\return
		The singleton class instance.
	*//******************************************************************/
    static T* Get()
    {
		// TODO: This causes the singleton to be unable to be initialized again once destroyed.
        std::call_once(initFlag, []() {
            instance.reset(new T());
        });
        return instance.get();
    }

	/*****************************************************************//*!
	\brief
		Frees memory occupied by the singleton class object.
	*//******************************************************************/
	static void Destroy()
	{
		instance.reset();
	}

	/*****************************************************************//*!
	\brief
		Check if the singleton instance exists.
	\return
		True if the singleton class object is initialized. False otherwise.
	*//******************************************************************/
	static bool IsInitialized()
	{
		return instance.get();
	}

protected:
	// Disable creating instances of this class
	ST<T>() = delete;
	virtual ~ST<T>() = delete;

	//! The singleton class object instance.
	static std::unique_ptr<T> instance;
	//! A flag to track whether the singleton has been initialized, to ensure thread safety.
	static std::once_flag initFlag;
};

template <typename T>
std::unique_ptr<T> ST<T>::instance = std::unique_ptr<T>();

template <typename T>
std::once_flag ST<T>::initFlag;