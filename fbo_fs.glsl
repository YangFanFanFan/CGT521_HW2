#version 430

uniform sampler2D texture;
uniform sampler2D pick_texture;
uniform int pass;
uniform int blur;


layout(location = 5) uniform float time;
layout(location = 2) uniform int pickedID;

in vec2 tex_coord;
flat in int InstanceID;

layout(location = 0) out vec4 fragcolor;           
layout(location = 1) out vec4 identifier;

void main(void)
{   
	if(pass == 1)
	{
		//pass the instanceID as identifier 
		identifier = vec4(vec3(InstanceID / 255.0f), 1.0); //either 0~11 or 255
		
		if(InstanceID == pickedID){
			fragcolor = texture2D(pick_texture, tex_coord);
			//fragcolor = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else{
			fragcolor = texture2D(texture, tex_coord);
			//fragcolor = vec4(0.0, 0.0, 0.0, 1.0);
		}		
		

		
	}
	else if(pass == 2)
	{
      //Lab assignment: Use texelFetch function and gl_FragCoord instead of using texture coordinates when reading from texture.

		//outline effect
		vec4 left = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(-1,0), 0);
		vec4 right = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(1,0), 0);
		vec4 above = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(0,1), 0);
		vec4 below = texelFetch(texture, ivec2(gl_FragCoord.xy) + ivec2(0,-1), 0);
		fragcolor = pow(right-left, vec4(2.0)) + pow(above-below, vec4(2.0));
		
	}
	else{
		fragcolor = vec4(1.0, 0.0, 1.0, 1.0); //error
	}

}


















