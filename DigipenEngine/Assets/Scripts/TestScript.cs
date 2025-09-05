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
    protected int value = 5;

	TestScript()
	{
	}

    // This method is called once when the script is first initialized
    public void OnCreate()
    {
       
    }

    // This method is called once per frame
    void OnUpdate(float dt)
    {
        Transform transform = this.transform;
        transform.localPosition = transform.localPosition + new vec3(1, 0, 0);

        //Text textComp = gameObject.GetComponent<Text>();

        //Debug.Log(textComp.text);
        //textComp.text = "Hello";
    }
}
