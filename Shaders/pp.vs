#version 330 core

layout(location = 0) in vec3 vertex;

out vec2 uv;

void main(){
	gl_Position = vec4(vertex,1);
	uv = (vertex.xy+vec2(1,1))/2.0;
}