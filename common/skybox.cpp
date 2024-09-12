// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "skybox.h"
#include "rendering.h"

#include <stb_image.h>

#include "res/nx.h"
#include "res/ny.h"
#include "res/nz.h"
#include "res/px.h"
#include "res/py.h"
#include "res/pz.h"

skybox::skybox()
{
    plus_x = nullptr;
    plus_y = nullptr;
    plus_z = nullptr;

    minus_x = nullptr;
    minus_y = nullptr;
    minus_z = nullptr;
}

void skybox::render(rs2::float3 cam_position)
{
    auto init = [](const uint8_t* buff, int length, std::shared_ptr<rs2::texture_buffer>& tex){
        if (tex) tex.reset();
        tex = std::make_shared<rs2::texture_buffer>();
        int x, y, comp;
        auto data = stbi_load_from_memory(buff, length, &x, &y, &comp, 3);
        tex->upload_image(x, y, data, GL_RGB);
        stbi_image_free(data);
    };
    if (!initialized)
    {
        init(cubemap_pz_png_data, cubemap_pz_png_size, plus_z);
        init(cubemap_nz_png_data, cubemap_nz_png_size, minus_z);
        init(cubemap_nx_png_data, cubemap_nx_png_size, minus_x);
        init(cubemap_px_png_data, cubemap_px_png_size, plus_x);
        init(cubemap_py_png_data, cubemap_py_png_size, plus_y);
        init(cubemap_ny_png_data, cubemap_ny_png_size, minus_y);
        initialized = true;
    }
    glEnable(GL_TEXTURE_2D);

    glColor3f(1.f, 1.f, 1.f);

    const auto r = 50.f;
    const auto re = 49.999f;

    glTranslatef(cam_position.x, cam_position.y, cam_position.z);

    glBindTexture(GL_TEXTURE_2D, plus_z->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.f, 1.f); glVertex3f(-r, r, re);
        glTexCoord2f(0.f, 0.f); glVertex3f(-r, -r, re);
        glTexCoord2f(1.f, 1.f); glVertex3f(r, r, re);
        glTexCoord2f(1.f, 0.f); glVertex3f(r, -r, re);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, minus_z->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(1.f, 1.f); glVertex3f(-r, r, -re);
        glTexCoord2f(1.f, 0.f); glVertex3f(-r, -r, -re);
        glTexCoord2f(0.f, 1.f); glVertex3f(r, r, -re);
        glTexCoord2f(0.f, 0.f); glVertex3f(r, -r, -re);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, minus_x->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(1.f, 0.f); glVertex3f(-re, -r, r);
        glTexCoord2f(0.f, 0.f); glVertex3f(-re, -r, -r);
        glTexCoord2f(1.f, 1.f); glVertex3f(-re, r, r);
        glTexCoord2f(0.f, 1.f); glVertex3f(-re, r, -r);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, plus_x->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.f, 0.f); glVertex3f(re, -r, r);
        glTexCoord2f(1.f, 0.f); glVertex3f(re, -r, -r);
        glTexCoord2f(0.f, 1.f); glVertex3f(re, r, r);
        glTexCoord2f(1.f, 1.f); glVertex3f(re, r, -r);
    }
    glEnd();


    glBindTexture(GL_TEXTURE_2D, minus_y->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.f, 0.f); glVertex3f(-r, re, r);
        glTexCoord2f(0.f, 1.f); glVertex3f(-r, re, -r);
        glTexCoord2f(1.f, 0.f); glVertex3f(r,  re, r);
        glTexCoord2f(1.f, 1.f); glVertex3f(r,  re, -r);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, plus_y->get_gl_handle());
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.f, 1.f); glVertex3f(-r, -re, r);
        glTexCoord2f(0.f, 0.f); glVertex3f(-r, -re, -r);
        glTexCoord2f(1.f, 1.f); glVertex3f(r,  -re, r);
        glTexCoord2f(1.f, 0.f); glVertex3f(r,  -re, -r);
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_TEXTURE_2D);

    glTranslatef(-cam_position.x, -cam_position.y, -cam_position.z);
}