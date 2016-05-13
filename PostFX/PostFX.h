
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
	GLint dim[2];
	FB() : FBO(0), tex(0) {}
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
	void _CopyFBO(GLuint src, GLuint dst, GLint *viewport);
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

	GLuint m_SSAO_1;
	GLint m_S1_Texture0;
	GLint m_S1_Local0;
	GLint m_S1_Local1;
	GLint m_S1_rings;

	GLuint m_SSAO_1b;
	GLint m_S1b_Texture0;
	GLint m_S1b_Local0;
	GLint m_S1b_Local1;
	GLint m_S1b_rings;

	GLuint m_SSAO_2;

public:
	CPostFX();
	~CPostFX();

	void Init();
	void Release();

	void Postprocess();
};