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

#define BAMEXPORT __declspec(dllexport)
#define BAMIMPORT __declspec(dllimport)

// Handy macro used to declare function in BAM namespace and create need variable in BAM_Internal namespace 
#define BAM_Function(x) decltype(BAM_Internal::BAM_##x) *x = NULL; }; namespace BAM_Internal { CGetProcAddressList list##x("BAM_" # x, &::BAM::x); }; namespace BAM {

#define ARRAY_ENTRIES(array)	(sizeof(array)/sizeof(array[0]))

#define DEFIW "#m350#"
#define DEFPW "#r160#"
#define DEFPW2 DEFPW"#+ #cfff#"
#define DEFIW "#m350#"
#define DEFPARAM "#r165##+ #cfff#"
#define DEFPARAM3 "#r225##+ #cfff#"

/// <summary>
/// Inside BAM_Internal we store needed tools to import BAM functions.
/// </summary>
namespace BAM_Internal {

	// Functions in BAM.dll
	BAMIMPORT void BAM_VMessageBox(const char *fmt, va_list ap);
	BAMIMPORT void BAM_VhudDebug(const char *fmt, va_list ap);
	BAMIMPORT void BAM_VhudDebugLong(const char *fmt, va_list ap);

	BAMIMPORT void BAM_MessageBox(const char *fmt, ...);
	BAMIMPORT void BAM_hudDebug(const char *fmt, ...);
	BAMIMPORT void BAM_hudDebugLong(const char *fmt, ...);

	BAMIMPORT void BAM_menu_create(int pluginID, const char *pname, const char *phelp);
	BAMIMPORT void BAM_menu_add_cam(int pluginID);
	BAMIMPORT void BAM_menu_add_info(int pluginID, const char *text);
	BAMIMPORT void BAM_menu_add_TL(int pluginID);
	BAMIMPORT void BAM_menu_add_Reality(int pluginID);

	BAMIMPORT void BAM_menu_add_param(int PluginID, const char *txt, double *value, double smallStep, double bigStep, const char *help);
	BAMIMPORT void BAM_menu_add_switch(int PluginID, const char *txt, int *value, const char *options[], int count, const char *help);
	BAMIMPORT void BAM_menu_add_key(int PluginID, const char *txt, DWORD *value);

	BAMIMPORT void BAM_SwapBuffers(HDC hDC);
	BAMIMPORT void BAM_SetScale(double s);
	BAMIMPORT void BAM_menu_additional_data(double **ppRealityTVSize, int **ppRealityUnits, double **ppRealitySmooth_time, double **ppRealitySmooth_dist, double **ppCfgY, double **ppCfgZ, double **ppCfgAngle, double **ppTIAngle, double **ppRealitySmoothTime, double **ppRealitySmoothDist);

	BAMIMPORT short BAM_GetAsyncButtonState(int btn);

	BAMIMPORT int BAM_create_submenu(int ParentSubMenuId);
	BAMIMPORT void BAM_menu_add_back_button(int SubMenuId);
	BAMIMPORT void BAM_menu_add_submenu(int ParentSubMenuId, const char *txt, int SubMenuID, const char *help);
	BAMIMPORT void BAM_menu_add_button(int PluginID, const char *txt, void (*func)(int));


	/// <summary>
	/// Helper class used for late binding of BAM functions
	/// </summary>
	class CGetProcAddressList
	{
		static std::vector<const char *> functionNames;
		static std::vector<void *> functionPointers;

	public:
		CGetProcAddressList(const char *functionName, void *functionPointer)
		{
			functionNames.push_back(functionName);
			functionPointers.push_back(functionPointer);
		}

		static void Hook(HMODULE module)
		{
			for (unsigned int i = 0; i < functionNames.size(); ++i)
				*(PROC *)functionPointers[i] = GetProcAddress(module, functionNames[i]);
		}
	};

	std::vector<const char *> CGetProcAddressList::functionNames;
	std::vector<void *> CGetProcAddressList::functionPointers;

	/// <summary>
	/// Initialize. Locate functions in BAM.dll
	/// </summary>
	/// <param name="pluginID">The plugin identifier.</param>
	/// <param name="bam_module">The bam_module handle.</param>
	void Init(int pluginID, HMODULE bam_module)
	{
		CGetProcAddressList::Hook(bam_module);
	}

	/// <summary>
	/// Builds the filename for cfg file.
	/// </summary>
	/// <param name="fname">The fname.</param>
	/// <param name="ext">The ext.</param>
	/// <returns></returns>
	const char *BuildFilename(const char *fname = NULL, const char *ext = NULL) {
		static char dir[_MAX_PATH + 1] = "";
		static char ret[_MAX_PATH + 1];
		if (dir[0] == 0) {
			GetCurrentDirectoryA(sizeof(dir), dir);
			strcat_s(dir, _MAX_PATH, "\\");
		}
		strcpy_s(ret, dir);
		if (fname != NULL) {
			strcat_s(ret, _MAX_PATH, fname);
			strcat_s(ret, _MAX_PATH, ext ? ext : ".cfg");
		}
		return ret;
	}


};

namespace BAM {

	void LoadCfg(const char *fname, void *cfg, size_t len) {
		// load settings
		const char *strPathName = ::BAM_Internal::BuildFilename(fname, ".cfg");

		FILE *fp;
		fopen_s(&fp, strPathName, "rb");
		if (fp != NULL) {
			fread(cfg, len, 1, fp);
			fclose(fp);
		}
	}

	void SaveCfg(const char *fname, void *cfg, size_t len) {
		const char *strPathName = ::BAM_Internal::BuildFilename(fname, ".cfg");

		FILE *fp;
		fopen_s(&fp, strPathName, "wb");
		if (fp != NULL) {
			fwrite(cfg, len, 1, fp);
			fclose(fp);
		}
	}

}