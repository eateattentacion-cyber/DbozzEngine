/**
 * @file pbr_lighting.glsl
 * @brief PBR lighting functions for DabozzEngine
 * 
 * Adapted from Godot Engine (MIT License)
 * https://github.com/godotengine/godot
 */

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

void ComputePBRLight(
    vec3 N,
    vec3 L,
    vec3 V,
    vec3 lightColor,
    float attenuation,
    vec3 albedo,
    float roughness,
    float metallic,
    float specular,
    inout vec3 diffuseLight,
    inout vec3 specularLight
) {
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

float GetOmniAttenuation(float distance, float invRadius, float decay) {
    float nd = distance * invRadius;
    nd *= nd;
    nd *= nd;
    nd = max(1.0 - nd, 0.0);
    nd *= nd;
    return nd * pow(max(distance, 0.0001), -decay);
}

float GetSpotAttenuation(vec3 lightDir, vec3 spotDir, float coneAngle, float coneAttenuation) {
    float scos = max(dot(-lightDir, spotDir), coneAngle);
    float spotRim = max(1e-4, (1.0 - scos) / (1.0 - coneAngle));
    return 1.0 - pow(spotRim, coneAttenuation);
}
