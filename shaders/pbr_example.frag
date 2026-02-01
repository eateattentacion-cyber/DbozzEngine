#version 330 core

// Example PBR Fragment Shader for DabozzEngine
// Shows how to use the PBR lighting functions

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Material properties
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao; // Ambient occlusion

// Textures (optional)
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform bool useTextures;

// Lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform int numLights;

// Camera
uniform vec3 camPos;

// Include PBR functions
#define M_PI 3.14159265359

float D_GGX(float NdotH, float roughness) {
    float a = NdotH * roughness;
    float k = roughness / (1.0 - NdotH * NdotH + a * a);
    float d = k * k * (1.0 / M_PI);
    return clamp(d, 0.0, 1.0);
}

float V_GGX(float NdotL, float NdotV, float alpha) {
    return 0.5 / mix(2.0 * NdotL * NdotV, NdotL + NdotV, alpha);
}

float SchlickFresnel(float u) {
    float m = 1.0 - u;
    float m2 = m * m;
    return m2 * m2 * m;
}

vec3 F0(float metallic, float specular, vec3 albedo) {
    float dielectric = 0.16 * specular * specular;
    return mix(vec3(dielectric), albedo, metallic);
}

float DiffuseBurley(float NdotL, float NdotV, float LdotH, float roughness) {
    float FD90_minus_1 = 2.0 * LdotH * LdotH * roughness - 0.5;
    float FdV = 1.0 + FD90_minus_1 * SchlickFresnel(NdotV);
    float FdL = 1.0 + FD90_minus_1 * SchlickFresnel(NdotL);
    return (1.0 / M_PI) * FdV * FdL;
}

void ComputePBRLight(vec3 N, vec3 L, vec3 V, vec3 lightColor, float attenuation,
                     vec3 albedo, float roughness, float metallic, float specular,
                     inout vec3 diffuseLight, inout vec3 specularLight) {
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    
    vec3 H = normalize(V + L);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    
    vec3 f0 = F0(metallic, specular, albedo);
    
    if (metallic < 1.0) {
        float diffuseBRDF = DiffuseBurley(NdotL, NdotV, LdotH, roughness) * NdotL;
        diffuseLight += lightColor * diffuseBRDF * attenuation;
    }
    
    if (roughness > 0.0) {
        float alpha = roughness * roughness;
        float D = D_GGX(NdotH, alpha);
        float G = V_GGX(NdotL, NdotV, alpha);
        float LdotH5 = SchlickFresnel(LdotH);
        float f90 = clamp(50.0 * f0.g, 0.0, 1.0);
        vec3 F = f0 + (f90 - f0) * LdotH5;
        vec3 specularBRDF = NdotL * D * F * G;
        specularLight += specularBRDF * lightColor * attenuation * specular;
    }
}

float GetOmniAttenuation(float distance, float radius, float decay) {
    float invRadius = 1.0 / radius;
    float nd = distance * invRadius;
    nd *= nd;
    nd *= nd;
    nd = max(1.0 - nd, 0.0);
    nd *= nd;
    return nd * pow(max(distance, 0.0001), -decay);
}

void main() {
    // Get material properties
    vec3 matAlbedo = albedo;
    float matMetallic = metallic;
    float matRoughness = roughness;
    float matAO = ao;
    
    if (useTextures) {
        matAlbedo = texture(albedoMap, TexCoords).rgb;
        matMetallic = texture(metallicMap, TexCoords).r;
        matRoughness = texture(roughnessMap, TexCoords).r;
        matAO = texture(aoMap, TexCoords).r;
    }
    
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - FragPos);
    
    // Accumulate lighting
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    
    // Process each light
    for (int i = 0; i < numLights; i++) {
        vec3 lightVec = lightPositions[i] - FragPos;
        float distance = length(lightVec);
        vec3 L = lightVec / distance;
        
        // Calculate attenuation (point light with radius 10.0, decay 1.0)
        float attenuation = GetOmniAttenuation(distance, 10.0, 1.0);
        
        ComputePBRLight(N, L, V, lightColors[i], attenuation,
                       matAlbedo, matRoughness, matMetallic, 1.0,
                       diffuseLight, specularLight);
    }
    
    // Ambient lighting (simple IBL approximation)
    vec3 ambient = vec3(0.03) * matAlbedo * matAO;
    
    // Combine lighting
    vec3 color = ambient + diffuseLight * matAlbedo * (1.0 - matMetallic) + specularLight;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, 1.0);
}
