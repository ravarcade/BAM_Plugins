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

struct Ccfg {
	double gamma;
	double BlurScale;
	double BlurStrength;
	int Enabled;
	double BlurAmount;

	double SSAO_range;
	double SSAO_scale;
	double SSAO_BlurScale;
	double SSAO_BlurStrength;

	int SSAO_mode;
	int SSAO_res;
	int SSAO_type;
	double SSAO_MAX;
	int SSAO_rings;
};

extern Ccfg cfg;

struct FB {
	GLuint FBO;
	GLuint tex;
	GLuint depthTex;
	GLint dim[2];
	FB() : FBO(0), tex(0), depthTex(0) {}
};

class CPostFX
{
private:
	bool m_doInit;
	int m_width;
	int m_height;
	int m_texWidth;
	int m_texHeight;

	void _Init();
	void _VerifyRes();
	void _CreateFBOs();
	bool _CreateFBO(FB &fb, int width, int height);
	bool _CreateFBOWithDepth(FB &fb, int width, int height);
	void _CopyFBO(GLuint src, GLuint dst, GLint *viewport);
	void _CopyFBOWithDepth(GLuint src, GLuint dst, GLint *viewport);
	void _CreateShaders();
	GLuint CPostFX::_CreateShaderProgram(const char *vertexSrc, const char *fragmentSrc);
	void _DrawQuad(GLint *dstRect, GLint *dstSize, GLint *srcRect, GLint *srcSize, GLint locPostion, GLint locTexture);
	void _DrawQuad2(GLint *dstRect, GLint *dstSize, GLint *srcRect, GLint *srcSize, GLint locPostion, GLint locTexture);

	FB m_FBOs[3];

	GLuint m_VBO;
	GLuint m_IBO;
	GLuint m_VAO;

	GLuint m_Pass1;
	GLint m_P1_ScalePosition;
	GLint m_P1_ScaleTexture;
	GLint m_P1_pfGamma;
	GLint m_P1_texel;
	GLint m_P1_tex;

	GLuint m_Pass2;
	GLint m_P2_ScalePosition;
	GLint m_P2_ScaleTexture;
	GLint m_P2_Orientation;
	GLint m_P2_BlurAmount;
	GLint m_P2_BlurScale;
	GLint m_P2_BlurStrength;
	GLint m_P2_TexelSize;
	GLint m_P2_tex;

	GLuint m_Pass3;
	GLint m_P3_ScalePosition;
	GLint m_P3_ScaleTexture;
	GLint m_P3_amount;
	GLint m_P3_tex;
	GLint m_P3_tex2;

	struct {
		GLuint Program;
		GLint ScalePosition;
		GLint ScaleTexture;
		GLint pixelSize;
		GLint aoRangeLevelAspect;
		GLint gamma;
		GLint frameBounds;

		GLint Texture0;
		GLint Texture1;
		GLint rings;
	} m_SSAO[2];
	
	GLuint m_SSAO_2;
	GLint m_S2_ScalePosition;
	GLint m_S2_ScaleTexture;
	GLint m_S2_ssao_max_blur_amount;
	GLint m_S2_tex;
	GLint m_S2_tex2;

public:
	CPostFX();
	~CPostFX();

	void Init();
	void Release();

	void Postprocess();
};