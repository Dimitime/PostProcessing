#version 330

in vec2 uv;

out vec4 color;

uniform sampler2D texture;
uniform float t;

void main(){
  vec2 texcoord = uv;
  texcoord.x = uv.x + sin(uv.y * 5*2*3.14 + t) / 100;
  texcoord.y = uv.y + cos(uv.x * 5*2*3.14 + t) / 100;
  
  color = texture2D(texture, texcoord);
}