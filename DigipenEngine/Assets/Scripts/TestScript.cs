using GlmSharp;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using EngineScripting;

public class TestScript : ComponentBase
{
    public int value = 5;

	TestScript()
	{
	}

    // This method is called once on the frame when the script is initialized
    void Start()
    {

    }

    // This method is called once per frame
    void Update(float dt)
    {
        Debug.Log(Time.DeltaTime.ToString());
    }
}
