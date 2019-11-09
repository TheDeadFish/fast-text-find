#include "stdshit.h"
#include "win32hlp.h"
#include "resource.h"
#include "findcore.h"
const char progName[] = "findText";

static HWND s_hwnd;
static char* s_path;
static char s_state;

void WINAPI mainDlgInit(HWND hwnd)
{
	s_hwnd = hwnd;
}

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
	size_t ec = FindList::find(s_path);
	PostMessage(s_hwnd, WM_APP, 0, ec);
	return 0;
}

DWORD WINAPI findThread(LPVOID str)
{
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
		if(ff.find(file.data)) {
			listBox_addStr(s_hwnd, IDC_RESULTS, 
				file.name, (LPARAM)&file);	
		}
	}

	PostMessage(s_hwnd, WM_APP, 0, 0);
	return 0;
}




void WINAPI onLoad(HWND hwnd)
{
	// select the folder
	if(s_state) { s_state = 2; return; }
	char* folder = selectFolder(hwnd, 0, 0);
	if(!folder) return; 
	free_repl(s_path, folder);
	
	// load the files
	setDlgItemText(hwnd, IDC_PATH, "");
	listBox_reset(hwnd, IDC_RESULTS);
	captureControl(IDC_LOAD);
	CreateThread(0, 0, loadThread, 0, 0, 0);
}

void WINAPI onFind(HWND hwnd)
{
	if(s_state) { s_state = 2; return; }
	char* str = getDlgItemText2(hwnd, IDC_QUERY);
	if(!str) return;

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



BOOL CALLBACK mainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DLGMSG_SWITCH(
		ON_MESSAGE(WM_INITDIALOG, mainDlgInit(hwnd))
		ON_MESSAGE(WM_CLOSE, EndDialog(hwnd, 0))
		
		ON_MESSAGE(WM_SETCURSOR, if(s_state) {
			SetCursor(LoadCursor(0, IDC_WAIT )); 
			msgResult = 1; })
			
		ON_MESSAGE(WM_APP, onComplete(hwnd, lParam))
		
	  CASE_COMMAND(
			ON_COMMAND(IDC_FIND, onFind(hwnd))
			ON_COMMAND(IDC_LOAD, onLoad(hwnd))
		,)
	,)
}

int main()
{
	DialogBoxW(NULL, MAKEINTRESOURCEW(IDD_DIALOG1), NULL, mainDlgProc);
}