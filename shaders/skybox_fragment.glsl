#version 330 core

out vec4 FragColor;

in vec3 cube_normal;

uniform float time;
uniform vec3 sun_direction;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec3 getSkyColor(vec3 dir) {
    float horizon = abs(dir.y);
    vec3 skyTop = vec3(0.3, 0.5, 0.9);
    vec3 skyHorizon = vec3(0.7, 0.8, 0.95);
    return mix(skyHorizon, skyTop, pow(horizon, 0.5));
}

float getCloudNoise(vec3 dir) {
    vec2 uv = dir.xz / (dir.y + 0.5);
    uv += time * 0.02;
    float clouds = fbm(uv * 3.0);
    clouds = smoothstep(0.4, 0.8, clouds);
    return clouds;
}

void main()
{
    vec3 dir = normalize(cube_normal);
    
    vec3 skyColor = getSkyColor(dir);
    
    float sunDot = max(dot(dir, normalize(sun_direction)), 0.0);
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * pow(sunDot, 256.0) * 2.0;
    sunColor += vec3(1.0, 0.8, 0.6) * pow(sunDot, 32.0) * 0.5;
    
    if (dir.y > 0.0) {
        float clouds = getCloudNoise(dir);
        skyColor = mix(skyColor, vec3(1.0), clouds * 0.8);
    }
    
    vec3 color = skyColor + sunColor;
    
    FragColor = vec4(color, 1.0);
}
