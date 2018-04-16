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

    void windowViewWillStart(tygra::Window * window) override;

    void windowViewDidReset(tygra::Window * window,
                            int width,
                            int height) override;

    void windowViewDidStop(tygra::Window * window) override;

    void windowViewRender(tygra::Window * window) override;

private:

    const scene::Context * scene_{ nullptr };

    GLuint terrain_sp_{ 0 };

    bool shade_normals_{ false };

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
