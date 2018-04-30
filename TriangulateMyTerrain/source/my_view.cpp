#include "my_view.hpp"
#include <tygra/FileHelper.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>

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

void MyView::windowViewWillStart(tygra::Window * window)
{
    assert(scene_ != nullptr);

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


    // X and -Z are on the ground, Y is up
    const float sizeX = scene_->getTerrainSizeX();
    const float sizeY = scene_->getTerrainSizeY();
    const float sizeZ = scene_->getTerrainSizeZ();

    const size_t number_of_patches = scene_->getTerrainPatchCount();
    // below is an example of accessing a control point from the second patch
    scene::Vector3 cp = scene_->getTerrainPatchPoint(1, 2, 3);


    tygra::Image displace_image =
        tygra::createImageFromPngFile(scene_->getTerrainDisplacementMapName());

    // below is an example of reading the red component of pixel(x,y) as a byte [0,255]
    uint8_t displacement = *(uint8_t*)displace_image.pixel(1, 2);

	//https://stackoverflow.com/questions/47086858/create-a-grid-in-opengl
    // below is placeholder code to create a tessellated quad
    // replace the hardcoded values with algorithms to create a tessellated quad

	GenerateTesselatedGrid(MyTerrain, displace_image.width(), displace_image.height(), sizeX, sizeZ, 8, 8);
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


	ApplyBezierSurface(MyTerrain, bezier_patches);


	/*for (int i = 0; i < MyTerrain.vertecies.size(); i++)
	{
		MyTerrain.vertecies[i] += MyTerrain.normals[i] * 300.f * ((float)(*(uint8_t*)displace_image.pixel((MyTerrain.UVCorrd[i].x )* (displace_image.width()-1), (MyTerrain.UVCorrd[i].y) * (displace_image.height()-1))))/255.f;
	}

	ComputeNormals(MyTerrain);


	for (int i = 0; i < MyTerrain.vertecies.size(); i++)
	{
		float f = FractionalBrownian(MyTerrain.vertecies[i].x, MyTerrain.vertecies[i].z, 0.5f, 16, 1);
		float k = KenPerlin(MyTerrain.UVCorrd[i].x, MyTerrain.UVCorrd[i].y);
		MyTerrain.vertecies[i] += MyTerrain.normals[i] * f;
	}

	ComputeNormals(MyTerrain);*/

    // below is indicative code for initialising a terrain VAO.

    glGenBuffers(1, &terrain_mesh_.element_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_mesh_.element_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		MyTerrain.elementArray.size() * sizeof(GLuint),
		MyTerrain.elementArray.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    terrain_mesh_.element_count = MyTerrain.elementArray.size();

    glGenBuffers(1, &terrain_mesh_.position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.position_vbo);
    glBufferData(GL_ARRAY_BUFFER, MyTerrain.vertecies.size() * sizeof(glm::vec3),
		MyTerrain.vertecies.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &terrain_mesh_.normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, MyTerrain.normals.size() * sizeof(glm::vec3),
		MyTerrain.normals.data(), GL_STATIC_DRAW);
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
}

void MyView::windowViewDidReset(tygra::Window * window,
                                int width,
                                int height)
{
    glViewport(0, 0, width, height);
}

void MyView::windowViewDidStop(tygra::Window * window)
{
    glDeleteProgram(terrain_sp_);
    glDeleteBuffers(1, &terrain_mesh_.position_vbo);
    glDeleteBuffers(1, &terrain_mesh_.normal_vbo);
    glDeleteBuffers(1, &terrain_mesh_.element_vbo);
    glDeleteVertexArrays(1, &terrain_mesh_.vao);
}

void MyView::windowViewRender(tygra::Window * window)
{
    assert(scene_ != nullptr);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const float aspect_ratio = viewport[2] / (float)viewport[3];

    const auto& camera = scene_->getCamera();
    glm::mat4 projection_xform = glm::perspective(
        glm::radians(camera.getVerticalFieldOfViewInDegrees()),
        aspect_ratio,
        camera.getNearPlaneDistance(),
        camera.getFarPlaneDistance());
    glm::vec3 camera_pos = (const glm::vec3&)camera.getPosition();
    glm::vec3 camera_at = camera_pos + (const glm::vec3&)camera.getDirection();
    glm::vec3 world_up{ 0, 1, 0 };
    glm::mat4 view_xform = glm::lookAt(camera_pos, camera_at, world_up);


    glClearColor(0.f, 0.f, 0.25f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, shade_normals_ ? GL_FILL : GL_LINE);

    glUseProgram(terrain_sp_);

    GLuint shading_id = glGetUniformLocation(terrain_sp_, "use_normal");
    glUniform1i(shading_id, shade_normals_);


    /* TODO: you are free to modify any of the drawing code below */


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

    if (terrain_mesh_.vao) {
        glBindVertexArray(terrain_mesh_.vao);
        //glDrawElements(GL_TRIANGLES, terrain_mesh_.element_count,
        //             GL_UNSIGNED_INT, 0);

		for(TerrainPatch& p : MyTerrain.patches)
			glDrawElementsBaseVertex(GL_TRIANGLES, p.elementAmount, GL_UNSIGNED_INT, 0, p.elementOffset);
    }
}

void MyView::GenerateTesselatedGrid(TerrainData& terrainData, int subU, int subV, int sizeU, int sizeV, int patchSizeU, int patchSizeV)
{

	int subUSize = sizeU / subU;
	int subVSize = sizeV / subV;

	float halfX = 0.5f*sizeU;
	float halfZ = 0.5f*sizeV;

	float dx = sizeU / (subU - 1);
	float dz = sizeV / (subV - 1);

	//For speed to use multiplication in the for loop
	float oneOverSizeU = 1.0f / (float)(subU-1);
	float oneOverSizeV = 1.0f / (float)(subV-1);

	terrainData.vertecies.resize(subU*subV);
	terrainData.normals.resize(subU*subV);
	terrainData.UVCorrd.resize(subU*subV);
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

	terrainData.elementArray.reserve(patchSizeV*patchSizeU);
	for (int i = 0; i < patchSizeV; i++)
	{
		for (int i2 = 0; i2 < patchSizeU; i2++)
		{
				int row1 = i * subU + i2;
				int row2 = (i + 1) * subU + i2;

				terrainData.elementArray.push_back(row2);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2 + 1);


				terrainData.elementArray.push_back(row1);
				terrainData.elementArray.push_back(row1 + 1);
				terrainData.elementArray.push_back(row2);
		}
	}

	float amountPatchesU = (float)sizeU / (float)patchSizeU;
	int fullAmountU = glm::floor(amountPatchesU);
	float amountPatchesV = (float)sizeV / (float)patchSizeV;
	int fullAmountV = glm::floor(amountPatchesV);

	for(int z = 0; z < fullAmountV; z++)
	{
		for(int x = 0; x < fullAmountU; x++)
		{
			TerrainPatch p;
			p.elementAmount = terrainData.elementArray.size();
			p.elementOffset = (x * patchSizeU) + (z * patchSizeV);

			terrainData.patches.push_back(p);
		}
	}


	//Generate for all verecees
	/*for(int z = 0; z < subV-1; z++)
	{
		for (int x = 0; x < subU-1; x++)
		{
			int row1 = z * subU + x;
			int row2 = (z + 1) * subU + x;

			terrainData.elementArray.push_back(row2);
			terrainData.elementArray.push_back(row1 + 1);
			terrainData.elementArray.push_back(row2 + 1);


			terrainData.elementArray.push_back(row1);
			terrainData.elementArray.push_back(row1 + 1);
			terrainData.elementArray.push_back(row2);
		}
	}*/
}

void MyView::ApplyBezierSurface(TerrainData& terrainData, std::vector<std::vector<glm::vec3>>& bezier_patch)
{
	for (int i = 0; i < terrainData.vertecies.size(); i++)
	{
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
		terrainData.vertecies[i] = BezierSurface(bezier_patch, u, terrainData.UVCorrd[i].y, current_bezier_batch);
	}
	ComputeNormals(terrainData);
}

glm::vec3 MyView::BezierSurface(std::vector<std::vector<glm::vec3>>& bezier_patch, float U, float V, int startingBatch)
{
	std::vector<glm::vec3> final_curve;
	if (startingBatch < 1)
		startingBatch = 1;
	for (int i = 0; i < 4; i++)
	{
		int bezier_patch_index = i + (4 * (startingBatch - 1));
		final_curve.push_back(BezierCurve(bezier_patch[bezier_patch_index], U));
	}
	return BezierCurve(final_curve, V);
}

glm::vec3 MyView::BezierCurve(std::vector<glm::vec3>& control_points, float t)
{
	//TODO: Make bezier using pascal triangle so it creates a path for an amount of different size
	return (1-t) * (1 - t) * (1 - t) * control_points[0] +
		    3 * t * (1-t) * (1-t) * control_points[1] +
		    3 * t * t * (1-t) * control_points[2] +
			t * t * t * control_points[3];
}

glm::vec3 MyView::BezierCurveTangent(std::vector<glm::vec3>& control_points, float t)
{
	//TODO: Make bezier using pascal triangle so it creates a path for an amount of different size
	return  3 * (1 - t) * (1 - t) * (control_points[1] - control_points[0]) +
			6 * (1 - t) * t *  (control_points[2] - control_points[1]) + 
			3 * t * t * (control_points[3] - control_points[2]);
}

void MyView::ComputeNormals(TerrainData & data)
{
	for (glm::vec3& normal : data.normals)
		normal = glm::vec3(0, 0, 0);

	for (int i = 0; i < data.elementArray.size(); i += 3)
	{
		glm::vec3 u = data.vertecies[data.elementArray[i + 1]] - data.vertecies[data.elementArray[i]];
		glm::vec3 v = data.vertecies[data.elementArray[i + 2]] - data.vertecies[data.elementArray[i]];

		glm::vec3 temp_normal = glm::normalize(glm::cross(u, v));

		data.normals[data.elementArray[i]] += temp_normal;
		data.normals[data.elementArray[i + 1]] += temp_normal;
		data.normals[data.elementArray[i + 2]] += temp_normal;
	}

	for (glm::vec3& normal : data.normals)
		normal = glm::normalize(normal);
}

float MyView::PerlinNoise(int x, int y)
{
	int n = x + y * 5;
	n = (n << 13) ^ n;
	int nn = ((n*((n*n * 15731) + 789221) + 1376312589) & 0x7fffffff);
	return 1.0 -  ((float)nn / 1073741824.0f);
}

float MyView::FractionalBrownian(float x, float y, float gain, int octaves, int hgrid)
{
	float total = 0.0f;
	float frequency = 1.0 / (float)hgrid;
	float amplitude = gain;
	float lacunarity = 2.0;

	for (int i =0; i < octaves; i++)
	{
		total += PerlinNoise((int)x * frequency, (int)y * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= gain;
	}

	return total;
}

float MyView::CosineLerp(float a, float b, float x)
{
	float ft = x * 3.1415926535897932384626433832795f;
	float f = (1.0f - cos(ft)) * 0.5f;
	return a * (1.0f - f) + b * f;
}

float MyView::KenPerlin(float xPos, float zPos)
{
	float s = PerlinNoise((int)xPos, (int)zPos);
	float t = PerlinNoise((int)xPos + 1, (int)zPos);
	float u = PerlinNoise((int)xPos, (int)zPos + 1);
	float v = PerlinNoise((int)xPos + 1, (int)zPos + 1);

	float c1 = CosineLerp(s, t, xPos);
	float c2 = CosineLerp(u,v, xPos);

	return CosineLerp(c1, c2, zPos);
}
