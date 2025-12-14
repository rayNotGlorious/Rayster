#include "logic/Camera.hpp"
#include <iostream>

size_t Camera::instance_counter = 0;


std::function<void(float)> Camera::GenerateMovementClosure() {
	return [=](float deltaTime) {
		DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);
		look = DirectX::XMVector3Transform(forward, rotation);

		DirectX::XMVECTOR delta{};
		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, look));
	
		if (Display::isKeyPressed(Key::W)) {
			delta = DirectX::XMVectorAdd(delta, DirectX::XMVectorScale(look, speed * deltaTime));
		}

		if (Display::isKeyPressed(Key::S)) {
			delta = DirectX::XMVectorSubtract(delta, DirectX::XMVectorScale(look, speed * deltaTime));
		}

		if (Display::isKeyPressed(Key::D)) {
			delta = DirectX::XMVectorAdd(delta, DirectX::XMVectorScale(right, speed * deltaTime));
		}

		if (Display::isKeyPressed(Key::A)) {
			delta = DirectX::XMVectorSubtract(delta, DirectX::XMVectorScale(right, speed * deltaTime));
		}

		if (Display::isKeyPressed(Key::SpaceBar)) {
			delta = DirectX::XMVectorAdd(delta, DirectX::XMVectorScale(up, speed * deltaTime));
		}

		if (Display::isKeyPressed(Key::LeftControl)) {
			delta = DirectX::XMVectorSubtract(delta, DirectX::XMVectorScale(up, speed * deltaTime));
		}

		setPosition(DirectX::XMVectorAdd(position, delta));
		
	};
}

Camera::Camera(DirectX::XMVECTOR position, DirectX::XMVECTOR look, float speed) : prefix{ "Camera" + instance_counter++ }, position{ position }, look{ look }, speed{ speed } {
	Display::registerFrameCallback(GenerateMovementClosure(), prefix);
}

Camera::~Camera() {
	Display::deregisterFrameCallback(prefix);
}

void Camera::setPosition(DirectX::XMVECTOR position) {
	this->position = position;
}

DirectX::XMMATRIX Camera::deriveViewMatrix() const {
	return DirectX::XMMatrixLookAtLH(position, DirectX::XMVectorAdd(look, position), up);
}

Camera::Camera(const Camera& camera) : prefix{ "Camera" + instance_counter++ }, position{ camera.position }, look{ camera.look }, speed{ camera.speed } {
	Display::registerFrameCallback(GenerateMovementClosure(), prefix);
}
