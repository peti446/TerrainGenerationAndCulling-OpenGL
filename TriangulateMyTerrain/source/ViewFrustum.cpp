#include "ViewFrustum.h"


ViewFrustum::ViewFrustum()
{
}


ViewFrustum::~ViewFrustum()
{
}

void ViewFrustum::createFrustum(glm::mat4 ProjectViewMatrix)
{
	//Transposes the project view matrix to extract the planes information
	ProjectViewMatrix = glm::transpose(ProjectViewMatrix);

	//Alocate memory for planes vector
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

float ViewFrustum::distance(const glm::vec4& plane, const glm::vec3& point) const
{
	//ax+by+cz+d = 0
	//Formula to calculate the distance between the point and the plane
	return (plane.x * point.x) + (plane.y * point.y) + (plane.z * point.z) + plane.w;
}

bool ViewFrustum::inFrustum(const ViewFrustum::AAB& box) const
{
	//Loop trought each plane to check with the current box
	for(glm::vec4 plane : m_planes)
	{
		//If the distance between the plane and the positive vertex (max vertex) of the Axis aligned box is negative, we can asume that
		//the hole box is outside the fustrum.
		//Note that the positive vertex will be on the side of the of the box that follow the normal, so for example, we are testing the the box againts the left plane
		//the positive vertex will be on the right side of the box.
		//So we can assume that the box is inside or intersection if the distance is 0 or geater.
		//We could then check if the negative vertex is outside to check if we are intersection instead of been fully inside, but it is not applicalbe to this
		if(distance(plane, box.getPositiveVertex(glm::normalize(glm::vec3(plane.x, plane.y, plane.z)))) < 0.0f)
		{
			return false;
		}
	}
	return true;
}
