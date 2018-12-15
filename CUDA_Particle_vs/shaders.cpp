/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#define STRINGIFY(A) #A


 // vertex shader for object
const char *vertexShader = STRINGIFY(
	uniform float pointRadius;  // point size in world space
	uniform float pointScale;   // scale to calculate size in pixels
	uniform float densityScale;
	uniform float densityOffset;
void main()
{
	// calculate window-space point size
	vec3 posEye = vec3(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
	float dist = length(posEye);
	gl_PointSize = pointRadius * (pointScale / dist);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	//gl_FrontColor = gl_Color;
}
);

// pixel shader for rendering points as shaded spheres
const char *spherePixelShader = STRINGIFY(
	uniform sampler2D tex;
void main()
{
	const vec3 lightDir = vec3(0.577, 0.577, 0.577);

	// calculate normal from texture coordinates
	vec3 N;
	N.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(N.xy, N.xy);

	vec2 texCoord = gl_TexCoord[0].st;
	vec3 color = texture2D(tex, texCoord.st).rgb;

	if ((mag > 1.0)) discard;   // kill pixels outside circle

	N.z = sqrt(1.0 - mag);

	// calculate lighting
	float diffuse = max(0.0, dot(lightDir, N));

	//gl_FragColor = gl_Color * diffuse;
	gl_FragColor = vec4(color * diffuse, 1.0f);
}
);



/////////////////////////////////////////////////////////////////////////////



//vertex shader for object in skybox
const char *skyBoxObjectVertexShader = STRINGIFY(
	varying vec3 Normal;
	varying vec3 Position;

	uniform mat4 model;
	void main()
{
	Normal = normalize(gl_NormalMatrix*gl_Normal);
	Position = vec3(model * gl_Vertex); //This Position output of the vertex shader is used to calculate the view direction vector in the fragment shader.

	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
}
);

// pixel shader for object in skybox
const char *skyBoxObjectPixelShader = STRINGIFY(
	varying vec3 Normal;
	varying vec3 Position;

	uniform samplerCube skybox;
	void main()
{
	vec3 I = normalize(Position);

	vec3 reflectRay = reflect(I, normalize(Normal));
	vec3 refractRay = refract(I, normalize(Normal), 0.65);
	gl_FragColor = texture(skybox, reflectRay);
}
);



/////////////////////////////////////////////////////////////////////////////



// vertex shader for skybox
const char *skyBoxVertexShader = STRINGIFY(
	attribute vec3 aPos;
	varying vec3 TexCoords;
	uniform mat4 projection;
	uniform mat4 view;

	void main()
{
	TexCoords = aPos;
	vec4 pos = projection * view * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
}
                           );

// pixel shader for skybox
const char *skyBoxPixelShader = STRINGIFY(
	varying vec3 TexCoords;
	uniform samplerCube skybox;
	void main()
{
	gl_FragColor = texture(skybox, TexCoords);
}
);
