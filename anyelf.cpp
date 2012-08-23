// anyelf.cpp : Defines the entry point for the DLL application.
//

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdlib.h>
#include <shellapi.h>
#include <malloc.h>
#include <richedit.h>
#include <commdlg.h>
#include <math.h> 

#include "anyelf.h"
#include "cunicode.h"
#include <elfio.hpp>

#define supportedextension1 L".c"
#define supportedextension2 L".cpp"
#define supportedextension3 L".h"
#define supportedextension4 L".pas"
/* Note: in C, double quotes inside a string need to be escaped with a backslash!  */
#define parsefunction "[0]=127 & [1]=\"E\" & [2]=\"L\" & [3]=\"F\""

HINSTANCE hinst;
HMODULE FLibHandle=0;
char inifilename[MAX_PATH]="anyelf.ini";  // Unused in this plugin, may be used to save data

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hinst=(HINSTANCE)hModule;
			break;
		case DLL_PROCESS_DETACH:
			if (FLibHandle)
				FreeLibrary(FLibHandle);
			FLibHandle=NULL;
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}

char* strlcpy(char* p,char*p2,int maxlen)
{
	if ((int)strlen(p2)>=maxlen) {
		strncpy(p,p2,maxlen);
		p[maxlen]=0;
	} else
		strcpy(p,p2);
	return p;
}

void searchAndReplace(std::string& value, std::string const& search,std::string const& replace) 
{ 
    std::string::size_type  next; 
 
    for(next = value.find(search);        // Try and find the first match 
        next != std::string::npos;        // next is npos if nothing was found 
        next = value.find(search,next)    // search for the next match starting after 
                                          // the last match that was found. 
       ) 
    { 
        // Inside the loop. So we found a match. 
        value.replace(next,search.length(),replace);   // Do the replacement. 
        next += replace.length();                      // Move to just after the replace 
                                                       // This is the point were we start 
                                                       // the next search from.  
    } 
}

int __stdcall ListNotificationReceived(HWND ListWin,int Message,WPARAM wParam,LPARAM lParam)
{
	switch (Message) {
	case WM_COMMAND:
		if (HIWORD(wParam)==EN_UPDATE) {
			int firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
			int linecount=SendMessage(ListWin,EM_GETLINECOUNT,0,0);
			if (linecount>0) {
				int percent=MulDiv(firstvisible,100,linecount);
				PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(percent,itm_percent),(LPARAM)ListWin);
			}
			return 0;
		}
		break;
	case WM_NOTIFY:
		break;
	case WM_MEASUREITEM:
		break;
	case WM_DRAWITEM:
		break;
	}
	return 0;
}


HWND __stdcall ListLoad(HWND ParentWin,char* FileToLoad,int ShowFlags)
{
//	WCHAR FileToLoadW[wdirtypemax];
//	return ListLoadW(ParentWin,awfilenamecopy(FileToLoadW,FileToLoad),ShowFlags);
	HWND hwnd;
	RECT r;

	// Extension is supported -> load file
    ELFIO::elfio reader;

    if ( !reader.load( FileToLoad ) ) {
        return NULL;
    }

	if (!FLibHandle) {
		int OldError = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		FLibHandle = LoadLibrary("Riched20.dll");
		if ((int)FLibHandle < HINSTANCE_ERROR) 
			FLibHandle = LoadLibrary("RICHED32.DLL");
		if ((int)FLibHandle < HINSTANCE_ERROR) 
			FLibHandle = NULL;
		SetErrorMode(OldError);
	}

    std::ostringstream oss;
    int error = elfdump( reader, oss );
    if ( error != 0 ) {
        return NULL;
    }
    std::string text = oss.str();
    searchAndReplace( text, "\n", "\r\n" );
    
    GetClientRect(ParentWin,&r);
	// Create window invisbile, only show when data fully loaded!
	hwnd=CreateWindow("EDIT","",WS_CHILD | ES_MULTILINE | ES_WANTRETURN | ES_READONLY |
		                        WS_HSCROLL | WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL,
		r.left,r.top,r.right-r.left,
		r.bottom-r.top,ParentWin,NULL,hinst,NULL);

	if ( hwnd ) {
		SendMessage(hwnd, EM_SETMARGINS, EC_LEFTMARGIN, 8);
		SendMessage(hwnd, EM_SETEVENTMASK, 0, ENM_UPDATE); //ENM_SCROLL doesn't work for thumb movements!

		PostMessage( ParentWin, WM_COMMAND, MAKELONG( lcp_ascii, itm_fontstyle ), (LPARAM)hwnd );

        HFONT font = (HFONT)GetStockObject( ANSI_FIXED_FONT );
        SendMessage( hwnd, WM_SETFONT, (WPARAM)font, MAKELPARAM( true, 0 ) );

        SendMessage( hwnd, WM_SETTEXT, 0, (LPARAM)text.c_str() ); 
        //SetWindowText( hwnd, oss.str().c_str() );

        PostMessage( ParentWin, WM_COMMAND, MAKELONG( 0, itm_percent ), (LPARAM)hwnd );

		ShowWindow( hwnd, SW_SHOW );
	}

	return hwnd;
}

int __stdcall ListLoadNextW(HWND ParentWin,HWND ListWin,WCHAR* FileToLoad,int ShowFlags)
{
	RECT r;
	DWORD w2;
	char *pdata;
	BOOL success=false;

	return ANYELF_OK;
}

int __stdcall ListLoadNext(HWND ParentWin,HWND ListWin,char* FileToLoad,int ShowFlags)
{
	WCHAR FileToLoadW[wdirtypemax];
	return ListLoadNextW(ParentWin,ListWin,awfilenamecopy(FileToLoadW,FileToLoad),ShowFlags);
}

int __stdcall ListSendCommand(HWND ListWin,int Command,int Parameter)
{
	switch (Command) {
	case lc_copy:
		SendMessage(ListWin,WM_COPY,0,0);
		return ANYELF_OK;
	case lc_newparams:
		PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(0,itm_next),(LPARAM)ListWin);
		return ANYELF_ERROR;
	case lc_selectall:
		SendMessage(ListWin,EM_SETSEL,0,-1);
		return ANYELF_OK;
	case lc_setpercent:
		int firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
		int linecount=SendMessage(ListWin,EM_GETLINECOUNT,0,0);
		if (linecount>0) {
			int pos=MulDiv(Parameter,linecount,100);
			SendMessage(ListWin,EM_LINESCROLL,0,pos-firstvisible);
			firstvisible=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
			// Place caret on first visible line!
			int firstchar=SendMessage(ListWin,EM_LINEINDEX,firstvisible,0);
			SendMessage(ListWin,EM_SETSEL,firstchar,firstchar);
			pos=MulDiv(firstvisible,100,linecount);
			// Update percentage display
			PostMessage(GetParent(ListWin),WM_COMMAND,MAKELONG(pos,itm_percent),(LPARAM)ListWin);
			return ANYELF_OK;
		}
		break;
	}
	return ANYELF_ERROR;
}

int _stdcall ListSearchText(HWND ListWin,char* SearchString,int SearchParameter)
{
	FINDTEXT find;
	int StartPos,Flags;

	if (SearchParameter & lcs_findfirst) {
		//Find first: Start at top visible line
		StartPos=SendMessage(ListWin,EM_LINEINDEX,SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0),0);
		SendMessage(ListWin,EM_SETSEL,StartPos,StartPos);
	} else {
		//Find next: Start at current selection+1
		SendMessage(ListWin,EM_GETSEL,(WPARAM)&StartPos,0);
		StartPos+=1;
	}

	find.chrg.cpMin=StartPos;
	find.chrg.cpMax=SendMessage(ListWin,WM_GETTEXTLENGTH,0,0);
	Flags=0;
	if (SearchParameter & lcs_wholewords)
		Flags |= FR_WHOLEWORD;
	if (SearchParameter & lcs_matchcase)
		Flags |= FR_MATCHCASE;
	if (!(SearchParameter & lcs_backwards))
		Flags |= FR_DOWN;
	find.lpstrText=SearchString;
	int index=SendMessage(ListWin, EM_FINDTEXT, Flags, (LPARAM)&find);
	if (index!=-1) {
	  int indexend=index+strlen(SearchString);
	  SendMessage(ListWin,EM_SETSEL,index,indexend);
	  int line=SendMessage(ListWin,EM_LINEFROMCHAR,index,0)-3;
	  if (line<0)
		  line=0;
      line-=SendMessage(ListWin,EM_GETFIRSTVISIBLELINE,0,0);
      SendMessage(ListWin,EM_LINESCROLL,0,line);
	  return ANYELF_OK;
	} else {
		SendMessage(ListWin,EM_SETSEL,-1,-1);  // Restart search at the beginning
	}
	return ANYELF_ERROR;
}

void __stdcall ListCloseWindow(HWND ListWin)
{
	DestroyWindow(ListWin);
	return;
}

int __stdcall ListPrint(HWND ListWin,char* FileToPrint,char* DefPrinter,int PrintFlags,RECT* Margins)
{
	PRINTDLG PrintDlgRec;
	memset(&PrintDlgRec,0,sizeof(PRINTDLG));
	PrintDlgRec.lStructSize=sizeof(PRINTDLG);

    PrintDlgRec.Flags= PD_ALLPAGES | PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
	PrintDlgRec.nFromPage   = 0xFFFF; 
	PrintDlgRec.nToPage     = 0xFFFF; 
    PrintDlgRec.nMinPage	= 1;
    PrintDlgRec.nMaxPage	= 0xFFFF;
    PrintDlgRec.nCopies		= 1;
    PrintDlgRec.hwndOwner	= ListWin;// MUST be Zero, otherwise crash!
	if (PrintDlg(&PrintDlgRec)) 
	{
		HDC hdc=PrintDlgRec.hDC;
		DOCINFO DocInfo;
		POINT offset,physsize,start,avail,printable;
		int LogX,LogY;
		RECT rcsaved;

		// Warning: PD_ALLPAGES is zero!
		BOOL PrintSel=(PrintDlgRec.Flags & PD_SELECTION);
		BOOL PrintPages=(PrintDlgRec.Flags & PD_PAGENUMS);
		int PageFrom=1;
		int PageTo=0x7FFF;
		if (PrintPages) {
			PageFrom=PrintDlgRec.nFromPage;
			PageTo=PrintDlgRec.nToPage;
			if (PageTo<=0) PageTo=0x7FFF;
		}


		memset(&DocInfo,0,sizeof(DOCINFO));
		DocInfo.cbSize=sizeof(DOCINFO);
		DocInfo.lpszDocName=FileToPrint;
		if (StartDoc(hdc,&DocInfo)) {
			SetMapMode(hdc,MM_LOMETRIC);
			offset.x=GetDeviceCaps(hdc,PHYSICALOFFSETX);
			offset.y=GetDeviceCaps(hdc,PHYSICALOFFSETY);
			DPtoLP(hdc,&offset,1);
			physsize.x=GetDeviceCaps(hdc,PHYSICALWIDTH);
			physsize.y=GetDeviceCaps(hdc,PHYSICALHEIGHT);
			DPtoLP(hdc,&physsize,1);

			start.x=Margins->left-offset.x;
			start.y=-Margins->top-offset.y;
			if (start.x<0)
				start.x=0;
			if (start.y>0)
				start.y=0;
			avail.x=GetDeviceCaps(hdc,HORZRES);
			avail.y=GetDeviceCaps(hdc,VERTRES);
			DPtoLP(hdc,&avail,1);

			printable.x=min(physsize.x-(Margins->right+Margins->left),avail.x-start.x);
			printable.y=max(physsize.y+(Margins->top+Margins->bottom),avail.y-start.y);
  			
			LogX=GetDeviceCaps(hdc, LOGPIXELSX);
			LogY=GetDeviceCaps(hdc, LOGPIXELSY);

			SendMessage(ListWin, EM_FORMATRANGE, 0, 0);

			FORMATRANGE Range;
			memset(&Range,0,sizeof(FORMATRANGE));
			Range.hdc=hdc;
			Range.hdcTarget=hdc;
			LPtoDP(hdc,&start,1);
			LPtoDP(hdc,&printable,1);
			Range.rc.left = start.x * 1440 / LogX;
			Range.rc.top = start.y * 1440 / LogY;
			Range.rc.right = (start.x+printable.x) * 1440 / LogX;
			Range.rc.bottom = (start.y+printable.y) * 1440 / LogY;
			SetMapMode(hdc,MM_TEXT);

			BOOL PrintAborted=false;
			Range.rcPage = Range.rc;
			rcsaved = Range.rc;
			int CurrentPage = 1;
			int LastChar = 0;
			int LastChar2= 0;
			int MaxLen = SendMessage(ListWin,WM_GETTEXTLENGTH,0,0);
			Range.chrg.cpMax = -1;
			if (PrintPages) {
				do {
			        Range.chrg.cpMin = LastChar;
					if (CurrentPage<PageFrom) {
						LastChar = SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
					} else {
						//waitform.ProgressLabel.Caption:=spage+inttostr(CurrentPage);
						//application.processmessages;
						LastChar = SendMessage(ListWin, EM_FORMATRANGE, 1, (LPARAM)&Range);
					}
					// Warning: At end of document, LastChar may be<MaxLen!!!
					if (LastChar!=-1 && LastChar < MaxLen) {
						Range.rc=rcsaved;                // Check whether another page comes
						Range.rcPage = Range.rc;
						Range.chrg.cpMin = LastChar;
						LastChar2= SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
						if (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1 &&
						  CurrentPage>=PageFrom && CurrentPage<PageTo) {
							EndPage(hdc);
						}
					}

					CurrentPage++;
					Range.rc=rcsaved;
					Range.rcPage = Range.rc;
				} while (LastChar < MaxLen && LastChar != -1 && LastChar<LastChar2 &&
					     (PrintPages && CurrentPage<=PageTo) && !PrintAborted);
			} else {
				if (PrintSel) {
					SendMessage(ListWin,EM_GETSEL,(WPARAM)&LastChar,(LPARAM)&MaxLen);
					Range.chrg.cpMax = MaxLen;
				}
				do {
					Range.chrg.cpMin = LastChar;
					//waitform.ProgressLabel.Caption:=spage+inttostr(CurrentPage);
					//waitform.ProgressLabel.update;
					//application.processmessages;
					LastChar = SendMessage(ListWin, EM_FORMATRANGE, 1, (LPARAM)&Range);

					// Warning: At end of document, LastChar may be<MaxLen!!!
					if (LastChar!=-1 && LastChar < MaxLen) {
						Range.rc=rcsaved;                // Check whether another page comes
						Range.rcPage = Range.rc;
						Range.chrg.cpMin = LastChar;
						LastChar2= SendMessage(ListWin, EM_FORMATRANGE, 0, (LPARAM)&Range);
						if (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1) {
							EndPage(hdc);
						}
					}
					CurrentPage++;
					Range.rc=rcsaved;
					Range.rcPage = Range.rc;
				} while (LastChar<LastChar2 && LastChar < MaxLen && LastChar != -1 && !PrintAborted);
			}
			if (PrintAborted)
				AbortDoc(hdc);
			else
				EndDoc(hdc);
		} //StartDoc
  
		SendMessage(ListWin, EM_FORMATRANGE, 0, 0);
		DeleteDC(PrintDlgRec.hDC);
	}
	if (PrintDlgRec.hDevNames)
		GlobalFree(PrintDlgRec.hDevNames);
	return 0;
}

void __stdcall ListGetDetectString(char* DetectString,int maxlen)
{
	strlcpy(DetectString,parsefunction,maxlen);
}

void __stdcall ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	strlcpy(inifilename,dps->DefaultIniName,MAX_PATH-1);
}

HBITMAP __stdcall ListGetPreviewBitmapW(WCHAR* FileToLoad,int width,int height,
    char* contentbuf,int contentbuflen)
{
	RECT r;
	WCHAR *p;
	HBITMAP retval=NULL;
	char* newbuf;
	int bigx,bigy,fntsize;
	HDC maindc,dc_small,dc_big;
	HBITMAP bmp_big,bmp_small,oldbmp_big,oldbmp_small;
	HFONT font,oldfont;
	POINT pt;
	char ch;
	OSVERSIONINFO vx;
	BOOL is_nt;

	// check for operating system:
	// Windows 9x does NOT support the HALFTONE stretchblt mode!
	vx.dwOSVersionInfoSize=sizeof(vx);
	GetVersionEx(&vx);
	is_nt=vx.dwPlatformId==VER_PLATFORM_WIN32_NT;

	p=wcsrchr(FileToLoad,'\\');
	if (!p)
		return NULL;
	p=wcsrchr(p,'.');
	if (!p || (_wcsicmp(p,supportedextension1)!=0 && _wcsicmp(p,supportedextension2)!=0
		   && _wcsicmp(p,supportedextension3)!=0 && _wcsicmp(p,supportedextension4)!=0))
		return NULL;
	
	if (!contentbuf || contentbuflen<=0)
		return NULL;

	
	ch=contentbuf[contentbuflen];
	contentbuf[contentbuflen]=0;
	contentbuf[contentbuflen]=ch;  // make sure that contentbuf is NOT modified!

	if (is_nt) {
		bigx=width*2;
		bigy=height*2;
		fntsize=10;
	} else {
		bigx=width*2;
		bigy=height*2;
		fntsize=5;
	}

	maindc=GetDC(GetDesktopWindow());
    dc_small=CreateCompatibleDC(maindc);
    dc_big=CreateCompatibleDC(maindc);
    bmp_big=CreateCompatibleBitmap(maindc,bigx,bigy);
    bmp_small=CreateCompatibleBitmap(maindc,width,height);
    ReleaseDC(GetDesktopWindow(),maindc);
    oldbmp_big=(HBITMAP)SelectObject(dc_big,bmp_big);
    r.left=0;
    r.top=0;
    r.right=bigx;
    r.bottom=bigy;
    font=CreateFont(-MulDiv(fntsize, GetDeviceCaps(dc_big, LOGPIXELSY), 72),0,0,0,FW_NORMAL,
      0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,"Arial");
    oldfont=(HFONT)SelectObject(dc_big,font);
    FillRect(dc_big,&r,(HBRUSH)GetStockObject(WHITE_BRUSH));
    DrawText(dc_big,"Hello World!",12,&r,DT_EXPANDTABS);
    SelectObject(dc_big,oldfont);
    DeleteObject(font);

    if (is_nt) {
		oldbmp_small=(HBITMAP)SelectObject(dc_small,bmp_small);
		SetStretchBltMode(dc_small,HALFTONE);
		SetBrushOrgEx(dc_small,0,0,&pt);
		StretchBlt(dc_small,0,0,width,height,dc_big,0,0,bigx,bigy,SRCCOPY);
		SelectObject(dc_small,oldbmp_small);
	    SelectObject(dc_big,oldbmp_big);
		DeleteObject(bmp_big);
    } else {
	    SelectObject(dc_big,oldbmp_big);
		DeleteObject(bmp_small);
		bmp_small=bmp_big;
    }	
    DeleteDC(dc_big);
    DeleteDC(dc_small);
	free(newbuf);
	return bmp_small;
}

HBITMAP __stdcall ListGetPreviewBitmap(char* FileToLoad,int width,int height,
    char* contentbuf,int contentbuflen)
{
	WCHAR FileToLoadW[wdirtypemax];
	return ListGetPreviewBitmapW(FileToLoadW,width,height,
		contentbuf,contentbuflen);
}

int _stdcall ListSearchDialog(HWND ListWin,int FindNext)
{
/*	if (FindNext)
		MessageBox(ListWin,"Find Next","test",0);
	else
		MessageBox(ListWin,"Find First","test",0);*/
	return ANYELF_ERROR;  // use ListSearchText instead!
}
