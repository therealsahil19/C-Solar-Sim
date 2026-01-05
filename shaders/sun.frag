#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform bool useTexture;
uniform sampler2D planetTexture;
uniform vec3 objectColor;
uniform float glowIntensity;

void main()
{
    vec3 baseColor;
    if (useTexture) {
        baseColor = texture(planetTexture, TexCoord).rgb;
    } else {
        baseColor = objectColor;
    }
    
    // Sun is emissive - no lighting calculation, just brighten
    vec3 emissive = baseColor * glowIntensity;
    FragColor = vec4(emissive, 1.0);
}
