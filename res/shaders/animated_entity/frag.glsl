#version 410 core
#include "../common/lights.glsl"

in VertexOut {
    // changed lighting_result to ws_position and ws_normal for task g
    vec3 ws_position;
    vec3 ws_normal;
    vec2 texture_coordinate;
} frag_in;

layout(location = 0) out vec4 out_colour;

// Global Data
uniform float inverse_gamma;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;

// Material properties from vert.glsl for task g
uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;

// Light Data from vert.glsl for task g
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif

// Global data from vert.glsl for task g
uniform vec3 ws_view_position;

void main() {
    // Per fragment lighting calculations from vert.glsl for task g
    vec3 ws_view_dir = normalize(ws_view_position - frag_in.ws_position);
    LightCalculatioData light_calculation_data = LightCalculatioData(frag_in.ws_position, ws_view_dir, normalize(frag_in.ws_normal));
    Material material = Material(diffuse_tint, specular_tint, ambient_tint, shininess);
    
    LightingResult lighting_result = total_light_calculation(light_calculation_data, material
        #if NUM_PL > 0
        ,point_lights
        #endif
    );
    
    // Resolve the per fragment lighting with texture sampling, changed frag_in.lighting_result to lighting_result for task g
    vec3 resolved_lighting = resolve_textured_light_calculation(lighting_result, diffuse_texture, specular_map_texture, frag_in.texture_coordinate);
    out_colour = vec4(resolved_lighting, 1.0f);
    out_colour.rgb = pow(out_colour.rgb, vec3(inverse_gamma));
}
