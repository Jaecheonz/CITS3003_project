#version 410 core
#include "../common/lights.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

out VertexOut {
    // changed lighting_result to ws_position and ws_normal for task g
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} vertex_out;

// Per instance data
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

// moved material properties variable declarations to frag.glsl for task g

// added for task e
uniform vec2 texture_scale;

// moved Light Data segment to frag.glsl for task g

// Global data
// removed ws_view_position for task g
uniform mat4 projection_view_matrix;

void main() {
    // Transform vertices
    // changed data type to output for task g
    vertex_out.ws_position = (model_matrix * vec4(vertex_position, 1.0f)).xyz;
    vertex_out.ws_normal = normal_matrix * normal;
    
    // modified for task e
    vertex_out.texture_coordinate = texture_coordinate * texture_scale;

    // moved lighting calculations to frag.glsl

    gl_Position = projection_view_matrix * vec4(vertex_out.ws_position, 1.0f);
}
