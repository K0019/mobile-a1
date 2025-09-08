using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
	internal class Test : EID
	{
		Test()
		{
		}

		// This method is called once when the script is first initialized
		public void OnCreate()
		{

			InternalCalls.GetPhysicsComp(e_ID, out body);
		}

		float speed;
		float accel;
		float airaccel;
		float jumpPower;
		Physics body;
		public bool isGrounded { get; private set; }
		public float horizontalMovement;

		public bool isFiring;
		public bool releaseWhenCycling;
		//Semi Auto Fix will automatically release the trigger when weapon is cycling.
		//Good for allowing enemies to "hold to fire" semi-auto weapon

		// This method is called once per frame
		void OnUpdate(float dt)
		{
			if (Input.GetCurrKey(KeyCode.A))
				horizontalMovement = -1.0f;
			else if (Input.GetCurrKey(KeyCode.D))
				horizontalMovement = 1.0f;
			else
				horizontalMovement = 0.0f;

			//Needs isGrounded check
			if (Input.GetKeyPressed(KeyCode.SPACE))
			{
				body.velocity += new Vector2(0.0f, jumpPower);
			}
			if (horizontalMovement < -1.0f)
				horizontalMovement = -1.0f;
			else if (horizontalMovement > 1.0f)
				horizontalMovement = 1.0f;

			if ((body.velocity.x < speed && horizontalMovement > 0) || (body.velocity.x > -speed && horizontalMovement < 0))
			{
				//body.AddForce(new Vector2(horizontalMovement * (isGrounded ? accel : airaccel), 0), ForceMode2D.Force);
				body.velocity += new Vector2(horizontalMovement * (isGrounded ? accel : airaccel), 0);
			}

			//isHealing = health < targetHealAmount;
			//if (health < targetHealAmount)
			//	health = Mathf.MoveTowards(health, targetHealAmount, healSpeed * Time.fixedDeltaTime);
			//healthBar.SetCurr((int)health);

		}
	}
}
