#define HIDEALL
using EngineScripting;
using GlmSharp;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;


[StructLayout(LayoutKind.Sequential)]
public class CameraController : ComponentBase
{
#if HIDEALL
	public GameObject focusedObject;
	public GameObject player;

	//private static readonly string[] cameraBlockingLayers = { "Default", "Environment" };
	//private static int cameraBlockingLayerMask;
	//public struct CameraData
	//{
	//	public float minDistance, maxDistance;
	//	public float minPitch, maxPitch;
	//	public vec3 offsetPosition;
	//
	//	public CameraData(float _minDistance, float _maxDistance, float _minPitch, float _maxPitch, vec3 _offsetPosition)
	//	{
	//		minDistance = _minDistance;
	//		maxDistance = _maxDistance;
	//		minPitch = _minPitch;
	//		maxPitch = _maxPitch;
	//		offsetPosition = _offsetPosition;
	//	}
	//}

	//private static readonly CameraData defaultCameraData = new CameraData(1.25f, 3f, -15f, 85f, new vec3(0.5f, 1.2f, 0));
	//public CameraData currentCameraData = defaultCameraData;

	public float cameraPitch;
	public float cameraYaw;
	public float cameraAutoZoomSpeed = 5f;

	public float targetCameraDistance;
	public float currentCameraDistance;
	public vec3 offsetPosition = new vec3(0.5f, 1.2f, 0);

	// This method is called once per frame
	void Update(float dt)
	{
		focusedObject = GameObject.Find("Player");
		if (focusedObject == gameObject) return;
		if (focusedObject == null)
		{
			//Debug.Log("No focused object!");
			return;
		}
		else
		{
			//Debug.Log("Attached to Ambulance!");
		}

		// Normal camera movement (including lerp if returning)
		HandleNormalCamera(dt);

	}

	void HandleNormalCamera(float dt)
	{
		Transform transform = this.transform;

		//if (cameraLock.triggered) CameraLock();

		// Read input
		vec2 cameraMovement = new vec2(dt*15, 0.0f);// * cameraSensitivityMultiplier;
		//vec2 cameraMovement = moveAction.ReadValue<vec2>() * cameraSensitivityMultiplier;
		//
		//
		cameraYaw -= cameraMovement.x;
		cameraPitch -= cameraMovement.y;
		
		cameraYaw = Mathf.Repeat(cameraYaw, 360f);
		cameraPitch = Mathf.Clamp(cameraPitch, -5.0f, 85.0f);
		
		float verticalFactor = Mathf.Sin(cameraPitch * Mathf.Deg2Rad);
		float horizontalFactor = Mathf.Cos(cameraPitch * Mathf.Deg2Rad);
		 vec3 calculatedCameraDirection = new vec3(
		 	horizontalFactor * Mathf.Cos(cameraYaw * Mathf.Deg2Rad),
		 	verticalFactor,
		 	horizontalFactor * Mathf.Sin(cameraYaw * Mathf.Deg2Rad)
		 );

		transform.forward = calculatedCameraDirection;

		//transform.rotation = new vec3(cameraYaw,cameraPitch,0.0f);


		vec3 desiredPosition = focusedObject.transform.position; ;
		 	//+ transform.right * offsetPosition.x
		 	//+ transform.up * offsetPosition.y
		 	//+ transform.forward * offsetPosition.z;
		 
		 //transform.position = desiredPosition;

		 // Handle zoom and collisions
		 UpdateCameraZoom(desiredPosition, calculatedCameraDirection, dt);

	}
	void UpdateCameraZoom(vec3 targetPos, vec3 direction, float dt)
	{
		Transform transform = this.transform;

		targetCameraDistance = Mathf.Clamp(targetCameraDistance,2.0f, 5.0f);
		currentCameraDistance = Mathf.MoveTowards(currentCameraDistance, targetCameraDistance, cameraAutoZoomSpeed * dt);
		
		//RaycastHit hit;
		//if (Physics.Raycast(targetPos, direction, out hit, currentCameraDistance, cameraBlockingLayerMask))
		//{
		//	currentCameraDistance = hit.distance;
		//}
		
		transform.position = targetPos + direction * currentCameraDistance;
	}

#else
	public GameObject focusedObject;
	public GameObject player;

	//[Header("Sensitivity")]
	public float cameraSensitivityMultiplier = 1f;
	public float cameraZoomMultiplier = 1f;

	//[Header("Camera Lock")]
	public GameObject cameraLock_GO;
	public GameObject enemyLock;
	private vec3 halfExtents = new vec3(2f, 2f, 2f);
	//public LayerMask cameraTargetLockMask;
	public float cameraLockOnDistance = 15f;
	private vec3 lastLockMidpoint;

	public float cameraAutoZoomSpeed = 5f;

	//[Header("Input")]
	//public InputAction moveAction;
	//public InputAction moveActionController;
	//public InputAction zoomAction;
	//public InputAction cameraLock;

	private static readonly string[] cameraBlockingLayers = { "Default", "Environment" };
	private static int cameraBlockingLayerMask;
	public struct CameraData
	{
		public float minDistance, maxDistance;
		public float minPitch, maxPitch;
		public vec3 offsetPosition;

		public CameraData(float _minDistance, float _maxDistance, float _minPitch, float _maxPitch, vec3 _offsetPosition)
		{
			minDistance = _minDistance;
			maxDistance = _maxDistance;
			minPitch = _minPitch;
			maxPitch = _maxPitch;
			offsetPosition = _offsetPosition;
		}
	}

	private static readonly CameraData defaultCameraData = new CameraData(1.25f, 3f, -15f, 85f, new vec3(0.5f, 1.2f, 0));
	public CameraData currentCameraData = defaultCameraData;


	public float cameraPitch;
	public float cameraYaw;

	public float targetCameraDistance;
	public float currentCameraDistance;

	public bool returningToPlayer = false;

	CameraController()
	{
	}
    //public Input
    // This method is called once on the frame when the script is initialized
    void Start()
    {
       
    }

	// This method is called once per frame
	void Update(float dt)
	{
		if (focusedObject == gameObject) return;

		// Reset focus to player if enemy lost
		if (enemyLock == null && focusedObject != player)
		{
			StartReturningToPlayer();
		}

		// Handle enemy lock mode
		if (focusedObject == cameraLock_GO && enemyLock != null)
		{
			HandleEnemyLock(dt);
			return;
		}

		// Normal camera movement (including lerp if returning)
		HandleNormalCamera(dt);

	}
	void HandleEnemyLock(float dt)
	{
		//if (cameraLock.triggered) CameraLock();
		//
		//// Midpoint between player and enemy
		//vec3 midpoint = (player.transform.position + enemyLock.transform.position) * 0.5f;
		//
		//// Smoothly move cameraLock_GO
		//cameraLock_GO.transform.position = vec3.Lerp(cameraLock_GO.transform.position, midpoint, Time.deltaTime * 10f);
		//
		//// Look at midpoint
		//vec3 lookDir = (midpoint - transform.position).normalized;
		//float fixedPitch = 35f;
		//Quaternion targetRotation = Quaternion.LookRotation(lookDir);
		//targetRotation = Quaternion.Euler(fixedPitch, targetRotation.eulerAngles.y, 0f);
		//transform.rotation = Quaternion.Lerp(transform.rotation, targetRotation, Time.deltaTime * 2.5f);
		//
		//// Dynamic zoom to keep both in frame
		//float distance = vec3.Distance(player.transform.position, enemyLock.transform.position);
		//float desiredDistance = Mathf.Clamp(distance * 0.5f + 5f, currentCameraData.minDistance, currentCameraData.maxDistance);
		//transform.position = vec3.Lerp(transform.position, midpoint - transform.forward * desiredDistance + currentCameraData.offsetPosition, Time.deltaTime * 5f);
		//
		//RaycastHit hit;
		//if (Physics.Raycast(transform.position, transform.forward, out hit, currentCameraDistance, cameraBlockingLayerMask))
		//{
		//	transform.position = hit.point;
		//}
	}

	void HandleNormalCamera(float dt)
	{
		Transform transform = this.transform;

		//if (cameraLock.triggered) CameraLock();

		// Read input
		vec2 cameraMovement = new vec2(dt,0.0f) * cameraSensitivityMultiplier;
		//vec2 cameraMovement = moveAction.ReadValue<vec2>() * cameraSensitivityMultiplier;
		
		
		cameraYaw -= cameraMovement.x;
		cameraPitch -= cameraMovement.y;

		cameraYaw = Mathf.Repeat(cameraYaw, 360f);
		cameraPitch = Mathf.Clamp(cameraPitch, currentCameraData.minPitch, currentCameraData.maxPitch);

		float verticalFactor = Mathf.Sin(cameraPitch * Mathf.Deg2Rad);
		float horizontalFactor = Mathf.Cos(cameraPitch * Mathf.Deg2Rad);
		vec3 calculatedCameraDirection = new vec3(
			horizontalFactor * Mathf.Cos(cameraYaw * Mathf.Deg2Rad),
			verticalFactor,
			horizontalFactor * Mathf.Sin(cameraYaw * Mathf.Deg2Rad)
		);
		transform.forward = -calculatedCameraDirection;

		vec3 desiredPosition = player.transform.position
			+ transform.right * currentCameraData.offsetPosition.x
			+ transform.up * currentCameraData.offsetPosition.y
			+ transform.forward * currentCameraData.offsetPosition.z;

		transform.position = desiredPosition;

		// Handle zoom and collisions
		UpdateCameraZoom(desiredPosition, calculatedCameraDirection);
	}

	public void CameraLock(float dt)
	{
		//if (focusedObject == player)
		//{
		//	RaycastHit[] hits = Physics.BoxCastAll(transform.position, halfExtents, Camera.main.transform.forward, transform.rotation, 10f, cameraTargetLockMask);
		//	foreach (RaycastHit hit in hits)
		//	{
		//		if (hit.transform.CompareTag("Player")) continue;
		//
		//		moveAction.Disable();
		//		currentCameraData.maxDistance = 5f;
		//		enemyLock = hit.transform.gameObject;
		//		focusedObject = cameraLock_GO;
		//		returningToPlayer = false;
		//		break;
		//	}
		//}
		//else
		//{
		//	StartReturningToPlayer();
		//}
	}

	void StartReturningToPlayer()
	{
		//Debug.Log("Camera Lock: Returning to player.");
		//currentCameraData.maxDistance = 3f;
		//moveAction.Enable();
		//returningToPlayer = true;
		//
		//vec3 targetPosition = vec3.Lerp(lastLockMidpoint, player.transform.position, Time.deltaTime * 5f);
		//transform.position = vec3.Lerp(transform.position, targetPosition + currentCameraData.offsetPosition, Time.deltaTime * 5f);
		//
		//Quaternion targetRotation = Quaternion.LookRotation(player.transform.position - transform.position);
		//transform.rotation = Quaternion.Lerp(transform.rotation, targetRotation, Time.deltaTime * 5f);
		//
		//Debug.Log($"{vec3.Distance(transform.position, player.transform.position + currentCameraData.offsetPosition)}");
		//if (vec3.Distance(transform.position, player.transform.position + currentCameraData.offsetPosition) < 7f)
		//{
		//	returningToPlayer = false;
		//	focusedObject = player;
		//}
	}

	void UpdateCameraZoom(vec3 targetPos, vec3 direction)
	{
		//targetCameraDistance = Mathf.Clamp(targetCameraDistance, currentCameraData.minDistance, currentCameraData.maxDistance);
		//currentCameraDistance = Mathf.MoveTowards(currentCameraDistance, targetCameraDistance, cameraAutoZoomSpeed * Time.deltaTime);
		//
		//RaycastHit hit;
		//if (Physics.Raycast(targetPos, direction, out hit, currentCameraDistance, cameraBlockingLayerMask))
		//{
		//	currentCameraDistance = hit.distance;
		//}
		//
		//transform.position = targetPos + direction * currentCameraDistance;
	}

	//public vec2 GetHorizontalLookDirection()
	//{
	//	vec2 output = new vec2(transform.forward.x, transform.forward.z);
	//	return output.normalized;
	//}
	//public vec2 GetHorizontalRightDirection()
	//{
	//	vec2 output = new vec2(transform.right.x, transform.right.z);
	//	return output.normalized;
	//}
#endif
}
