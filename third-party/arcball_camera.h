#ifndef ARCBALL_CAMERA_H
#define ARCBALL_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Flags for tweaking the view matrix
#define ARCBALL_CAMERA_LEFT_HANDED_BIT 1

// * eye:
//     * Current eye position. Will be updated to new eye position.
// * target:
//     * Current look target position. Will be updated to new position.
// * up:
//     * Camera's "up" direction. Will be updated to new up vector.
// * view (optional):
//     * The matrix that will be updated with the new view transform. Previous contents don't matter.
// * delta_time_seconds:
//     * Amount of seconds passed since last update.
// * zoom_per_tick:
//     * How much the camera should zoom with every scroll wheel tick.
// * pan_speed:
//     * How fast the camera should pan when holding middle click.
// * rotation_multiplier:
//     * For amplifying the rotation speed. 1.0 means 1-1 mapping between arcball rotation and camera rotation.
// * screen_width/screen_height:
//     * Dimensions of the screen the camera is being used in (the window size).
// * x0, x1:
//     * Previous and current x coordinate of the mouse, respectively.
// * y0, y1:
//     * Previous and current y coordinate of the mouse, respectively.
// * midclick_held:
//     * Whether the middle click button is currently held or not.
// * rclick_held:
//     * Whether the right click button is currently held or not.
// * delta_scroll_ticks:
//     * How many scroll wheel ticks passed since the last update (signed number)
// * flags:
//     * For producing a different view matrix depending on your conventions.
void arcball_camera_update(
    float eye[3],
    float target[3],
    float up[3],
    float view[16],
    float delta_time_seconds,
    float zoom_per_tick,
    float pan_speed,
    float rotation_multiplier,
    int screen_width, int screen_height,
    int x0, int x1,
    int y0, int y1,
    int midclick_held,
    int rclick_held,
    int delta_scroll_ticks,
    unsigned int flags);

// Utility for producing a look-to matrix without having to update a camera.
void arcball_camera_look_to(
    const float eye[3],
    const float look[3],
    const float up[3],
    float view[16],
    unsigned int flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ARCBALL_CAMERA_H

#ifdef ARCBALL_CAMERA_IMPLEMENTATION

#include <math.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void arcball_camera_update(
    float eye[3],
    float target[3],
    float up[3],
    float view[16],
    float delta_time_seconds,
    float zoom_per_tick,
    float pan_speed,
    float rotation_multiplier,
    int screen_width, int screen_height,
    int px_x0, int px_x1,
    int px_y0, int px_y1,
    int midclick_held,
    int rclick_held,
    int delta_scroll_ticks,
    unsigned int flags)
{
    // check preconditions
    {
        float up_len = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
        assert(fabsf(up_len - 1.0f) < 0.000001f);

        float to_target[3] = {
            target[0] - eye[0],
            target[1] - eye[1],
            target[2] - eye[2],
        };
        float to_target_len = sqrtf(to_target[0] * to_target[0] + to_target[1] * to_target[1] + to_target[2] * to_target[2]);
        assert(to_target_len > 1e-6);
    }

    // right click is held, then mouse movements implement rotation.
    if (rclick_held)
    {
        float x0 = (float)(px_x0 - screen_width / 2);
        float x1 = (float)(px_x1 - screen_width / 2);
        float y0 = (float)((screen_height - px_y0 - 1) - screen_height / 2);
        float y1 = (float)((screen_height - px_y1 - 1) - screen_height / 2);
        float arcball_radius = (float)(screen_width > screen_height ? screen_width : screen_height);

        // distances to center of arcball
        float dist0 = sqrtf(x0 * x0 + y0 * y0);
        float dist1 = sqrtf(x1 * x1 + y1 * y1);

        float z0;
        if (dist0 > arcball_radius)
        {
            // initial click was not on the arcball, so just do nothing.
            goto end_rotate;
        }
        else
        {
            // compute depth of intersection using good old pythagoras
            z0 = sqrtf(arcball_radius * arcball_radius - x0 * x0 - y0 * y0);
        }

        float z1;
        if (dist1 > arcball_radius)
        {
            // started inside the ball but went outside, so clamp it.
            x1 = (x1 / dist1) * arcball_radius;
            y1 = (y1 / dist1) * arcball_radius;
            dist1 = arcball_radius;
            z1 = 0.0f;
        }
        else
        {
            // compute depth of intersection using good old pythagoras
            z1 = sqrtf(arcball_radius * arcball_radius - x1 * x1 - y1 * y1);
        }

        // rotate intersection points according to where the eye is
        {
            float to_eye_unorm[3] = {
                eye[0] - target[0],
                eye[1] - target[1],
                eye[2] - target[2]
            };
            float to_eye_len = sqrtf(to_eye_unorm[0] * to_eye_unorm[0] + to_eye_unorm[1] * to_eye_unorm[1] + to_eye_unorm[2] * to_eye_unorm[2]);
            float to_eye[3] = {
                to_eye_unorm[0] / to_eye_len,
                to_eye_unorm[1] / to_eye_len,
                to_eye_unorm[2] / to_eye_len
            };

            float across[3] = {
                -(to_eye[1] * up[2] - to_eye[2] * up[1]),
                -(to_eye[2] * up[0] - to_eye[0] * up[2]),
                -(to_eye[0] * up[1] - to_eye[1] * up[0])
            };

            // matrix that transforms standard coordinates to be relative to the eye
            float eye_m[9] = {
                across[0], across[1], across[2],
                up[0], up[1], up[2],
                to_eye[0], to_eye[1], to_eye[2]
            };

            float new_p0[3] = {
                eye_m[0] * x0 + eye_m[3] * y0 + eye_m[6] * z0,
                eye_m[1] * x0 + eye_m[4] * y0 + eye_m[7] * z0,
                eye_m[2] * x0 + eye_m[5] * y0 + eye_m[8] * z0,
            };

            float new_p1[3] = {
                eye_m[0] * x1 + eye_m[3] * y1 + eye_m[6] * z1,
                eye_m[1] * x1 + eye_m[4] * y1 + eye_m[7] * z1,
                eye_m[2] * x1 + eye_m[5] * y1 + eye_m[8] * z1,
            };

            x0 = new_p0[0];
            y0 = new_p0[1];
            z0 = new_p0[2];

            x1 = new_p1[0];
            y1 = new_p1[1];
            z1 = new_p1[2];
        }

        // compute quaternion between the two vectors (http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final)
        float qw, qx, qy, qz;
        {
            float norm_u_norm_v = sqrtf((x0 * x0 + y0 * y0 + z0 * z0) * (x1 * x1 + y1 * y1 + z1 * z1));
            qw = norm_u_norm_v + (x0 * x1 + y0 * y1 + z0 * z1);

            if (qw < 1.e-6f * norm_u_norm_v)
            {
                /* If u and v are exactly opposite, rotate 180 degrees
                 * around an arbitrary orthogonal axis. Axis normalisation
                 * can happen later, when we normalise the quaternion. */
                qw = 0.0f;
                if (fabsf(x0) > fabsf(z0))
                {
                    qx = -y0;
                    qy = x0;
                    qz = 0.0f;
                }
                else
                {
                    qx = 0.0f;
                    qy = -z0;
                    qz = y0;
                }
            }
            else
            {
                /* Otherwise, build quaternion the standard way. */
                qx = y0 * z1 - z0 * y1;
                qy = z0 * x1 - x0 * z1;
                qz = x0 * y1 - y0 * x1;
            }

            float q_len = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
            qx /= q_len;
            qy /= q_len;
            qz /= q_len;
            qw /= q_len;
        }

        // amplify the quaternion's rotation by the multiplier
        // this is done by slerp(Quaternion.identity, q, multiplier)
        // math from http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
        {
            // cos(angle) of the quaternion
            float c = qw;
            if (c > 0.9995f)
            {
                // if the angle is small, linearly interpolate and normalize.
                qx = rotation_multiplier * qx;
                qy = rotation_multiplier * qy;
                qz = rotation_multiplier * qz;
                qw = 1.0f + rotation_multiplier * (qw - 1.0f);
                float q_len = sqrtf(qx * qx + qy * qy + qz * qz + qw * qw);
                qx /= q_len;
                qy /= q_len;
                qz /= q_len;
                qw /= q_len;
            }
            else
            {
                // clamp to domain of acos for robustness
                if (c < -1.0f)
                    c = -1.0f;
                else if (c > 1.0f)
                    c = 1.0f;
                // angle of the initial rotation
                float theta_0 = acosf(c);
                // apply multiplier to rotation
                float theta = theta_0 * rotation_multiplier;

                // compute the quaternion normalized difference
                float qx2 = qx;
                float qy2 = qy;
                float qz2 = qz;
                float qw2 = qw - c;
                float q2_len = sqrtf(qx2 * qx2 + qy2 * qy2 + qz2 * qz2 + qw2 * qw2);
                qx2 /= q2_len;
                qy2 /= q2_len;
                qz2 /= q2_len;
                qw2 /= q2_len;

                // do the slerp
                qx = qx2 * sinf(theta);
                qy = qy2 * sinf(theta);
                qz = qz2 * sinf(theta);
                qw = cosf(theta) + qw2 * sinf(theta);
            }
        }

        // vector from the target to the eye, which will be rotated according to the arcball's arc.
        float to_eye[3] = { eye[0] - target[0], eye[1] - target[1], eye[2] - target[2] };

        // convert quaternion to matrix (note: row major)
        float qmat[9] = {
            (1.0f - 2.0f * qy * qy - 2.0f * qz * qz), 2.0f * (qx * qy + qw * qz), 2.0f * (qx * qz - qw * qy),
            2.0f * (qx * qy - qw * qz), (1.0f - 2.0f * qx * qx - 2.0f * qz * qz), 2.0f * (qy * qz + qw * qx),
            2.0f * (qx * qz + qw * qy), 2.0f * (qy * qz - qw * qx), (1.0f - 2.0f * qx * qx - 2.0f * qy * qy)
        };

        // compute rotated vector
        float to_eye2[3] = {
            to_eye[0] * qmat[0] + to_eye[1] * qmat[1] + to_eye[2] * qmat[2],
            to_eye[0] * qmat[3] + to_eye[1] * qmat[4] + to_eye[2] * qmat[5],
            to_eye[0] * qmat[6] + to_eye[1] * qmat[7] + to_eye[2] * qmat[8]
        };

        // compute rotated up vector
        float up2[3] = {
            up[0] * qmat[0] + up[1] * qmat[1] + up[2] * qmat[2],
            up[0] * qmat[3] + up[1] * qmat[4] + up[2] * qmat[5],
            up[0] * qmat[6] + up[1] * qmat[7] + up[2] * qmat[8]
        };

        float up2_len = sqrtf(up2[0] * up2[0] + up2[1] * up2[1] + up2[2] * up2[2]);
        up2[0] /= up2_len;
        up2[1] /= up2_len;
        up2[2] /= up2_len;

        // update eye position
        eye[0] = target[0] + to_eye2[0];
        eye[1] = target[1] + to_eye2[1];
        eye[2] = target[2] + to_eye2[2];

        // update up vector
        up[0] = up2[0];
        up[1] = up2[1];
        up[2] = up2[2];
    }
end_rotate:

    // if midclick is held, then mouse movements implement translation
    if (midclick_held)
    {
        int dx = -(px_x0 - px_x1);
        int dy = (px_y0 - px_y1);

        float to_eye_unorm[3] = {
            eye[0] - target[0],
            eye[1] - target[1],
            eye[2] - target[2]
        };
        float to_eye_len = sqrtf(to_eye_unorm[0] * to_eye_unorm[0] + to_eye_unorm[1] * to_eye_unorm[1] + to_eye_unorm[2] * to_eye_unorm[2]);
        float to_eye[3] = {
            to_eye_unorm[0] / to_eye_len,
            to_eye_unorm[1] / to_eye_len,
            to_eye_unorm[2] / to_eye_len
        };

        float across[3] = {
            -(to_eye[1] * up[2] - to_eye[2] * up[1]),
            -(to_eye[2] * up[0] - to_eye[0] * up[2]),
            -(to_eye[0] * up[1] - to_eye[1] * up[0])
        };

        float pan_delta[3] = {
            delta_time_seconds * pan_speed * (dx * across[0] + dy * up[0]),
            delta_time_seconds * pan_speed * (dx * across[1] + dy * up[1]),
            delta_time_seconds * pan_speed * (dx * across[2] + dy * up[2]),
        };

        eye[0] += pan_delta[0];
        eye[1] += pan_delta[1];
        eye[2] += pan_delta[2];
        
        target[0] += pan_delta[0];
        target[1] += pan_delta[1];
        target[2] += pan_delta[2];
    }

    // compute how much scrolling happened
    float zoom_dist = zoom_per_tick * delta_scroll_ticks;

    // the direction that the eye will move when zoomed
    float to_target[3] = {
        target[0] - eye[0],
        target[1] - eye[1],
        target[2] - eye[2],
    };

    float to_target_len = sqrtf(to_target[0] * to_target[0] + to_target[1] * to_target[1] + to_target[2] * to_target[2]);

    // if the zoom would get you too close, clamp it.
    if (!rclick_held)
    {
        if (zoom_dist >= to_target_len - 0.00001f)
        {
            zoom_dist = to_target_len - 0.00001f;
        }
    }

    // normalize the zoom direction
    float look[3] = {
        to_target[0] / to_target_len,
        to_target[1] / to_target_len,
        to_target[2] / to_target_len,
    };

    float eye_zoom[3] = {
        look[0] * zoom_dist,
        look[1] * zoom_dist,
        look[2] * zoom_dist
    };

    eye[0] += eye_zoom[0];
    eye[1] += eye_zoom[1];
    eye[2] += eye_zoom[2];

    if (rclick_held)
    {
        // affect target too if right click is held
        // this allows you to move forward and backward (as opposed to zoom)
        target[0] += eye_zoom[0];
        target[1] += eye_zoom[1];
        target[2] += eye_zoom[2];
    }

    arcball_camera_look_to(eye, look, up, view, flags);
}

void arcball_camera_look_to(
    const float eye[3],
    const float look[3],
    const float up[3],
    float view[16],
    unsigned int flags)
{
    if (!view)
        return;
        
    float look_len = sqrtf(look[0] * look[0] + look[1] * look[1] + look[2] * look[2]);
    float up_len = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);

    assert(fabsf(look_len - 1.0f) < 0.000001f);
    assert(fabsf(up_len - 1.0f) < 0.000001f);

    // up'' = normalize(up)
    float up_norm[3] = { up[0] / up_len, up[1] / up_len, up[2] / up_len };

    // f = normalize(look)
    float f[3] = { look[0] / look_len, look[1] / look_len, look[2] / look_len };

    // s = normalize(cross(f, up2))
    float s[3] = {
        f[1] * up_norm[2] - f[2] * up_norm[1],
        f[2] * up_norm[0] - f[0] * up_norm[2],
        f[0] * up_norm[1] - f[1] * up_norm[0]
    };
    float s_len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] /= s_len;
    s[1] /= s_len;
    s[2] /= s_len;

    // u = normalize(cross(normalize(s), f))
    float u[3] = {
        s[1] * f[2] - s[2] * f[1],
        s[2] * f[0] - s[0] * f[2],
        s[0] * f[1] - s[1] * f[0]
    };
    float u_len = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    u[0] /= u_len;
    u[1] /= u_len;
    u[2] /= u_len;

    if (!(flags & ARCBALL_CAMERA_LEFT_HANDED_BIT))
    {
        // in a right-handed coordinate system, the camera's z looks away from the look direction.
        // this gets flipped again later when you multiply by a right-handed projection matrix
        // (notice the last row of gluPerspective, which makes it back into a left-handed system after perspective division)
        f[0] = -f[0];
        f[1] = -f[1];
        f[2] = -f[2];
    }

    // t = [s;u;f] * -eye
    float t[3] = {
        s[0] * -eye[0] + s[1] * -eye[1] + s[2] * -eye[2],
        u[0] * -eye[0] + u[1] * -eye[1] + u[2] * -eye[2],
        f[0] * -eye[0] + f[1] * -eye[1] + f[2] * -eye[2]
    };

    // m = [s,t[0]; u,t[1]; -f,t[2]];
    view[0] = s[0];
    view[1] = u[0];
    view[2] = f[0];
    view[3] = 0.0f;
    view[4] = s[1];
    view[5] = u[1];
    view[6] = f[1];
    view[7] = 0.0f;
    view[8] = s[2];
    view[9] = u[2];
    view[10] = f[2];
    view[11] = 0.0f;
    view[12] = t[0];
    view[13] = t[1];
    view[14] = t[2];
    view[15] = 1.0f;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ARCBALL_CAMERA_IMPLEMENTATION
