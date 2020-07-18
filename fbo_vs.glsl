#version 400  

uniform mat4 PVM;
uniform int pass;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec2 tex_coord;  
flat out int InstanceID;

void main(void)
{
	
	if(pass == 1)
	{
		float a = 0.2f;
		vec3 offset[12] = vec3[12](
			vec3(-3*a,-3*a, 0.0f), vec3(-3*a,-a, 0.0f), vec3(-3*a,a, 0.0f), vec3(-3*a,3*a, 0.0f),
			vec3(0.0f,-3*a, 0.0f), vec3(0.0f,-a, 0.0f), vec3(0.0f,a, 0.0f), vec3(0.0f,3*a, 0.0f),
			vec3(3*a,-3*a, 0.0f), vec3(3*a,-a, 0.0f), vec3(3*a,a, 0.0f), vec3(3*a,3*a, 0.0f)
		);
		vec3 trans_pos = pos_attrib +  offset[gl_InstanceID];
		gl_Position = PVM*vec4(trans_pos, 1.0);

		tex_coord = tex_coord_attrib;
		InstanceID = gl_InstanceID;
		
	}
	else if(pass == 2)
	{
		gl_Position = vec4(pos_attrib, 1.0);
		tex_coord = 0.5*pos_attrib.xy + vec2(0.5);
		//InstanceID = gl_InstanceID;
	}else{
		gl_Position = PVM * vec4(pos_attrib, 1.0);
		tex_coord = tex_coord_attrib;
	}
}
