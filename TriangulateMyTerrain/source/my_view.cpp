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

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<GLuint> elements;
	
	int N = 3;
	for(int x=0; x <= N; x++)
	{
		for(int z=0; z <= N; z++)
		{
			positions.push_back(glm::vec3(((float)-z / (float)N), 0, ((float)x / (float)N)));
			normals.push_back(glm::vec3(0,1,0));
		}
	}

	//index 0 = row1+z
	//index 1 = row1+z+1
	//index 3 = row2+z
	//index 2 = row2+z+1
	// *------* // First * -> index 3, second * -> index 2
	// |      |
	// |      |
	// *------* // First * -> index 0, Second * -> index 1
	for (int x = 0; x < N; x++)
	{
		for (int z = 0; z < N; z++)
		{
			int row1 = x * (N + 1);
			int row2 = (x + 1) * (N + 1);

			if ((z % 2 != 0 && x % 2 != 0) || (x % 2 == 0 && z % 2 == 0))
			{
				elements.push_back(row1 + z);
				elements.push_back(row1 + z + 1);
				elements.push_back(row2 + z);

				elements.push_back(row1 + z + 1);
				elements.push_back(row2 + z + 1);
				elements.push_back(row2 + z);
			}
			else
			{
				elements.push_back(row1 + z);
				elements.push_back(row1 + z + 1);
				elements.push_back(row2 + z + 1);

				elements.push_back(row1 + z);
				elements.push_back(row2 + z + 1);
				elements.push_back(row2 + z);
			}
		}
	}


    /*std::vector<glm::vec3> positions = { { 0, 0, 0 }, { sizeX, 0, 0 },
                                         {sizeX, 0, -sizeZ}, { 0, 0, -sizeZ} };
    std::vector<glm::vec3> normals = { { 0, 1, 0 }, { 0, 1, 0 },
                                       { 0, 1, 0 }, { 0, 1, 0 } };
    std::vector<GLuint> elements = { 0, 1, 2, 0, 2, 3 };*/


    // below is indicative code for initialising a terrain VAO.

    glGenBuffers(1, &terrain_mesh_.element_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain_mesh_.element_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        elements.size() * sizeof(GLuint),
        elements.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    terrain_mesh_.element_count = elements.size();

    glGenBuffers(1, &terrain_mesh_.position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.position_vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                 positions.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &terrain_mesh_.normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrain_mesh_.normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3),
                 normals.data(), GL_STATIC_DRAW);
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
        glDrawElements(GL_TRIANGLES, terrain_mesh_.element_count,
                       GL_UNSIGNED_INT, 0);
    }
}
