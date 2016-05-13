#include "stdafx.h"

#define PLUGIN_ID_NUM 0x4001
#define BAM_USE_OnSwapBuffers
#include "BAM_inc.h"

#include "PostFX.h"

const char *YesNo_txt[] = {	"No", "Yes" };
const char *ssao_modes[] = { "desktop", "arcade" };
const char *ssao_res[] = { "low", "hi" };
const char *ssao_type[] = { "off", "fastest / low", "fastest / hi", "fast / low", "fast / hi", "normal / low", "normal / hi", "slow / low", "slow / hi", "manual" };

CPostFX PostFX;

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

	if (cfg.Enabled || hudCounter)
		PostFX.Postprocess();
}
