#type vertex
#version 460
			
layout (location = 0) in vec2 a_Position;
layout (location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
	gl_Position = vec4(a_Position.x, a_Position.y, 0.0f, 1.0f);
	v_TexCoord = a_TexCoord;
}

#type fragment
#version 460
			
out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;

void main()
{
	color = texture(u_Texture, v_TexCoord);
}