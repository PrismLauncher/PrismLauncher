// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
// https://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/cube/vshader.glsl

// Dylan Schooner - 2025
// Modification: Implemented final Z-NDC re-inversion to compensate
// for rigid OpenGL 2.0 context forcing glClearDepth(1.0).
// This flips the high-precision Reverse Z output to the standard [0, W] range.

#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform mat4 mvp_matrix;
uniform mat4 model_matrix;

attribute vec4 a_position;
attribute vec2 a_texcoord;

varying vec2 v_texcoord;

void main()
{
    // Calculate vertex position in screen space
    gl_Position = mvp_matrix * model_matrix * a_position;

    // Invert the z component of our Reverse Z matrix back to standard NDC
    float near_z  = gl_Position.z;
    float w_c     = gl_Position.w;
    gl_Position.z = w_c - near_z;

    // Pass texture coordinate to fragment shader
    // Value will be automatically interpolated to fragments inside polygon faces
    v_texcoord = a_texcoord;
}
