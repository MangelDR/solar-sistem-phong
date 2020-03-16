#version 330

in vec2 v_uv;
in vec3 v_normal; 
in vec3 v_pos;

out vec4 fragColor;

uniform sampler2D u_texture; 
uniform sampler2D u_texture_spec; 
uniform sampler2D u_normal_map; 


uniform vec3 u_light_dir;
uniform vec3 u_ambient;
uniform vec3 u_light_color; 
uniform vec3 u_eye; 
uniform float u_glossiness;

void main(void)
{
	float specular = 0;

	vec3 texture_color = texture(u_texture, v_uv).xyz;
	vec3 texture_spec = texture(u_texture_spec, v_uv).xyz;
	vec3 texture_normal = texture(u_normal_map, v_uv).xyz;
	texture_normal = normalize(texture_normal * 2.0 - vec3(1.0));

	vec3 N = normalize (v_normal + texture_normal);
	vec3 L = normalize (u_light_dir);
	vec3 NT = normalize(N * texture_normal);

	float NdotL = max(dot(N, L), 0.0); //Lambertian
	if (NdotL > 0){
		vec3 R = reflect (-L, N);
		vec3 E = normalize (u_eye - v_pos);

		float RdotE = max(0.0, dot (R, E));
		specular = pow (RdotE, u_glossiness);
	}

	vec3 ambient_color = texture_color * u_ambient; 
	vec3 diffuse_color = texture_color * NdotL; 
	vec3 specular_color = texture_spec * u_light_color * specular;

	// We're just going to paint the interpolated colour from the vertex shader
	fragColor =  vec4(ambient_color + diffuse_color + specular_color, 1.0);
}
