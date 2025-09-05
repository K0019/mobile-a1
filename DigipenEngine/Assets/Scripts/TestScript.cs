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
        value = 10;
    }

    // This method is called once per frame
    void Update(float dt)
    {
        Transform transform = this.transform;
        transform.localPosition = transform.localPosition + new vec3(1, 0, 0);

        value += 1;
        Debug.Log(value.ToString());

        //Text textComp = gameObject.GetComponent<Text>();

        //Debug.Log(textComp.text);
        //textComp.text = "Hello";
    }
}
