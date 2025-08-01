#pragma once

#include "qgl.h"

typedef struct FBO_Container_s
{
	GLuint s_hBackBufferFBO;
	GLuint s_hBackBufferCB;
	GLuint s_hBackBufferDB;
	GLuint s_hBackBufferTex;
	GLuint s_hBackBufferTex2;
	GLuint s_hBackBufferTex3;
	GLuint s_hBackBufferTex4;
	GLuint s_hBackBufferDepthTex;
	GLuint s_hBackBufferStencilView;
	int iWidth;
	int iHeight;
	int iTextureColorFormat;
	int iTextureDepthFormat;
}FBO_Container_t;

class CDrawArrayAttrib
{
public:
	uint32_t NumVertices{};
	uint32_t NumInstances{ 1 };
	uint32_t StartVertexLocation{};
	uint32_t FirstInstanceLocation{};
};

class CDrawIndexAttrib
{
public:
	uint32_t NumIndices{};
	uint32_t NumInstances{ 1 };
	uint32_t FirstIndexLocation{};
	uint32_t BaseVertex{};
	uint32_t FirstInstanceLocation{};
};

class CIndirectDrawAttrib
{
public:
	uint64_t DrawArgsOffset{};
	uint32_t DrawCount{ 1 };
};

typedef struct vertex3f_s
{
	vec3_t	v;
}vertex3f_t;