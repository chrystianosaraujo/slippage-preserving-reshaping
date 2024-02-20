#version 410

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;

out vec3 VPosition;
out vec3 VNormal;
//flat out vec3 VNormal;

uniform mat3 NormalMatrix;
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVP;

void main()
{
    VNormal = normalize(NormalMatrix * VertexNormal); 
    VPosition = (ModelViewMatrix * vec4(VertexPosition, 1.0)).xyz;

    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
