/******************************************************************************/
/*!
\file   CSharpTesting.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief
	This file contains a class to test C# method calling from C++ and vice versa.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using EngineScripting;

public class CSharpTesting : EID
{
    public float MyPublicFloatVar = 5.0f;
    public int MyPublicIntVar = 10;
    public string MyPublicStringVar = "Hello World!";
    /*****************************************************************//*!
    \brief
        Initialisation Method 
    \return
        None
    *//******************************************************************/
    CSharpTesting()
    {
        Console.WriteLine(GetType().Name + " instance has been created!");
        InternalCalls.CppNativeLog("Testing Testing", 5003);

        EngineScripting.Vector3 pos = new Vector3(10.1f, 5.3f, 3.6f);
        EngineScripting.Vector3 res = Log(pos);
        Console.WriteLine(res.x);

        Console.WriteLine("Dot of pos = " + InternalCalls.CppNativeLog_VectorDot(ref pos).ToString());

        Console.WriteLine(MyPublicFloatVar.ToString());
        Console.WriteLine(MyPublicIntVar.ToString());
        Console.WriteLine(MyPublicStringVar);

    }

    /*****************************************************************//*!
    \brief
        Function called after class instance is created and attached to
        entity
    \return
        None
    *//******************************************************************/
    public void OnCreate()
    {
        Console.WriteLine(GetType().Name + ": OnCreate Invoked");
        Console.WriteLine("Entity handle: " + e_ID.ToString());
    }

    /*****************************************************************//*!
    \brief
        Function called when the entity the class instance is attached to is
        updated in the ecs system
    \param[in] ts
        delta time
    \return
        None
    *//******************************************************************/
    public void OnUpdate(float ts)
    {
        //Console.WriteLine(GetType().Name + ": OnUpdate Invoked");
        //Console.WriteLine("Updated: " + ts.ToString());
        if (Input.GetKeyReleased(KeyCode.P))
        {
            Debug.Log("P has been pressed");
            PrintPublicVars();
        }
    }

    #region C# methods to call from C++
    /*****************************************************************//*!
    \brief
        C# Method call test for printing c# variables from c++
    \return
        None
    *//******************************************************************/
    public void PrintPublicVars()
    {
        Console.WriteLine("MyIntVar: " + MyPublicIntVar.ToString());
        Console.WriteLine("MyFloatVar: " + MyPublicFloatVar.ToString());
        Console.WriteLine("MyStringVar: " + MyPublicStringVar);
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that requires no params
    \return
        None
    *//******************************************************************/
    public void NoParamFunc()
    {
        string functionName = MethodBase.GetCurrentMethod().Name;
        Console.WriteLine($"{functionName}: This function call has no params!");
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that requires one param
    \param[in] value
        A regular int value to test
    \return
        None
    *//******************************************************************/
    public void OneValueParamFunc(int value)
    {
        string functionName = MethodBase.GetCurrentMethod().Name;
        Console.WriteLine($"{functionName}: This function call has one INT param whose value = {value}");
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that requires two param
    \param[in] value1
        A regular int value to test
    \param[in] value2
        A regular int value to test
    \return
        None
    *//******************************************************************/
    public void TwoValueParamFunc(int value1, int value2)
    {
        string functionName = MethodBase.GetCurrentMethod().Name;
        Console.WriteLine($"{functionName}: This function call has two INT param where value1 = {value1} and value2 = {value2}");
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that requires a string
    \param[in] message
        C++ string to transfer to C# via mono
    \return
        None
    *//******************************************************************/
    public void PrintStringMessageFunc(string message)
    {
        Console.WriteLine($"{message}");
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that requires no params
    \return
        None
    *//******************************************************************/
    public void PrintFloatVar()
    {
        Console.WriteLine("MyPublicFloatVar = {0:F}", MyPublicFloatVar);
    }

    /*****************************************************************//*!
    \brief
        C# Method to call from C++ that will update C# values
    \param[in] value
        Amount to add to C# variable
    \return
        None
    *//******************************************************************/
    private void IncrementFloatVar(float value)
    {
        MyPublicFloatVar += value;
    }

    /*****************************************************************//*!
    \brief
        C# Method to test internal calls with a reference and an output
        pointer
    \param[in] p
        reference to Vector3 in C#
    \param[in,out] result
        Vector3 pointer to recieve C++ vector3 manipulation.
    \return
        C# Vector3 with manipulated values from C++ code
    *//******************************************************************/
    private Vector3 Log(Vector3 p)
    {
        InternalCalls.CppNativeLog_Vector(ref p, out Vector3 result);
        return result;
        //return new Vector3(0, 0, 0);
    }
    #endregion


}