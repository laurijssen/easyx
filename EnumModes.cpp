#include <EnumModes.h>
#include "resource.h"

// this one to call. dllInstance is a global
extern "C" __declspec(dllexport) 
void EnumerateDevices()
{
	DialogBox(dllInstance, MAKEINTRESOURCE(IDD_ENUMDIALOG), GetDesktopWindow(), EnumDevicesProc);
}

BOOL CALLBACK EnumDevicesProc(HWND hDlg, UINT imsg, WPARAM wParam, LPARAM lParam)
{
	long index;

	switch( imsg )
	{
		case WM_COMMAND :
			if( LOWORD(wParam) == IDOK )
			{
				index = SendMessage(GetDlgItem(hDlg, IDC_ENUMLIST), LB_GETCURSEL, 0, 0);
				if( index == LB_ERR )
				{
					MessageBox(hDlg, "Please select a device from the list", "Error", MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				else
				{
					GUID FAR *guid = (GUID *)SendMessage(GetDlgItem(hDlg, IDC_ENUMLIST), LB_GETITEMDATA, index, 0);
					EASYDRAW::DeviceGUID = guid; // set the guid of the device in easydraw
				}

				EndDialog(hDlg, TRUE);
			}
			return TRUE;

		case WM_INITDIALOG :
			DirectDrawEnumerate(EnumDDrawDevice, GetDlgItem(hDlg, IDC_ENUMLIST));
			return TRUE;
	}

	return FALSE;
}

BOOL WINAPI EnumDDrawDevice(GUID FAR *lpGUID, LPSTR lpDriverDescription,
				    					LPSTR lpDriverName, LPVOID lpContext)
{
	long iIndex;
	HWND hListBox = (HWND)lpContext;
	LPVOID lpDevice = NULL;

	iIndex = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)lpDriverDescription);

	if( iIndex != LB_ERR )
	{
		lpDevice = new GUID;
		if( !lpDevice ) return FALSE;
		memcpy(lpDevice, lpGUID, sizeof(GUID));

		SendMessage(hListBox, LB_SETITEMDATA, iIndex, (LPARAM)lpDevice);
	}
	else
	{
		return DDENUMRET_CANCEL;
	}

	return DDENUMRET_OK;
}
