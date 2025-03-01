#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D shadowMap;
uniform float near_plane;
uniform float far_plane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC (Normalized Device Coordinate)
    // 转换成线性值
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

float Linearize(float d) {
  return d * near_plane / (far_plane + d * (near_plane - far_plane));
}

void main()
{             
    float depthValue = texture(shadowMap, TexCoord).r;
    // FragColor = vec4(vec3(Linearize(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
}