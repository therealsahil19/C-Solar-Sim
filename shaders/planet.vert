#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceMatrix; // For instanced rendering

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform bool isInstanced;

void main()
{
    mat4 finalModel = isInstanced ? aInstanceMatrix : model;
    FragPos = vec3(finalModel * vec4(aPos, 1.0));
    
    if (isInstanced) {
        Normal = mat3(transpose(inverse(finalModel))) * aNormal;
    } else {
        Normal = normalMatrix * aNormal;
    }
    
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
