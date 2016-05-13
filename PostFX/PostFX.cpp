#include "stdafx.h"
#include "PostFX.h"

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

void CPostFX::Release()
{
	// Release resources
	for (auto &fb : m_FBOs)
	{
		if (fb.FBO)
			glDeleteFramebuffers(1, &fb.FBO);
		if (fb.tex)
			glDeleteTextures(1, &fb.tex);
		fb.FBO = 0;
		fb.tex = 0;
	}

	if (m_Pass1)
		glDeleteProgram(m_Pass1);

	if (m_VAO)
		glDeleteVertexArrays(1, &m_VAO);

	if (m_VBO)
		glDeleteBuffers(1, &m_VBO);
	if (m_IBO)
		glDeleteBuffers(1, &m_IBO);

	m_VAO = 0;
	m_VBO = 0;
	m_IBO = 0;
	m_Pass1 = 0;

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

	glBindFramebuffer(GL_FRAMEBUFFER, 5);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 5);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	glDisable(GL_SCISSOR_TEST);
	glScissor(0, 0, fullScreenViewport[2], fullScreenViewport[3]);
	glViewport(0, 0, fullScreenViewport[2], fullScreenViewport[3]);

	glDisable(GL_MULTISAMPLE);

	// copy current fbo to fbo[0]
	_CopyFBO(currentFBO, m_FBOs[0].FBO, fullScreenViewport);

	// pass 1 - shring & gamma
	// render fbo[0] to fbo[1]
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOs[1].FBO);

	glBindTexture(GL_TEXTURE_2D, m_FBOs[0].tex);
	glUseProgram(m_Pass1);
	glUniform1i(m_P1_tex, 0);
	glUniform1f(m_P1_pfGamma, (GLfloat)lcfg.gamma);
	glUniform2fv(m_P1_texel, 1, texel);
	GLint target[4] = { 0, 0, fullScreenViewport[2] / 2, fullScreenViewport[3] / 2 };
	GLint source[4] = { 0, 0, fullScreenViewport[2], fullScreenViewport[3] };
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

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 5);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 5);
	glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

	// finall
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
	_CreateFBO(m_FBOs[0], m_texWidth, m_texHeight);
	_CreateFBO(m_FBOs[1], m_texWidth/2, m_texHeight/2);
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

void CPostFX::_CopyFBO(GLuint src, GLuint dst, GLint *viewport)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);

	glBlitFramebuffer(viewport[0], viewport[1], viewport[2], viewport[3],
		0, 0, viewport[2], viewport[3],
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
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

	m_SSAO_1 = _CreateShaderProgram("SSAO_1_VS", "SSAO_1_FS");
	m_S1_Texture0 = glGetUniformLocation(m_SSAO_1, "Texture0");
	m_S1_Local0 = glGetUniformLocation(m_SSAO_1, "Local0");
	m_S1_Local1 = glGetUniformLocation(m_SSAO_1, "Local1");
	m_S1_rings = glGetUniformLocation(m_SSAO_1, "rings");

	m_SSAO_1b = _CreateShaderProgram("SSAO_1b_VS", "SSAO_1b_FS");
	m_S1b_Texture0 = glGetUniformLocation(m_SSAO_1b, "Texture0");
	m_S1b_Local0 = glGetUniformLocation(m_SSAO_1b, "Local0");
	m_S1b_Local1 = glGetUniformLocation(m_SSAO_1b, "Local1");
	m_S1b_rings = glGetUniformLocation(m_SSAO_1b, "rings");

	m_SSAO_2 = _CreateShaderProgram("SSAO_1_VS", "SSAO_2_FS");

	/*
	pglUniform1i(pglGetUniformLocation(SSAO_1, "Texture0"), 0);
	pglUniform4f(pglGetUniformLocation(SSAO_1, "Local0"), float(2.0f * zNear), float(zFar - zNear), lz, (float)h / 2);
	pglUniform4f(pglGetUniformLocation(SSAO_1, "Local1"), 0.0, 1.0f, (float)cfg.SSAO_scale, (float)range);
	pglUniform1i(pglGetUniformLocation(SSAO_1, "rings"), rings);
	*/
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