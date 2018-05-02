#include "ViewFrustum.h"


ViewFrustum::ViewFrustum()
{
}


ViewFrustum::~ViewFrustum()
{
}

void ViewFrustum::createFrustum(glm::mat4 ProjectViewMatrix)
{

	ProjectViewMatrix = glm::transpose(ProjectViewMatrix);

	m_planes.reserve(6);
	//Left plane
	m_planes.push_back(ProjectViewMatrix[3] + ProjectViewMatrix[0]);
	//Right plane
	m_planes.push_back(ProjectViewMatrix[3] - ProjectViewMatrix[0]);

	//Top plane
	m_planes.push_back(ProjectViewMatrix[3] - ProjectViewMatrix[1]);
	//Bottom plae
	m_planes.push_back(ProjectViewMatrix[3] + ProjectViewMatrix[1]);

	//Near plane
	m_planes.push_back(ProjectViewMatrix[3] + ProjectViewMatrix[2]);
	//Far plane
	m_planes.push_back(ProjectViewMatrix[3] - ProjectViewMatrix[2]);
}

float ViewFrustum::distance(const glm::vec4 plane, const glm::vec3 point) const
{
	return (plane.x * point.x) + (plane.y * point.y) + (plane.z * point.z) + plane.w;
}

bool ViewFrustum::inFrustum(const ViewFrustum::AAB& box) const
{
	for(glm::vec4 plane : m_planes)
	{

		/*if (distance(plane, glm::vec3(box.maxPoint.x, box.maxPoint.y, box.maxPoint.z)) < 0
			&& 
			distance(plane, glm::vec3(box.maxPoint.x, box.maxPoint.y, box.minPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.maxPoint.x, box.minPoint.y, box.minPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.maxPoint.x, box.minPoint.y, box.maxPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.minPoint.x, box.maxPoint.y, box.maxPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.minPoint.x, box.maxPoint.y, box.minPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.minPoint.x, box.minPoint.y, box.maxPoint.z)) < 0
			&&
			distance(plane, glm::vec3(box.minPoint.x, box.minPoint.y, box.minPoint.z)) < 0) {
			return false;
		}*/
		//If negative they are behind a plane and thus outside the frustum imediatly
		if(distance(plane, box.getPositiveVertex(glm::normalize(glm::vec3(plane.x, plane.y, plane.z)))) < 0.0f)
		{
			return false;
		}
		//else if (distance(plane, box.getNegativeVertex(glm::normalize(glm::vec3(plane.x, plane.y, plane.z)))) < 0.0f) {
		//	return true;
		//}
	}
	return true;
}
