#version 330

in vec2 uv;

out vec4 color;

uniform sampler2D texture;
uniform mat3 kernel;
uniform float radius;

void main(){
	vec2 texcoord = uv;
    
    vec3 col = vec3(0.0,0.0,0.0);
	
	for (int j=-1; j<=1; j++)
	{
		for (int i=-1; i<=1; i++)
		{
			//int index = (i+1)*3+(j+1);
			col += kernel[i+1][j+1]*vec3(texture2D(texture, vec2(texcoord.x+i*radius, texcoord.y+j*radius)));
		}
	}
    color = vec4(col, 1.0);
}