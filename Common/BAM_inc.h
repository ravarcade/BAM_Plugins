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

#include "BAM.h"

namespace BAM {

	// List of used functions from BAM.h
	BAM_Function(VMessageBox);
	BAM_Function(VhudDebug);
	BAM_Function(VhudDebugLong);

	BAM_Function(MessageBox);
	BAM_Function(hudDebug);
	BAM_Function(hudDebugLong);

	BAM_Function(menu_create);
	BAM_Function(menu_add_cam);
	BAM_Function(menu_add_info);
	BAM_Function(menu_add_TL);
	BAM_Function(menu_add_Reality);

	BAM_Function(SwapBuffers);

	BAM_Function(menu_add_switch);
	BAM_Function(menu_add_param);
	BAM_Function(menu_add_key);

	BAM_Function(SetScale);
	BAM_Function(menu_additional_data);

	BAM_Function(GetAsyncButtonState);
};

// functions called from BAM.dll - they must be implemented.
void OnLoad();
void OnUnload();
void OnPluginStart();
void OnPluginStop();
void OnHudDisplay();

extern "C" {
	BAMEXPORT int BAM_load(HMODULE bam_module)
	{
		BAM_Internal::Init(PLUGIN_ID_NUM, bam_module);
		OnLoad();
		return S_OK;
	}

	BAMEXPORT void BAM_PluginStart() {
		OnPluginStart();
	}

	BAMEXPORT void BAM_PluginStop() {
		OnPluginStop();
	}
	
	BAMEXPORT void BAM_unload() {
		OnUnload();
	}

	BAMEXPORT void BAM_hudDisplay() {
		OnHudDisplay();
	}
};

int GetNumberOfCamViews();
#ifdef BAM_USE_GetNumberOfCamViews
int GetNumberOfCamViews();
extern "C" {
	BAMEXPORT int BAM_hudCamView() {
		return GetNumberOfCamViews();
	}
};
#endif


#ifdef BAM_USE_OnUpdateVP
void OnUpdateVP(float *V, float *P, int eye);
extern "C" {
	BAMEXPORT void BAM_updateVP(float *V, float *P, int eye)
	{
		OnUpdateVP(V, P, eye);
	}
};
#endif

#ifdef BAM_USE_OnSwapBuffers
void OnSwapBuffers(HDC hDC);
extern "C" {
	BAMEXPORT void BAM_swapBuffers(HDC hDC)
	{
		OnSwapBuffers(hDC);
	}
}
#endif