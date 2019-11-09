#version 330
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

out Vertex
{
	vec3 worldPosition;
	vec3 normal;
};

uniform mat4 ViewProject;
uniform mat4 Model;
uniform mat4 ModelInvT;

void main()
{
	vec4 p = Model*vec4(vPosition.xyz,1);
	normal = normalize((ModelInvT*vec4(vNormal.xyz,0)).xyz);
	worldPosition = p.xyz;
	gl_Position = ViewProject*p;
}
