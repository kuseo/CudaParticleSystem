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


#include <math.h>
#include <assert.h>
#include <stdio.h>

// OpenGL Graphics includes
#define HELPERGL_EXTERN_GL_FUNC_IMPLEMENTATION
#include "helper_gl.h"


#include "render_particles.h"
#include "shaders.h"

#include "Bitmap.h"

#ifndef M_PI
#define M_PI    3.1415926535897932384626433832795
#endif

ParticleRenderer::ParticleRenderer()
    : m_pos(0),
      m_numParticles(0),
      m_pointSize(1.0f),
      m_particleRadius(0.125f * 0.5f),
      m_program(0),
      m_vbo(0),
      m_colorVBO(0)
{
    _initGL();
	initTexture();
	initSkyBoxTexture();
	SkyBoxDataTransfer();
}

ParticleRenderer::~ParticleRenderer()
{
    m_pos = 0;
}

void ParticleRenderer::setPositions(float *pos, int numParticles)
{
    m_pos = pos;
    m_numParticles = numParticles;
}

void ParticleRenderer::setVertexBuffer(unsigned int vbo, int numParticles)
{
    m_vbo = vbo;
    m_numParticles = numParticles;
}

void ParticleRenderer::SkyBoxDataTransfer()
{
	glGenVertexArrays(1, &skybox_vao);
	glGenBuffers(1, &skybox_vbo);
	glBindVertexArray(skybox_vao);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(skybox_program, "aPos"));
	glVertexAttribPointer(glGetAttribLocation(skybox_program, "aPos"), 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}


void ParticleRenderer::_drawPoints()
{
    if (!m_vbo)
    {
        glBegin(GL_POINTS);
        {
            int k = 0;

            for (int i = 0; i < m_numParticles; ++i)
            {
                glVertex3fv(&m_pos[k]);
                k += 4;
            }
        }
        glEnd();
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexPointer(4, GL_FLOAT, 0, 0);
        glEnableClientState(GL_VERTEX_ARRAY);

        if (m_colorVBO)
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
            glColorPointer(4, GL_FLOAT, 0, 0);
            glEnableClientState(GL_COLOR_ARRAY);
        }

        glDrawArrays(GL_POINTS, 0, m_numParticles);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
    }
}

void ParticleRenderer::display(DisplayMode mode /* = PARTICLE_POINTS */)
{
    switch (mode)
    {
        case PARTICLE_POINTS:
            glColor3f(1, 1, 1);
            glPointSize(m_pointSize);
            _drawPoints();
            break;

        default:
        case PARTICLE_SPHERES:
            glEnable(GL_POINT_SPRITE_ARB);
            glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
			glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);

            glUseProgram(m_program);

			glActiveTexture(GL_TEXTURE0);

			glUniform1f(glGetUniformLocation(m_program, "pointScale"), m_window_h / tanf(m_fov*0.5f*(float)M_PI / 180.0f));
			glUniform1f(glGetUniformLocation(m_program, "pointRadius"), m_particleRadius);

			glBindTexture(GL_TEXTURE_2D, textureID);

            glColor3f(1, 1, 1);
            _drawPoints();

            glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_POINT_SPRITE_ARB);
            break;
    }
}

void ParticleRenderer::displaySpheres(float * model)
{
	int k = 0;
	float *pos;
	GLUquadricObj *qobj = gluNewQuadric();
	gluQuadricOrientation(qobj, GLU_OUTSIDE);
	gluQuadricNormals(qobj, GLU_SMOOTH);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	pos = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	
	
	glUseProgram(skyboxObject_program);

	glActiveTexture(GL_TEXTURE0);

	glUniformMatrix4fv(glGetUniformLocation(skyboxObject_program, "model"), 1, GL_FALSE, model);


	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

	for (int i = 0; i < m_numParticles; ++i)
	{
		glPushMatrix();
		glTranslatef(pos[k], pos[k + 1], pos[k + 2]);
		gluSphere(qobj, m_particleRadius, 30, 30);
		k += 4;
		glPopMatrix();
	}
	glUseProgram(0);

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void ParticleRenderer::displaySkyBox(float * view, float * projection)
{
	glDepthFunc(GL_LEQUAL);

	glUseProgram(skybox_program);

	glActiveTexture(GL_TEXTURE0);

	glUniformMatrix4fv(glGetUniformLocation(skybox_program, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(skybox_program, "projection"), 1, GL_FALSE, projection);

	glBindVertexArray(skybox_vao);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glUseProgram(0);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glDepthFunc(GL_LESS);
}


GLuint
ParticleRenderer::_compileProgram(const char *vsource, const char *fsource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertexShader, 1, &vsource, 0);
    glShaderSource(fragmentShader, 1, &fsource, 0);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    // check if program linked
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char temp[256];
        glGetProgramInfoLog(program, 256, 0, temp);
        printf("Failed to link program:\n%s\n", temp);
        glDeleteProgram(program);
        program = 0;
    }

    return program;
}

void ParticleRenderer::_initGL()
{
    m_program = _compileProgram(vertexShader, spherePixelShader);
	skyboxObject_program = _compileProgram(skyBoxObjectVertexShader, skyBoxObjectPixelShader);
	skybox_program = _compileProgram(skyBoxVertexShader, skyBoxPixelShader);
}

void ParticleRenderer::initTexture()
{
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	const char path[] = "textures/ball1.jpg";
	int n = sizeof(path) / sizeof(*path);
	char* filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path);
	Bitmap bmp = Bitmap::bitmapFromFile(filePath);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glBindTexture(GL_TEXTURE_2D, 0);

	//glUniform1i(glGetUniformLocation(m_program, "tex"), 0);
}

void ParticleRenderer::initSkyBoxTexture()
{
	glGenTextures(1, &skyboxID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

	const char path0[] = "textures/cubemap1.jpg";
	int n = sizeof(path0) / sizeof(*path0);
	char* filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path0);
	Bitmap bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	const char path1[] = "textures/cubemap2.jpg";
	n = sizeof(path1) / sizeof(*path1);
	filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path1);
	bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	const char path2[] = "textures/cubemap3.jpg";
	n = sizeof(path2) / sizeof(*path2);
	filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path2);
	bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	const char path3[] = "textures/cubemap4.jpg";
	n = sizeof(path3) / sizeof(*path3);
	filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path3);
	bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	const char path4[] = "textures/cubemap5.jpg";
	n = sizeof(path4) / sizeof(*path4);
	filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path4);
	bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	const char path5[] = "textures/cubemap6.jpg";
	n = sizeof(path5) / sizeof(*path5);
	filePath = (char*)malloc(n * sizeof(char));
	strcpy(filePath, path5);
	bmp = Bitmap::bitmapFromFile(filePath);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	//glUniform1i(glGetUniformLocation(skybox_program, "skybox"), 0);
}