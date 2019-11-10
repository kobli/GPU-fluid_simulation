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

void main() {
	vec3 L = normalize(CameraPos - worldPosition.xyz);
	float di = clamp(dot(L,normal), 0,1);
	fColor = vec4(clamp((Mat.ambientK + Mat.diffuseK*di), 0,1)*Mat.color.rgb, 1);
}
