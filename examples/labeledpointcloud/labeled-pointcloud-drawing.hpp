// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include "common/labeled-point-cloud-utilities.h"
#include "common/float3.h"


// Handles all the OpenGL calls needed to display the labeled point cloud
void draw_labeled_pointcloud(float width, float height, glfw_state& app_state, 
    const rs2::vertex* vertices, const uint8_t* labels, size_t vertices_count)
{
    if( vertices_count == 0 || ! vertices || ! labels )
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
    for (int i = 0; i < vertices_count; ++i)
    {
        // Set the vertex color from the label value
        auto label = labels[i];
        auto color = label_to_color3f[static_cast<rs2_point_cloud_label>(label)];
        glColor3f(color.x, color.y, color.z);

        // Draw the vertex
        // Note: LPC is in Robot coordinate system and not optical coordinate system.
        // Normally we need to use the extrinsic to do this conversion.
        // Currently in this example we use the conversion hard-coded
        rs2::vertex vtx = { -vertices[i].y, vertices[i].z, -vertices[i].x };
        glVertex3fv(std::move(vtx));
    }

    // OpenGL cleanup
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}
