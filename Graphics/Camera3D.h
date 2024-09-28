#pragma once
#include "GameObject3D.h" 
#define radians(x) ((x / 360.0f) * XM_2PI)        
using namespace DirectX;

class Camera3D : public GameObject3D 
{
public:
	Camera3D();
	void SetProjectionValues(float fovDegrees, float aspectRatio, float nearZ, float farZ);

	const XMMATRIX& GetViewMatrix() const;
	const XMMATRIX& GetProjectionMatrix() const;
 
private:
	void UpdateMatrix() override; 

	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;
};