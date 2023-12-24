// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include "common/labeled-point-cloud-utilities.h"
#include "common/float3.h"


// Handles all the OpenGL calls needed to display the labeled point cloud
void draw_labeled_pointcloud(float width, float height, glfw_state& app_state, 
    const std::vector<rs2::vertex> vertices_vec, const std::vector<uint8_t> labels_vec)
{
    if (vertices_vec.size() == 0 || labels_vec.size() == 0)
        return;

    // OpenGL commands that prep screen for the pointcloud
    glLoadIdentity();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluPerspective(60, width / height, 0.01f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    gluLookAt(0, 0, 0, 0, 0, 1, 0, 1, 0);

    glTranslatef(0, 0, +0.5f + app_state.offset_y * 0.05f);
    glRotated(app_state.pitch, 1, 0, 0);
    glRotated(app_state.yaw, 0, 1, 0);
    glTranslatef(0, 0, -0.5f);

    glPointSize(width / 640);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, app_state.tex.get_gl_handle());
    float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
    glBegin(GL_POINTS);

    auto label_to_color3f = rs2::labeled_point_cloud_utilities::get_label_to_color3f();
    /* this segment actually renders the labeled pointcloud */
    for (int i = 0; i < vertices_vec.size(); ++i)
    {
        auto label = labels_vec[i];
        auto vertice = vertices_vec[i];
        auto color = label_to_color3f[static_cast<rs2_point_cloud_label>(label)];
        GLfloat vert[3] = {-vertice.y, vertice.z, -vertice.x};
        glVertex3fv(vert);
        glColor3f(color.x, color.y, color.z);
    }

    // OpenGL cleanup
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}
