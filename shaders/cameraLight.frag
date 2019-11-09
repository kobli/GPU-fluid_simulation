#version 330
out vec4 fColor;
uniform vec3 CameraPos;

in Vertex
{
	vec3 worldPosition;
	vec3 normal;
};

struct Material {
	vec4 color;
	float ambientK;
	float diffuseK;
	float specularK;
	float shininess;
};
uniform Material Mat;

vec3 lightColor = vec3(1,1,1);

void main() {
	vec3 L = normalize(CameraPos - worldPosition.xyz);
	float di = Mat.diffuseK*clamp(dot(L,normal), 0,1);
	fColor = vec4(clamp(di*lightColor, 0,1), 1);
	fColor = vec4(1,1,1,1); //TODO
}
