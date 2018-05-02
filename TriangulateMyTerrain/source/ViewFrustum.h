#pragma once
#include <glm/glm.hpp>
#include <tgl/tgl.h>
#include <scene/context.hpp>
#include <vector>

class ViewFrustum
{
public:

	//Axis Aligned Box structy
	struct AAB {
		glm::vec3 maxPoint;
		glm::vec3 minPoint;

		const glm::vec3& getPositiveVertex(glm::vec3 normal) const
		{
			glm::vec3 positive = minPoint;
			if (normal.x >= 0)
				positive.x = maxPoint.x;
			if (normal.y >= 0)
				positive.y = maxPoint.y;
			if (normal.z >= 0)
				positive.z = maxPoint.z;

			return positive;
		}

		const glm::vec3& getNegativeVertex(glm::vec3 normal) const
		{
			glm::vec3 negative = maxPoint;
			if (normal.x <= 0)
				negative.x = minPoint.x;
			if (normal.y <= 0)
				negative.y = minPoint.y;
			if (normal.z <= 0)
				negative.z = minPoint.z;

			return negative;
		}
	};

	ViewFrustum();
	~ViewFrustum();

	void createFrustum(glm::mat4 ProjectViewMatrix);

	bool inFrustum(const ViewFrustum::AAB& box) const;

private:

	float distance(const glm::vec4 plane, const glm::vec3 point) const;

	std::vector<glm::vec4> m_planes;
};

