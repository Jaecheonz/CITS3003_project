#version 410 core
#include "../common/lights.glsl"
#include "../common/maths.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in vec4 bone_weights;
layout(location = 4) in uvec4 bone_indices;

out VertexOut {
    // changed lighting_result to ws_position and ws_normal for task g
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} vertex_out;

// Per instance data
uniform mat4 model_matrix;

// moved material properties to frag.glsl for task g

// added for task e
uniform vec2 texture_scale;

// moved light data to frag.glsl for task g

// Animation Data
uniform mat4 bone_transforms[BONE_TRANSFORMS];

// Global data
// moved ws_view_position to frag.glsl for task g
uniform mat4 projection_view_matrix;

// removed specular_map_texture for task g

void main() {
    // Transform vertices
    float sum = dot(bone_weights, vec4(1.0f));

    mat4 bone_transform =
        bone_weights[0] * bone_transforms[bone_indices[0]]
        + bone_weights[1] * bone_transforms[bone_indices[1]]
        + bone_weights[2] * bone_transforms[bone_indices[2]]
        + bone_weights[3] * bone_transforms[bone_indices[3]]
        + (1.0f - sum) * mat4(1.0f);

    mat4 animation_matrix = model_matrix * bone_transform;
    mat3 normal_matrix = cofactor(animation_matrix);
    
    // changed variable type to out to pass the world space position and normal to frag.glsl for task g
    vertex_out.ws_position = (animation_matrix * vec4(vertex_position, 1.0f)).xyz;
    vertex_out.ws_normal = normal_matrix * normal;
    
    // modified for task e
    vertex_out.texture_coordinate = texture_coordinate * texture_scale;

    // moved light calcs to frag.glsl for task g
    gl_Position = projection_view_matrix * vec4(vertex_out.ws_position, 1.0f);
}
