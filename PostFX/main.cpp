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

#define PLUGIN_ID_NUM 0x4001
#define BAM_USE_OnSwapBuffers
#include "BAM_inc.h"

#include "PostFX.h"

const char *YesNo_txt[] = {	"No", "Yes" };
const char *ssao_modes[] = { "desktop", "arcade" };
const char *ssao_res[] = { "low", "hi" };
const char *ssao_type[] = { "off", "fastest / low", "fastest / hi", "fast / low", "fast / hi", "normal / low", "normal / hi", "slow / low", "slow / hi", "manual" };
#define ARRAY_ENTRIES(array)	(sizeof(array)/sizeof(array[0]))

struct ssao_preset {
	double scale;
	double range;
	double blurScale;
	double blurStrength;
	int res;
	double max;
	int rings;
} ssao_presets_desktop[] = {
	30, 20.0, 1.5, 0.08, 0, 0.6, 1,
	15, 20.0, 1.5, 0.08, 0, 0.6, 1,
	30, 20.0, 1.5, 0.08, 0, 0.5, 1,
	15, 20.0, 1.5, 0.08, 0, 0.6, 2,
	30, 20.0, 1.5, 0.08, 0, 0.5, 2,
	15, 20.0, 1.5, 0.08, 0, 0.6, 3,
	30, 20.0, 1.5, 0.08, 0, 0.5, 3,
	15, 20.0, 1.5, 0.08, 1, 0.6, 3,
	30, 20.0, 1.5, 0.08, 1, 0.5, 3

}, ssao_presets_arcade[] = {
		30.0, 20.0, 1.5, 0.08, 0, 0.6, 1,
		15.0, 20.0, 1.5, 0.08, 0, 0.6, 1,
		30.0, 20.0, 1.5, 0.08, 0, 0.5, 1,
		15.0, 20.0, 1.5, 0.08, 0, 0.6, 2,
		30.0, 20.0, 1.5, 0.08, 0, 0.5, 2,
		15.0, 20.0, 1.5, 0.08, 0, 0.6, 3,
		30.0, 20.0, 1.5, 0.08, 0, 0.5, 3,
		15.0, 20.0, 1.5, 0.08, 1, 0.6, 3,
		30.0, 20.0, 1.5, 0.08, 1, 0.5, 3
	};

CPostFX PostFX;

extern double Z_Near, Z_Far, SSAO_AddlMult;

void OnLoad()
{
	BAM::menu_create(PLUGIN_ID_NUM, "PostFX", "Bloom effect");
	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-Bloom effect"DEFIW);

	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Enabled:"DEFPW"#+ #cfff#%s", &cfg.Enabled, YesNo_txt, ARRAY_ENTRIES(YesNo_txt), "");

	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Gamma:"DEFPW2"%.2f", &cfg.gamma, 0.01, 0.2, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-BlurScale:"DEFPW2"%.2f", &cfg.BlurScale, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-BlurStrength:"DEFPW2"%.2f", &cfg.BlurStrength, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-BlurAmount:"DEFPW2"%.2f", &cfg.BlurAmount, 0.05, 1.0, "");

	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-SSAO"DEFIW);
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Mode:"DEFPW"#+ #cfff#%s", &cfg.SSAO_mode, ssao_modes, ARRAY_ENTRIES(ssao_modes), "");
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Type:"DEFPW"#+ #cfff#%s", &cfg.SSAO_type, ssao_type, ARRAY_ENTRIES(ssao_type) - 1, "");

	BAM::menu_add_info(PLUGIN_ID_NUM, "#c777##-#-SSAO - manual"DEFIW);
	BAM::menu_add_switch(PLUGIN_ID_NUM, "#-Res:"DEFPW"#+ #cfff#%s", &cfg.SSAO_res, ssao_res, ARRAY_ENTRIES(ssao_res), "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Amount:"DEFPW2"%.2f", &cfg.SSAO_scale, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Range:"DEFPW2"%.2f", &cfg.SSAO_range, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Max:"DEFPW2"%.2f", &cfg.SSAO_MAX, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-BScale:"DEFPW2"%.2f", &cfg.SSAO_BlurScale, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-BStrength:"DEFPW2"%.2f", &cfg.SSAO_BlurStrength, 0.05, 1.0, "");

	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Z_Near:"DEFPW2"%.2f", &Z_Near, 0.05, 1.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-Z_Far:"DEFPW2"%.2f", &Z_Far, 10.0, 100.0, "");
	BAM::menu_add_param(PLUGIN_ID_NUM, "#-AddlMult:"DEFPW2"%.2f", &SSAO_AddlMult, 0.1, 1.0, "");
	
	BAM::LoadCfg("PostFX", &cfg, sizeof(cfg));
}

void OnUnload()
{	
}

void OnPluginStart()
{
//	BAM::MessageBox("debug");
	GLenum nGlewError = glewInit();
	PostFX.Init();
}

void OnPluginStop()
{
	PostFX.Release();
}

int hudCounter = 0;
void OnHudDisplay()
{
	hudCounter = 3;
}

void OnSwapBuffers(HDC hDC)
{
	// save cfg file if settings are changed
	static Ccfg oldcfg = { -1 };
	if (memcmp(&oldcfg, &cfg, sizeof(cfg)) != 0)	{
		BAM::SaveCfg("PostFX", &cfg, sizeof(cfg));
		memcpy(&oldcfg, &cfg, sizeof(cfg));
	}

	// if hud is displayed when do postprocessing
	if (hudCounter > 0 && hudCounter < 4) {
		--hudCounter;
	}
	else {
		hudCounter = 0;
	}

	if (cfg.Enabled || hudCounter) {
		if (true) {
			ssao_preset *p = cfg.SSAO_mode == 0 ? ssao_presets_desktop : ssao_presets_arcade;
			if (cfg.SSAO_type < ARRAY_ENTRIES(ssao_type) - 1) {
				cfg.SSAO_res = p[cfg.SSAO_type].res;
				cfg.SSAO_scale = p[cfg.SSAO_type].scale/2;
				cfg.SSAO_range = p[cfg.SSAO_type].range;
				cfg.SSAO_BlurStrength = p[cfg.SSAO_type].blurStrength;
				cfg.SSAO_BlurScale = p[cfg.SSAO_type].blurScale;
				cfg.SSAO_MAX = p[cfg.SSAO_type].max+0.3;
				cfg.SSAO_rings = p[cfg.SSAO_type].rings;
			}
		}

		PostFX.Postprocess();
	}
}


void BAM_hud(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	BAM::VhudDebug(fmt, ap);
	va_end(ap);
}
