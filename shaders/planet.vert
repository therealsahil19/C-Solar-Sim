#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceMatrix; // For instanced rendering

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 LocalPos; // Added for procedural UV mapping
flat out vec3 ModelYAxis; // Debug: Y column of model matrix


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
        // Optimized: For uniform scaling, normal matrix simplifies to normalized mat3(model)
        // This eliminates expensive inverse() calls per vertex
        Normal = normalize(mat3(finalModel) * aNormal);
    } else {
        Normal = normalMatrix * aNormal;
    }
    
    TexCoord = aTexCoord;
    LocalPos = aPos; // Pass local coordinates
    ModelYAxis = vec3(finalModel[1]); // Y column of model matrix
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
