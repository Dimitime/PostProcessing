#version 330 core

in vec3 vert;
in vec3 normal;

layout(location = 0) out vec4 color;

void main(){
	vec3 light_pos = vec3(0.0, -10.0, 10.0);
	vec3 light_ambient = vec3(0.2, 0.2, 0.2);
	vec3 light_diffuse = vec3(1.0, 1.0, 1.0);
	vec3 light_specular = vec3(1.0, 1.0, 1.0);

	vec3 mat_ambient = 0.5*normal;
	vec3 mat_diffuse = vec3(0.2, 0.2, 0.2);
	vec3 mat_specular = vec3(0.8, 0.8, 0.8);

	float mat_shininess = 3.0;

    vec3 pixcolor = vec3(0.0, 0.0, 0.0);
	vec3 light_position = light_pos.xyz+vert;

	float intensity = dot(normalize(light_position), normalize(normal));
	vec3 reflection = reflect(normalize(-light_position),normalize(normal));
	float phong_intensity = dot(normalize(reflection), normalize(-vert));
	
	//We'll save the specular value in the alpha for use in bloom
	vec3 spec = light_specular * clamp(mat_specular*pow(max(phong_intensity, 0.0),mat_shininess) ,0.0, 1.0);

	pixcolor += light_ambient * mat_ambient +
			    light_diffuse * clamp(mat_diffuse*max(intensity, 0.0), 0.0, 1.0);

	float bloom = 0.0;
	if (spec.x > 0.0)
		bloom = spec.x;
	
	color = vec4(pixcolor,bloom);
}