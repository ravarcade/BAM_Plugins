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

class CGLRenderModel
{
public:
	CGLRenderModel(const std::string & sRenderModelName);
	~CGLRenderModel();

	bool BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }

private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};

struct Ccfg {
	int Enabled;
	double CamX, CamY, CamZ;
	int Quality;
	DWORD HomeKey;
	int VSync;
	DWORD HomeKey2;
	double Delay;
	int TrackingMode;
	double WorldScale;
	double PixelDensity;
	int ShowControllers;

	DWORD key_Forward, key_Backward, key_Left, key_Right, key_Up, key_Down;
	double move_speed;
	int moveEnabled;
	DWORD key_FreeCamSwitch;
};

extern Ccfg cfg;

enum EEye {
	LEFT = 0,
	RIGHT = 1
};

/// <summary>
/// Quality constants. 
/// </summary>
enum EQuality {
	LOW = 0,
	MEDIUM = 1,
	HIGH = 2,
	UBER = 3,
	ULTRA = 4
};

/// <summary>
/// Store openGL FrameBuffers and Textures used to render for single eye.
/// </summary>
struct FramebufferDesc
{
	GLuint m_nDepthBufferId;
	GLuint m_nRenderTextureId;
	GLuint m_nRenderFramebufferId;
	GLuint m_nResolveTextureId;
	GLuint m_nResolveFramebufferId;
};

/// <summary>
/// Used to control OpenVR.
/// </summary>
class COpenVR_client
{
public:
	COpenVR_client();
	~COpenVR_client();

	bool Init(EQuality quality);
	bool Init(int quality) { return Init((EQuality)quality); }

	const char *GetErrorMessage() { return m_ErrorMsg; }

	void UpdateVP(float *V, float *P, int eye);
	void Submit();
	void Reset();
	void CustomReset() 
	{ 
		m_bDoCustomHomeReset = true; 
		m_movePos[0] = m_movePos[1] = m_movePos[2] = 0;
	}


	bool IsWindowSizeSet() { return m_nWindowWidth != 0; }
	void SetWindowSize(uint32_t w, uint32_t h) { m_nWindowWidth = w; m_nWindowHeight = h; }

	void SetQuality(EQuality quality) { m_NextQuality = quality; }
	void SetQuality(int quality) { m_NextQuality = (EQuality)quality; }

	void SetShowControllers(bool v) { m_ShowControllers = v; }
	void Move(float *v, float speed);
	void MoveCombine()
	{
		cfg.CamX += m_movePos[0];
		cfg.CamY += m_movePos[1];
		cfg.CamZ += m_movePos[2];
		m_movePos[0] = m_movePos[1] = m_movePos[2] = 0;
	}

private:
	void _GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	void _GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	bool _CreateFrameBuffer(int nWidth, int nHeight, int nMSAA, FramebufferDesc &framebufferDesc);
	void _RenderDistortion();
	bool _SetupStereoRenderTargets(EQuality quality);
	void _CompileShaderPrograms();
	GLuint _CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader);
	void _SetupDistortion();
	void _ChangeQuality();
	void _ReleaseRenderTargets();
	void _DoCustomHomeReset(const float *pose);
	void _PoseTranslationResetForDK1(const float *pose);

	vr::DistortionCoordinates_t _ComputeDistortion(vr::EVREye eEye, float fU, float fV);

	// some math helpers
	void _Identity(float *m);
	void _Transpose(float *d, float *s);
	void _Mul(float *O, const float *A, const float *B);
	void _MulVert(float *O, const float *A, const float *B, bool sw = false);
	void _ConvertSteamVRMatrix(float *d, const float *s);
	void _ConvertSteamVRMatrixWithoutInversion(float *d, const float *s);
	bool _InitHMD();
	void _UpdateProjection();

	// ======================================================================== Render models
	void _SetupRenderModels();
	void _SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
	CGLRenderModel *_FindOrLoadRenderModel(const char *pchRenderModelName);
	void _RenderModels();
	void _ProcessEvents();

	std::vector< CGLRenderModel * > m_vecRenderModels;
	CGLRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
	GLuint m_unRenderModelProgramID;
	GLint m_nRenderModelMatrixLocation;
	float m_move[3];
	float m_moveSpeed;
	float m_movePos[3];

	// ========================================================================

	std::string _GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL);
private:
	vr::IVRSystem *m_pHMD;
	EQuality m_Quality;
	EQuality m_NextQuality;
	char m_ErrorMsg[1024];
	vr::TrackedDevicePose_t m_TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

	std::string m_strDriver;
	std::string m_strDisplay;

	float m_fNearClip;
	float m_fFarClip;

	float m_mProjection[2][16];
	float m_mEyePosition[2][16];
	float m_mPose[16];
	float m_mPoseHome[16];

	// used to setup renderer
	uint32_t m_nRecommendedRenderWidth;
	uint32_t m_nRecommendedRenderHeight;
	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;
	FramebufferDesc m_Framebuffers[2];

	int m_Eye;

	uint32_t m_nWindowWidth;
	uint32_t m_nWindowHeight;
	GLuint m_unLensProgramID;
	GLuint m_unLensVAO;
	GLuint m_glIDVertBuffer;
	GLuint m_glIDIndexBuffer;

	uint32_t m_uiIndexSize;

	float m_PixelDensity;
	bool m_UseSplitScreen;
	bool m_bVblank;
	bool m_bGlFinishHack;

	bool m_bDoCustomHomeReset;
	bool m_DoPoseTranslationResetForDK1;

	float m_FBO_PixelDensity;
	uint32_t m_FBO_Width;
	uint32_t m_FBO_Height;
	bool m_FBO_SplitScreen;

	bool m_ShowControllers;
	double m_WorldScale;
};

extern int _DoPostPresentHandoff;
extern int _SplitSubmit;
extern int _DisableView;
extern int _DisableSubmit;
extern int _PoseWait;
