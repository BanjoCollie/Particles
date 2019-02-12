#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 xyzs;
layout (location = 2) in vec4 color;

out vec2 TexCoord;
out vec4 TintColor;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	//gl_Position = projection * view * (vec4(xyzs.xyz, 1.0) + model * vec4((aPos * xyzs.w), 0.0));

	vec3 cameraRightWorldSpace = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 cameraUpWorldSpace = vec3(view[0][1], view[1][1], view[2][1]);

	vec3 viewPos = xyzs.xyz 
		+ cameraRightWorldSpace * aPos.x * xyzs.w 
		+ cameraUpWorldSpace * aPos.y * xyzs.w;

	gl_Position = projection * view * vec4(viewPos, 1.0);

	TexCoord = aPos.xy + vec2(0.5, 0.5);

	TintColor = color;
}