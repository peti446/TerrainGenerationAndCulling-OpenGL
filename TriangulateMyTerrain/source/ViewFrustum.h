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
		//Poits of the AAB
		glm::vec3 maxPoint;
		glm::vec3 minPoint;

		//Formula to get the positive vertex givien a normal from a plane
		//This is done as it depends where the normal is facing (thus where the plane is) to decide wich point is the positive point
		//(the max point of the AAB) so we can easily calculate if the box is inside or not.
		//If the normal is negative, the min position from the AAB will be used as the positive(max)
		//position, and if the normal is positive the max point form the ABB will be used as the positive(max) point.
		//Basicly(Short explanation), if the normal is negative, the max and min positions are flipped arround.
		//And so when it comes to test, if the positive position is outside we can assume that the box is compleatly outside the view frustum.
		const glm::vec3 getPositiveVertex(glm::vec3 normal) const
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

		//Formula to get the negative vertex fiven a normal from a plane
		//The same principal as the positive vertex applies to this formula but the oter way around, if the normal es negative the negative vertex (min point)
		//will be the maxPoint of the AAB, but if it is positive the min position will be the negative (min) position
		const glm::vec3 getNegativeVertex(glm::vec3 normal) const
		{
			glm::vec3 negative = maxPoint;
			if (normal.x >= 0)
				negative.x = minPoint.x;
			if (normal.y >= 0)
				negative.y = minPoint.y;
			if (normal.z >= 0)
				negative.z = minPoint.z;

			return negative;
		}
	};

	ViewFrustum();
	~ViewFrustum();

	void createFrustum(glm::mat4 ProjectViewMatrix);

	bool inFrustum(const ViewFrustum::AAB& box) const;

private:

	float distance(const glm::vec4& plane, const glm::vec3& point) const;

	std::vector<glm::vec4> m_planes;
};

