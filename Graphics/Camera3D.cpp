#include "Camera3D.h"

Camera3D::Camera3D()
{
	this->pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	this->posVector = XMLoadFloat3(&this->pos);
	this->rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
	this->rotVector = XMLoadFloat3(&this->rot);
	this->UpdateMatrix(); 
}

void Camera3D::SetProjectionValues(float fovDegrees, float aspectRatio, float nearZ, float farZ)
{
	float fovRadians = radians(fovDegrees); 
	this->projectionMatrix = XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nearZ, farZ);
}

const XMMATRIX& Camera3D::GetViewMatrix() const
{
	return this->viewMatrix;
}

const XMMATRIX& Camera3D::GetProjectionMatrix() const
{
	return this->projectionMatrix;
}

void Camera3D::UpdateMatrix()
{
	XMMATRIX camRotationMatrix = XMMatrixRotationRollPitchYaw(this->rot.x, this->rot.y, this->rot.z);
	XMVECTOR camTarget = XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, camRotationMatrix);
	camTarget += this->posVector;
	XMVECTOR upDir = XMVector3TransformCoord(this->DEFAULT_UP_VECTOR, camRotationMatrix);
	this->viewMatrix = XMMatrixLookAtLH(this->posVector, camTarget, upDir);

	this->UpdateDirectionVectors(); 
}