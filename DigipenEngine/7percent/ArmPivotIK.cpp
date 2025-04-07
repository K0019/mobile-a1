#include "ArmPivotIK.h"

ArmPivotIKComponent::ArmPivotIKComponent() :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    upperArmEntity{ nullptr }, lowerArmEntity{ nullptr }, handEntity{ nullptr }, shoulderOffset(0, 0),// About -160 degrees,        // Can't bend forward
    maxExtensionAngle(180.0f), // 180 degrees
    preferredElbowAngle(-1.57f), // -90 degrees
    smoothingFactor(0.8f),
    currentShoulderAngle(0.0f),
    currentElbowAngle(-1.57f),
    elbowPositionOffset(0, 0),
    upperArmRotation(0.0f),
    forearmRotation(0.0f) {
}

#ifdef IMGUI_ENABLED
void ArmPivotIKComponent::EditorDraw(ArmPivotIKComponent& comp)
{
    // Entity references
    if (ImGui::CollapsingHeader("Entity References", ImGuiTreeNodeFlags_DefaultOpen))
    {
        comp.upperArmEntity.EditorDraw("Upper Arm Entity");
        comp.lowerArmEntity.EditorDraw("Lower Arm Entity");
        comp.handEntity.EditorDraw("Hand Entity");
    }
    
    // Configuration
    if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat2("Shoulder Offset", &comp.shoulderOffset.x, 0.1f);
        
        // Convert radians to degrees for the interface
        float extensionAngleDegrees = comp.maxExtensionAngle;
        // Angle constraints in degrees (allow positive values)
        ImGui::Text("Angle Constraints (degrees)");
    
        if (ImGui::DragFloat("Max Extension Angle", &extensionAngleDegrees, 1.0f, 0.0f, 180.0f))
        {
            comp.maxExtensionAngle = extensionAngleDegrees;
        }
    
        ImGui::Text("0° = No bend (rigid), 180° = Complete flexibility");
        
        // Motion control
        ImGui::DragFloat("Smoothing Factor", &comp.smoothingFactor, 0.01f, 0.0f, 0.99f);
    }
    
    // Debug visualization
    ImGui::Checkbox("Debug Draw", &comp.debugDraw);
    
    // Current state (read-only)
    if (ImGui::CollapsingHeader("Current State (Read-only)"))
    {
        float shoulderDegrees = glm::degrees(comp.currentShoulderAngle);
        float elbowDegrees = glm::degrees(comp.currentElbowAngle);
        
        ImGui::Text("Current Shoulder Angle: %.2f deg", shoulderDegrees);
        ImGui::Text("Current Elbow Angle: %.2f deg", elbowDegrees);
        ImGui::Text("Total Arm Angle: %.2f deg", shoulderDegrees + elbowDegrees);
        ImGui::Text("Elbow Position Offset: (%.2f, %.2f)", 
                    comp.elbowPositionOffset.x, comp.elbowPositionOffset.y);
        ImGui::Text("Upper Arm Rotation: %.2f deg", comp.upperArmRotation);
        ImGui::Text("Forearm Rotation: %.2f deg", comp.forearmRotation);
    }
    
    // Helpful instructions
    if (ImGui::CollapsingHeader("Help"))
    {
        ImGui::TextWrapped("This component applies IK to position arm segments based on hand position.");
        ImGui::BulletText("Upper Arm: Entity for shoulder to elbow segment");
        ImGui::BulletText("Lower Arm: Entity for elbow to wrist segment");
        ImGui::BulletText("Hand: Target entity that arms will reach for");
        ImGui::Spacing();
        ImGui::TextWrapped("Angle Controls (in degrees):");
        ImGui::BulletText("Min/Max Elbow: Limits how much the elbow can bend");
        ImGui::BulletText("Negative values bend inward (like human elbow)");
        ImGui::BulletText("Positive values bend outward (reverse elbow)");
        ImGui::BulletText("Preferred: Natural resting pose when possible");
        ImGui::Spacing();
        ImGui::TextWrapped("Higher smoothing values make movement more fluid but less responsive.");
    }
}
#endif




ArmPivotIKSystem::ArmPivotIKSystem() : System_Internal{ &ArmPivotIKSystem::UpdateArmPivotIKComp } {
}

void ArmPivotIKSystem::UpdateArmPivotIKComp(ArmPivotIKComponent& comp)
{
    if(!comp.upperArmEntity.IsValidReference() || !comp.lowerArmEntity.IsValidReference() || !comp.handEntity.IsValidReference())
    {
        return;
    }
    float dt = GameTime::FixedDt();
     //comp.debugDraw = true;
    Transform& entityTransform = ecs::GetEntityTransform(&comp);
    Transform& upperArmTransform = comp.upperArmEntity->GetTransform();
    Transform& lowerArmTransform = comp.lowerArmEntity->GetTransform();
    Transform& handTransform = comp.handEntity->GetTransform();
    // Calculate world positions
    Vector2 shoulderPos = entityTransform.GetWorldPosition() + comp.shoulderOffset;
    Vector2 targetPos = handTransform.GetWorldPosition();
    // Get bone lengths
    float upperArmLength = upperArmTransform.GetWorldScale().x; 
    float lowerArmLength = lowerArmTransform.GetWorldScale().x;  
    if(upperArmLength < 0.0001f || lowerArmLength < 0.0001f) {
        return;
    }
    Vector2 toTarget = targetPos - shoulderPos;
    float targetDistance = toTarget.Length();

    // Constrain target distance to what's physically possible
    float maxReach = upperArmLength + lowerArmLength;
    float minReach = abs(upperArmLength - lowerArmLength);
    if(targetDistance > maxReach * 0.999f)
    {
        // Target is too far, scale back
        targetDistance = maxReach * 0.999f;
    }
    else if(targetDistance < minReach * 1.001f && targetDistance > 0.001f)
    {
        // Target is too close, push out
        targetDistance = minReach * 1.001f;
    }
    // Calculate elbow angle using law of cosines
    float cosElbow = (upperArmLength * upperArmLength + lowerArmLength * lowerArmLength -
                      targetDistance * targetDistance) / (2 * upperArmLength * lowerArmLength);
    cosElbow = std::min(std::max(cosElbow, -1.0f), 1.0f); // Ensure valid cosine value
    float interiorAngle = acos(cosElbow);
    float maxExtensionAngleRad = glm::radians(comp.maxExtensionAngle);
    // Apply the constraint
    if (interiorAngle > maxExtensionAngleRad) {
        interiorAngle = maxExtensionAngleRad;
    }
    float baseAngle = atan2(toTarget.y, toTarget.x);
    float elbowSign = (toTarget.x > 0) ? 1.0f : -1.0f;
    float targetElbowAngle = elbowSign * (glm::pi<float>() - interiorAngle);
    // Calculate shoulder angle
    float cosShoulderToTarget = (upperArmLength * upperArmLength + targetDistance * targetDistance -
                                 lowerArmLength * lowerArmLength) / (2 * upperArmLength * targetDistance);
    cosShoulderToTarget = std::min(std::max(cosShoulderToTarget, -1.0f), 1.0f);
    float offsetAngle = acos(cosShoulderToTarget);
    float targetShoulderAngle = baseAngle - elbowSign * offsetAngle;
    if(comp.smoothingFactor > 0)
    {
        // Lerp between current angles and target angles
        comp.currentShoulderAngle = util::Lerp(comp.currentShoulderAngle, targetShoulderAngle, 1.0f - pow(comp.smoothingFactor, dt));
        comp.currentElbowAngle = util::Lerp(comp.currentElbowAngle, targetElbowAngle, 1.0f - pow(comp.smoothingFactor, dt));
    }
    else
    {
        comp.currentShoulderAngle = targetShoulderAngle;
        comp.currentElbowAngle = targetElbowAngle;
    }

    // Calculate elbow position
    Vector2 elbowPos = shoulderPos + Vector2(cos(comp.currentShoulderAngle), sin(comp.currentShoulderAngle)) * upperArmLength;
    comp.elbowPositionOffset = elbowPos - shoulderPos;
    comp.upperArmRotation = glm::degrees(comp.currentShoulderAngle);
    comp.forearmRotation = glm::degrees(comp.currentShoulderAngle + comp.currentElbowAngle);
    Vector2 upperArmOffset = Vector2(upperArmLength/2, 0).Rotate(glm::radians(comp.upperArmRotation));
    upperArmTransform.SetWorldPosition(shoulderPos + upperArmOffset);
    upperArmTransform.SetWorldRotation(comp.upperArmRotation);
    Vector2 lowerArmOffset = Vector2(-lowerArmLength/2, 0).Rotate(glm::radians(comp.forearmRotation + 180));
    lowerArmTransform.SetWorldPosition(elbowPos + lowerArmOffset);
    lowerArmTransform.SetWorldRotation(comp.forearmRotation); 
#ifdef IMGUI_ENABLED
if(comp.debugDraw)
{
    // Draw basic arm segments
    Vector2 shoulder_pos = entityTransform.GetWorldPosition() + comp.shoulderOffset;
    Vector2 elbow_pos = shoulder_pos + comp.elbowPositionOffset;
    Vector2 handPos = handTransform.GetWorldPosition();

    util::DrawLine(shoulder_pos, elbow_pos, Vector3(0.0f, 1.0f, 0.0f), 1.0f); // Upper arm (green)
    util::DrawLine(elbow_pos, handPos, Vector3(0.0f, 0.0f, 1.0f), 1.0f);      // Lower arm (blue)
    Vector2 upperArmDir = (elbow_pos - shoulder_pos).Normalize().Rotate(glm::radians(-180.0f));
    Vector2 lowerArmDir = (handPos - elbow_pos).Normalize();
    
    // Calculate the angle between the two arms using dot product
    float dotProduct = upperArmDir.Dot(lowerArmDir);
    dotProduct = std::min(std::max(dotProduct, -1.0f), 1.0f);
    float angleBetween = acos(dotProduct);
    
    // Determine whether lowerArm is clockwise or counterclockwise from upperArm
    float cross = upperArmDir.x * lowerArmDir.y - upperArmDir.y * lowerArmDir.x;
    bool clockwise = cross < 0;
    
    // Draw the arc
    const int segments = 20;
    float arcRadius = lowerArmLength * 0.2f;
    
    // Get the base angle for the upper arm
    float upperArmAngle = atan2(upperArmDir.y, upperArmDir.x);
    
    // Draw arc from upper arm to lower arm
    Vector2 prevPoint = elbow_pos + upperArmDir * arcRadius;
    
    for(int i = 0; i <= segments; ++i)
    {
        float t = (float)i / segments;
        float currentAngle;
        
        if (clockwise) {
            currentAngle = upperArmAngle - t * angleBetween;
        } else {
            currentAngle = upperArmAngle + t * angleBetween;
        }
        
        Vector2 point = elbow_pos + Vector2(cos(currentAngle), sin(currentAngle)) * arcRadius;
        util::DrawLine(prevPoint, point, Vector3(1.0f, 0.5f, 0.0f), 0.5f);  // Orange arc
        prevPoint = point;
    }
    
    // Draw maximum extension line
    float maxExtensionAngle = glm::radians(comp.maxExtensionAngle);
    float maxAngle;
    
    if (clockwise) {
        maxAngle = upperArmAngle - maxExtensionAngle;
    } else {
        maxAngle = upperArmAngle + maxExtensionAngle;
    }
    
    Vector2 maxPoint = elbow_pos + Vector2(cos(maxAngle), sin(maxAngle)) * arcRadius * 1.2f;
    util::DrawLine(elbow_pos, maxPoint, Vector3(1.0f, 0.0f, 0.0f), 0.8f);  // Red line
}
#endif
}

