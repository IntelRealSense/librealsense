/* If you are using a C++ compiler to compile tinyfiledialogs.c (maybe renamed with an extension ".cpp")
then comment out << extern "C" >> bellow in this header file) */

/*_________
 /         \ tinyfiledialogs.h v3.9.0 [Nov 3, 2022] zlib licence
 |tiny file| Unique header file created [November 9, 2014]
 | dialogs | Copyright (c) 2014 - 2021 Guillaume Vareille http://ysengrin.com
 \____  ___/ http://tinyfiledialogs.sourceforge.net
      \|     git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd
 ____________________________________________
|                                            |
|   email: tinyfiledialogs at ysengrin.com   |
|____________________________________________|
 ________________________________________________________________________________
|  ____________________________________________________________________________  |
| |                                                                            | |
| | on windows:                                                                | |
| |  - for UTF-16, use the wchar_t functions at the bottom of the header file  | |
| |  - _wfopen() requires wchar_t                                              | |
| |                                                                            | |
| |  - in tinyfiledialogs, char is UTF-8 by default (since v3.6)               | |
| |  - but fopen() expects MBCS (not UTF-8)                                    | |
| |  - if you want char to be MBCS: set tinyfd_winUtf8 to 0                    | |
| |                                                                            | |
| |  - alternatively, tinyfiledialogs provides                                 | |
| |                        functions to convert between UTF-8, UTF-16 and MBCS | |
| |____________________________________________________________________________| |
|________________________________________________________________________________|

If you like tinyfiledialogs, please upvote my stackoverflow answer
https://stackoverflow.com/a/47651444

- License -
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TINYFILEDIALOGS_H
#define TINYFILEDIALOGS_H

#ifdef	__cplusplus
/* if tinydialogs.c is compiled as C++ code rather than C code, you may need to comment this out
				and the corresponding closing bracket near the end of this file. */
extern "C" {
#endif

/******************************************************************************************************/
/**************************************** UTF-8 on Windows ********************************************/
/******************************************************************************************************/
#ifdef _WIN32
/* On windows, if you want to use UTF-8 ( instead of the UTF-16/wchar_t functions at the end of this file )
Make sure your code is really prepared for UTF-8 (on windows, functions like fopen() expect MBCS and not UTF-8) */
extern int tinyfd_winUtf8; /* on windows char strings can be 1:UTF-8(default) or 0:MBCS */
/* for MBCS change this to 0, in tinyfiledialogs.c or in your code */

/* Here are some functions to help you convert between UTF-16 UTF-8 MBSC */
char * tinyfd_utf8toMbcs(char const * aUtf8string);
char * tinyfd_utf16toMbcs(wchar_t const * aUtf16string);
wchar_t * tinyfd_mbcsTo16(char const * aMbcsString);
char * tinyfd_mbcsTo8(char const * aMbcsString);
wchar_t * tinyfd_utf8to16(char const * aUtf8string);
char * tinyfd_utf16to8(wchar_t const * aUtf16string);
#endif
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

/************* 3 funtions for C# (you don't need this in C or C++) : */
char const * tinyfd_getGlobalChar(char const * aCharVariableName); /* returns NULL on error */
int tinyfd_getGlobalInt(char const * aIntVariableName); /* returns -1 on error */
int tinyfd_setGlobalInt(char const * aIntVariableName, int aValue); /* returns -1 on error */
/* aCharVariableName: "tinyfd_version" "tinyfd_needs" "tinyfd_response"
   aIntVariableName : "tinyfd_verbose" "tinyfd_silent" "tinyfd_allowCursesDialogs"
				      "tinyfd_forceConsole" "tinyfd_assumeGraphicDisplay" "tinyfd_winUtf8"
**************/


extern char tinyfd_version[8]; /* contains tinyfd current version number */
extern char tinyfd_needs[]; /* info about requirements */
extern int tinyfd_verbose; /* 0 (default) or 1 : on unix, prints the command line calls */
extern int tinyfd_silent; /* 1 (default) or 0 : on unix, hide errors and warnings from called dialogs */

/* Curses dialogs are difficult to use, on windows they are only ascii and uses the unix backslah */
extern int tinyfd_allowCursesDialogs; /* 0 (default) or 1 */

extern int tinyfd_forceConsole;  /* 0 (default) or 1 */
/* for unix & windows: 0 (graphic mode) or 1 (console mode).
0: try to use a graphic solution, if it fails then it uses console mode.
1: forces all dialogs into console mode even when an X server is present,
   it can use the package dialog or dialog.exe.
   on windows it only make sense for console applications */

extern int tinyfd_assumeGraphicDisplay; /* 0 (default) or 1  */
/* some systems don't set the environment variable DISPLAY even when a graphic display is present.
set this to 1 to tell tinyfiledialogs to assume the existence of a graphic display */

extern char tinyfd_response[1024];
/* if you pass "tinyfd_query" as aTitle,
the functions will not display the dialogs
but will return 0 for console mode, 1 for graphic mode.
tinyfd_response is then filled with the retain solution.
possible values for tinyfd_response are (all lowercase)
for graphic mode:
  windows_wchar windows applescript kdialog zenity zenity3 matedialog
  shellementary qarma yad python2-tkinter python3-tkinter python-dbus
  perl-dbus gxmessage gmessage xmessage xdialog gdialog
for console mode:
  dialog whiptail basicinput no_solution */

void tinyfd_beep(void);

int tinyfd_notifyPopup(
	char const * aTitle, /* NULL or "" */
	char const * aMessage, /* NULL or "" may contain \n \t */
	char const * aIconType); /* "info" "warning" "error" */
		/* return has only meaning for tinyfd_query */

int tinyfd_messageBox(
	char const * aTitle , /* NULL or "" */
	char const * aMessage , /* NULL or "" may contain \n \t */
	char const * aDialogType , /* "ok" "okcancel" "yesno" "yesnocancel" */
	char const * aIconType , /* "info" "warning" "error" "question" */
	int aDefaultButton ) ;
		/* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */

char * tinyfd_inputBox(
	char const * aTitle , /* NULL or "" */
	char const * aMessage , /* NULL or "" (\n and \t have no effect) */
	char const * aDefaultInput ) ;  /* NULL = passwordBox, "" = inputbox */
		/* returns NULL on cancel */

char * tinyfd_saveFileDialog(
	char const * aTitle , /* NULL or "" */
	char const * aDefaultPathAndFile , /* NULL or "" */
	int aNumOfFilterPatterns , /* 0  (1 in the following example) */
	char const * const * aFilterPatterns , /* NULL or char const * lFilterPatterns[1]={"*.txt"} */
	char const * aSingleFilterDescription ) ; /* NULL or "text files" */
		/* returns NULL on cancel */

char * tinyfd_openFileDialog(
	char const * aTitle, /* NULL or "" */
	char const * aDefaultPathAndFile, /* NULL or "" */
	int aNumOfFilterPatterns , /* 0 (2 in the following example) */
	char const * const * aFilterPatterns, /* NULL or char const * lFilterPatterns[2]={"*.png","*.jpg"}; */
	char const * aSingleFilterDescription, /* NULL or "image files" */
	int aAllowMultipleSelects ) ; /* 0 or 1 */
		/* in case of multiple files, the separator is | */
		/* returns NULL on cancel */

char * tinyfd_selectFolderDialog(
	char const * aTitle, /* NULL or "" */
	char const * aDefaultPath); /* NULL or "" */
		/* returns NULL on cancel */

char * tinyfd_colorChooser(
	char const * aTitle, /* NULL or "" */
	char const * aDefaultHexRGB, /* NULL or "#FF0000" */
	unsigned char const aDefaultRGB[3] , /* unsigned char lDefaultRGB[3] = { 0 , 128 , 255 }; */
	unsigned char aoResultRGB[3] ) ; /* unsigned char lResultRGB[3]; */
		/* returns the hexcolor as a string "#FF0000" */
		/* aoResultRGB also contains the result */
		/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
		/* aDefaultRGB and aoResultRGB can be the same array */
		/* returns NULL on cancel */


/************ WINDOWS ONLY SECTION ************************/
#ifdef _WIN32

/* windows only - utf-16 version */
int tinyfd_notifyPopupW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aMessage, /* NULL or L"" may contain \n \t */
	wchar_t const * aIconType); /* L"info" L"warning" L"error" */

/* windows only - utf-16 version */
int tinyfd_messageBoxW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aMessage, /* NULL or L"" may contain \n \t */
	wchar_t const * aDialogType, /* L"ok" L"okcancel" L"yesno" */
	wchar_t const * aIconType, /* L"info" L"warning" L"error" L"question" */
	int aDefaultButton ); /* 0 for cancel/no , 1 for ok/yes */
		/* returns 0 for cancel/no , 1 for ok/yes */

/* windows only - utf-16 version */
wchar_t * tinyfd_inputBoxW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aMessage, /* NULL or L"" (\n nor \t not respected) */
	wchar_t const * aDefaultInput); /* NULL passwordBox, L"" inputbox */

/* windows only - utf-16 version */
wchar_t * tinyfd_saveFileDialogW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aDefaultPathAndFile, /* NULL or L"" */
	int aNumOfFilterPatterns, /* 0 (1 in the following example) */
	wchar_t const * const * aFilterPatterns, /* NULL or wchar_t const * lFilterPatterns[1]={L"*.txt"} */
	wchar_t const * aSingleFilterDescription); /* NULL or L"text files" */
		/* returns NULL on cancel */

/* windows only - utf-16 version */
wchar_t * tinyfd_openFileDialogW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aDefaultPathAndFile, /* NULL or L"" */
	int aNumOfFilterPatterns , /* 0 (2 in the following example) */
	wchar_t const * const * aFilterPatterns, /* NULL or wchar_t const * lFilterPatterns[2]={L"*.png","*.jpg"} */
	wchar_t const * aSingleFilterDescription, /* NULL or L"image files" */
	int aAllowMultipleSelects ) ; /* 0 or 1 */
		/* in case of multiple files, the separator is | */
		/* returns NULL on cancel */

/* windows only - utf-16 version */
wchar_t * tinyfd_selectFolderDialogW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aDefaultPath); /* NULL or L"" */
		/* returns NULL on cancel */

/* windows only - utf-16 version */
wchar_t * tinyfd_colorChooserW(
	wchar_t const * aTitle, /* NULL or L"" */
	wchar_t const * aDefaultHexRGB, /* NULL or L"#FF0000" */
	unsigned char const aDefaultRGB[3], /* unsigned char lDefaultRGB[3] = { 0 , 128 , 255 }; */
	unsigned char aoResultRGB[3]); /* unsigned char lResultRGB[3]; */
		/* returns the hexcolor as a string L"#FF0000" */
		/* aoResultRGB also contains the result */
		/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
		/* aDefaultRGB and aoResultRGB can be the same array */
		/* returns NULL on cancel */

#endif /*_WIN32 */

#ifdef	__cplusplus
} /*extern "C"*/
#endif

#endif /* TINYFILEDIALOGS_H */

/*
 ________________________________________________________________________________
|  ____________________________________________________________________________  |
| |                                                                            | |
| | on windows:                                                                | |
| |  - for UTF-16, use the wchar_t functions at the bottom of the header file  | |
| |  - _wfopen() requires wchar_t                                              | |
| |                                                                            | |
| |  - in tinyfiledialogs, char is UTF-8 by default (since v3.6)               | |
| |  - but fopen() expects MBCS (not UTF-8)                                    | |
| |  - if you want char to be MBCS: set tinyfd_winUtf8 to 0                    | |
| |                                                                            | |
| |  - alternatively, tinyfiledialogs provides                                 | |
| |                        functions to convert between UTF-8, UTF-16 and MBCS | |
| |____________________________________________________________________________| |
|________________________________________________________________________________|

- This is not for ios nor android (it works in termux though).
- The files can be renamed with extension ".cpp" as the code is 100% compatible C C++
  (just comment out << extern "C" >> in the header file)
- Windows is fully supported from XP to 10 (maybe even older versions)
- C# & LUA via dll, see files in the folder EXTRAS
- OSX supported from 10.4 to latest (maybe even older versions)
- Do not use " and ' as the dialogs will be displayed with a warning
  instead of the title, message, etc...
- There's one file filter only, it may contain several patterns.
- If no filter description is provided,
  the list of patterns will become the description.
- On windows link against Comdlg32.lib and Ole32.lib
  (on windows the no linking claim is a lie)
- On unix: it tries command line calls, so no such need (NO LINKING).
- On unix you need one of the following:
  applescript, kdialog, zenity, matedialog, shellementary, qarma, yad,
  python (2 or 3)/tkinter/python-dbus (optional), Xdialog
  or curses dialogs (opens terminal if running without console).
- One of those is already included on most (if not all) desktops.
- In the absence of those it will use gdialog, gxmessage or whiptail
  with a textinputbox. If nothing is found, it switches to basic console input,
  it opens a console if needed (requires xterm + bash).
- for curses dialogs you must set tinyfd_allowCursesDialogs=1
- You can query the type of dialog that will be used (pass "tinyfd_query" as aTitle)
- String memory is preallocated statically for all the returned values.
- File and path names are tested before return, they should be valid.
- tinyfd_forceConsole=1; at run time, forces dialogs into console mode.
- On windows, console mode only make sense for console applications.
- On windows, console mode is not implemented for wchar_T UTF-16.
- Mutiple selects are not possible in console mode.
- The package dialog must be installed to run in curses dialogs in console mode.
  It is already installed on most unix systems.
- On osx, the package dialog can be installed via
  http://macappstore.org/dialog or http://macports.org
- On windows, for curses dialogs console mode,
  dialog.exe should be copied somewhere on your executable path.
  It can be found at the bottom of the following page:
  http://andrear.altervista.org/home/cdialog.php
*/
