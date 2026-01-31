#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform int hasAnimation;

void main()
{
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    
    if (hasAnimation == 1) {
        for(int i = 0; i < 4; i++) {
            if(aBoneIds[i] == -1) 
                continue;
            if(aBoneIds[i] >= MAX_BONES) {
                totalPosition = vec4(aPos, 1.0);
                break;
            }
            vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0);
            totalPosition += localPosition * aWeights[i];
            vec3 localNormal = mat3(finalBonesMatrices[aBoneIds[i]]) * aNormal;
            totalNormal += localNormal * aWeights[i];
        }
    } else {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }
    
    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * model * totalPosition;
}
