#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;       // Sun position (usually origin)
uniform vec3 viewPos;        // Camera position
uniform vec3 objectColor;    // Fallback color if no texture
uniform bool useTexture;
uniform sampler2D planetTexture;

// Lighting parameters
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;

void main()
{
    // Get base color from texture or uniform
    vec3 baseColor;
    if (useTexture) {
        baseColor = texture(planetTexture, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }
    
    // Ambient lighting
    vec3 ambient = ambientStrength * baseColor;
    
    // Diffuse lighting (Lambertian)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * baseColor;
    
    // Specular lighting (Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    // Combine lighting components
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
