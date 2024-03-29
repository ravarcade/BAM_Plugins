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

// main.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "scriptCommands.h"

void BAM_msg(const char *fmt, ...);

#define PLUGIN_ID_NUM 0x1003
#define BAM_USE_OnUpdateVP
#define BAM_USE_OnSwapBuffers
#define BAM_USE_GetNumberOfCamViews
#include "BAM_inc.h"

#include "OpenVR_client.h"

COpenVR_client *ovr = NULL;
const char *quality_modes[] = { "Off", "2x", "4x", "6x", "8x" };
const char *cfgFileName = "OpenVR";
const char *vsync[] = { "Off", "On" };
const char *trackingMode[] = { "Seated", "Standing" };
const char *poseWaitTxt[] = { "[1]", "[2]", "[3]", "[4]" };
const char *showViveControllersTxt[] = { "Hide", "Show w/BAM menu", "Show Allways" };

// pointers to BAM table cfg data
double *pCfgY, *pCfgZ, *pCfgAngle;
OpenVrCOM* sc = nullptr;

void OnLoad()
{
#ifdef _DEBUG
	//	BAM::MessageBox("Now attache debuger to Future Pinball.exe\n[menu: DEBUG -> Attache to process..]");
#endif
	BAM::menu_create(PLUGIN_ID_NUM, "OpenVR", "by #cff0#Rafal Janicki");
	BAM::menu_add_cam(PLUGIN_ID_NUM);
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Tracking orgin:"DEFPW"#+ #cfff#%s", &cfg.TrackingMode, trackingMode, ARRAY_ENTRIES(trackingMode), "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Scale:"DEFPW2"%.2f", &cfg.WorldScale, 0.01, 0.1, "");
	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-Start cam position"DEFIW);
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-X:"DEFPW2"%.2f", &cfg.CamX, 1.0, 20.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Y:"DEFPW2"%.2f", &cfg.CamY, 1.0, 20.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Z:"DEFPW2"%.2f", &cfg.CamZ, 1.0, 20.0, "");
	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-Other"DEFIW);
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-AA Mode:"DEFPW"#+ #cfff#%s", &cfg.Quality, quality_modes, ARRAY_ENTRIES(quality_modes), "");
//	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Pixel Density:"DEFPW2"%.2f", &cfg.PixelDensity, 0.01, 0.05, "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-VSync:"DEFPW"#+ #cfff#%s", &cfg.VSync, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_key(PLUGIN_ID_NUM, "#-Home key:"DEFPW"#+%s#r85#", &cfg.HomeKey);
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Show controlers:"DEFPW"#+ #cfff#%s", &cfg.ShowControllers, showViveControllersTxt, ARRAY_ENTRIES(showViveControllersTxt), "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Frame latency:"DEFPW2"%.0f", &cfg.Delay, 1., 1, "Number of frames rendered before display.[-1 , 2]");
#ifdef _DEBUG
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-PresentHandoff:"DEFPW"#+ #cfff#%s", &_DoPostPresentHandoff, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-SplitSubmit:"DEFPW"#+ #cfff#%s", &_SplitSubmit, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-DisableView:"DEFPW"#+ #cfff#%s", &_DisableView, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-DisableSubmit:"DEFPW"#+ #cfff#%s", &_DisableSubmit, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-PoseWait:"DEFPW"#+ #cfff#%s", &_PoseWait, poseWaitTxt, ARRAY_ENTRIES(poseWaitTxt), "");
#endif

	//BAM::sub
	int submenuid = BAM::create_submenu(PLUGIN_ID_NUM);
	BAM::menu_add_info(submenuid, "#c777##-Free Cam Options"DEFIW);
	BAM::menu_add_back_button(submenuid);
	BAM::menu_add_switch(submenuid, "#-Enable Free Cam:"DEFPW"#+ #cfff#%s", &cfg.moveEnabled, vsync, ARRAY_ENTRIES(vsync), "");
	BAM::menu_add_param(submenuid, "#-Move speed:"DEFPW2"%.0f", &cfg.move_speed, 1., 1, "");
	BAM::menu_add_button(submenuid, "#-Set Default Camera Position"DEFIW, [](int) { if (ovr) ovr->MoveCombine(); });
	BAM::menu_add_info(submenuid, "#c777##-Redefine Keys"DEFIW);
	BAM::menu_add_key(submenuid, "#-Forward:"DEFPW"#+%s#r85#", &cfg.key_Forward);
	BAM::menu_add_key(submenuid, "#-Back:"DEFPW"#+%s#r85#", &cfg.key_Backward);
	BAM::menu_add_key(submenuid, "#-Left:"DEFPW"#+%s#r85#", &cfg.key_Left);
	BAM::menu_add_key(submenuid, "#-Right:"DEFPW"#+%s#r85#", &cfg.key_Right);
	BAM::menu_add_key(submenuid, "#-Up:"DEFPW"#+%s#r85#", &cfg.key_Up);
	BAM::menu_add_key(submenuid, "#-Down:"DEFPW"#+%s#r85#", &cfg.key_Down);
	BAM::menu_add_key(submenuid, "#-Free Cam On/Off:"DEFPW"#+%s#r85#", &cfg.key_FreeCamSwitch);
	BAM::menu_add_param(submenuid, "#-Rotation speed:"DEFPW2"%.0f", &cfg.rot_speed, 1., 1, "");
	BAM::menu_add_key(submenuid, "#-Rot-Left:"DEFPW"#+%s#r85#", &cfg.key_rotLeft);
	BAM::menu_add_key(submenuid, "#-Rot-Right:"DEFPW"#+%s#r85#", &cfg.key_rotRight);
	BAM::menu_add_back_button(submenuid);

	BAM::menu_add_submenu(PLUGIN_ID_NUM, "Free Cam Options"DEFIW, submenuid, "");

	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-Standard options"DEFIW);
	BAM::menu_add_TL(PLUGIN_ID_NUM);
	BAM::menu_add_Reality(PLUGIN_ID_NUM);
	// Load configuration
	BAM::LoadCfg(cfgFileName, &cfg, sizeof(cfg));

	// Set pointers to BAM cfg table fields
	BAM::menu_additional_data(NULL, NULL, NULL, NULL, &pCfgY, &pCfgZ, &pCfgAngle, NULL, NULL, NULL);
}

int GetNumberOfCamViews() { return 0; }

void OnUnload() { OnPluginStop(); }

void OnPluginStart()
{
	ovr = new COpenVR_client();

	//	if (!ovr->InitFBOnly((EQuality)cfg.Quality))
	if (!ovr->Init((EQuality)cfg.Quality))
	{
		BAM::hudDebugLong(ovr->GetErrorMessage());
	}
}

void OnPluginStop()
{
	BAM_msg("OnPluginStop()");
	if (ovr)
	{
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER, 0);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
		BAM_msg("OnPluginStop()-call delete ovr");
		delete ovr;
		ovr = NULL;
	}
	// release reasources
	if (sc) {
		delete sc;
		sc = nullptr;
	}
	BAM_msg("OnPluginStop()-exit");
}

void dbgMatrix(const char *name, float *m)
{
	BAM::hudDebug("%s:\n[%.4f, %.4f, %.4f, %.4f]\n[%.4f, %.4f, %.4f, %.4f]\n[%.4f, %.4f, %.4f, %.4f]\n[%.4f, %.4f, %.4f, %.4f]\n",
		name,
		m[0], m[1], m[2], m[3],
		m[4], m[5], m[6], m[7],
		m[8], m[9], m[10], m[11],
		m[12], m[13], m[14], m[15]
		);
}

int HudIsVisibleCounter = 0;
void OnHudDisplay()
{
	HudIsVisibleCounter = 5;
	if (ovr)
	{
	}
}

void OnUpdateVP(float *V, float *P, int eye)
{
	if (HudIsVisibleCounter > 0)
	{
		--HudIsVisibleCounter;
	}
	if (ovr)
		ovr->SetShowControllers((cfg.ShowControllers == 1 && HudIsVisibleCounter > 0) || cfg.ShowControllers == 2);

	if (cfg.WorldScale < 0.2)
		cfg.WorldScale = 0.2;
	if (cfg.WorldScale > 5.0)
		cfg.WorldScale = 5.0;

	cfg.Delay = round(cfg.Delay);
	if (cfg.Delay < -1.0)
		cfg.Delay = -1.0;
	if (cfg.Delay > 2.0)
		cfg.Delay = 2.0;

	BAM::SetScale(cfg.WorldScale);
	static int lastEye = -1;
	if (lastEye == eye && eye == LEFT)
		eye = RIGHT;
	lastEye = eye;

	if (ovr)
	{
		ovr->UpdateVP(V, P, eye);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(P);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(V);
	}
}

int ReadKeyAsync(int key)
{
	key &= 0x1ff; // 256 keys + 256 joy buttons
	//	SHORT state = GetAsyncKeyState(key);
	SHORT state = BAM::GetAsyncButtonState(key);
	return (state << 8) != 0;
}


int ReadKeyboard(int key)
{
	int ret = 0;

	key &= 0x1ff; // 256 keys + 256 joy buttons
	static SHORT old_states[512];
	static bool once = true;
	if (once)
	{
		for (unsigned int i = 0; i<512; ++i)
		{
			old_states[i] = 0;
		}

		once = false;
	}

//	SHORT state = GetAsyncKeyState(key);
	SHORT state = BAM::GetAsyncButtonState(key);
	if (((state << 8) != 0) && ((old_states[key] << 8) == 0)) {
		ret = 1;
	}

	old_states[key] = state;
	return ret;
}

HDC global_hDC;

void DoSwapBuffers()
{
	BAM::SwapBuffers(global_hDC);
}

void BAM_log(const char *fmt, ...)
{
	return;
	va_list ap;
	va_start(ap, fmt);
	BAM::VhudDebugLong(fmt, ap);
	va_end(ap);
}

void BAM_hud(const char *fmt, ...)
{
//	return;
	va_list ap;
	va_start(ap, fmt);
	BAM::VhudDebug(fmt, ap);
	va_end(ap);
}

void BAM_msg(const char *fmt, ...)
{
	return;
	va_list ap;
	va_start(ap, fmt);
	BAM::VMessageBox(fmt, ap);
	va_end(ap);
}

void OnSwapBuffers(HDC hDC)
{
	global_hDC = hDC;

	if (!ovr->IsWindowSizeSet())
	{
		RECT rc = { 0 };
		HWND hWnd = ::WindowFromDC(hDC);
		::GetClientRect(hWnd, &rc);
		LONG w = rc.left - rc.right, h = rc.top - rc.bottom;
		if (w < 0) w = -w;
		if (h < 0) h = -h;
		ovr->SetWindowSize(w, h);
	}

	// configuration save
	static Ccfg oldcfg = { -1 };
	if (memcmp(&oldcfg, &cfg, sizeof(cfg)) != 0)	{
		BAM::SaveCfg(cfgFileName, &cfg, sizeof(cfg));
		memcpy(&oldcfg, &cfg, sizeof(cfg));
	}

	if (ovr)
	{
		ovr->SetQuality(cfg.Quality);
		ovr->Submit();
		if (ReadKeyboard(cfg.HomeKey))
		{
			ovr->Reset();
			BAM::hudDebugLong("[reset]");
			ovr->CustomReset();
		}

		if (ReadKeyboard(cfg.key_FreeCamSwitch))
		{
			cfg.moveEnabled = cfg.moveEnabled ? 0 : 1;
		}

		if (cfg.moveEnabled) {
			static float move[4];
			move[0] = move[1] = move[2] = move[3] = 0;

			if (ReadKeyAsync(cfg.key_Forward))
			{
				BAM::hudDebug("[W]");
				move[2] = -1;
			}
			if (ReadKeyAsync(cfg.key_Backward))
			{
				BAM::hudDebug("[S]");
				move[2] = 1;
			}
			if (ReadKeyAsync(cfg.key_Left))
			{
				BAM::hudDebug("[A]");
				move[0] = -1;
			}
			if (ReadKeyAsync(cfg.key_Right))
			{
				BAM::hudDebug("[D]");
				move[0] = 1;
			}

			if (ReadKeyAsync(cfg.key_Up))
			{
				BAM::hudDebug("[U]");
				move[1] = 1;
			}
			if (ReadKeyAsync(cfg.key_Down))
			{
				BAM::hudDebug("[d]");
				move[1] = -1;
			}

			if (ReadKeyAsync(cfg.key_rotLeft))
			{
				move[3] = -1;
			}

			if (ReadKeyAsync(cfg.key_rotRight))
			{
				move[3] = 1;
			}

			if (move[0] || move[1] || move[2] || move[3])
			{
				ovr->Move(move, static_cast<float>(cfg.move_speed), static_cast<float>(cfg.rot_speed));
			}
		}
	}
}

extern "C" {

	// Set icom = xBAM.Get("icom")
	BAMEXPORT IDispatch* BAM_GetCOM()
	{
		if (sc == nullptr)
			sc = new OpenVrCOM();

		return sc;
	}
}