#include "stdafx.h"
#include "OpenVR_client.h"
#include "Tools_inc.h"

// internal config
int _DoPostPresentHandoff = 0; // don't call PostPresentHandoff after frame submit
int _SplitSubmit = 0; // don't interlenc rendering and frame submision
int _DisableView = 0; // don't disable view of left eye on monitor
int _DisableSubmit = 0; // don't disable frame submisiont to HMD
int _PoseWait = 0; // call PoseWait when UpdateUV is called

void DoSwapBuffers();
void BAM_log(const char *fmt, ...);
void BAM_msg(const char *fmt, ...);
void BAM_hud(const char *fmt, ...);

Ccfg cfg = {
	1,
	0, 650, 500, // cam pos: x=0, y=500, z=650
	0, // AA Mode : NoAA
	VK_F12, // HomeKey = F12
	1, // VSync on
	VK_F11, // HomeKey2 = F11 <- not used
	0.0, // Frame Delay
	0, // Tracking Orgin: Sitead
	1.0 // World Scale
};

const double FPGround = -950.0;

// ================================ DEBUG =================================================
#include <iostream>

using std::cout;
using std::endl;

void APIENTRY openglCallbackFunction(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const GLvoid* userParam)
{
	std::ostream &o = std::cerr;

	o << "---------------------opengl-callback-start------------" << endl;
	o << "message: " << message << endl;
	o << "type: ";
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		o << "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		o << "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		o << "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		o << "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		o << "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		o << "OTHER";
		break;
	}
	o << endl;

	o << "id: " << id << endl;
	o << "severity: ";
	switch (severity){
	case GL_DEBUG_SEVERITY_LOW:
		o << "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		o << "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		o << "HIGH";
		break;
	}
	o << endl;
	o << "---------------------opengl-callback-end--------------" << endl;
}

void dbg_init()
{
	return;
#ifndef _DEBUG
	return;
#endif
	if (glDebugMessageCallback)
	{
		glEnable(GL_DEBUG_OUTPUT);

		glDebugMessageCallback(openglCallbackFunction, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "Starting debug messaging service");
	}
}
// ========================================================================================

COpenVR_client::COpenVR_client() :
	m_fNearClip(5.0f),
	m_fFarClip(15000.0f),
	m_pHMD(NULL),
	m_nWindowWidth(0),
	m_nWindowHeight(0),
	m_unLensProgramID(0),
	m_unLensVAO(0),
	m_glIDVertBuffer(0),
	m_glIDIndexBuffer(0),
	m_bVblank(true),
	m_bGlFinishHack(true),
	m_Eye(0),
	m_bDoCustomHomeReset(false)
{
	memset(&m_Framebuffers, 0, sizeof(m_Framebuffers));
}


COpenVR_client::~COpenVR_client()
{
	if (m_pHMD)
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();
	}

	BAM_msg("~COpenVR_client - delete framebuffers");
	for (int eye = 0; eye < 2; ++eye)
	{
		auto &fb = m_Framebuffers[eye];
		if (fb.m_nDepthBufferId)
			glDeleteRenderbuffers(1, &fb.m_nDepthBufferId);
		if (fb.m_nRenderTextureId)
			glDeleteTextures(1, &fb.m_nRenderTextureId);
		if (fb.m_nRenderFramebufferId)
			glDeleteFramebuffers(1, &fb.m_nRenderFramebufferId);
		if (fb.m_nResolveTextureId)
			glDeleteTextures(1, &fb.m_nResolveTextureId);
		if (fb.m_nResolveFramebufferId)
			glDeleteFramebuffers(1, &fb.m_nResolveFramebufferId);

		fb.m_nDepthBufferId = 0;
		fb.m_nRenderTextureId = 0;
		fb.m_nRenderFramebufferId = 0;
		fb.m_nResolveTextureId = 0;
		fb.m_nResolveFramebufferId = 0;
	}
	memset(&m_Framebuffers, 0, sizeof(m_Framebuffers));

	BAM_msg("~COpenVR_client - delete VAO");

	if (m_unLensVAO != 0)
	{
		glDeleteVertexArrays(1, &m_unLensVAO);
		m_unLensVAO = 0;
	}

	if (m_unLensProgramID)
	{
		glDeleteProgram(m_unLensProgramID);
		m_unLensProgramID = 0;
	}

	glDeleteBuffers(1, &m_glIDVertBuffer);
	glDeleteBuffers(1, &m_glIDIndexBuffer);
	m_glIDVertBuffer = 0;
	m_glIDIndexBuffer = 0;
}

// ============================================================================ COpenVR_client - public methods

/// <summary>
/// Initializes OpenVR, create framebuffers based on quality param.
/// </summary>
/// <param name="quality">The quality.</param>
/// <returns>false if fail.</returns>
bool COpenVR_client::Init(EQuality quality)
{
	bool ret = _InitHMD();
	if (!ret)
	{
		m_pHMD = NULL;
		m_nRecommendedRenderWidth = 1182;
		m_nRecommendedRenderHeight = 1464;
	}

	// Initialize glew.
	GLenum nGlewError = glewInit();
	if (nGlewError != GLEW_OK)
	{
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString(nGlewError));
		dbglog("ErrMsg: %s", m_ErrorMsg);
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	// Create render buffers
	if (!_SetupStereoRenderTargets(quality))
	{
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "%s - Error initializing FrameBuffers! %s\n", __FUNCTION__);
		dbglog("ErrMsg: %s", m_ErrorMsg);
		return false;
	}

	_CompileShaderPrograms();

	_SetupDistortion();

	return ret;
}

/// <summary>
/// Updates the View and Projection Matrix based on tracking data
/// </summary>
/// <param name="V">The pointer to View matrix.</param>
/// <param name="P">The pointer to Projection Matrix.</param>
/// <param name="eye">The eye. (0 = left, 1 = right).</param>
void COpenVR_client::UpdateVP(float *V, float *P, int eye)
{
	m_Eye = eye;
	vr::TrackedDevicePose_t *rTrackedDevicePose = m_TrackedDevicePose;

	if (m_pHMD) {
		// update Pose when we draw left eye
		if (eye == LEFT) {
			if (_PoseWait == 0) {
				tmpLog("T\n");
				vr::VRCompositor()->WaitGetPoses(m_TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
				tmpLog("WaitGetPose(%d)\n", _PoseWait);
			}

			if (rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
			{
				float fDisplayFrequency = m_pHMD->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
				float fFrameDuration = 1.f / fDisplayFrequency;
				float fVsyncToPhotons = m_pHMD->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
				float fSecondsSinceLastVsync; m_pHMD->GetTimeSinceLastVsync(&fSecondsSinceLastVsync, NULL);

				float fPredictedSecondsFromNow = fFrameDuration - fSecondsSinceLastVsync + fVsyncToPhotons + (float)(fFrameDuration*(cfg.Delay+1.0));

				if (_PoseWait != 3) {
					tmpLog("T\n");
					m_pHMD->GetDeviceToAbsoluteTrackingPose(
						cfg.TrackingMode ? vr::TrackingUniverseStanding : vr::TrackingUniverseSeated,
						fPredictedSecondsFromNow, rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount);
					tmpLog("GetDeviceToAbsoluteTrackingPose(%d)\n", _PoseWait);
				}

//				_ConvertSteamVRMatrix(m_mPose, &rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[0][0]);
				static float tmpPose[16];
				_ConvertSteamVRMatrix(tmpPose, &rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[0][0]);
				if (m_bDoCustomHomeReset)
					_DoCustomHomeReset(tmpPose);
				_Mul(m_mPose, m_mPoseHome, tmpPose);
			}
		}

		// Set view and projection matrix
		// P = Projection
		memcpy_s(P, sizeof(float[16]), m_mProjection[eye], sizeof(float[16]));

		// V = eyePoseLeft * Pose
		float tmpV[16];
		_Mul(tmpV, m_mPose, m_mEyePosition[eye]);

		// BAM uses milimeter, OVR maters, so i simply scale position. Not obvious, but works.
		float scale = 1000.0;
		tmpV[12] *= scale;
		tmpV[13] *= scale;
		tmpV[14] *= scale;

		float pX = (float)cfg.CamX;
		float pY = (float)cfg.CamY;
		float pZ = (float)cfg.CamZ;

		if (cfg.TrackingMode)
		{
			pY = (float)(FPGround);
		}

		float T[16] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			(float)-cfg.WorldScale * pX, (float)-cfg.WorldScale * pY, (float)-cfg.WorldScale * pZ, 1,
		};
		_Mul(V, T, tmpV);
	}

	// prepare next eye framebuffer
	if (m_Quality > LOW)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);

	auto &fb = m_Framebuffers[m_Eye];
	glBindFramebuffer(GL_FRAMEBUFFER, fb.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void COpenVR_client::Submit()
{
	m_bVblank = cfg.VSync == 1;

	// capture current eye framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Framebuffers[m_Eye].m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Framebuffers[m_Eye].m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	if (_SplitSubmit && !_DisableSubmit)
	{
		if (m_pHMD)
		{
			vr::Texture_t EyeTextures[2] = {
				{ (void*)m_Framebuffers[LEFT].m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma },
				{ (void*)m_Framebuffers[RIGHT].m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma }
			};

			tmpLog("T\n");
			auto result = vr::VRCompositor()->Submit(m_Eye == LEFT ? vr::Eye_Left : vr::Eye_Right, &EyeTextures[m_Eye]);
			tmpLog("Submit(%d, %d)\n", m_Eye, m_Framebuffers[m_Eye].m_nResolveTextureId);

			if (result != vr::VRCompositorError_None) {
				dbglog("submit error: %d\n", result);
			}
		}
	}

	// present framebuffers to screen
	if (m_Eye == RIGHT)
	{
		if (!_DisableView) {
			tmpLog("T\n");
			_RenderDistortion();
			tmpLog("_RenderDistortion()\n");
		}

		if (m_pHMD)
		{
			if (!_SplitSubmit && !_DisableSubmit)
			{
				tmpLog("T\n");
				vr::Texture_t leftEyeTexture = { (void*)m_Framebuffers[LEFT].m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
				auto result = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
				tmpLog("Submit(%d, %d)\n", vr::Eye_Left, m_Framebuffers[LEFT].m_nResolveTextureId);

				if (result != vr::VRCompositorError_None) {
					dbglog("submit-error: %d\n", result);
				}

				vr::Texture_t rightEyeTexture = { (void*)m_Framebuffers[RIGHT].m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
				result = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
				tmpLog("Submit(%d, %d)\n", vr::Eye_Right, m_Framebuffers[RIGHT].m_nResolveTextureId);

				if (result != vr::VRCompositorError_None) {
					dbglog("submit-error: %d\n", result);
				}
			}

			if (_DoPostPresentHandoff) {
				tmpLog("T\n");
				vr::VRCompositor()->PostPresentHandoff();
				tmpLog("PostPresentHandoff()\n");
			}
		}
		if (_PoseWait == 1) {
			tmpLog("T\n");
			vr::VRCompositor()->WaitGetPoses(m_TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
			tmpLog("WaitGetPose(%d)\n", _PoseWait);
		}

		if (m_bVblank && m_bGlFinishHack)
		{
			//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
			// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
			// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
			// 1/29/2014 mikesart
			glFinish();
		}

		// SwapWindow
		DoSwapBuffers();
		if (_PoseWait == 2) {
			tmpLog("T\n");
			vr::VRCompositor()->WaitGetPoses(m_TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
			tmpLog("WaitGetPose(%d)\n", _PoseWait);
		}

		BAM_hud("%s", tmpLogBuf);
		clearTmpLog();

		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Flush and wait for swap.
		if (m_bVblank)
		{
			glFlush();
			glFinish();
		}
	}

	if (m_NextQuality != m_Quality)
	{
		_ChangeQuality();
	}
}

void _ChangeQuality()
{

}

void COpenVR_client::Reset()
{
	if (m_pHMD)
	{
		m_pHMD->ResetSeatedZeroPose();
	}
}

// ============================================================================ COpenVR_client - testing


void Invert(float *a)
{
	float x = a[12], y = a[13], z = a[14];
	a[12] = -(a[0] * x + a[1] * y + a[2] * z);
	a[13] = -(a[4] * x + a[5] * y + a[6] * z);
	a[14] = -(a[8] * x + a[9] * y + a[10] * z);

	x = a[1];
	y = a[2];
	z = a[6];
	a[1] = a[4];
	a[2] = a[8];
	a[6] = a[9];
	a[4] = x;
	a[8] = y;
	a[9] = z;
}

// ============================================================================ COpenVR_client - private methods

bool COpenVR_client::_InitHMD()
{
	// Init HMD.
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		dbglog("ErrMsg: %s", m_ErrorMsg);
		return false;
	}

	// Get driver and display
	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = _GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	m_strDisplay = _GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
	dbglog("drv: %s", m_strDriver.c_str());
	dbglog("serial: %s", m_strDisplay.c_str());

	// Get some tracking data
	_GetHMDMatrixProjectionEye(vr::Eye_Left);
	_GetHMDMatrixProjectionEye(vr::Eye_Right);
	_GetHMDMatrixPoseEye(vr::Eye_Left);
	_GetHMDMatrixPoseEye(vr::Eye_Right);

	// Initialize VRCompositor
	if (!vr::VRCompositor())
	{
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "Compositor initialization failed. See log file for details\n");
		dbglog("ErrMsg: %s", m_ErrorMsg);
		return false;
	}

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRecommendedRenderWidth, &m_nRecommendedRenderHeight);

	_Identity(m_mPoseHome);
	return true;
}

void COpenVR_client::_Identity(float *m)
{
	memset(m, 0, sizeof(float[16]));
	m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void COpenVR_client::_Mul(float *O, float *A, float *B)
{
	O[0] = A[0] * B[0] + A[1] * B[4] + A[2] * B[8] + A[3] * B[12];
	O[1] = A[0] * B[1] + A[1] * B[5] + A[2] * B[9] + A[3] * B[13];
	O[2] = A[0] * B[2] + A[1] * B[6] + A[2] * B[10] + A[3] * B[14];
	O[3] = A[0] * B[3] + A[1] * B[7] + A[2] * B[11] + A[3] * B[15];

	O[4] = A[4] * B[0] + A[5] * B[4] + A[6] * B[8] + A[7] * B[12];
	O[5] = A[4] * B[1] + A[5] * B[5] + A[6] * B[9] + A[7] * B[13];
	O[6] = A[4] * B[2] + A[5] * B[6] + A[6] * B[10] + A[7] * B[14];
	O[7] = A[4] * B[3] + A[5] * B[7] + A[6] * B[11] + A[7] * B[15];

	O[8] = A[8] * B[0] + A[9] * B[4] + A[10] * B[8] + A[11] * B[12];
	O[9] = A[8] * B[1] + A[9] * B[5] + A[10] * B[9] + A[11] * B[13];
	O[10] = A[8] * B[2] + A[9] * B[6] + A[10] * B[10] + A[11] * B[14];
	O[11] = A[8] * B[3] + A[9] * B[7] + A[10] * B[11] + A[11] * B[15];

	O[12] = A[12] * B[0] + A[13] * B[4] + A[14] * B[8] + A[15] * B[12];
	O[13] = A[12] * B[1] + A[13] * B[5] + A[14] * B[9] + A[15] * B[13];
	O[14] = A[12] * B[2] + A[13] * B[6] + A[14] * B[10] + A[15] * B[14];
	O[15] = A[12] * B[3] + A[13] * B[7] + A[14] * B[11] + A[15] * B[15];
}

void COpenVR_client::_Transpose(float *d, float *s)
{
	d[0] = s[0];
	d[1] = s[4];
	d[2] = s[8];
	d[3] = s[12];

	d[4] = s[1];
	d[5] = s[5];
	d[6] = s[9];
	d[7] = s[13];

	d[8] = s[2];
	d[9] = s[6];
	d[10] = s[10];
	d[11] = s[14];

	d[12] = s[3];
	d[13] = s[7];
	d[14] = s[11];
	d[15] = s[15];
}

void COpenVR_client::_GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	float *m = m_mProjection[nEye];

	if (!m_pHMD)
		return _Identity(m);

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip, vr::API_OpenGL);

	_Transpose(m, &mat.m[0][0]);
}

void COpenVR_client::_ConvertSteamVRMatrix(float *d, float *s)
{
	float x = s[3], y = s[7], z = s[11];

	d[0] = s[0];
	d[1] = s[1];
	d[2] = s[2];
	d[3] = 0.0f;

	d[4] = s[4];
	d[5] = s[5];
	d[6] = s[6];
	d[7] = 0.0f;

	d[8] = s[8];
	d[9] = s[9];
	d[10] = s[10];
	d[11] = 0.0f;

	d[12] = -(s[0] * x + s[4] * y + s[8] * z);
	d[13] = -(s[1] * x + s[5] * y + s[9] * z);
	d[14] = -(s[2] * x + s[6] * y + s[10] * z);
	d[15] = 1.0f;
}

void COpenVR_client::_DoCustomHomeReset(const float *pose)
{
	m_bDoCustomHomeReset = false;

	const float *M = pose;
	float a = atan2(M[2], M[10]);
	float *R = m_mPoseHome;
	memset(R, 0, sizeof(float[16]));

	float c = cosf(a);
	float s = sinf(a);

	R[0] = c;
	R[2] = -s;
	R[8] = s;
	R[10] = c;
	R[5] = 1;
	R[15] = 1;

	float *T = m_mPoseHome;
	float x = M[12], y = M[13], z = M[14];
	T[12] = -(M[0] * x + M[1] * y + M[2] * z);
	T[13] = -(M[4] * x + M[5] * y + M[6] * z);
	T[14] = -(M[8] * x + M[9] * y + M[10] * z);

	if (cfg.TrackingMode) {
		T[13] = 0;
		return;
	}
}

std::string COpenVR_client::_GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
	if (!m_pHMD)
		return "";

	uint32_t unRequiredBufferLen = m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

void COpenVR_client::_GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	float *m = m_mEyePosition[nEye];

	if (!m_pHMD)
		return _Identity(m);

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);

	_ConvertSteamVRMatrix(m, &matEyeRight.m[0][0]);
}

bool COpenVR_client::_SetupStereoRenderTargets(EQuality quality)
{
	m_nRenderWidth = m_nRecommendedRenderWidth;
	m_nRenderHeight = m_nRecommendedRenderHeight;

	int nMSAA = 1;
	m_Quality = quality;
	switch (quality)
	{
	case LOW:
		nMSAA = 1;
		break;
	case MEDIUM:
		nMSAA = 2;
		break;
	case HIGH:
		nMSAA = 4;
		break;
	case UBER:
		nMSAA = 6;
		break;
	case ULTRA:
		nMSAA = 8;
		break;
	}

	_CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, nMSAA, m_Framebuffers[0]);
	_CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, nMSAA, m_Framebuffers[1]);

	return true;
}

bool COpenVR_client::_CreateFrameBuffer(int nWidth, int nHeight, int nMSAA, FramebufferDesc &framebufferDesc)
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glRenderbufferStorageMultisample(GL_RENDERBUFFER, nMSAA, GL_DEPTH24_STENCIL8, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nMSAA, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		dbglog("CreateFrameBuffer() - fail");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void COpenVR_client::_ChangeQuality()
{
	// release render targets
	for (int eye = 0; eye < 2; ++eye)
	{
		auto &fb = m_Framebuffers[eye];
		if (fb.m_nDepthBufferId)
			glDeleteRenderbuffers(1, &fb.m_nDepthBufferId);
		if (fb.m_nRenderTextureId)
			glDeleteTextures(1, &fb.m_nRenderTextureId);
		if (fb.m_nRenderFramebufferId)
			glDeleteFramebuffers(1, &fb.m_nRenderFramebufferId);
		if (fb.m_nResolveTextureId)
			glDeleteTextures(1, &fb.m_nResolveTextureId);
		if (fb.m_nResolveFramebufferId)
			glDeleteFramebuffers(1, &fb.m_nResolveFramebufferId);

		fb.m_nDepthBufferId = 0;
		fb.m_nRenderTextureId = 0;
		fb.m_nRenderFramebufferId = 0;
		fb.m_nResolveTextureId = 0;
		fb.m_nResolveFramebufferId = 0;
	}

	// create new render targets
	_SetupStereoRenderTargets(m_NextQuality);
}

void COpenVR_client::_CompileShaderPrograms()
{
	m_unLensProgramID = _CompileGLShader(
		"Distortion",

		// vertex shader
		"#version 410 core\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec2 v2UVredIn;\n"
		"layout(location = 2) in vec2 v2UVGreenIn;\n"
		"layout(location = 3) in vec2 v2UVblueIn;\n"
		"noperspective  out vec2 v2UVred;\n"
		"noperspective  out vec2 v2UVgreen;\n"
		"noperspective  out vec2 v2UVblue;\n"
		"void main()\n"
		"{\n"
		"	v2UVred = v2UVredIn;\n"
		"	v2UVgreen = v2UVGreenIn;\n"
		"	v2UVblue = v2UVblueIn;\n"
		"	gl_Position = position;\n"
		"}\n",

		// fragment shader
		"#version 410 core\n"
		"uniform sampler2D mytexture;\n"

		"noperspective  in vec2 v2UVred;\n"
		"noperspective  in vec2 v2UVgreen;\n"
		"noperspective  in vec2 v2UVblue;\n"

		"out vec4 outputColor;\n"

		"void main()\n"
		"{\n"
		"	float fBoundsCheck = ( (dot( vec2( lessThan( v2UVgreen.xy, vec2(0.05, 0.05)) ), vec2(1.0, 1.0))+dot( vec2( greaterThan( v2UVgreen.xy, vec2( 0.95, 0.95)) ), vec2(1.0, 1.0))) );\n"
		"	if( fBoundsCheck > 1.0 )\n"
		"	{ outputColor = vec4( 0, 0, 0, 1.0 ); }\n"
		"	else\n"
		"	{\n"
		"		float red = texture(mytexture, v2UVred).x;\n"
		"		float green = texture(mytexture, v2UVgreen).y;\n"
		"		float blue = texture(mytexture, v2UVblue).z;\n"
		"		outputColor = vec4( red, green, blue, 1.0  ); }\n"
		"}\n"
		);
}

GLuint COpenVR_client::_CompileGLShader(const char *pchShaderName, const char *pchVertexShader, const char *pchFragmentShader)
{
	GLuint unProgramID = glCreateProgram();

	GLuint nSceneVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(nSceneVertexShader, 1, &pchVertexShader, NULL);
	glCompileShader(nSceneVertexShader);

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneVertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "%s - Unable to compile vertex shader %d!\n", pchShaderName, nSceneVertexShader);
		glDeleteProgram(unProgramID);
		glDeleteShader(nSceneVertexShader);
		return 0;
	}
	glAttachShader(unProgramID, nSceneVertexShader);
	glDeleteShader(nSceneVertexShader); // the program hangs onto this once it's attached

	GLuint  nSceneFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(nSceneFragmentShader, 1, &pchFragmentShader, NULL);
	glCompileShader(nSceneFragmentShader);

	GLint fShaderCompiled = GL_FALSE;
	glGetShaderiv(nSceneFragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
	if (fShaderCompiled != GL_TRUE)
	{
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "%s - Unable to compile fragment shader %d!\n", pchShaderName, nSceneFragmentShader);
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
		sprintf_s(m_ErrorMsg, sizeof(m_ErrorMsg), "%s - Error linking program %d!\n", pchShaderName, unProgramID);
		glDeleteProgram(unProgramID);
		return 0;
	}

	glUseProgram(unProgramID);
	glUseProgram(0);

	return unProgramID;
}

struct Vector2
{
	float x;
	float y;

	Vector2(float _x, float _y) : x(_x), y(_y) {}
	Vector2() : x(0), y(0) {}
};

struct VertexDataLens
{
	Vector2 position;
	Vector2 texCoordRed;
	Vector2 texCoordGreen;
	Vector2 texCoordBlue;
};

void COpenVR_client::_SetupDistortion()
{
	GLushort m_iLensGridSegmentCountH = 43;
	GLushort m_iLensGridSegmentCountV = 43;

	float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
	float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

	float u, v = 0;

	std::vector<VertexDataLens> vVerts(0);
	VertexDataLens vert;

	//left eye distortion verts
	float Xoffset = -1;
	for (int y = 0; y<m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
//			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);
			vert.position = Vector2(Xoffset + u * 2, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = _ComputeDistortion(vr::Eye_Left, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	//right eye distortion verts
	Xoffset = 0;
	for (int y = 0; y<m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = _ComputeDistortion(vr::Eye_Right, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}
	m_uiIndexSize = vIndices.size();

	glGenVertexArrays(1, &m_unLensVAO);
	glBindVertexArray(m_unLensVAO);

	glGenBuffers(1, &m_glIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size()*sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size()*sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

vr::DistortionCoordinates_t COpenVR_client::_ComputeDistortion(vr::EVREye eEye, float fU, float fV)
{
//	if (m_pHMD)
//		return m_pHMD->ComputeDistortion(eEye, fU, fV);

	vr::DistortionCoordinates_t d;

	// lame
	d.rfBlue[0] = d.rfGreen[0] = d.rfRed[0] = fU;
	d.rfBlue[1] = d.rfGreen[1] = d.rfRed[1] = fV;
	return d;
}

void COpenVR_client::_RenderDistortion()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);

	glBindVertexArray(m_unLensVAO);
	glUseProgram(m_unLensProgramID);

	//render left lens (first half of index array )
	glBindTexture(GL_TEXTURE_2D, m_Framebuffers[LEFT].m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, 0);
	/*
	//render right lens (second half of index array )
	glBindTexture(GL_TEXTURE_2D, m_Framebuffers[RIGHT].m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));
	*/
	glBindVertexArray(0);
	glUseProgram(0);
}
