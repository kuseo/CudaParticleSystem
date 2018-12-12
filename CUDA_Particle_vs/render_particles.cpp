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
	initSkyBox();
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
			glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1f(glGetUniformLocation(m_program, "pointScale"), m_window_h / tanf(m_fov*0.5f*(float)M_PI/180.0f));
            glUniform1f(glGetUniformLocation(m_program, "pointRadius"), m_particleRadius);

            glColor3f(1, 1, 1);
            _drawPoints();

            glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_POINT_SPRITE_ARB);
            break;
    }
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
	skybox_program = _compileProgram(skyBoxVertexShader, skyBoxPixelShader);
}

/***********
* new code *
************/
void ParticleRenderer::initTexture()
{
	glActiveTexture(GL_TEXTURE0);
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
	glUniform1i(glGetUniformLocation(m_program, "tex"), 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticleRenderer::initSkyBox()
{
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &skyboxID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

	Bitmap bmp = Bitmap::bitmapFromFile("textures/cubemap1.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 0, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	bmp = Bitmap::bitmapFromFile("textures/cubemap2.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	bmp = Bitmap::bitmapFromFile("textures/cubemap3.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	bmp = Bitmap::bitmapFromFile("textures/cubemap4.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	bmp = Bitmap::bitmapFromFile("textures/cubemap5.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	bmp = Bitmap::bitmapFromFile("textures/cubemap6.jpg");
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, 0, GL_RGB, bmp.width(), bmp.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixelBuffer());

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
