#include "stdshit.h"
#include "win32hlp.h"
#include "resource.h"
#include "findcore.h"
#include "resize.h"

const char progName[] = "findText";

extern "C" void WINAPI OpenAs_RunDLLW(HWND hwnd, 
	HINSTANCE hAppInstance, LPCWSTR lpwszCmdLine, int nCmdShow);
wxstr WINAPI widenF(char* str) { return widen(xstr(str)); }

void execute_file(HWND hwnd, LPCWSTR str)
{
	// explore the folder
	if(GetKeyState(VK_SHIFT) < 0) {
		int ec = int(ShellExecuteW(hwnd, L"explore", 
			getPath0(str), NULL, NULL, SW_SHOW));
		if(ec > 32) return;
		contError(hwnd, "failed to open folder");
		return;
	}

	// perform shellexecute
	if(GetKeyState(VK_CONTROL) >= 0) { 
		int ec = int(ShellExecuteW(hwnd,
			L"edit", str, NULL, NULL, SW_SHOW));
		if(ec > 32) return;
		ec = int(ShellExecuteW(hwnd,
			NULL, str, NULL, NULL, SW_SHOW));
		if(ec > 32) return; 
	}
	
	// open-with fallback
	OpenAs_RunDLLW(hwnd, (HINSTANCE)0,
		str, SW_SHOWNORMAL);
}

static HWND s_hwnd;
static char* s_path;
static char s_state;
static char s_loaded;
static WndResize s_resize;

void WINAPI captureControl(int ctrl)
{
	EnableDlgItem(s_hwnd, IDC_LOAD, ctrl != IDC_FIND);
	EnableDlgItem(s_hwnd, IDC_FIND, ctrl != IDC_LOAD);
	EnableDlgItem(s_hwnd, IDC_QUERY, !ctrl);

	s_state = !!ctrl;
}

bool status_check(void) {
	static unsigned lastTime;
	unsigned curTime = GetTickCount();
	if((curTime-lastTime) < 20) return false;
	lastTime = curTime; return true;
}

void size_status(cch* path)
{
	// format the 
	size_t size = FindList::total_size;
	cch* fstr = "KB"; float fscale = (1.0/1024);
	if(size >= 100*1024) { fstr = "MB";
		fscale = (1.0/(1024*1024));
	if(size >= 100*1024*1024) { fstr = "GB";
		fscale = (1.0/(1024*1024*1024)); }}
	
	char buff[64];	
	sprintf(buff, "%.2f %s", size * fscale, fstr);
	SetDlgItemTextA(s_hwnd, IDC_TOTSIZE, buff);
	setDlgItemText(s_hwnd, IDC_PATH, path);
}

namespace FindList {
size_t progress(cch* name, int code) {
	if(code == 0) {
		if(status_check()) { size_status(name); }
	} else {
		listBox_addStr(s_hwnd, IDC_RESULTS, 
			Xstrfmt("%d: %s", code, name));
	}
	return s_state != 1 ? 1 : 0;
}
}

DWORD WINAPI loadThread(LPVOID param)
{
	s_loaded = false;
	size_t ec = FindList::find(s_path);
	s_loaded = !ec && FindList::list.len;
	PostMessage(s_hwnd, WM_APP, 0, ec);
	return 0;
}

DWORD WINAPI findThread(LPVOID str)
{
	int flags = 0;
	if(IsDlgButtonChecked(s_hwnd, IDC_OPTWORD)) flags |= 1;
	if(IsDlgButtonChecked(s_hwnd, IDC_OPTDEF)) flags |= 2;

	FastFind::IcmpFind ff(xstr((char*)str));
	for(int i = 0; i < FindList::list.len; i++)
	{
		// update progress
		if(status_check()) { char buff[64];
		sprintf(buff, "%d / %d", i, FindList::list.len);
		SetDlgItemTextA(s_hwnd, IDC_PATH, buff); }
		if(s_state != 1) break;
		
		// search for string
		auto& file = FindList::list[i];
		if(!str || ff.find(file.data, flags)) {
			listBox_addStr(s_hwnd, IDC_RESULTS, file.name, i);	
		}
	}

	PostMessage(s_hwnd, WM_APP, 0, 0);
	return 0;
}


void WINAPI onLoad_(HWND hwnd, char* folder)
{
	if(!folder) return; 
	free_repl(s_path, 
		pathCatF(folder, ""));
	
	// load the files
	setDlgItemText(hwnd, IDC_PATH, "");
	listBox_reset(hwnd, IDC_RESULTS);
	captureControl(IDC_LOAD);
	CreateThread(0, 0, loadThread, 0, 0, 0);
}

void WINAPI onLoad(HWND hwnd)
{
	// select the folder
	if(s_state) { s_state = 2; return; }
	char* folder = selectFolder(hwnd, 0, 0);
	onLoad_(hwnd, folder);
}

void WINAPI onFind(HWND hwnd)
{
	if(s_state) { s_state = 2; return; }
	char* str = getDlgItemText2(hwnd, IDC_QUERY);
	listBox_reset(hwnd, IDC_RESULTS);	
	captureControl(IDC_FIND);
	CreateThread(0, 0, findThread, 
		(LPVOID)str, 0, 0);
}

void WINAPI onComplete(HWND hwnd, LPARAM lParam)
{
	size_status(s_path);
	if(lParam && (s_state != 2))
		contError(hwnd, "load failed");
	captureControl(0);
}

void WINAPI onEnter(HWND hwnd)
{
	HWND hFocus = GetFocus();
	if(GetDlgItem(hwnd, IDC_QUERY) == hFocus) {
		return onFind(hwnd); }
	if(GetDlgItem(hwnd, IDC_RESULTS) == hFocus) {
	
		// get data slot
		int iSel = listBox_getData(hwnd, IDC_RESULTS, 
			listBox_getCurSel(hwnd, IDC_RESULTS));
		if(iSel < 0) return;
		
		// open the file
		auto& file = FindList::list[iSel];
		execute_file(hwnd, widenF(
			pathCat(s_path, file.name)));
	}


}

void WINAPI mainDlgInit(HWND hwnd)
{
	s_hwnd = hwnd;
	
	s_resize.init(hwnd);
	s_resize.add(hwnd, IDC_PATH, HOR_BOTH);
	s_resize.add(hwnd, IDC_LOAD, HOR_RIGH);
	s_resize.add(hwnd, IDC_QUERY, HOR_BOTH);
	s_resize.add(hwnd, IDC_FIND, HOR_RIGH);
	s_resize.add(hwnd, IDC_OPTWORD, HOR_RIGH);
	s_resize.add(hwnd, IDC_OPTWCH, HOR_RIGH);
	s_resize.add(hwnd, IDC_OPTDEF, HOR_RIGH);
	s_resize.add(hwnd, IDC_RESULTS, HVR_BOTH);
	
	size_status(0);
}

void WINAPI onClose(HWND hwnd)
{
	if(s_loaded) {
		int opt = MessageBoxA(hwnd, 
			"Are you sure you want to close?",
			"Fast Text Find", MB_YESNO);
		if(opt != IDYES) {
			ShowWindow(hwnd, SW_MINIMIZE);
			return;
		}
	}

	EndDialog(hwnd, 0);
}

void onDropFiles(HWND hwnd, LPARAM wParam)
{
	if(s_state) return;
	xArray<xstr> files = hDropGet((HANDLE)wParam);
	onLoad_(hwnd, files[0].release());
}

BOOL CALLBACK mainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DLGMSG_SWITCH(
		ON_MESSAGE(WM_INITDIALOG, mainDlgInit(hwnd))
		ON_MESSAGE(WM_CLOSE, onClose(hwnd))
		
		ON_MESSAGE(WM_SETCURSOR, if(s_state) {
			SetCursor(LoadCursor(0, IDC_WAIT )); 
			msgResult = 1; } else { return FALSE; })
			
		ON_MESSAGE(WM_DROPFILES, onDropFiles(hwnd, wParam))
		
		ON_MESSAGE(WM_APP, onComplete(hwnd, lParam))
		
		ON_MESSAGE(WM_SIZE, s_resize.resize(hwnd, wParam, lParam))		
		
		
	  CASE_COMMAND(
			ON_COMMAND(IDOK, onEnter(hwnd))
			ON_COMMAND(IDC_FIND, onFind(hwnd))
			ON_COMMAND(IDC_LOAD, onLoad(hwnd))
			ON_CONTROL(LBN_DBLCLK, IDC_RESULTS, onEnter(hwnd))
			
		,)
	,)
}

int main()
{
	DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_DIALOG1), NULL, mainDlgProc);
}
