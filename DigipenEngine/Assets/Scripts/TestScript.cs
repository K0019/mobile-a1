using GlmSharp;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using EngineScripting;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public class TestScript : ComponentBase
{
    public int value = 5;

	TestScript()
	{
	}

    // This method is called once on the frame when the script is initialized
    void Start()
    {
        Debug.Log("TestScript started with value " + value.ToString());
    }

    // This method is called once per frame
    void Update(float dt)
    {
        Transform transform = this.transform;
        transform.position = transform.position + dt;

        value += 1;
    }
}
