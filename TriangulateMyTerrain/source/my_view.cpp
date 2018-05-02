#include "my_view.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <limits>

MyView::MyView()
{
}

MyView::~MyView() {
}

void MyView::setScene(const scene::Context * scene)
{
    scene_ = scene;
}

void MyView::toggleShading()
{
    shade_normals_ = !shade_normals_;
}

void MyView::ToggleDebugDrawCalls()
{
	m_showPatchRenderInfo = !m_showPatchRenderInfo;
}

void MyView::ExecuteAProfileQuerry()
{
	if (m_ExecuteQuerryInfo)
		return;

	m_ExecuteQuerryInfo = true;
}

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

#pragma region Shaders Program Creation
    GLint compile_status = 0;
    GLint link_status = 0;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertex_shader_string
        = tygra::createStringFromFile("resource:///terrain_vs.glsl");
    const char *vertex_shader_code = vertex_shader_string.c_str();
    glShaderSource(vertex_shader, 1,
                   (const GLchar **)&vertex_shader_code, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        const int string_length = 1024;
        GLchar log[string_length] = "";
        glGetShaderInfoLog(vertex_shader, string_length, NULL, log);
        std::cerr << log << std::endl;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragment_shader_string
        = tygra::createStringFromFile("resource:///terrain_fs.glsl");
    const char *fragment_shader_code = fragment_shader_string.c_str();
    glShaderSource(fragment_shader, 1,
                   (const GLchar **)&fragment_shader_code, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compile_status);
    if (compile_status != GL_TRUE) {
        const int string_length = 1024;
        GLchar log[string_length] = "";
        glGetShaderInfoLog(fragment_shader, string_length, NULL, log);
        std::cerr << log << std::endl;
    }
    terrain_sp_ = glCreateProgram();
    glAttachShader(terrain_sp_, vertex_shader);
    glDeleteShader(vertex_shader);
    glAttachShader(terrain_sp_, fragment_shader);
    glDeleteShader(fragment_shader);
    glLinkProgram(terrain_sp_);

    glGetProgramiv(terrain_sp_, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        const int string_length = 1024;
        GLchar log[string_length] = "";
        glGetProgramInfoLog(terrain_sp_, string_length, NULL, log);
        std::cerr << log << std::endl;
    }

#pragma endregion

#pragma region Set up data for the terrain
	//Get the displacement map
    tygra::Image displace_image = tygra::createImageFromPngFile(scene_->getTerrainDisplacementMapName());


	// X and -Z are on the ground, Y is up
	const float sizeX = scene_->getTerrainSizeX();
	const float sizeY = scene_->getTerrainSizeY();
	const float sizeZ = scene_->getTerrainSizeZ();

	//Bezier patches
	const size_t number_of_patches = scene_->getTerrainPatchCount();

	//Get the bezier patches
	std::vector<std::vector<glm::vec3>> bezier_patches;
	for (int i = 0; i < number_of_patches; i++)
	{
		for (int v = 0; v < 4; v++) {
			std::vector<glm::vec3> curve;
			for (int u = 0; u < 4; u++) {
				curve.push_back((glm::vec3&)scene_->getTerrainPatchPoint(i, u, v));
			}
			bezier_patches.push_back(curve);
		}
	}
#pragma endregion

#pragma region Create Terrain and apply effects to it
	//First lets generate a grid with the same resolution as the image
	GenerateTesselatedGrid(m_terrainData, displace_image.width(), displace_image.height(), sizeX, sizeZ);
	//Then we apply the bezier to the terrain, afterwards we compute the normals
	ApplyBezierSurface(m_terrainData, bezier_patches);
	//We apply then the dislacement map upon the terrain in its new state (with the bezier applied to it) , afterwards we compute the normals
	ApplyDisplacementMap(m_terrainData, displace_image);
	//Then we apply some noise to it (We are using here Brownian noise but I got the KenPerlin function too that could be used), , afterwards we compute the normals
	ApplyBrownianNoiseToMap(m_terrainData, 0.5f, 8, 2.0f);
	//The last step is splitting up the terrain in patches, we are using 16 by 16 paches in this instance
	SetupTerrainPatches(m_terrainData, 16, 16);
#pragma endregion

#pragma region Creation of the VAO

    glGenBuffers(1, &terrain_mesh_.element_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_mesh_.element_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m_terrainData.elementArray.size() * sizeof(GLuint),
		m_terrainData.elementArray.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    terrain_mesh_.element_count = m_terrainData.elementArray.size();

    glGenBuffers(1, &terrain_mesh_.position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.position_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_terrainData.vertecies.size() * sizeof(glm::vec3),
		m_terrainData.vertecies.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &terrain_mesh_.normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_terrainData.normals.size() * sizeof(glm::vec3),
		m_terrainData.normals.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &terrain_mesh_.vao);
    glBindVertexArray(terrain_mesh_.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_mesh_.element_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.position_vbo);
    glEnableVertexAttribArray(kVertexPosition);
    glVertexAttribPointer(kVertexPosition, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), TGL_BUFFER_OFFSET(0));
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.normal_vbo);
    glEnableVertexAttribArray(kVertexNormal);
    glVertexAttribPointer(kVertexNormal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(glm::vec3), TGL_BUFFER_OFFSET(0));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

#pragma endregion
}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
	//Update the view port
    glViewport(0, 0, width, height);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
	//Clear up the data when closing
    glDeleteProgram(terrain_sp_);
    glDeleteBuffers(1, &terrain_mesh_.position_vbo);
    glDeleteBuffers(1, &terrain_mesh_.normal_vbo);
    glDeleteBuffers(1, &terrain_mesh_.element_vbo);
    glDeleteVertexArrays(1, &terrain_mesh_.vao);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);

	//Set up aspect ratio
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const float aspect_ratio = viewport[2] / (float)viewport[3];

#pragma region Set up View and Projection matrix
    const auto& camera = scene_->getCamera();
    glm::mat4 projection_xform = glm::perspective(glm::radians(camera.getVerticalFieldOfViewInDegrees()),
													aspect_ratio,
													camera.getNearPlaneDistance(),
													camera.getFarPlaneDistance());
    glm::vec3 camera_pos = (const glm::vec3&)camera.getPosition();
    glm::vec3 camera_at = camera_pos + (const glm::vec3&)camera.getDirection();
    glm::vec3 world_up{ 0, 1, 0 };
    glm::mat4 view_xform = glm::lookAt(camera_pos, camera_at, world_up);

#pragma endregion

#pragma region Prepare openGL to render a new frame (Clean the buffers enable cull and depth test ...)
    glClearColor(0.f, 0.f, 0.25f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, shade_normals_ ? GL_FILL : GL_LINE);

    glUseProgram(terrain_sp_);
#pragma endregion

#pragma region Pass the data to the shader
	GLuint shading_id = glGetUniformLocation(terrain_sp_, "use_normal");
	glUniform1i(shading_id, shade_normals_);

    glm::mat4 world_xform = glm::mat4(1);
    glm::mat4 view_world_xform = view_xform * world_xform;

    GLuint projection_xform_id = glGetUniformLocation(terrain_sp_,
                                                      "projection_xform");
    glUniformMatrix4fv(projection_xform_id, 1, GL_FALSE,
                       glm::value_ptr(projection_xform));

    GLuint view_world_xform_id = glGetUniformLocation(terrain_sp_,
                                                      "view_world_xform");
    glUniformMatrix4fv(view_world_xform_id, 1, GL_FALSE,
                       glm::value_ptr(view_world_xform));
#pragma endregion

#pragma region Draw every Patch inside the view Frustum

	//Update the frstum planes
	m_frustum.createFrustum(projection_xform * view_xform);

	//Check if we got an VAO
	if (terrain_mesh_.vao) {
		//Bind the VAO in question
		glBindVertexArray(terrain_mesh_.vao);

		//Draw only the patches inside the view frustum
		int count = 0;
		for (TerrainPatch& p : m_terrainData.patches)
		{
			//Check if the current pach is inside or intersection with the frustum
			if(m_frustum.inFrustum(p.BoundingBox))
			{
				//Draw it
				glDrawElementsBaseVertex(GL_TRIANGLES, p.elementAmount, GL_UNSIGNED_INT, 0, p.elementOffset);
				count++;
			}
		}

		if(m_showPatchRenderInfo)
		{
			//Debug MSG to show how many paches have been drawn
			std::cout << "Paches Rendering: " << count << " out of " << m_terrainData.patches.size() << std::endl;
		}
    }
#pragma endregion
}

void MyView::GenerateTesselatedGrid(TerrainData& terrainData, int subU, int subV, int sizeU, int sizeV)
{

	//Set Terrain data to use later in the reindex
	terrainData.subU = subU;
	terrainData.subV = subV;
	terrainData.sizeU = sizeU;
	terrainData.sizeV = sizeV;

	//Set up data to generate the grid
	int subUSize = sizeU / subU;
	int subVSize = sizeV / subV;

	float halfX = 0.5f*sizeU;
	float halfZ = 0.5f*sizeV;

	float dx = sizeU / (subU - 1);
	float dz = sizeV / (subV - 1);

	//For speed to use multiplication in the for loop
	float oneOverSizeU = 1.0f / (float)(subU-1);
	float oneOverSizeV = 1.0f / (float)(subV-1);

	//Allocate memory for the vectors containing the terrain data
	terrainData.vertecies.resize(subU*subV);
	terrainData.normals.resize(subU*subV);
	terrainData.UVCorrd.resize(subU*subV);
	//Generate the grid
	for(int z = 0; z < subV; z++)
	{
		int zPos = halfZ -  z * dz;
		float v = z * oneOverSizeV;
		for(int x = 0; x < subU; x++)
		{
			int xPos = -halfX + x*dx;
			float u = x * oneOverSizeU;
			terrainData.vertecies[z * subU + x] = (glm::vec3(xPos, 0, zPos));
			terrainData.normals[z * subU + x] = (glm::vec3(0,1,0));
			terrainData.UVCorrd[z * subU + x] = (glm::vec2(u,v));
		}

	}

	//Alocate memory for the elements vector
	terrainData.elementArray.reserve((subU-1)*(subV-1)*6);
	//Generate triangles indecies for the grid
	for(int z = 0; z < subV-1; z++)
	{
		for (int x = 0; x < subU - 1; x++)
		{
			//Calulations of rows
			int row1 = z * subU + x;
			int row2 = (z + 1) * subU + x;

			//To obtain a diamond shape like pattern we need to check the x and Z position of the current quad.
			//And then check if both are either divisible by 2 or not, to make the triangles in the other way
			//For reference:
			// (Row2) * ----- * (Row2 + 1)
			//		  |       |
			//        |       |
			// (Row1) * ----- * (Row1 + 1)
			if ((z % 2 != 0 && x % 2 != 0) || (x % 2 == 0 && z % 2 == 0)) {
				//First Triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2);
				//Second Triangle
				terrainData.elementArray.push_back(row2);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2 + 1);
			}
			else {
				//First triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2 + 1);
				//Second Triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row2 + 1);
				terrainData.elementArray.push_back(row2);
			}
		}
	}
}

void MyView::SetupTerrainPatches(TerrainData& terrainData, int patchSizeU, int patchSizeV)
{
	//Make sure that the patch size can create an exact ammount of paches based on size of the grid, so we dont end up needing to draw 1/2 of a pach
	float tempUAmount = (float)terrainData.subU / (float)patchSizeU;
	float tempVAmount = (float)terrainData.subV / (float)patchSizeV;
	while (glm::ceil(tempUAmount) != tempUAmount) {
		patchSizeU++;
		tempUAmount = (float)terrainData.subU / (float)patchSizeU;
	}
	while (glm::ceil(tempVAmount) != tempVAmount) {
		patchSizeV++;
		tempVAmount = (float)terrainData.subV / (float)patchSizeV;
	}

	//Values to to create patches
	int patchAmountU = (int)tempUAmount;
	int patchAmountV = (int)tempVAmount;


	//Clear the eelement array
	terrainData.elementArray.clear();
	//Reserve memory for the element array
	terrainData.elementArray.reserve((patchSizeU-1)*(patchSizeV-1)*6);
	//Generate the new infex data for the current patch
	for (int z = 0; z < patchSizeV-1; z++)
	{
		for (int x = 0; x < patchSizeU-1; x++)
		{
			//Calulations of rows
			int row1 = z * terrainData.subU + x;
			int row2 = (z + 1) * terrainData.subU + x;

			//To obtain a diamond shape like pattern we need to check the x and Z position of the current quad.
			//And then check if both are either divisible by 2 or not, to make the triangles in the other way
			//For reference:
			// (Row2) * ----- * (Row2 + 1)
			//		  |       |
			//        |       |
			// (Row1) * ----- * (Row1 + 1)
			if ((z % 2 != 0 && x % 2 != 0) || (x % 2 == 0 && z % 2 == 0)) {
				//First triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2);
				//Second triangle
				terrainData.elementArray.push_back(row2);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2 + 1);
			}
			else
			{
				//First triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2 + 1);
				//Second triangle
				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row2 + 1);
				terrainData.elementArray.push_back(row2);
			}
		}
	}

	//Alocate memory for the patches
	terrainData.patches.resize(patchAmountU*patchAmountV);
	//Create the paches
	for (int z = 0; z < patchAmountV; z++)
	{
		for (int x = 0; x < patchAmountU; x++)
		{
			//Create a terrain patch
			TerrainPatch p;
			//element array size (Im using this as it might chhange but in this case i did not)
			p.elementAmount = terrainData.elementArray.size();
			//Calculate the offset to later drawn them
			p.elementOffset = (x * (patchSizeU-1)) + (z * ((patchSizeV-1) * ((patchSizeU) * patchAmountU)));

			//Generate the Axis Aligned Box
			ViewFrustum::AAB box;
			//infinity variable to use as max and min points
			float infinity = std::numeric_limits<float>::infinity();
			//The max point is set to -infinity and the min point to infinity (so the first point is set as the max and min)
			box.maxPoint = glm::vec3(-infinity, -infinity, -infinity);
			box.minPoint = -box.maxPoint;
			//Traverse trought each vertex of the patch
			for (int i = 0; i < terrainData.elementArray.size(); i++)
			{
				//Current pos
				glm::vec3 pos = terrainData.vertecies[terrainData.elementArray[i] + p.elementOffset];

				//Check if current X pos is the new max or min of the Axis aligned bounding box
				if (box.maxPoint.x < pos.x)
					box.maxPoint.x = pos.x;
				else if (box.minPoint.x > pos.x)
					box.minPoint.x = pos.x;

				//Check if current Y pos is the new max or min of the Axis aligned bounding box
				if (box.maxPoint.y < pos.y)
					box.maxPoint.y = pos.y;
				else if (box.minPoint.y > pos.y)
					box.minPoint.y = pos.y;

				//Check if current Z pos is the new max or min of the Axis aligned bounding box
				if (box.maxPoint.z < pos.z)
					box.maxPoint.z = pos.z;
				else if (box.minPoint.z > pos.z)
					box.minPoint.z = pos.z;
			}

			//Ad the bounding box to the patch
			p.BoundingBox = box;

			//Add the pach to the terrain data (Could use push back, put it is better to keep them in order)
			terrainData.patches[z * patchAmountU + x] = p;
		}
	}
}

void MyView::ApplyBezierSurface(TerrainData& terrainData, std::vector<std::vector<glm::vec3>>& bezier_patch)
{
	//Loops trought every vertex and applies the bezier surface formula to it
	for (int i = 0; i < terrainData.vertecies.size(); i++)
	{
		//Make the UV coordinates from 0-1 to work across two patches
		//If the X coordinate is > 0.5 the second paches will be used, if it is below the first pach will be used
		float u = terrainData.UVCorrd[i].x;
		int current_bezier_batch = 1;
		if (u < 0.5f) {
			u = u * 2.0f;
		}
		else
		{
			u = (u - 0.5f) * 2.0f;
			current_bezier_batch = 2;
		}
		//Apply the Bezier surface on the current pach
		terrainData.vertecies[i] = BezierSurface(bezier_patch, u, terrainData.UVCorrd[i].y, current_bezier_batch);
	}
	//Recalculate the normals as we changed the terrain
	ComputeNormals(terrainData);
}

void MyView::ApplyDisplacementMap(TerrainData& terrainData, const tygra::Image& displacementMap)
{
	//Scalar for the dispalcement map to give it more depth
	const float DesplacementMapScalar = 300.0f;

	//Precomputed variables so we dont need to calculate it for every vertex
	int widthCeroBased = displacementMap.width() - 1;
	int heightCeroBased = displacementMap.height() - 1;
	float oneOver255 = 1.0f / 255.0f;

	//Apply the noise to every vertex of the terrain
	for (int i = 0; i < terrainData.vertecies.size(); i++)
	{
		//Apply the pixel displacement on the normal
		terrainData.vertecies[i] += terrainData.normals[i]
								  * DesplacementMapScalar
								  * ((float)(*(uint8_t*)displacementMap.pixel(terrainData.UVCorrd[i].x * widthCeroBased, terrainData.UVCorrd[i].y * heightCeroBased)))
								  * oneOver255;
	}

	//Recaluclate the normals as we just changed the terrain
	ComputeNormals(terrainData);
}

void MyView::ApplyBrownianNoiseToMap(TerrainData & terrainData, float gain, int octaves, float lacunarity)
{
	const float noiseScalar = 15.0f;
	for (int i = 0; i < terrainData.vertecies.size(); i++)
	{
		float noise = FractionalBrownian(terrainData.vertecies[i].x, terrainData.vertecies[i].z, gain, octaves, terrainData.sizeU / terrainData.subU, lacunarity);
		terrainData.vertecies[i] += terrainData.normals[i] * (noiseScalar * noise);
	}
	//Recalculate the normals as we changed the positions of the the vertecies of the terrain
	ComputeNormals(terrainData);
}

void MyView::ApplyKenPerlin(TerrainData & terrainData)
{
	//Scalar to apply to the 0-1 value from the noise so noticeable in the world
	const float NoiseScalar = 15.0;

	//Apply the noise to every vertex of the terrain
	for (int i = 0; i < terrainData.vertecies.size(); i++)
	{
		//Make KenPerlin noice to the range 0-1 as by default it returns it from -1 to 1;
		float noise = 0.5 + 0.5 * KenPerlin(terrainData.vertecies[i].x, terrainData.vertecies[i].z);
		//Multiply the noise value by the scalar and then the result by the normal and add it to the vertex position
		terrainData.vertecies[i] += terrainData.normals[i] * (noise * NoiseScalar);
	}

	//Recalculate the normals as we changed the positions of the the vertecies of the terrain
	ComputeNormals(terrainData);
}

glm::vec3 MyView::BezierSurface(std::vector<std::vector<glm::vec3>>& bezier_patch, float U, float V, int startingBatch)
{
	//Vector to hold the position to execute bezier later on
	std::vector<glm::vec3> final_curve;

	//Chose the current starting batch
	if (startingBatch < 1)
		startingBatch = 1;

	//Loop trough the 4 bezier points to get the points and form another one
	for (int i = 0; i < 4; i++)
	{
		int bezier_patch_index = i + (4 * (startingBatch - 1));
		final_curve.push_back(BezierCurve(bezier_patch[bezier_patch_index], U));
	}

	//Final Bezier curve to return the 3D position in space
	return BezierCurve(final_curve, V);
}

glm::vec3 MyView::BezierCurve(std::vector<glm::vec3>& control_points, float t)
{
	//Constant bezier formula for 4 control points
	return (1-t) * (1 - t) * (1 - t) * control_points[0] +
		    3 * t * (1-t) * (1-t) * control_points[1] +
		    3 * t * t * (1-t) * control_points[2] +
			t * t * t * control_points[3];
}

glm::vec3 MyView::BezierCurveTangent(std::vector<glm::vec3>& control_points, float t)
{
	//Constant bezier tangent formula for 4 control points
	return  3 * (1 - t) * (1 - t) * (control_points[1] - control_points[0]) +
			6 * (1 - t) * t *  (control_points[2] - control_points[1]) + 
			3 * t * t * (control_points[3] - control_points[2]);
}

void MyView::ComputeNormals(TerrainData & data)
{
	//Reset the normals to a 0 state
	for (glm::vec3& normal : data.normals)
		normal = glm::vec3(0, 0, 0);

	//Calculate the normal for each face and add it to the normal to each vertex from that face
	for (int i = 0; i < data.elementArray.size(); i += 3)
	{
		//Get U and V
		glm::vec3 u = data.vertecies[data.elementArray[i + 1]] - data.vertecies[data.elementArray[i]];
		glm::vec3 v = data.vertecies[data.elementArray[i + 2]] - data.vertecies[data.elementArray[i]];

		//Calculate the normal
		glm::vec3 temp_normal = glm::normalize(glm::cross(u, v));

		//Add the current face normal to the vertecies forming the face
		data.normals[data.elementArray[i]] += temp_normal;
		data.normals[data.elementArray[i + 1]] += temp_normal;
		data.normals[data.elementArray[i + 2]] += temp_normal;
	}

	//Normalize all the normals as we added multiple normals together
	for (glm::vec3& normal : data.normals)
		normal = glm::normalize(normal);
}

float MyView::FractionalBrownian(float x, float y, float gain, int octaves, int hgrid, float lacunarity) const
{
	//Total is the height value
	float total = 0.0f;
	//Starting frequency, decreased each octave by the amount specified lacunarity
	float frequency = 1.0 / (float)hgrid;
	//Amplitude starts as gain, and each octave it is multiplied by the gain
	float amplitude = gain;

	//Loop trought each octave
	for (int i = 0; i < octaves; i++)
	{
		//Add the noise to the total
		total += PerlinNoise((int)(x * frequency), (int)(y * frequency)) * amplitude;
		//Decrease frequency
		frequency *= lacunarity;
		//Increase amplitude
		amplitude *= gain;
	}

	//return the final height value
	return total;
}

float MyView::PerlinNoise(int x, int y) const
{
	int n = x + y * 57;
	n = (n << 13) ^ n;
	int nn = ((n*((n*n * 15731) + 789221) + 1376312589) & 0x7fffffff);
	return 1.0 - ((float)nn / 1073741824.0f);
}

float MyView::CosineLerp(float a, float b, float x) const
{
	float ft = x * 3.1415926535897932384626433832795f;
	float f = (1.0f - cos(ft)) * 0.5f;
	return a * (1.0f - f) + b * f;
}

float MyView::KenPerlin(float xPos, float zPos) const
{
	float s = PerlinNoise((int)xPos, (int)zPos);
	float t = PerlinNoise((int)xPos + 1, (int)zPos);
	float u = PerlinNoise((int)xPos, (int)zPos + 1);
	float v = PerlinNoise((int)xPos + 1, (int)zPos + 1);

	float c1 = CosineLerp(s, t, xPos);
	float c2 = CosineLerp(u,v, xPos);

	return CosineLerp(c1, c2, zPos);
}
