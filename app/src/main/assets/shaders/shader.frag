#version 300 es
precision mediump float;

in vec3 Color;
out vec4 color;

void main() {
	color = vec4(Color, 1.0);
}
