#include <Engine/NavMesh.h>

namespace navmesh 
{
	struct NavMeshAgentData
	{
		float radius;
		float height;
		float velocity;
		bool active;
	};

	class NavMeshAgentComp
		: public IRegisteredComponent<NavMeshAgentComp>
		, public IEditorComponent<NavMeshAgentComp>
		, public ecs::IComponentCallbacks
	{
	public:
		NavMeshAgentComp();

		void OnAttached() override;
		void OnDetached() override;

		dtCrowdAgent* GetAgent();
		Vec3 GetTargetPos() const;
		void SetAgent(dtCrowdAgent* agentPtr);
		void SetTargetPos(Vec3 const& pos);
		void SetAgentPos(Vec3 const& pos);
		void SetAgentID(int id);

		static const int WALKABLE_HEIGHT;
		static const int WALKABLE_RADIUS;
		static const int WALKABLE_CLIMB;

	private:
		dtCrowdAgent* agent;
		int agentID;

		virtual void EditorDraw() override;

	public:
		property_vtable()
	};

	class NavMeshAgentSystem
		: public ecs::System<NavMeshAgentSystem>
	{
	public:
		NavMeshAgentSystem();

		void OnAdded() override;
		bool PreRun() override;
		void OnRemoved() override;

		dtCrowd* GetCrowdSystem();

		static const int MAX_AGENT_COUNT;
		static const float MAX_AGENT_RADIUS;

	private:
		dtCrowd* crowdSystem;
	};
}
property_begin(navmesh::NavMeshAgentComp)
{
}
property_vend_h(navmesh::NavMeshAgentComp)