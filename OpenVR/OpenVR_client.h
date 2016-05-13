
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
	void CustomReset() { m_bDoCustomHomeReset = true; }

	bool IsWindowSizeSet() { return m_nWindowWidth != 0; }
	void SetWindowSize(uint32_t w, uint32_t h) { m_nWindowWidth = w; m_nWindowHeight = h; }

	void SetQuality(EQuality quality) { m_NextQuality = quality; }
	void SetQuality(int quality) { m_NextQuality = (EQuality)quality; }

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
	void _DoCustomHomeReset(const float *pose);
	vr::DistortionCoordinates_t _ComputeDistortion(vr::EVREye eEye, float fU, float fV);

	// some math helpers
	void _Identity(float *m);
	void _Transpose(float *d, float *s);
	void _Mul(float *O, float *A, float *B);
	void _ConvertSteamVRMatrix(float *d, float *s);
	bool _InitHMD();

	//	void SetViewport(int x, int y, int w, int h);
	//	void SetFramebuffer(GLuint fb);

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

	bool m_bVblank;
	bool m_bGlFinishHack;

	bool m_bDoCustomHomeReset;
};

extern int _DoPostPresentHandoff;
extern int _SplitSubmit;
extern int _DisableView;
extern int _DisableSubmit;
extern int _PoseWait;
