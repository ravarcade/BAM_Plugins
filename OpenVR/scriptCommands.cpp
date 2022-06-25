#include "stdafx.h"
#include "scriptCommands.h"
#include "scriptCommands_i.c"

namespace {
	const float deg2rad = 3.1415926535897932384626433832795f / 180.0f;
	float* rot(float rot)
	{
		float c = cosf(rot * deg2rad);
		float s = sinf(rot * deg2rad);
		static float R[16] = {
			c, 0, -s, 0,
			0, 1, 0, 0,
			s, 0, c, 0,
			0, 0, 0, 1
		};
		R[0] = c;
		R[2] = -s;
		R[8] = s;
		R[10] = c;
		return R;
	}
}
OpenVrCOM::OpenVrCOM()
{
	// load type lib for this class from DLL
	wchar_t moduleName[2048];
	GetModuleFileNameW(dll_hModule, moduleName, sizeof(moduleName));
	auto l = lstrlenW(moduleName);
	moduleName[l] = '\\';
	moduleName[l + 1] = '1';
	moduleName[l + 2] = 0;
	ITypeLib* pTypeLib = NULL;
	LoadTypeLib(moduleName, &pTypeLib);
	HRESULT hr = pTypeLib->GetTypeInfoOfGuid(IID_IOpenVrCOM, &m_pTypeInfo);
}

OpenVrCOM::~OpenVrCOM()
{
	m_pTypeInfo->Release();
}

HRESULT STDMETHODCALLTYPE OpenVrCOM::Msg(/* [in] */ BSTR txt)
{
	MessageBoxW(0, txt, L"OpenVrCOM", 0);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE OpenVrCOM::RotateRoom(/* [in] */ float angle)
{
	this->angle = angle;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE OpenVrCOM::CenterRoom(/* [in] */ float x, /* [in] */ float y, /* [in] */ float z)
{
	this->x = x;
	this->y = y;
	this->z = z;

	return S_OK;
}

float* OpenVrCOM::GetRotationRoom()
{
	return rot(angle);
}

float* OpenVrCOM::GetCenterRoom(float s)
{
	static float T[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
	};
	T[12] = -x * s;
	T[13] = -y * s;
	T[14] = -z * s;
	return T;
}