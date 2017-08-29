#version 300 es
layout(location = 0) in vec2 vpos;

uniform vec2 modelOffset;
uniform float modelScale;

void main() {
	vec2 finalPos = (modelScale * vpos) + modelOffset;
	gl_Position = vec4(vpos, 0.0, 1.0);
}
\0