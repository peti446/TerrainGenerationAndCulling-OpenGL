#pragma once
#include <glm/glm.hpp>
class ViewFrustum
{
public:
	//https://stackoverflow.com/questions/13665932/calculating-the-viewing-frustum-in-a-3d-space
	ViewFrustum();
	~ViewFrustum();

	void createFristim(glm::vec3 pos, glm::vec3 dir, float nearPlaneDistance, float farPlaneDistance);

	bool inFrustum(glm::vec3 pos);
};

