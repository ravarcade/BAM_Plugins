/**
*  Copyright (C) 2016 Rafal Janicki
*
*  This software is provided 'as-is', without any express or implied
*  warranty.  In no event will the authors be held liable for any damages
*  arising from the use of this software.
*
*  Permission is granted to anyone to use this software for any purpose,
*  including commercial applications, and to alter it and redistribute it
*  freely, subject to the following restrictions:
*
*  1. The origin of this software must not be misrepresented; you must not
*     claim that you wrote the original software. If you use this software
*     in a product, an acknowledgment in the product documentation would be
*     appreciated but is not required.
*  2. Altered source versions must be plainly marked as such, and must not be
*     misrepresented as being the original software.
*  3. This notice may not be removed or altered from any source distribution.
*
*  Rafal Janicki
*  ravarcade@gmail.com
*/

#include "stdafx.h"
#include "PostFX.h"
void BAM_hud(const char *fmt, ...);
double Z_Near = 1.0;
double Z_Far = 10000.0;

// ============================================================================ Tools ===

LPVOID GetResource(const char *resData, const char *resType = "TEXT", DWORD *outLen = NULL)
{
	static HMODULE dll_hModule = NULL;

	if (!dll_hModule)
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetResource, &dll_hModule);

	HRSRC hResource = FindResourceA(dll_hModule, resData, resType);
	if (!hResource)
		return NULL;

	HGLOBAL hLoadedResource = LoadResource(dll_hModule, hResource);
	if (!hLoadedResource)
		return NULL;

	LPVOID pLockedResource = LockResource(hLoadedResource);
	if (!pLockedResource)
		return NULL;

	if (outLen != NULL)
		*outLen = SizeofResource(dll_hModule, hResource);

	return pLockedResource;
}

const char *GetResourceText(const char *resData)
{
	DWORD olen;

	// we have to ad '\0' and end of string, so we create tmp with size+1 and add '\0' at end
	const char * vs = (const char *)GetResource(resData, "TEXT", &olen);
	char *tmp = new char[olen + 1];
	memcpy_s(tmp, olen + 1, vs, olen);
	tmp[olen] = 0;

	return tmp;
}

// ============================================================================

Ccfg cfg = { 5.0, 1.5, 1.5, 0, 1.0, 60.0, 10.0, 2.5, 0.15, 0, 0, 0, 0.3 };

const int MIN_TEX_WIDTH = 2048;
const int MIN_TEX_HEIGHT = 2048;

CPostFX::CPostFX() :
	m_doInit(true),
	m_texWidth(0),
	m_texHeight(0),
	m_Pass1(0)
{

}

CPostFX::~CPostFX()
{

}

void CPostFX::Init()
{
	m_doInit = true;
}

#define GLDELETE(x,y) { if (y) { glDelete##x(1, &y); y = 0; } }

void CPostFX::Release()
{
	// Release resources
	for (auto &fb : m_FBOs)
	{
		GLDELETE(Framebuffers, fb.FBO);
		GLDELETE(Textures, fb.tex);
		GLDELETE(Textures, fb.depthTex);
	}

	GLDELETE(VertexArrays, m_VAO);
	GLDELETE(Buffers, m_VBO);
	GLDELETE(Buffers, m_IBO);

	if (m_Pass1) {
		glDeleteProgram(m_Pass1);
		m_Pass1 = 0;
	}

	m_texWidth = 0;
	m_texHeight = 0;

	m_doInit = true;
}


void CPostFX::Postprocess()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLint fullScreenViewport[4] = { 0, 0, viewport[0] + viewport[2], viewport[1] + viewport[3] };
	if (m_doInit)
		_Init();

	Ccfg lcfg;
	memcpy(&lcfg, &cfg, sizeof(cfg));
	if (!lcfg.Enabled) {
		lcfg.BlurAmount = 0.0;
	}

	GLfloat texel[2] = { 1.0f / m_texWidth, 1.0f / m_texHeight };

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBindVertexArray(m_VAO);

	GLint currentFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

	// freaking voodoo ... without it on my notebook gfx card driver don't use right framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 5);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 5);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	glDisable(GL_SCISSOR_TEST);
	glScissor(0, 0, fullScreenViewport[2], fullScreenViewport[3]);
	glViewport(0, 0, fullScreenViewport[2], fullScreenViewport[3]);

	glDisable(GL_MULTISAMPLE);

	// copy current fbo to fbo[0]
	_CopyFBOWithDepth(currentFBO, m_FBOs[0].FBO, fullScreenViewport);
	GLint source[4] = { 0, 0, fullScreenViewport[2], fullScreenViewport[3] };

	bool SSAO = true;

	if (!SSAO)
	{
		// pass 1 - shrink & gamma
		// render fbo[0] to fbo[1]
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[1].FBO);

		glBindTexture(GL_TEXTURE_2D, m_FBOs[0].tex);
		glUseProgram(m_Pass1);
		glUniform1i(m_P1_tex, 0);
		glUniform1f(m_P1_pfGamma, (GLfloat)lcfg.gamma);
		glUniform2fv(m_P1_texel, 1, texel);
		GLint target[4] = { 0, 0, fullScreenViewport[2] / 2, fullScreenViewport[3] / 2 };
		_DrawQuad(target, fullScreenViewport + 2, source, m_FBOs[0].dim, m_P1_ScalePosition, m_P1_ScaleTexture);


		// pass 2 - blur - vertical
		// render fbo[1] to fbo[2]
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[2].FBO);

		glBindTexture(GL_TEXTURE_2D, m_FBOs[1].tex);
		glUseProgram(m_Pass2);
		glUniform1i(m_P2_tex, 0);
		glUniform1i(m_P2_Orientation, 1);
		glUniform1i(m_P2_BlurAmount, 5);
		glUniform1f(m_P2_BlurScale, (GLfloat)lcfg.BlurScale);
		glUniform1f(m_P2_BlurStrength, (GLfloat)lcfg.BlurStrength);
		glUniform1f(m_P2_TexelSize, (GLfloat)0.5f / m_FBOs[1].dim[0]);
		_DrawQuad(target, fullScreenViewport + 2, target, m_FBOs[1].dim, m_P2_ScalePosition, m_P2_ScaleTexture);

		// pass 2 - blur - horizontal
		// render fbo[2] to fbo[1]
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[1].FBO);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[2].tex);
		glUniform1i(m_P2_Orientation, 0);
		_DrawQuad(target, fullScreenViewport + 2, target, m_FBOs[2].dim, m_P2_ScalePosition, m_P2_ScaleTexture);

		// pass 3 - combine fbo[0] & fbo[1]
		glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
		glUseProgram(m_Pass3);
		glUniform1i(m_P3_tex, 0);
		glUniform1i(m_P3_tex2, 1);
		glUniform1f(m_P3_amount, (GLfloat)lcfg.BlurAmount);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[1].tex);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[0].tex);

		_DrawQuad(fullScreenViewport, fullScreenViewport + 2, target, m_FBOs[2].dim, m_P3_ScalePosition, m_P3_ScaleTexture);
	}
	else
	{
		// SSAO - pass 1
		int w = fullScreenViewport[2];
		int h = fullScreenViewport[3];

		// SSAO param
		double zNear = Z_Near, zFar = Z_Far;

		int rings = cfg.SSAO_rings;
		float lz = (float)w / 2;
		lz = (float)(lz * (4.0 / (1.0 + rings)));

		zNear = 1; zFar = 1000.0;
		double range = cfg.SSAO_range;
		if (cfg.SSAO_mode) {
			range = range / 999.0;
		}

		GLint dstSsao[4] = { 0, 0,
			cfg.SSAO_res ? fullScreenViewport[2] : fullScreenViewport[2] / 2,
			cfg.SSAO_res ? fullScreenViewport[3] : fullScreenViewport[3] / 2
		};
		
		
		BAM_hud("cfg.SSAO_BlurScale = %.3f\n", cfg.SSAO_BlurScale);
		BAM_hud("cfg.SSAO_BlurStrength = %.3f\n", cfg.SSAO_BlurStrength);
		BAM_hud("cfg.SSAO_MAX = %.3f\n", cfg.SSAO_MAX);
		BAM_hud("cfg.SSAO_mode = %d\n", cfg.SSAO_mode);
		BAM_hud("cfg.SSAO_range = %.3f\n", range);
		BAM_hud("cfg.SSAO_res = %d\n", cfg.SSAO_res);
		BAM_hud("cfg.SSAO_rings = %d\n", rings);
		BAM_hud("cfg.SSAO_scale = %.3f\n", cfg.SSAO_scale);
		BAM_hud("cfg.SSAO_type = %d\n", cfg.SSAO_type);
		BAM_hud("\nZ_Near = %.3f\n", zNear);
		BAM_hud("\nZ_Far = %.3f\n", zFar);


		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[0].depthTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[0].tex);
		glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[1].FBO);

		auto &p = m_SSAO[cfg.SSAO_mode ? 1 : 0];

		glUseProgram(p.Program);
		glUniform1i(p.Texture0, 0);
		glUniform1i(p.Texture1, 1);
		glUniform4f(p.pixelSize, float(2.0f * zNear), float(zFar - zNear), 1.0f / m_FBOs[0].dim[0], 1.0f / m_FBOs[0].dim[1]);
		glUniform4f(p.aoRangeLevelAspect, (float)cfg.SSAO_scale * 0.0003f, (float)range, (float)m_FBOs[0].dim[0] / m_FBOs[0].dim[1], (float)cfg.SSAO_scale);
		glUniform1i(p.rings, rings == 0 ? 3 : rings);
		glUniform1f(p.gamma, (GLfloat)lcfg.gamma);
		_DrawQuad(dstSsao, fullScreenViewport + 2, source, m_FBOs[0].dim, p.ScalePosition, p.ScaleTexture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);

		if (false) {
			// pass 2 - blur - vertical
			// render fbo[1] to fbo[2]
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[2].FBO);

			glBindTexture(GL_TEXTURE_2D, m_FBOs[1].tex);
			glUseProgram(m_Pass2);
			glUniform1i(m_P2_tex, 0);
			glUniform1i(m_P2_Orientation, 1);
			glUniform1i(m_P2_BlurAmount, 5);
			glUniform1f(m_P2_BlurScale, (GLfloat)lcfg.BlurScale);
			glUniform1f(m_P2_BlurStrength, (GLfloat)lcfg.BlurStrength);
			glUniform1f(m_P2_TexelSize, (GLfloat)0.5f / m_FBOs[1].dim[0]);
			_DrawQuad(dstSsao, fullScreenViewport + 2, dstSsao, m_FBOs[1].dim, m_P2_ScalePosition, m_P2_ScaleTexture);

			// pass 2 - blur - horizontal
			// render fbo[2] to fbo[1]
			glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[1].FBO);
			glBindTexture(GL_TEXTURE_2D, m_FBOs[2].tex);
			glUniform1i(m_P2_Orientation, 0);
			_DrawQuad(dstSsao, fullScreenViewport + 2, dstSsao, m_FBOs[2].dim, m_P2_ScalePosition, m_P2_ScaleTexture);

		}

		// pass 3 - combine fbo[0] & fbo[1]
		glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
		//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(m_SSAO_2);
		glUniform1i(m_S2_tex, 0);
		glUniform1i(m_S2_tex2, 1);
		glUniform2f(m_S2_ssao_max_blur_amount, (GLfloat)cfg.SSAO_MAX, (GLfloat)lcfg.BlurAmount);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[1].tex);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_FBOs[0].tex);

		_DrawQuad(fullScreenViewport, fullScreenViewport + 2, fullScreenViewport, m_FBOs[0].dim, m_S2_ScalePosition, m_S2_ScaleTexture);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 5);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 5);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	// final
	glEnable(GL_MULTISAMPLE);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

// =============================================================================== private
void CPostFX::_Init()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	m_width = viewport[2] + viewport[0]; // full screen
	m_height = viewport[3] + viewport[1];

	// calc tex width
	int w = m_width < MIN_TEX_WIDTH ? MIN_TEX_WIDTH : m_width;
	int h = m_height < MIN_TEX_HEIGHT ? MIN_TEX_HEIGHT : m_height;
	if (w > m_texWidth || h > m_texHeight)
	{
		Release();
		m_texWidth = w;
		m_texHeight = h;
		_CreateFBOs();
		_CreateShaders();
	}
	m_doInit = false;
}

void CPostFX::_VerifyRes()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int w = viewport[2] + viewport[0];
	int h = viewport[3] + viewport[1];
	if (w != m_width || h != m_height)
	{
		m_doInit = true;
	}
}

void CPostFX::_CreateFBOs()
{
//	_CreateFBO(m_FBOs[0], m_texWidth, m_texHeight);
	_CreateFBOWithDepth(m_FBOs[0], m_texWidth, m_texHeight);
	_CreateFBO(m_FBOs[1], m_texWidth / 2, m_texHeight / 2);
	_CreateFBO(m_FBOs[2], m_texWidth/2, m_texHeight/2);
}

bool CPostFX::_CreateFBO(FB &fb, int nWidth, int nHeight)
{
	fb.dim[0] = nWidth;
	fb.dim[1] = nHeight;

	glGenFramebuffers(1, &fb.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.FBO);

	glGenTextures(1, &fb.tex);
	glBindTexture(GL_TEXTURE_2D, fb.tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.tex, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

bool CPostFX::_CreateFBOWithDepth(FB &fb, int nWidth, int nHeight)
{
	fb.dim[0] = nWidth;
	fb.dim[1] = nHeight;

	glGenFramebuffers(1, &fb.FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.FBO);

	glGenTextures(1, &fb.tex);
	glBindTexture(GL_TEXTURE_2D, fb.tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.tex, 0);

	glGenTextures(1, &fb.depthTex);
	glBindTexture(GL_TEXTURE_2D, fb.depthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, nWidth, nHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT + 0, GL_TEXTURE_2D, fb.depthTex, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void CPostFX::_CopyFBO(GLuint src, GLuint dst, GLint *viewport)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);

	glBlitFramebuffer(viewport[0], viewport[1], viewport[2], viewport[3],
		0, 0, viewport[2], viewport[3],
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void CPostFX::_CopyFBOWithDepth(GLuint src, GLuint dst, GLint *viewport)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);

	static bool OneStep = false;

	if (OneStep) {
		glBlitFramebuffer(viewport[0], viewport[1], viewport[2], viewport[3],
			0, 0, viewport[2], viewport[3],
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
	else {
		glBlitFramebuffer(viewport[0], viewport[1], viewport[2], viewport[3],
			0, 0, viewport[2], viewport[3],
			GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBlitFramebuffer(viewport[0], viewport[1], viewport[2], viewport[3],
			0, 0, viewport[2], viewport[3],
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}

void CPostFX::_CreateShaders()
{
	static struct SVBO {
		GLfloat vert[2];
		GLfloat tex[2];
	} VBO_DATA[4] = {
		 0, 0, 0, 0,
		 1, 0, 1, 0,
		 1, 1, 1, 1,
		 0, 1, 0, 1
	};

	static GLushort IBO_DATA[] = {
		0, 1, 2,
		0, 2, 3
	};


	// prepare VAO
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	// prepare VBO
	glGenBuffers(1, &m_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VBO_DATA), &VBO_DATA[0], GL_STATIC_DRAW);

	// prepare IBO
	glGenBuffers(1, &m_IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IBO_DATA), &IBO_DATA[0], GL_STATIC_DRAW);


	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VBO_DATA[0]), (void *)offsetof(SVBO, vert));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VBO_DATA[0]), (void *)offsetof(SVBO, tex));

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	m_Pass1 = _CreateShaderProgram("VS", "Pass1");
	glUseProgram(m_Pass1);
	m_P1_ScalePosition = glGetUniformLocation(m_Pass1, "ScalePosition");
	m_P1_ScaleTexture = glGetUniformLocation(m_Pass1, "ScaleTexture");
	m_P1_pfGamma = glGetUniformLocation(m_Pass1, "pfGamma");
	m_P1_texel = glGetUniformLocation(m_Pass1, "texel");
	m_P1_tex = glGetUniformLocation(m_Pass1, "tex");

	m_Pass2 = _CreateShaderProgram("Pass2_VS", "Pass2_FS");
	m_P2_ScalePosition = glGetUniformLocation(m_Pass2, "ScalePosition");
	m_P2_ScaleTexture = glGetUniformLocation(m_Pass2, "ScaleTexture");
	m_P2_Orientation = glGetUniformLocation(m_Pass2, "Orientation");
	m_P2_BlurAmount = glGetUniformLocation(m_Pass2, "BlurAmount");
	m_P2_BlurScale = glGetUniformLocation(m_Pass2, "BlurScale");
	m_P2_BlurStrength = glGetUniformLocation(m_Pass2, "BlurStrength");
	m_P2_TexelSize = glGetUniformLocation(m_Pass2, "TexelSize");
	m_P2_tex = glGetUniformLocation(m_Pass2, "tex");

	m_Pass3 = _CreateShaderProgram("VS", "Pass3_FS");
	m_P3_ScalePosition = glGetUniformLocation(m_Pass3, "ScalePosition");
	m_P3_ScaleTexture = glGetUniformLocation(m_Pass3, "ScaleTexture");
	m_P3_amount = glGetUniformLocation(m_Pass3, "amount");
	m_P3_tex = glGetUniformLocation(m_Pass3, "tex");
	m_P3_tex2 = glGetUniformLocation(m_Pass3, "tex2");

	// =====
	for (int i = 0; i < sizeof(m_SSAO)/sizeof(m_SSAO[0]); ++i)
	{
		auto &p = m_SSAO[i];
		p.Program = _CreateShaderProgram("SSAO_1_VS",i == 0 ? "SSAO_1_FS" : "SSAO_1b_FS" );

		p.ScalePosition = glGetUniformLocation(p.Program, "ScalePosition");
		p.ScaleTexture = glGetUniformLocation(p.Program, "ScaleTexture");
		p.pixelSize = glGetUniformLocation(p.Program, "pixelSize");
		p.aoRangeLevelAspect = glGetUniformLocation(p.Program, "aoRangeLevelAspect");
		p.gamma = glGetUniformLocation(p.Program, "gamma");
		p.Texture0 = glGetUniformLocation(p.Program, "Texture0");
		p.Texture1 = glGetUniformLocation(p.Program, "Texture1");
		p.rings = glGetUniformLocation(p.Program, "rings");
	}

	m_SSAO_2 = _CreateShaderProgram("SSAO_1_VS", "SSAO_2_FS");
	m_S2_ScalePosition = glGetUniformLocation(m_SSAO_2, "ScalePosition");
	m_S2_ScaleTexture = glGetUniformLocation(m_SSAO_2, "ScaleTexture");
	m_S2_ssao_max_blur_amount = glGetUniformLocation(m_SSAO_2, "ssao_max_blur_amount");
	m_S2_tex = glGetUniformLocation(m_SSAO_2, "tex");
	m_S2_tex2 = glGetUniformLocation(m_SSAO_2, "tex2");

	glUseProgram(0);
}

GLuint CPostFX::_CreateShaderProgram(const char *vertexSrc, const char *fragmentSrc)
{
	GLuint unProgramID = glCreateProgram();

	// set attrib locations
	glBindAttribLocation(unProgramID, 0, "position");
	glBindAttribLocation(unProgramID, 1, "texture");

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	const char * vs = GetResourceText(vertexSrc);
	glShaderSource(nSceneVertexShader, 1, &vs, NULL);
	glCompileShader(nSceneVertexShader);
	delete vs;

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		GLint log_length;
		glGetShaderiv(nSceneVertexShader, GL_INFO_LOG_LENGTH, &log_length);
		GLchar *log = (GLchar *)malloc(log_length);
		glGetShaderInfoLog(nSceneVertexShader, log_length, NULL, log);
		log[0] = 0;

		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneVertexShader);
		return 0;
	}
	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char * fs = GetResourceText(fragmentSrc);
	glShaderSource(nSceneFragmentShader, 1, &fs, NULL);
	glCompileShader(nSceneFragmentShader);
	delete fs;

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneFragmentShader);
		return 0;
	}

	glAttachShader(unProgramID, nSceneFragmentShader);
	glDeleteShader(nSceneFragmentShader); // the program hangs onto this once it's attached

	glLinkProgram(unProgramID);

	GLint programSuccess = GL_TRUE;
	glGetProgramiv(unProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE)
	{
		glDeleteProgram(unProgramID);
		return 0;
	}

	glUseProgram(unProgramID);
	glUseProgram(0);

	return unProgramID;
}
	
void CPostFX::_DrawQuad(GLint *dstRect, GLint *dstSize, GLint *srcRect, GLint *srcSize, GLint locPosition, GLint locTexture)
{
	if (locPosition != -1 && locTexture != -1) {
		GLfloat p[4] = {
			2.0f*(GLfloat)(dstRect[2]) / dstSize[0], 2.0f*(GLfloat)(dstRect[3]) / dstSize[1], (GLfloat)(2.0f*dstRect[0]) / dstSize[0] - 1.0f, (GLfloat)(2.0f*dstRect[1]) / dstSize[1] - 1.0f };

		GLfloat t[4] = {
			(GLfloat)(srcRect[2]) / srcSize[0], (GLfloat)(srcRect[3]) / srcSize[1], (GLfloat)srcRect[0] / srcSize[0], (GLfloat)srcRect[1] / srcSize[1] };

		glUniform4fv(locPosition, 1, p);
		glUniform4fv(locTexture, 1, t);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

void CPostFX::_DrawQuad2(GLint *dstRect, GLint *dstSize, GLint *srcRect, GLint *srcSize, GLint locPosition, GLint locTexture)
{
	if (locPosition != -1 && locTexture != -1) {
		GLfloat p[4] = {
			2.0f*(GLfloat)(dstRect[2]) / dstSize[0], 2.0f*(GLfloat)(dstRect[3]) / dstSize[1], (GLfloat)(2.0f*dstRect[0]) / dstSize[0] - 1.0f, (GLfloat)(2.0f*dstRect[1]) / dstSize[1] - 1.0f };

		GLfloat t[4] = {
			(GLfloat)(srcRect[2]) / srcSize[0], (GLfloat)(srcRect[3]) / srcSize[1], (GLfloat)srcRect[0] / srcSize[0], (GLfloat)srcRect[1] / srcSize[1] };

		glUniform4fv(locPosition, 1, p);
		glUniform4fv(locTexture, 1, t);
	}

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}