#version 430 core

out vec4 fragColor;

in vec3 normWorld;
in vec2 uv;

void main() {
    vec3 color = normalize(normWorld) * 0.5 + 0.5;
    fragColor = vec4(color, 1.0);
}
