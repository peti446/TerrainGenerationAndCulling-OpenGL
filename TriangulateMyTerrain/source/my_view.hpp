#pragma once

#include <scene/context.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <glm/glm.hpp>

class MyView : public tygra::WindowViewDelegate
{
public:

    MyView();

    ~MyView();

    void setScene(const scene::Context * scene);

    void toggleShading();

private:

	//Need to declare it before the function declarations
	struct TerrainPatch
	{
		GLuint elementOffset;
		GLuint elementAmount;
		glm::vec2 maxCoord;
		glm::vec2 minCoord;
	};
	struct TerrainData
	{
		std::vector<glm::vec3> vertecies;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> UVCorrd;
		std::vector<GLuint> elementArray;
		std::vector<TerrainPatch> patches;
	};


    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;


	void GenerateTesselatedGrid(TerrainData& terrainData, int subU, int subV, int sizeU, int sizeV, int patchSizeU, int patchSizeV);

	void ApplyBezierSurface(TerrainData& terrainData, std::vector<std::vector<glm::vec3>>& bezier_patch);

	glm::vec3 BezierSurface(std::vector<std::vector<glm::vec3>>& bezier_patch, float U, float V, int startingBatch = 1);

	glm::vec3 BezierCurve(std::vector<glm::vec3>& control_points, float t);

	glm::vec3 BezierCurveTangent(std::vector<glm::vec3>& control_points, float t);

	void ComputeNormals(TerrainData& data);

	float PerlinNoise(int x, int y);
	
	float FractionalBrownian(float x, float y, float gain, int octaves, int hgrid);

	float CosineLerp(float a, float b, float x);

	float KenPerlin(float xPos, float zPos);
private:

    const scene::Context * scene_{ nullptr };

    GLuint terrain_sp_{ 0 };

    bool shade_normals_{ false };

	TerrainData MyTerrain;

    struct MeshGL
    {
        GLuint position_vbo{ 0 };
        GLuint normal_vbo{ 0 };
        GLuint element_vbo{ 0 };
        GLuint vao{ 0 };
        int element_count{ 0 };
    };
    MeshGL terrain_mesh_;

    enum
    {
        kVertexPosition = 0,
        kVertexNormal = 1,
    };

};
