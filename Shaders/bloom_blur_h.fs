#version 330

in vec2 uv;

out vec4 color;

uniform sampler2D texture;
uniform float radius;

void main(){
	vec2 texcoord = uv;
    
	vec4 tcol1 = texture2D(texture, vec2(texcoord.x, texcoord.y));
    vec3 col = vec3(tcol1);
//	if (tcol1.w != 0.0) {
		float blur[9] = float[](0.05, 0.09, 0.12, 0.15, 0.18, 0.15, 0.12, 0.09, 0.05);

		for (int i=-4; i<=4; i++) {
			vec4 tcol2 = texture2D(texture, vec2(texcoord.x + i*radius, texcoord.y));
			if (tcol2.w > 0.0 && tcol2.w < 1.0)
				col += vec3(tcol2.w,tcol2.w,tcol2.w)*blur[i+4];
		}
//	}
	
    color = vec4(col, tcol1.w);
}