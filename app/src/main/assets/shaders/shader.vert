#version 300 es
layout(location = 0) in vec2 vpos;
layout(location = 1) in vec3 color;

uniform vec2 modelOffset;
uniform float modelScale;
uniform mat2 modelRotation;
uniform float ratio;

out vec3 Color;

void main() {
	vec2 finalPos = modelRotation * (modelScale * vpos) + modelOffset;
	Color = color;
	finalPos.y *= ratio;
	gl_Position = vec4(finalPos, 0.0, 1.0);
}
