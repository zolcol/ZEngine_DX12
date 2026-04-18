#pragma once

class Scene;
struct TransformComponent;
struct CameraComponent;

class EditorCameraController
{
public:
	EditorCameraController() = default;
	~EditorCameraController() = default;

	void Update(Scene* scene, float dt);

private:
	float m_MoveSpeed = 5.0f;
	float m_LookSensitivity = 0.002f; 

	// Vận tốc hiện tại
	DirectX::XMFLOAT3 m_MoveVelocity = { 0, 0, 0 };
	DirectX::XMFLOAT2 m_LookVelocity = { 0, 0 };

	// Hệ số làm mượt (Càng cao càng khựng, càng thấp càng trôi)
	const float m_Damping = 10.0f;

	const float MAX_PITCH = DirectX::XMConvertToRadians(89.0f);
};

