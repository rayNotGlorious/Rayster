#pragma once

#include "common.h"
#include "display/Display.hpp"

class Camera {
	static size_t instance_counter;
	std::string prefix;
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR look;
	DirectX::XMVECTOR forward{ 0.0f, 0.0f, 1.0f, 0.0f };
	DirectX::XMVECTOR up{ 0.0f, 1.0f, 0.0f, 0.0f };
	float yaw = 0.0f;
	float pitch = 0.0f;
	float speed;

	std::function<void(float)> GenerateMovementClosure();
public:
	Camera(DirectX::XMVECTOR position, DirectX::XMVECTOR look, float speed);
	~Camera();

	void setPosition(DirectX::XMVECTOR position);
	DirectX::XMMATRIX deriveViewMatrix() const;

	// The keyboard listener must be copied as well.
	Camera(const Camera& camera);
	// Camera& operator=(const Camera& camera);
};