#include "pch.h"
#include "EditorCameraController.h"
#include "Input.h"
#include "Scene.h"
#include "CoreComponent.h"

void EditorCameraController::Update(Scene* scene, float dt)
{
	using namespace DirectX;

	scene->GetRegistry().view<TransformComponent, CameraComponent>().each([&](TransformComponent& transform, CameraComponent& camera)
	{
		if (!camera.IsPrimary) return;

		// 1. XOAY CAMERA MƯỢT (Damping Rotation)
		if (Input::IsMouseButtonPressed(0))
		{
			XMFLOAT2 mouseDelta = Input::GetMouseDelta();
			// Nạp vận tốc mục tiêu dựa trên delta chuột
			XMVECTOR targetLookVel = XMVectorSet(mouseDelta.x * m_LookSensitivity * 100.0f, mouseDelta.y * m_LookSensitivity * 100.0f, 0, 0);
			XMVECTOR currentLookVel = XMLoadFloat2(&m_LookVelocity);

			// Nội suy vận tốc để tránh bị giật khi di chuyển chuột
			currentLookVel = XMVectorLerp(currentLookVel, targetLookVel, 15.0f * dt);
			XMStoreFloat2(&m_LookVelocity, currentLookVel);
		}
		else
		{
			// Giảm vận tốc xoay về 0 khi không giữ chuột
			XMVECTOR currentLookVel = XMLoadFloat2(&m_LookVelocity);
			currentLookVel = XMVectorLerp(currentLookVel, XMVectorZero(), m_Damping * dt);
			XMStoreFloat2(&m_LookVelocity, currentLookVel);
		}

		// Áp dụng vận tốc xoay vào transform
		transform.Rotation.y += m_LookVelocity.x * dt;
		transform.Rotation.x += m_LookVelocity.y * dt;

		// Giới hạn Pitch
		if (transform.Rotation.x > MAX_PITCH) transform.Rotation.x = MAX_PITCH;
		if (transform.Rotation.x < -MAX_PITCH) transform.Rotation.x = -MAX_PITCH;


		// 2. DI CHUYỂN MƯỢT (Damping Movement)
		XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(transform.Rotation.x, transform.Rotation.y, 0);
		XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
		XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotationMatrix);
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		XMVECTOR moveInput = XMVectorZero();

		if (Input::IsKeyPressed('W')) moveInput += forward;
		if (Input::IsKeyPressed('S')) moveInput -= forward;
		if (Input::IsKeyPressed('A')) moveInput -= right;
		if (Input::IsKeyPressed('D')) moveInput += right;
		if (Input::IsKeyPressed(VK_SHIFT)) moveInput -= up;
		if (Input::IsKeyPressed(VK_SPACE)) moveInput += up;

		float currentSpeed = m_MoveSpeed;
		if (Input::IsKeyPressed(VK_CONTROL)) currentSpeed *= 3.0f;

		// Tính toán vận tốc mục tiêu dựa trên input
		if (XMVector3LengthSq(moveInput).m128_f32[0] > 0)
		{
			moveInput = XMVector3Normalize(moveInput) * currentSpeed;
			XMVECTOR currentVel = XMLoadFloat3(&m_MoveVelocity);
			currentVel = XMVectorLerp(currentVel, moveInput, m_Damping * dt);
			XMStoreFloat3(&m_MoveVelocity, currentVel);
		}
		else
		{
			XMVECTOR currentVel = XMLoadFloat3(&m_MoveVelocity);
			currentVel = XMVectorLerp(currentVel, XMVectorZero(), m_Damping * dt);
			XMStoreFloat3(&m_MoveVelocity, currentVel);
		}

		// Áp dụng vận tốc di chuyển vào transform
		XMVECTOR pos = XMLoadFloat3(&transform.Position);
		pos += XMLoadFloat3(&m_MoveVelocity) * dt;
		XMStoreFloat3(&transform.Position, pos);

		// 3. Zoom / Speed adjustment
		float wheel = Input::GetMouseWheelDelta();
		if (wheel != 0)
		{
			m_MoveSpeed += wheel * 2.0f;
			if (m_MoveSpeed < 0.1f) m_MoveSpeed = 0.1f;
		}
	});
}
