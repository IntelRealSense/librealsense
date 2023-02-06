/* this file can be renamed with extension ".cpp" and compiled as C++.
The code is 100% compatible C C++
(just comment out << extern "C" >> in the header file) */

/*_________
 /         \ tinyfiledialogs.c v3.9.0 [Nov 3, 2022] zlib licence
 |tiny file| Unique code file created [November 9, 2014]
 | dialogs | Copyright (c) 2014 - 2021 Guillaume Vareille http://ysengrin.com
 \____  ___/ http://tinyfiledialogs.sourceforge.net
      \|     git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd
              ____________________________________________
             |                                            |
             |   email: tinyfiledialogs at ysengrin.com   |
             |____________________________________________|
  _________________________________________________________________________________
 |                                                                                 |
 | the windows only wchar_t UTF-16 prototypes are at the bottom of the header file |
 |_________________________________________________________________________________|
  _________________________________________________________
 |                                                         |
 | on windows: - since v3.6 char is UTF-8 by default       |
 |             - if you want MBCS set tinyfd_winUtf8 to 0  |
 |             - functions like fopen expect MBCS          |
 |_________________________________________________________|

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
-----------

Thanks for contributions, bug corrections & thorough testing to:
- Don Heyse http://ldglite.sf.net for bug corrections & thorough testing!
- Paul Rouget
*/


#ifndef __sun
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2 /* to accept POSIX 2 in old ANSI C standards */
#endif
#endif

#if !defined(_WIN32) && ( defined(__GNUC__) || defined(__clang__) )
#if !defined(_GNU_SOURCE)
 #define _GNU_SOURCE /* used only to resolve symbolic links. Can be commented out */
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
 #ifdef __BORLANDC__
  #define _getch getch
 #endif
 #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0500
 #endif
 #include <windows.h>
 #include <commdlg.h>
 #include <shlobj.h>
 #include <conio.h>
 #include <direct.h>
 #define TINYFD_NOCCSUNICODE
 #define SLASH "\\"
#else
 #include <limits.h>
 #include <unistd.h>
 #include <dirent.h> /* on old systems try <sys/dir.h> instead */
 #include <termios.h>
 #include <sys/utsname.h>
 #include <signal.h> /* on old systems try <sys/signal.h> instead */
 #define SLASH "/"
#endif /* _WIN32 */

#include "tinyfiledialogs.h"

#define MAX_PATH_OR_CMD 1024 /* _MAX_PATH or MAX_PATH */

#ifndef MAX_MULTIPLE_FILES
#define MAX_MULTIPLE_FILES 1024
#endif
#define LOW_MULTIPLE_FILES 32

char tinyfd_version[8] = "3.9.0";

/******************************************************************************************************/
/**************************************** UTF-8 on Windows ********************************************/
/******************************************************************************************************/
#ifdef _WIN32
/* if you want to use UTF-8 ( instead of the UTF-16/wchar_t functions at the end of tinyfiledialogs.h )
Make sure your code is really prepared for UTF-8 (on windows, functions like fopen() expect MBCS and not UTF-8) */
int tinyfd_winUtf8 = 1; /* on windows char strings can be 1:UTF-8(default) or 0:MBCS */
/* for MBCS change this to 0, here or in your code */
#endif
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

int tinyfd_verbose = 0 ; /* on unix: prints the command line calls */
int tinyfd_silent = 1 ; /* 1 (default) or 0 : on unix, hide errors and warnings from called dialogs */

/* Curses dialogs are difficult to use, on windows they are only ascii and uses the unix backslah */
int tinyfd_allowCursesDialogs = 0 ; /* 0 (default) or 1 */
int tinyfd_forceConsole = 0 ; /* 0 (default) or 1 */
/* for unix & windows: 0 (graphic mode) or 1 (console mode).
0: try to use a graphic solution, if it fails then it uses console mode.
1: forces all dialogs into console mode even when the X server is present.
   it can use the package dialog or dialog.exe.
   on windows it only make sense for console applications */

int tinyfd_assumeGraphicDisplay = 0; /* 0 (default) or 1  */
/* some systems don't set the environment variable DISPLAY even when a graphic display is present.
set this to 1 to tell tinyfiledialogs to assume the existence of a graphic display */


char tinyfd_response[1024];
/* if you pass "tinyfd_query" as aTitle,
the functions will not display the dialogs
but and return 0 for console mode, 1 for graphic mode.
tinyfd_response is then filled with the retain solution.
possible values for tinyfd_response are (all lowercase)
for graphic mode:
  windows_wchar windows applescript kdialog zenity zenity3 matedialog
  shellementary qarma yad python2-tkinter python3-tkinter python-dbus
  perl-dbus gxmessage gmessage xmessage xdialog gdialog
for console mode:
  dialog whiptail basicinput no_solution */

static int gWarningDisplayed = 0 ;
static char gTitle[]="missing software! (we will try basic console input)";

#ifdef _WIN32
char tinyfd_needs[] = "\
 ___________\n\
/           \\ \n\
| tiny file |\n\
|  dialogs  |\n\
\\_____  ____/\n\
      \\|\
\ntiny file dialogs on Windows needs:\
\n   a graphic display\
\nor dialog.exe (curses console mode)\
\nor a console for basic input";
#else
char tinyfd_needs[] = "\
 ___________\n\
/           \\ \n\
| tiny file |\n\
|  dialogs  |\n\
\\_____  ____/\n\
      \\|\
\ntiny file dialogs on UNIX needs:\
\n   applescript or kdialog or yad or Xdialog\
\nor zenity (or matedialog or shellementary or qarma)\
\nor python (2 or 3) + tkinter + python-dbus (optional)\
\nor dialog (opens console if needed) ** Disabled by default **/\
\nor xterm + bash (opens console for basic input)\
\nor existing console for basic input";
#endif

#ifdef _MSC_VER
#pragma warning(disable:4996) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#pragma warning(disable:4100) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#pragma warning(disable:4706) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#endif

static int getenvDISPLAY(void)
{
	return tinyfd_assumeGraphicDisplay || getenv("DISPLAY");
}


static char * getCurDir(void)
{
	static char lCurDir[MAX_PATH_OR_CMD];
	return getcwd(lCurDir, sizeof(lCurDir));
}


static char * getPathWithoutFinalSlash(
        char * aoDestination, /* make sure it is allocated, use _MAX_PATH */
        char const * aSource) /* aoDestination and aSource can be the same */
{
        char const * lTmp ;
        if ( aSource )
        {
                lTmp = strrchr(aSource, '/');
                if (!lTmp)
                {
                        lTmp = strrchr(aSource, '\\');
                }
                if (lTmp)
                {
                        strncpy(aoDestination, aSource, lTmp - aSource );
                        aoDestination[lTmp - aSource] = '\0';
                }
                else
                {
                        * aoDestination = '\0';
                }
        }
        else
        {
                * aoDestination = '\0';
        }
        return aoDestination;
}


static char * getLastName(
        char * aoDestination, /* make sure it is allocated */
        char const * aSource)
{
        /* copy the last name after '/' or '\' */
        char const * lTmp ;
        if ( aSource )
        {
                lTmp = strrchr(aSource, '/');
                if (!lTmp)
                {
                        lTmp = strrchr(aSource, '\\');
                }
                if (lTmp)
                {
                        strcpy(aoDestination, lTmp + 1);
                }
                else
                {
                        strcpy(aoDestination, aSource);
                }
        }
        else
        {
                * aoDestination = '\0';
        }
        return aoDestination;
}


static void ensureFinalSlash( char * aioString )
{
        if ( aioString && strlen( aioString ) )
        {
                char * lastcar = aioString + strlen( aioString ) - 1 ;
                if ( strncmp( lastcar , SLASH , 1 ) )
                {
                        strcat( lastcar , SLASH ) ;
                }
        }
}


static void Hex2RGB( char const aHexRGB[8] , unsigned char aoResultRGB[3] )
{
        char lColorChannel[8] ;
        if ( aoResultRGB )
        {
                if ( aHexRGB )
                {
                        strcpy(lColorChannel, aHexRGB ) ;
                        aoResultRGB[2] = (unsigned char)strtoul(lColorChannel+5,NULL,16);
                        lColorChannel[5] = '\0';
                        aoResultRGB[1] = (unsigned char)strtoul(lColorChannel+3,NULL,16);
                        lColorChannel[3] = '\0';
                        aoResultRGB[0] = (unsigned char)strtoul(lColorChannel+1,NULL,16);
/* printf("%d %d %d\n", aoResultRGB[0], aoResultRGB[1], aoResultRGB[2]); */
                }
                else
                {
                        aoResultRGB[0]=0;
                        aoResultRGB[1]=0;
                        aoResultRGB[2]=0;
                }
        }
}

static void RGB2Hex( unsigned char const aRGB[3], char aoResultHexRGB[8] )
{
        if ( aoResultHexRGB )
        {
                if ( aRGB )
                {
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
    sprintf(aoResultHexRGB, "#%02hhx%02hhx%02hhx", aRGB[0], aRGB[1], aRGB[2]);
#else
    sprintf(aoResultHexRGB, "#%02hx%02hx%02hx", aRGB[0], aRGB[1], aRGB[2]);
#endif
                         /*printf("aoResultHexRGB %s\n", aoResultHexRGB);*/
                }
                else
                {
                        aoResultHexRGB[0]=0;
                        aoResultHexRGB[1]=0;
                        aoResultHexRGB[2]=0;
                }
        }
}


void tfd_replaceSubStr( char const * aSource, char const * aOldSubStr,
                        char const * aNewSubStr, char * aoDestination )
{
        char const * pOccurence ;
        char const * p ;
        char const * lNewSubStr = "" ;
        size_t lOldSubLen = strlen( aOldSubStr ) ;

        if ( ! aSource )
        {
                * aoDestination = '\0' ;
                return ;
        }
        if ( ! aOldSubStr )
        {
                strcpy( aoDestination , aSource ) ;
                return ;
        }
        if ( aNewSubStr )
        {
                lNewSubStr = aNewSubStr ;
        }
        p = aSource ;
        * aoDestination = '\0' ;
        while ( ( pOccurence = strstr( p , aOldSubStr ) ) != NULL )
        {
                strncat( aoDestination , p , pOccurence - p ) ;
                strcat( aoDestination , lNewSubStr ) ;
                p = pOccurence + lOldSubLen ;
        }
        strcat( aoDestination , p ) ;
}


static int filenameValid( char const * aFileNameWithoutPath )
{
        if ( ! aFileNameWithoutPath
          || ! strlen(aFileNameWithoutPath)
          || strpbrk(aFileNameWithoutPath , "\\/:*?\"<>|") )
        {
                return 0 ;
        }
        return 1 ;
}

#ifndef _WIN32

static int fileExists( char const * aFilePathAndName )
{
        FILE * lIn ;
        if ( ! aFilePathAndName || ! strlen(aFilePathAndName) )
        {
                return 0 ;
        }
        lIn = fopen( aFilePathAndName , "r" ) ;
        if ( ! lIn )
        {
                return 0 ;
        }
        fclose( lIn ) ;
        return 1 ;
}

#endif


static void wipefile(char const * aFilename)
{
        int i;
        struct stat st;
        FILE * lIn;

        if (stat(aFilename, &st) == 0)
        {
                if ((lIn = fopen(aFilename, "w")))
                {
                        for (i = 0; i < st.st_size; i++)
                        {
                                fputc('A', lIn);
                        }
                        fclose(lIn);
                }
        }
}


int tfd_quoteDetected(char const * aString)
{
	char const * p;

	if (!aString) return 0;

	p = aString;
	while ((p = strchr(p, '\'')))
	{
		return 1;
	}

	p = aString;
	while ((p = strchr(p, '\"')))
	{
		return 1;
	}

	return 0;
}


char const * tinyfd_getGlobalChar(char const * aCharVariableName) /* to be called from C# (you don't need this in C or C++) */
{
	if (!aCharVariableName || !strlen(aCharVariableName)) return NULL;
	else if (!strcmp(aCharVariableName, "tinyfd_version")) return tinyfd_version;
	else if (!strcmp(aCharVariableName, "tinyfd_needs")) return tinyfd_needs;
	else if (!strcmp(aCharVariableName, "tinyfd_response")) return tinyfd_response;
	else return NULL ;
}


int tinyfd_getGlobalInt(char const * aIntVariableName) /* to be called from C# (you don't need this in C or C++) */
{
	if ( !aIntVariableName || !strlen(aIntVariableName) ) return -1 ;
	else if ( !strcmp(aIntVariableName, "tinyfd_verbose") ) return tinyfd_verbose ;
	else if ( !strcmp(aIntVariableName, "tinyfd_silent") ) return tinyfd_silent ;
	else if ( !strcmp(aIntVariableName, "tinyfd_allowCursesDialogs") ) return tinyfd_allowCursesDialogs ;
	else if ( !strcmp(aIntVariableName, "tinyfd_forceConsole") ) return tinyfd_forceConsole ;
	else if ( !strcmp(aIntVariableName, "tinyfd_assumeGraphicDisplay") ) return tinyfd_assumeGraphicDisplay ;
#ifdef _WIN32
	else if ( !strcmp(aIntVariableName, "tinyfd_winUtf8") ) return tinyfd_winUtf8 ;
#endif
	else return -1;
}


int tinyfd_setGlobalInt(char const * aIntVariableName, int aValue) /* to be called from C# (you don't need this in C or C++) */
{
	if (!aIntVariableName || !strlen(aIntVariableName)) return -1 ;
	else if (!strcmp(aIntVariableName, "tinyfd_verbose")) { tinyfd_verbose = aValue; return tinyfd_verbose; }
	else if (!strcmp(aIntVariableName, "tinyfd_silent")) { tinyfd_silent = aValue; return tinyfd_silent; }
	else if (!strcmp(aIntVariableName, "tinyfd_allowCursesDialogs")) { tinyfd_allowCursesDialogs = aValue; return tinyfd_allowCursesDialogs; }
	else if (!strcmp(aIntVariableName, "tinyfd_forceConsole")) { tinyfd_forceConsole = aValue; return tinyfd_forceConsole; }
	else if (!strcmp(aIntVariableName, "tinyfd_assumeGraphicDisplay")) { tinyfd_assumeGraphicDisplay = aValue; return tinyfd_assumeGraphicDisplay; }
#ifdef _WIN32
	else if (!strcmp(aIntVariableName, "tinyfd_winUtf8")) { tinyfd_winUtf8 = aValue; return tinyfd_winUtf8; }
#endif
	else return -1;
}


#ifdef _WIN32
static int powershellPresent(void)
{ /*only on vista and above (or installed on xp)*/
    static int lPowershellPresent = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE* lIn;
    char const* lString = "powershell.exe";

    if (lPowershellPresent < 0)
    {
        if (!(lIn = _popen("where powershell.exe", "r")))
        {
            lPowershellPresent = 0;
            return 0;
        }
        while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
        {
        }
        _pclose(lIn);
        if (lBuff[strlen(lBuff) - 1] == '\n')
        {
            lBuff[strlen(lBuff) - 1] = '\0';
        }
        if (strcmp(lBuff + strlen(lBuff) - strlen(lString), lString))
        {
            lPowershellPresent = 0;
        }
        else
        {
            lPowershellPresent = 1;
        }
    }
    return lPowershellPresent;
}

static int windowsVersion(void)
{
#if !defined(__MINGW32__) || defined(__MINGW64_VERSION_MAJOR)
    typedef LONG NTSTATUS  ;
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod;
    RtlGetVersionPtr lFxPtr;
    RTL_OSVERSIONINFOW lRovi = { 0 };

    hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        lFxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (lFxPtr)
        {
            lRovi.dwOSVersionInfoSize = sizeof(lRovi);
            if (!lFxPtr(&lRovi))
            {
                return lRovi.dwMajorVersion;
            }
        }
    }
#endif
    if (powershellPresent()) return 6; /*minimum is vista or installed on xp*/
    return 0;
}


static void replaceChr(char * aString, char aOldChr, char aNewChr)
{
	char * p;

	if (!aString) return;
	if (aOldChr == aNewChr) return;

	p = aString;
	while ((p = strchr(p, aOldChr)))
	{
		*p = aNewChr;
		p++;
	}
	return;
}


#if !defined(WC_ERR_INVALID_CHARS)
/* undefined prior to Vista, so not yet in MINGW header file */
#define WC_ERR_INVALID_CHARS 0x00000000 /* 0x00000080 for MINGW maybe ? */
#endif

static int sizeUtf16From8(char const * aUtf8string)
{
	return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		aUtf8string, -1, NULL, 0);
}


static int sizeUtf16FromMbcs(char const * aMbcsString)
{
	return MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
		aMbcsString, -1, NULL, 0);
}


static int sizeUtf8(wchar_t const * aUtf16string)
{
	return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
		aUtf16string, -1, NULL, 0, NULL, NULL);
}


static int sizeMbcs(wchar_t const * aMbcsString)
{
	int lRes = WideCharToMultiByte(CP_ACP, 0,
		aMbcsString, -1, NULL, 0, NULL, NULL);
	/* DWORD licic = GetLastError(); */
	return lRes;
}


wchar_t* tinyfd_mbcsTo16(char const* aMbcsString)
{
    static wchar_t* lMbcsString = NULL;
    int lSize;

    free(lMbcsString);
    if (!aMbcsString) { lMbcsString = NULL; return NULL; }
    lSize = sizeUtf16FromMbcs(aMbcsString);
    if (lSize)
    {
        lMbcsString = (wchar_t*)malloc(lSize * sizeof(wchar_t));
        lSize = MultiByteToWideChar(CP_ACP, 0, aMbcsString, -1, lMbcsString, lSize);
    }
    else wcscpy(lMbcsString, L"");
    return lMbcsString;
}


wchar_t * tinyfd_utf8to16(char const * aUtf8string)
{
	static wchar_t * lUtf16string = NULL;
	int lSize;

	free(lUtf16string);
	if (!aUtf8string) {lUtf16string = NULL; return NULL;}
	lSize = sizeUtf16From8(aUtf8string);
    if (lSize)
    {
        lUtf16string = (wchar_t*)malloc(lSize * sizeof(wchar_t));
        lSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            aUtf8string, -1, lUtf16string, lSize);
        return lUtf16string;
    }
    else
    {
        /* let's try mbcs anyway */
        lUtf16string = NULL;
        return tinyfd_mbcsTo16(aUtf8string);
    }
}


char * tinyfd_utf16toMbcs(wchar_t const * aUtf16string)
{
	static char * lMbcsString = NULL;
	int lSize;

	free(lMbcsString);
	if (!aUtf16string) { lMbcsString = NULL; return NULL; }
	lSize = sizeMbcs(aUtf16string);
    if (lSize)
    {
        lMbcsString = (char*)malloc(lSize);
        lSize = WideCharToMultiByte(CP_ACP, 0, aUtf16string, -1, lMbcsString, lSize, NULL, NULL);
    }
    else strcpy(lMbcsString, "");
	return lMbcsString;
}


char * tinyfd_utf8toMbcs(char const * aUtf8string)
{
	wchar_t const * lUtf16string;
	lUtf16string = tinyfd_utf8to16(aUtf8string);
	return tinyfd_utf16toMbcs(lUtf16string);
}


char * tinyfd_utf16to8(wchar_t const * aUtf16string)
{
	static char * lUtf8string = NULL;
	int lSize;

	free(lUtf8string);
	if (!aUtf16string) { lUtf8string = NULL; return NULL; }
	lSize = sizeUtf8(aUtf16string);
    if (lSize)
    {
        lUtf8string = (char*)malloc(lSize);
        lSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, aUtf16string, -1, lUtf8string, lSize, NULL, NULL);
    }
    else strcpy(lUtf8string, "");
	return lUtf8string;
}


char * tinyfd_mbcsTo8(char const * aMbcsString)
{
	wchar_t const * lUtf16string;
	lUtf16string = tinyfd_mbcsTo16(aMbcsString);
	return tinyfd_utf16to8(lUtf16string);
}


void tinyfd_beep(void)
{
    if (windowsVersion() > 5) Beep(440, 300);
    else MessageBeep(MB_OK);
}


static void wipefileW(wchar_t const * aFilename)
{
        int i;
        FILE * lIn;
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
        struct _stat st;
        if (_wstat(aFilename, &st) == 0)
#else
        struct __stat64 st;
        if (_wstat64(aFilename, &st) == 0)
#endif
        {
                if ((lIn = _wfopen(aFilename, L"w")))
                {
                        for (i = 0; i < st.st_size; i++)
                        {
                                fputc('A', lIn);
                        }
                        fclose(lIn);
                }
        }
}


static wchar_t * getPathWithoutFinalSlashW(
        wchar_t * aoDestination, /* make sure it is allocated, use _MAX_PATH */
        wchar_t const * aSource) /* aoDestination and aSource can be the same */
{
        wchar_t const * lTmp;
        if (aSource)
        {
                lTmp = wcsrchr(aSource, L'/');
                if (!lTmp)
                {
                        lTmp = wcsrchr(aSource, L'\\');
                }
                if (lTmp)
                {
                        wcsncpy(aoDestination, aSource, lTmp - aSource);
                        aoDestination[lTmp - aSource] = L'\0';
                }
                else
                {
                        *aoDestination = L'\0';
                }
        }
        else
        {
                *aoDestination = L'\0';
        }
        return aoDestination;
}


static wchar_t * getLastNameW(
        wchar_t * aoDestination, /* make sure it is allocated */
        wchar_t const * aSource)
{
        /* copy the last name after '/' or '\' */
        wchar_t const * lTmp;
        if (aSource)
        {
                lTmp = wcsrchr(aSource, L'/');
                if (!lTmp)
                {
                        lTmp = wcsrchr(aSource, L'\\');
                }
                if (lTmp)
                {
                        wcscpy(aoDestination, lTmp + 1);
                }
                else
                {
                        wcscpy(aoDestination, aSource);
                }
        }
        else
        {
                *aoDestination = L'\0';
        }
        return aoDestination;
}


static void Hex2RGBW(wchar_t const aHexRGB[8], unsigned char aoResultRGB[3])
{
        wchar_t lColorChannel[8];
        if (aoResultRGB)
        {
                if (aHexRGB)
                {
                        wcscpy(lColorChannel, aHexRGB);
                        aoResultRGB[2] = (unsigned char)wcstoul(lColorChannel + 5, NULL, 16);
                        lColorChannel[5] = '\0';
                        aoResultRGB[1] = (unsigned char)wcstoul(lColorChannel + 3, NULL, 16);
                        lColorChannel[3] = '\0';
                        aoResultRGB[0] = (unsigned char)wcstoul(lColorChannel + 1, NULL, 16);
                        /* printf("%d %d %d\n", aoResultRGB[0], aoResultRGB[1], aoResultRGB[2]); */
                }
                else
                {
                        aoResultRGB[0] = 0;
                        aoResultRGB[1] = 0;
                        aoResultRGB[2] = 0;
                }
        }
}


static void RGB2HexW( unsigned char const aRGB[3], wchar_t aoResultHexRGB[8])
{
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
	wchar_t const * const lPrintFormat = L"#%02hhx%02hhx%02hhx";
#else
	wchar_t const * const lPrintFormat = L"#%02hx%02hx%02hx";
#endif

        if (aoResultHexRGB)
        {
                if (aRGB)
                {
                        /* wprintf(L"aoResultHexRGB %s\n", aoResultHexRGB); */
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
					swprintf(aoResultHexRGB, 8, lPrintFormat, aRGB[0], aRGB[1], aRGB[2]);
#else
					swprintf(aoResultHexRGB, lPrintFormat, aRGB[0], aRGB[1], aRGB[2]);
#endif

                }
                else
                {
                        aoResultHexRGB[0] = 0;
                        aoResultHexRGB[1] = 0;
                        aoResultHexRGB[2] = 0;
                }
        }
}


static int dirExists(char const * aDirPath)
{
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
    struct _stat lInfo;
#else
    struct __stat64 lInfo;
#endif
        wchar_t * lTmpWChar;
        int lStatRet;
		size_t lDirLen;

		if (!aDirPath)
			return 0;
		lDirLen = strlen(aDirPath);
		if (!lDirLen)
			return 1;
		if ( (lDirLen == 2) && (aDirPath[1] == ':') )
			return 1;

        if (tinyfd_winUtf8)
        {
			lTmpWChar = tinyfd_utf8to16(aDirPath);
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
            lStatRet = _wstat(lTmpWChar, &lInfo);
#else
            lStatRet = _wstat64(lTmpWChar, &lInfo);
#endif
            if (lStatRet != 0)
                    return 0;
            else if (lInfo.st_mode & S_IFDIR)
                    return 1;
            else
                        return 0;
        }
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
        else if (_stat(aDirPath, &lInfo) != 0)
#else
        else if (_stat64(aDirPath, &lInfo) != 0)
#endif
                return 0;
        else if (lInfo.st_mode & S_IFDIR)
                return 1;
        else
                return 0;
}


static int fileExists(char const * aFilePathAndName)
{
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
    struct _stat lInfo;
#else
    struct __stat64 lInfo;
#endif
        wchar_t * lTmpWChar;
        int lStatRet;
        FILE * lIn;

        if (!aFilePathAndName || !strlen(aFilePathAndName))
        {
                return 0;
        }

        if (tinyfd_winUtf8)
        {
			lTmpWChar = tinyfd_utf8to16(aFilePathAndName);
#if defined(__MINGW32_MAJOR_VERSION) && !defined(__MINGW64__) && (__MINGW32_MAJOR_VERSION <= 3)
            lStatRet = _wstat(lTmpWChar, &lInfo);
#else
            lStatRet = _wstat64(lTmpWChar, &lInfo);
#endif

            if (lStatRet != 0)
                    return 0;
            else if (lInfo.st_mode & _S_IFREG)
                    return 1;
            else
                    return 0;
        }
        else
        {
                lIn = fopen(aFilePathAndName, "r");
                if (!lIn)
                {
                        return 0;
                }
                fclose(lIn);
                return 1;
        }
}

static void replaceWchar(wchar_t * aString,
	wchar_t aOldChr,
	wchar_t aNewChr)
{
	wchar_t * p;

	if (!aString)
	{
		return ;
	}

	if (aOldChr == aNewChr)
	{
		return ;
	}

	p = aString;
	while ((p = wcsrchr(p, aOldChr)))
	{
		*p = aNewChr;
#ifdef TINYFD_NOCCSUNICODE
		p++;
#endif
		p++;
	}
	return ;
}


static int quoteDetectedW(wchar_t const * aString)
{
	wchar_t const * p;

	if (!aString) return 0;

	p = aString;
	while ((p = wcsrchr(p, L'\'')))
	{
		return 1;
	}

	p = aString;
	while ((p = wcsrchr(p, L'\"')))
	{
		return 1;
	}

	return 0;
}

#endif /* _WIN32 */

/* source and destination can be the same or ovelap*/
static char * ensureFilesExist(char * aDestination,
        char const * aSourcePathsAndNames)
{
        char * lDestination = aDestination;
        char const * p;
        char const * p2;
        size_t lLen;

        if (!aSourcePathsAndNames)
        {
                return NULL;
        }
        lLen = strlen(aSourcePathsAndNames);
        if (!lLen)
        {
                return NULL;
        }

        p = aSourcePathsAndNames;
        while ((p2 = strchr(p, '|')) != NULL)
        {
                lLen = p2 - p;
                memmove(lDestination, p, lLen);
                lDestination[lLen] = '\0';
                if (fileExists(lDestination))
                {
                        lDestination += lLen;
                        *lDestination = '|';
                        lDestination++;
                }
                p = p2 + 1;
        }
        if (fileExists(p))
        {
                lLen = strlen(p);
                memmove(lDestination, p, lLen);
                lDestination[lLen] = '\0';
        }
        else
        {
                *(lDestination - 1) = '\0';
        }
        return aDestination;
}

#ifdef _WIN32

static int __stdcall EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
        wchar_t lTitleName[MAX_PATH];
        wchar_t const* lDialogTitle = (wchar_t const *) lParam;

        GetWindowTextW(hwnd, lTitleName, MAX_PATH);
        /* wprintf(L"lTitleName %ls \n", lTitleName);  */

        if (wcscmp(lDialogTitle, lTitleName) == 0)
        {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                return 0;
        }
        return 1;
}


static void hiddenConsoleW(wchar_t const * aString, wchar_t const * aDialogTitle, int aInFront)
{
        STARTUPINFOW StartupInfo;
        PROCESS_INFORMATION ProcessInfo;

        if (!aString || !wcslen(aString) ) return;

        memset(&StartupInfo, 0, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(STARTUPINFOW);
        StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_HIDE;

        if (!CreateProcessW(NULL, (LPWSTR)aString, NULL, NULL, FALSE,
                                CREATE_NEW_CONSOLE, NULL, NULL,
                                &StartupInfo, &ProcessInfo))
        {
                return; /* GetLastError(); */
        }

        WaitForInputIdle(ProcessInfo.hProcess, INFINITE);
        if (aInFront)
        {
                while (EnumWindows(EnumThreadWndProc, (LPARAM)aDialogTitle)) {}
        }
        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
}


int tinyfd_messageBoxW(
        wchar_t const * aTitle, /* NULL or "" */
        wchar_t const * aMessage, /* NULL or ""  may contain \n and \t */
        wchar_t const * aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
        wchar_t const * aIconType, /* "info" "warning" "error" "question" */
        int aDefaultButton) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
        int lBoxReturnValue;
        UINT aCode;

        if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return 1; }

		if (quoteDetectedW(aTitle)) return tinyfd_messageBoxW(L"INVALID TITLE WITH QUOTES", aMessage, aDialogType, aIconType, aDefaultButton);
		if (quoteDetectedW(aMessage)) return tinyfd_messageBoxW(aTitle, L"INVALID MESSAGE WITH QUOTES", aDialogType, aIconType, aDefaultButton);

        if (aIconType && !wcscmp(L"warning", aIconType))
        {
                aCode = MB_ICONWARNING;
        }
        else if (aIconType && !wcscmp(L"error", aIconType))
        {
                aCode = MB_ICONERROR;
        }
        else if (aIconType && !wcscmp(L"question", aIconType))
        {
                aCode = MB_ICONQUESTION;
        }
        else
        {
                aCode = MB_ICONINFORMATION;
        }

        if (aDialogType && !wcscmp(L"okcancel", aDialogType))
        {
                aCode += MB_OKCANCEL;
                if (!aDefaultButton)
                {
                        aCode += MB_DEFBUTTON2;
                }
        }
        else if (aDialogType && !wcscmp(L"yesno", aDialogType))
        {
                aCode += MB_YESNO;
                if (!aDefaultButton)
                {
                        aCode += MB_DEFBUTTON2;
                }
        }
        else if (aDialogType && !wcscmp(L"yesnocancel", aDialogType))
        {
            aCode += MB_YESNOCANCEL;
            if (aDefaultButton == 1)
            {
                aCode += MB_DEFBUTTON1;
            }
            else if (aDefaultButton == 2)
            {
                aCode += MB_DEFBUTTON2;
            }
            else
            {
                aCode += MB_DEFBUTTON3;
            }
        }
        else
        {
                aCode += MB_OK;
        }

        aCode += MB_TOPMOST;

        lBoxReturnValue = MessageBoxW(GetForegroundWindow(), aMessage, aTitle, aCode);

        if ( (lBoxReturnValue == IDNO) && (aDialogType && !wcscmp(L"yesnocancel", aDialogType)) )
        {
            return 2;
        }
        else if ( (lBoxReturnValue == IDOK) || (lBoxReturnValue == IDYES) )
        {
            return 1;
        }
        else
        {
            return 0;
        }
}


/* return has only meaning for tinyfd_query */
int tinyfd_notifyPopupW(
        wchar_t const * aTitle, /* NULL or L"" */
        wchar_t const * aMessage, /* NULL or L"" may contain \n \t */
        wchar_t const * aIconType) /* L"info" L"warning" L"error" */
{
        wchar_t * lDialogString;
        size_t lTitleLen;
        size_t lMessageLen;
        size_t lDialogStringLen;

        if (aTitle && !wcscmp(aTitle, L"tinyfd_query")) { strcpy(tinyfd_response, "windows_wchar"); return 1; }
        
        if (quoteDetectedW(aTitle)) return tinyfd_notifyPopupW(L"INVALID TITLE WITH QUOTES", aMessage, aIconType);
		if (quoteDetectedW(aMessage)) return tinyfd_notifyPopupW(aTitle, L"INVALID MESSAGE WITH QUOTES", aIconType);

        lTitleLen = aTitle ? wcslen(aTitle) : 0;
        lMessageLen = aMessage ? wcslen(aMessage) : 0;
        lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
        lDialogString = (wchar_t *)malloc(2 * lDialogStringLen);
        if (!lDialogString) return 0;

        wcscpy(lDialogString, L"powershell.exe -command \"\
function Show-BalloonTip {\
[cmdletbinding()] \
param( \
[string]$Title = ' ', \
[string]$Message = ' ', \
[ValidateSet('info', 'warning', 'error')] \
[string]$IconType = 'info');\
[system.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms') | Out-Null ; \
$balloon = New-Object System.Windows.Forms.NotifyIcon ; \
$path = Get-Process -id $pid | Select-Object -ExpandProperty Path ; \
$icon = [System.Drawing.Icon]::ExtractAssociatedIcon($path) ;");

        wcscat(lDialogString, L"\
$balloon.Icon = $icon ; \
$balloon.BalloonTipIcon = $IconType ; \
$balloon.BalloonTipText = $Message ; \
$balloon.BalloonTipTitle = $Title ; \
$balloon.Text = 'tinyfiledialogs' ; \
$balloon.Visible = $true ; \
$balloon.ShowBalloonTip(5000)};\
Show-BalloonTip");

        if (aTitle && wcslen(aTitle))
        {
                wcscat(lDialogString, L" -Title '");
                wcscat(lDialogString, aTitle);
                wcscat(lDialogString, L"'");
        }
        if (aMessage && wcslen(aMessage))
        {
                wcscat(lDialogString, L" -Message '");
                wcscat(lDialogString, aMessage);
                wcscat(lDialogString, L"'");
        }
        if (aMessage && wcslen(aIconType))
        {
                wcscat(lDialogString, L" -IconType '");
                wcscat(lDialogString, aIconType);
                wcscat(lDialogString, L"'");
        }
        wcscat(lDialogString, L"\"");

        /* wprintf ( L"lDialogString: %ls\n" , lDialogString ) ; */

        hiddenConsoleW(lDialogString, aTitle, 0);
        free(lDialogString);
        return 1;
}


wchar_t * tinyfd_inputBoxW(
        wchar_t const * aTitle, /* NULL or L"" */
        wchar_t const * aMessage, /* NULL or L"" (\n and \t have no effect) */
        wchar_t const * aDefaultInput) /* L"" , if NULL it's a passwordBox */
{
        static wchar_t lBuff[MAX_PATH_OR_CMD];
        wchar_t * lDialogString;
        FILE * lIn;
        FILE * lFile;
        int lResult;
        size_t lTitleLen;
        size_t lMessageLen;
        size_t lDialogStringLen;

        if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return (wchar_t *)1; }

		if (quoteDetectedW(aTitle)) return tinyfd_inputBoxW(L"INVALID TITLE WITH QUOTES", aMessage, aDefaultInput);
		if (quoteDetectedW(aMessage)) return tinyfd_inputBoxW(aTitle, L"INVALID MESSAGE WITH QUOTES", aDefaultInput);
		if (quoteDetectedW(aDefaultInput)) return tinyfd_inputBoxW(aTitle, aMessage, L"INVALID DEFAULT_INPUT WITH QUOTES");

        lTitleLen =  aTitle ? wcslen(aTitle) : 0 ;
        lMessageLen =  aMessage ? wcslen(aMessage) : 0 ;
        lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
        lDialogString = (wchar_t *)malloc(2 * lDialogStringLen);

        if (aDefaultInput)
        {
			swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                lDialogStringLen,
#endif
                L"%ls\\tinyfd.vbs", _wgetenv(L"TEMP"));
        }
        else
        {
                swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                        lDialogStringLen,
#endif
                L"%ls\\tinyfd.hta", _wgetenv(L"TEMP"));
        }
        lIn = _wfopen(lDialogString, L"w");
        if (!lIn)
        {
                free(lDialogString);
                return NULL;
        }

        if ( aDefaultInput )
        {
                wcscpy(lDialogString, L"Dim result:result=InputBox(\"");
                if (aMessage && wcslen(aMessage))
                {
					wcscpy(lBuff, aMessage);
					replaceWchar(lBuff, L'\n', L' ');
					wcscat(lDialogString, lBuff);
                }
                wcscat(lDialogString, L"\",\"");
                if (aTitle) wcscat(lDialogString, aTitle);
                wcscat(lDialogString, L"\",\"");

                if (aDefaultInput && wcslen(aDefaultInput))
                {
					wcscpy(lBuff, aDefaultInput);
					replaceWchar(lBuff, L'\n', L' ');
					wcscat(lDialogString, lBuff);
                }
                wcscat(lDialogString, L"\"):If IsEmpty(result) then:WScript.Echo 0");
                wcscat(lDialogString, L":Else: WScript.Echo \"1\" & result : End If");
        }
        else
        {
                wcscpy(lDialogString, L"\n\
<html>\n\
<head>\n\
<title>");
                if (aTitle) wcscat(lDialogString, aTitle);
                wcscat(lDialogString, L"</title>\n\
<HTA:APPLICATION\n\
ID = 'tinyfdHTA'\n\
APPLICATIONNAME = 'tinyfd_inputBox'\n\
MINIMIZEBUTTON = 'no'\n\
MAXIMIZEBUTTON = 'no'\n\
BORDER = 'dialog'\n\
SCROLL = 'no'\n\
SINGLEINSTANCE = 'yes'\n\
WINDOWSTATE = 'hidden'>\n\
\n\
<script language = 'VBScript'>\n\
\n\
intWidth = Screen.Width/4\n\
intHeight = Screen.Height/6\n\
ResizeTo intWidth, intHeight\n\
MoveTo((Screen.Width/2)-(intWidth/2)),((Screen.Height/2)-(intHeight/2))\n\
result = 0\n\
\n\
Sub Window_onLoad\n\
txt_input.Focus\n\
End Sub\n\
\n");

                wcscat(lDialogString, L"\
Sub Window_onUnload\n\
Set objFSO = CreateObject(\"Scripting.FileSystemObject\")\n\
Set oShell = CreateObject(\"WScript.Shell\")\n\
strTempFolder = oShell.ExpandEnvironmentStrings(\"%TEMP%\")\n\
Set objFile = objFSO.CreateTextFile(strTempFolder & \"\\tinyfd.txt\",True,True)\n\
If result = 1 Then\n\
objFile.Write 1 & txt_input.Value\n\
Else\n\
objFile.Write 0\n\
End If\n\
objFile.Close\n\
End Sub\n\
\n\
Sub Run_ProgramOK\n\
result = 1\n\
window.Close\n\
End Sub\n\
\n\
Sub Run_ProgramCancel\n\
window.Close\n\
End Sub\n\
\n");

                wcscat(lDialogString, L"Sub Default_Buttons\n\
If Window.Event.KeyCode = 13 Then\n\
btn_OK.Click\n\
ElseIf Window.Event.KeyCode = 27 Then\n\
btn_Cancel.Click\n\
End If\n\
End Sub\n\
\n\
</script>\n\
</head>\n\
<body style = 'background-color:#EEEEEE' onkeypress = 'vbs:Default_Buttons' align = 'top'>\n\
<table width = '100%' height = '80%' align = 'center' border = '0'>\n\
<tr border = '0'>\n\
<td align = 'left' valign = 'middle' style='Font-Family:Arial'>\n");

                wcscat(lDialogString, aMessage ? aMessage : L"");

                wcscat(lDialogString, L"\n\
</td>\n\
<td align = 'right' valign = 'middle' style = 'margin-top: 0em'>\n\
<table  align = 'right' style = 'margin-right: 0em;'>\n\
<tr align = 'right' style = 'margin-top: 5em;'>\n\
<input type = 'button' value = 'OK' name = 'btn_OK' onClick = 'vbs:Run_ProgramOK' style = 'width: 5em; margin-top: 2em;'><br>\n\
<input type = 'button' value = 'Cancel' name = 'btn_Cancel' onClick = 'vbs:Run_ProgramCancel' style = 'width: 5em;'><br><br>\n\
</tr>\n\
</table>\n\
</td>\n\
</tr>\n\
</table>\n");

                wcscat(lDialogString, L"<table width = '100%' height = '100%' align = 'center' border = '0'>\n\
<tr>\n\
<td align = 'left' valign = 'top'>\n\
<input type = 'password' id = 'txt_input'\n\
name = 'txt_input' value = '' style = 'float:left;width:100%' ><BR>\n\
</td>\n\
</tr>\n\
</table>\n\
</body>\n\
</html>\n\
"               ) ;
        }
        fputws(lDialogString, lIn);
        fclose(lIn);

        if (aDefaultInput)
        {
                swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                        lDialogStringLen,
#endif
                        L"%ls\\tinyfd.txt",_wgetenv(L"TEMP"));

#ifdef TINYFD_NOCCSUNICODE
				lFile = _wfopen(lDialogString, L"w");
				fputc(0xFF, lFile);
				fputc(0xFE, lFile);
#else
				lFile = _wfopen(lDialogString, L"wt, ccs=UNICODE"); /*or ccs=UTF-16LE*/
#endif
				fclose(lFile);

                wcscpy(lDialogString, L"cmd.exe /c cscript.exe //U //Nologo ");
                wcscat(lDialogString, L"\"%TEMP%\\tinyfd.vbs\" ");
                wcscat(lDialogString, L">> \"%TEMP%\\tinyfd.txt\"");
        }
        else
        {
                wcscpy(lDialogString,
                        L"cmd.exe /c mshta.exe \"%TEMP%\\tinyfd.hta\"");
        }

        /* wprintf ( "lDialogString: %ls\n" , lDialogString ) ; */

        hiddenConsoleW(lDialogString, aTitle, 1);

        swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                lDialogStringLen,
#endif
				L"%ls\\tinyfd.txt", _wgetenv(L"TEMP"));
		/* wprintf(L"lDialogString: %ls\n", lDialogString); */
#ifdef TINYFD_NOCCSUNICODE
		if (!(lIn = _wfopen(lDialogString, L"r")))
#else
		if (!(lIn = _wfopen(lDialogString, L"rt, ccs=UNICODE"))) /*or ccs=UTF-16LE*/
#endif
		{
                _wremove(lDialogString);
                free(lDialogString);
                return NULL;
        }

		memset(lBuff, 0, MAX_PATH_OR_CMD * sizeof(wchar_t) );

#ifdef TINYFD_NOCCSUNICODE
		fgets((char *)lBuff, 2*MAX_PATH_OR_CMD, lIn);
#else
		fgetws(lBuff, MAX_PATH_OR_CMD, lIn);
#endif
		fclose(lIn);
		wipefileW(lDialogString);
		_wremove(lDialogString);

		if (aDefaultInput)
		{
			swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                        lDialogStringLen,
#endif
                        L"%ls\\tinyfd.vbs", _wgetenv(L"TEMP"));
        }
        else
        {
                swprintf(lDialogString,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && !(defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))
                        lDialogStringLen,
#endif
                        L"%ls\\tinyfd.hta", _wgetenv(L"TEMP"));
        }
        _wremove(lDialogString);
        free(lDialogString);
        /* wprintf( L"lBuff: %ls\n" , lBuff ) ; */
#ifdef TINYFD_NOCCSUNICODE
		lResult = !wcsncmp(lBuff+1, L"1", 1);
#else
		lResult = !wcsncmp(lBuff, L"1", 1);
#endif

        /* printf( "lResult: %d \n" , lResult ) ; */
        if (!lResult)
        {
            return NULL ;
        }

        /* wprintf( "lBuff+1: %ls\n" , lBuff+1 ) ; */

#ifdef TINYFD_NOCCSUNICODE
		if (aDefaultInput)
		{
			lDialogStringLen = wcslen(lBuff) ;
			lBuff[lDialogStringLen - 1] = L'\0';
			lBuff[lDialogStringLen - 2] = L'\0';
		}
		return lBuff + 2;
#else
		if (aDefaultInput) lBuff[wcslen(lBuff) - 1] = L'\0';
		return lBuff + 1;
#endif
}


wchar_t * tinyfd_saveFileDialogW(
        wchar_t const * aTitle, /* NULL or "" */
        wchar_t const * aDefaultPathAndFile, /* NULL or "" */
        int aNumOfFilterPatterns, /* 0 */
        wchar_t const * const * aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
        wchar_t const * aSingleFilterDescription) /* NULL or "image files" */
{
        static wchar_t lBuff[MAX_PATH_OR_CMD];
        wchar_t lDirname[MAX_PATH_OR_CMD];
        wchar_t lDialogString[MAX_PATH_OR_CMD];
        wchar_t lFilterPatterns[MAX_PATH_OR_CMD] = L"";
        wchar_t * p;
        wchar_t * lRetval;
		wchar_t const * ldefExt = NULL;
		int i;
        HRESULT lHResult;
        OPENFILENAMEW ofn = {0};

        if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return (wchar_t *)1; }

		if (quoteDetectedW(aTitle)) return tinyfd_saveFileDialogW(L"INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (quoteDetectedW(aDefaultPathAndFile)) return tinyfd_saveFileDialogW(aTitle, L"INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (quoteDetectedW(aSingleFilterDescription)) return tinyfd_saveFileDialogW(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, L"INVALID FILTER_DESCRIPTION WITH QUOTES");
		for (i = 0; i < aNumOfFilterPatterns; i++)
		{
			if (quoteDetectedW(aFilterPatterns[i])) return tinyfd_saveFileDialogW(L"INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL);
		}

        lHResult = CoInitializeEx(NULL, 0);

        getPathWithoutFinalSlashW(lDirname, aDefaultPathAndFile);
        getLastNameW(lBuff, aDefaultPathAndFile);

        if (aNumOfFilterPatterns > 0)
        {
			ldefExt = aFilterPatterns[0];

                if (aSingleFilterDescription && wcslen(aSingleFilterDescription))
                {
                        wcscpy(lFilterPatterns, aSingleFilterDescription);
                        wcscat(lFilterPatterns, L"\n");
                }
                wcscat(lFilterPatterns, aFilterPatterns[0]);
                for (i = 1; i < aNumOfFilterPatterns; i++)
                {
                        wcscat(lFilterPatterns, L";");
                        wcscat(lFilterPatterns, aFilterPatterns[i]);
                }
                wcscat(lFilterPatterns, L"\n");
                if (!(aSingleFilterDescription && wcslen(aSingleFilterDescription)))
                {
                        wcscpy(lDialogString, lFilterPatterns);
                        wcscat(lFilterPatterns, lDialogString);
                }
                wcscat(lFilterPatterns, L"All Files\n*.*\n");
                p = lFilterPatterns;
                while ((p = wcschr(p, L'\n')) != NULL)
                {
                        *p = L'\0';
                        p++;
                }
        }

        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = GetForegroundWindow();
        ofn.hInstance = 0;
        ofn.lpstrFilter = wcslen(lFilterPatterns) ? lFilterPatterns : NULL;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = lBuff;

        ofn.nMaxFile = MAX_PATH_OR_CMD;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = MAX_PATH_OR_CMD/2;
        ofn.lpstrInitialDir = wcslen(lDirname) ? lDirname : NULL;
        ofn.lpstrTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST ;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
		ofn.lpstrDefExt = ldefExt;
        ofn.lCustData = 0L;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (GetSaveFileNameW(&ofn) == 0)
        {
                lRetval = NULL;
        }
        else
        {
                lRetval = lBuff;
        }

        if (lHResult == S_OK || lHResult == S_FALSE)
        {
                CoUninitialize();
        }
        return lRetval;
}


wchar_t * tinyfd_openFileDialogW(
        wchar_t const * aTitle, /* NULL or "" */
		wchar_t const * aDefaultPathAndFile, /* NULL or "" */
        int aNumOfFilterPatterns, /* 0 */
		wchar_t const * const * aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
		wchar_t const * aSingleFilterDescription, /* NULL or "image files" */
        int aAllowMultipleSelects) /* 0 or 1 ; -1 to free allocated memory and return */
{
        size_t lLengths[MAX_MULTIPLE_FILES];
        wchar_t lDirname[MAX_PATH_OR_CMD];
        wchar_t lFilterPatterns[MAX_PATH_OR_CMD] = L"";
        wchar_t lDialogString[MAX_PATH_OR_CMD];
        wchar_t * lPointers[MAX_MULTIPLE_FILES+1];
        wchar_t * p;
        int i, j;
		size_t lBuffLen;
		DWORD lFullBuffLen;
        HRESULT lHResult;
        OPENFILENAMEW ofn = { 0 };
		static wchar_t * lBuff = NULL;

		free(lBuff);
		lBuff = NULL;
		if (aAllowMultipleSelects < 0) return (wchar_t *)0;

		if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return (wchar_t *)1; }

		if (quoteDetectedW(aTitle)) return tinyfd_openFileDialogW(L"INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
		if (quoteDetectedW(aDefaultPathAndFile)) return tinyfd_openFileDialogW(aTitle, L"INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
		if (quoteDetectedW(aSingleFilterDescription)) return tinyfd_openFileDialogW(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, L"INVALID FILTER_DESCRIPTION WITH QUOTES", aAllowMultipleSelects);
		for (i = 0; i < aNumOfFilterPatterns; i++)
		{
			if (quoteDetectedW(aFilterPatterns[i])) return tinyfd_openFileDialogW(L"INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL, aAllowMultipleSelects);
		}

		if (aAllowMultipleSelects)
		{
			lFullBuffLen = MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD + 1;
			lBuff = (wchar_t*)(malloc(lFullBuffLen * sizeof(wchar_t)));
			if (!lBuff)
			{
				lFullBuffLen = LOW_MULTIPLE_FILES * MAX_PATH_OR_CMD + 1;
				lBuff = (wchar_t*)( malloc( lFullBuffLen * sizeof(wchar_t)));
			}
		}
		else
		{
			lFullBuffLen = MAX_PATH_OR_CMD + 1;
			lBuff = (wchar_t*)(malloc(lFullBuffLen * sizeof(wchar_t)));
		}
		if (!lBuff) return NULL;

        lHResult = CoInitializeEx(NULL, 0);

        getPathWithoutFinalSlashW(lDirname, aDefaultPathAndFile);
        getLastNameW(lBuff, aDefaultPathAndFile);

        if (aNumOfFilterPatterns > 0)
        {
                if (aSingleFilterDescription && wcslen(aSingleFilterDescription))
                {
                        wcscpy(lFilterPatterns, aSingleFilterDescription);
                        wcscat(lFilterPatterns, L"\n");
                }
                wcscat(lFilterPatterns, aFilterPatterns[0]);
                for (i = 1; i < aNumOfFilterPatterns; i++)
                {
                        wcscat(lFilterPatterns, L";");
                        wcscat(lFilterPatterns, aFilterPatterns[i]);
                }
                wcscat(lFilterPatterns, L"\n");
                if (!(aSingleFilterDescription && wcslen(aSingleFilterDescription)))
                {
                        wcscpy(lDialogString, lFilterPatterns);
                        wcscat(lFilterPatterns, lDialogString);
                }
                wcscat(lFilterPatterns, L"All Files\n*.*\n");
                p = lFilterPatterns;
                while ((p = wcschr(p, L'\n')) != NULL)
                {
                        *p = L'\0';
                        p++;
                }
        }

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = GetForegroundWindow();
        ofn.hInstance = 0;
        ofn.lpstrFilter = wcslen(lFilterPatterns) ? lFilterPatterns : NULL;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = lBuff;
		ofn.nMaxFile = lFullBuffLen;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = MAX_PATH_OR_CMD / 2;
        ofn.lpstrInitialDir = wcslen(lDirname) ? lDirname : NULL;
        ofn.lpstrTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
        ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = NULL;
        ofn.lCustData = 0L;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (aAllowMultipleSelects)
        {
                ofn.Flags |= OFN_ALLOWMULTISELECT;
        }

        if (GetOpenFileNameW(&ofn) == 0)
        {
			free(lBuff);
			lBuff = NULL;
        }
        else
        {
                lBuffLen = wcslen(lBuff);
                lPointers[0] = lBuff + lBuffLen + 1;
                if (aAllowMultipleSelects && (lPointers[0][0] != L'\0'))
				{
                        i = 0;
                        do
                        {
                                lLengths[i] = wcslen(lPointers[i]);
                                lPointers[i + 1] = lPointers[i] + lLengths[i] + 1;
                                i++;
						} while (lPointers[i][0] != L'\0' && i < MAX_MULTIPLE_FILES );
						if (i > MAX_MULTIPLE_FILES)
						{
							free(lBuff);
							lBuff = NULL;
						}
						else
						{
							i--;
							p = lBuff + lFullBuffLen - 1;
							*p = L'\0';
							for (j = i; j >= 0; j--)
							{
								p -= lLengths[j];
								memmove(p, lPointers[j], lLengths[j] * sizeof(wchar_t));
								p--;
								*p = L'\\';
								p -= lBuffLen;
								memmove(p, lBuff, lBuffLen*sizeof(wchar_t));
								p--;
								*p = L'|';
							}
							p++;
							wcscpy(lBuff, p);
							lBuffLen = wcslen(lBuff);
						}
				}
				if (lBuff) lBuff = (wchar_t*)(realloc(lBuff, (lBuffLen + 1) * sizeof(wchar_t)));
        }

        if (lHResult == S_OK || lHResult == S_FALSE)
        {
                CoUninitialize();
        }
		return lBuff;
}


BOOL CALLBACK BrowseCallbackProcW_enum(HWND hWndChild, LPARAM lParam)
{
    wchar_t buf[255];
    GetClassNameW(hWndChild, buf, sizeof(buf));
    if (wcscmp(buf, L"SysTreeView32") == 0) {
        HTREEITEM hNode = TreeView_GetSelection(hWndChild);
        TreeView_EnsureVisible(hWndChild, hNode);
        return FALSE;
    }
    return TRUE;
}


static int __stdcall BrowseCallbackProcW(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    switch (uMsg) {
        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
            break;
        case BFFM_SELCHANGED:
            EnumChildWindows(hwnd, BrowseCallbackProcW_enum, 0);
    }
    return 0;
}

wchar_t * tinyfd_selectFolderDialogW(
        wchar_t const * aTitle, /* NULL or "" */
        wchar_t const * aDefaultPath) /* NULL or "" */
{
        static wchar_t lBuff[MAX_PATH_OR_CMD];
		wchar_t * lRetval;

        BROWSEINFOW bInfo;
        LPITEMIDLIST lpItem;
        HRESULT lHResult;

        if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return (wchar_t *)1; }

		if (quoteDetectedW(aTitle)) return tinyfd_selectFolderDialogW(L"INVALID TITLE WITH QUOTES", aDefaultPath);
		if (quoteDetectedW(aDefaultPath)) return tinyfd_selectFolderDialogW(aTitle, L"INVALID DEFAULT_PATH WITH QUOTES");

        lHResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

        bInfo.hwndOwner = GetForegroundWindow();
        bInfo.pidlRoot = NULL;
        bInfo.pszDisplayName = lBuff;
        bInfo.lpszTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
        if (lHResult == S_OK || lHResult == S_FALSE)
        {
                bInfo.ulFlags = BIF_USENEWUI;
        }
        bInfo.lpfn = BrowseCallbackProcW;
        bInfo.lParam = (LPARAM)aDefaultPath;
        bInfo.iImage = -1;

        lpItem = SHBrowseForFolderW(&bInfo);
        if (!lpItem)
		{
			lRetval = NULL;
		}
		else
        {
                SHGetPathFromIDListW(lpItem, lBuff);
				lRetval = lBuff ;
        }

        if (lHResult == S_OK || lHResult == S_FALSE)
        {
                CoUninitialize();
        }
		return lRetval;
}


wchar_t * tinyfd_colorChooserW(
        wchar_t const * aTitle, /* NULL or "" */
        wchar_t const * aDefaultHexRGB, /* NULL or "#FF0000"*/
        unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
        unsigned char aoResultRGB[3]) /* { 0 , 0 , 0 } */
{
        static wchar_t lResultHexRGB[8];
        CHOOSECOLORW cc;
        COLORREF crCustColors[16];
        unsigned char lDefaultRGB[3];
        int lRet;

        HRESULT lHResult;

        if (aTitle&&!wcscmp(aTitle, L"tinyfd_query")){ strcpy(tinyfd_response, "windows_wchar"); return (wchar_t *)1; }

		if (quoteDetectedW(aTitle)) return tinyfd_colorChooserW(L"INVALID TITLE WITH QUOTES", aDefaultHexRGB, aDefaultRGB, aoResultRGB);
		if (quoteDetectedW(aDefaultHexRGB)) return tinyfd_colorChooserW(aTitle, L"INVALID DEFAULT_HEX_RGB WITH QUOTES", aDefaultRGB, aoResultRGB);

        lHResult = CoInitializeEx(NULL, 0);

        if ( aDefaultHexRGB )
        {
                Hex2RGBW(aDefaultHexRGB, lDefaultRGB);
        }
        else
        {
                lDefaultRGB[0] = aDefaultRGB[0];
                lDefaultRGB[1] = aDefaultRGB[1];
                lDefaultRGB[2] = aDefaultRGB[2];
        }

        /* we can't use aTitle */
        cc.lStructSize = sizeof(CHOOSECOLOR);
        cc.hwndOwner = GetForegroundWindow();
        cc.hInstance = NULL;
        cc.rgbResult = RGB(lDefaultRGB[0], lDefaultRGB[1], lDefaultRGB[2]);
        cc.lpCustColors = crCustColors;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR ;
        cc.lCustData = 0;
        cc.lpfnHook = NULL;
        cc.lpTemplateName = NULL;

        lRet = ChooseColorW(&cc);

        if (!lRet)
        {
                return NULL;
        }

        aoResultRGB[0] = GetRValue(cc.rgbResult);
        aoResultRGB[1] = GetGValue(cc.rgbResult);
        aoResultRGB[2] = GetBValue(cc.rgbResult);

        RGB2HexW(aoResultRGB, lResultHexRGB);

        if (lHResult == S_OK || lHResult == S_FALSE)
        {
                CoUninitialize();
        }

        return lResultHexRGB;
}


static int messageBoxWinGui(
	char const * aTitle, /* NULL or "" */
	char const * aMessage, /* NULL or ""  may contain \n and \t */
	char const * aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
	char const * aIconType, /* "info" "warning" "error" "question" */
	int aDefaultButton) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
	int lIntRetVal;
	wchar_t lTitle[128] = L"";
	wchar_t * lMessage = NULL;
	wchar_t lDialogType[16] = L"";
	wchar_t lIconType[16] = L"";
	wchar_t * lTmpWChar;

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aMessage)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aMessage);
		else lTmpWChar = tinyfd_mbcsTo16(aMessage);
		lMessage = (wchar_t *) malloc((wcslen(lTmpWChar) + 1)* sizeof(wchar_t));
		if (lMessage) wcscpy(lMessage, lTmpWChar);
	}
	if (aDialogType)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDialogType);
		else lTmpWChar = tinyfd_mbcsTo16(aDialogType);
		wcscpy(lDialogType, lTmpWChar);
	}
	if (aIconType)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aIconType);
		else lTmpWChar = tinyfd_mbcsTo16(aIconType);
		wcscpy(lIconType, lTmpWChar);
	}

	lIntRetVal = tinyfd_messageBoxW(lTitle, lMessage, lDialogType, lIconType, aDefaultButton);

	free(lMessage);

	return lIntRetVal;
}


static int notifyWinGui(
	char const * aTitle, /* NULL or "" */
	char const * aMessage, /* NULL or "" may NOT contain \n nor \t */
	char const * aIconType)
{
	wchar_t lTitle[128] = L"";
	wchar_t * lMessage = NULL;
	wchar_t lIconType[16] = L"";
	wchar_t * lTmpWChar;

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aMessage)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aMessage);
		else lTmpWChar = tinyfd_mbcsTo16(aMessage);
		lMessage = (wchar_t *) malloc((wcslen(lTmpWChar) + 1)* sizeof(wchar_t));
        if (lMessage) wcscpy(lMessage, lTmpWChar);
	}
	if (aIconType)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aIconType);
		else lTmpWChar = tinyfd_mbcsTo16(aIconType);
		wcscpy(lIconType, lTmpWChar);
	}

	tinyfd_notifyPopupW(lTitle, lMessage, lIconType);

	free(lMessage);

	return 1;
}


static int inputBoxWinGui(
	char * aoBuff,
	char const * aTitle, /* NULL or "" */
	char const * aMessage, /* NULL or "" may NOT contain \n nor \t */
	char const * aDefaultInput) /* "" , if NULL it's a passwordBox */
{
	wchar_t lTitle[128] = L"";
	wchar_t * lMessage = NULL;
	wchar_t lDefaultInput[MAX_PATH_OR_CMD] = L"";
	wchar_t * lTmpWChar;
	char * lTmpChar;

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aMessage)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aMessage);
		else lTmpWChar = tinyfd_mbcsTo16(aMessage);
		lMessage = (wchar_t *) malloc((wcslen(lTmpWChar) + 1)* sizeof(wchar_t));
      if (lMessage) wcscpy(lMessage, lTmpWChar);
	}
	if (aDefaultInput)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDefaultInput);
		else lTmpWChar = tinyfd_mbcsTo16(aDefaultInput);
		wcscpy(lDefaultInput, lTmpWChar);
        lTmpWChar = tinyfd_inputBoxW(lTitle, lMessage, lDefaultInput);
	}
    else lTmpWChar = tinyfd_inputBoxW(lTitle, lMessage, NULL);

	free(lMessage);

	if (!lTmpWChar)
	{
		aoBuff[0] = '\0';
		return 0;
	}

	if (tinyfd_winUtf8) lTmpChar = tinyfd_utf16to8(lTmpWChar);
	else lTmpChar = tinyfd_utf16toMbcs(lTmpWChar);

	strcpy(aoBuff, lTmpChar);

	return 1;
}


static char * saveFileDialogWinGui(
	char * aoBuff,
	char const * aTitle, /* NULL or "" */
	char const * aDefaultPathAndFile, /* NULL or "" */
	int aNumOfFilterPatterns, /* 0 */
	char const * const * aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
	char const * aSingleFilterDescription) /* NULL or "image files" */
{
	wchar_t lTitle[128] = L"";
	wchar_t lDefaultPathAndFile[MAX_PATH_OR_CMD] = L"";
	wchar_t lSingleFilterDescription[128] = L"";
	wchar_t * * lFilterPatterns;
	wchar_t * lTmpWChar;
	char * lTmpChar;
	int i;

	lFilterPatterns = (wchar_t **)malloc(aNumOfFilterPatterns*sizeof(wchar_t *));
	for (i = 0; i < aNumOfFilterPatterns; i++)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aFilterPatterns[i]);
		else lTmpWChar = tinyfd_mbcsTo16(aFilterPatterns[i]);
		lFilterPatterns[i] = (wchar_t *)malloc((wcslen(lTmpWChar) + 1) * sizeof(wchar_t *));
      if (lFilterPatterns[i]) wcscpy(lFilterPatterns[i], lTmpWChar);
	}

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aDefaultPathAndFile)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDefaultPathAndFile);
		else lTmpWChar = tinyfd_mbcsTo16(aDefaultPathAndFile);
		wcscpy(lDefaultPathAndFile, lTmpWChar);
	}
	if (aSingleFilterDescription)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aSingleFilterDescription);
		else lTmpWChar = tinyfd_mbcsTo16(aSingleFilterDescription);
		wcscpy(lSingleFilterDescription, lTmpWChar);
	}

	lTmpWChar = tinyfd_saveFileDialogW(
		lTitle,
		lDefaultPathAndFile,
		aNumOfFilterPatterns,
		(wchar_t const**) lFilterPatterns, /*stupid cast for gcc*/
		lSingleFilterDescription);

	for (i = 0; i < aNumOfFilterPatterns; i++)
	{
		free(lFilterPatterns[i]);
	}
	free(lFilterPatterns);

	if (!lTmpWChar)
	{
		return NULL;
	}

	if (tinyfd_winUtf8) lTmpChar = tinyfd_utf16to8(lTmpWChar);
	else lTmpChar = tinyfd_utf16toMbcs(lTmpWChar);
	strcpy(aoBuff, lTmpChar);
	if (tinyfd_winUtf8) (void)tinyfd_utf16to8(NULL);
	else (void)tinyfd_utf16toMbcs(NULL);

	return aoBuff;
}


static char * openFileDialogWinGui(
	char const * aTitle, /*  NULL or "" */
	char const * aDefaultPathAndFile, /*  NULL or "" */
	int aNumOfFilterPatterns, /* 0 */
	char const * const * aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
	char const * aSingleFilterDescription, /* NULL or "image files" */
	int aAllowMultipleSelects) /* 0 or 1 */
{
	wchar_t lTitle[128] = L"";
	wchar_t lDefaultPathAndFile[MAX_PATH_OR_CMD] = L"";
	wchar_t lSingleFilterDescription[128] = L"";
	wchar_t * * lFilterPatterns;
	wchar_t * lTmpWChar;
	char * lTmpChar;
	int i;

	lFilterPatterns = (wchar_t * *)malloc(aNumOfFilterPatterns*sizeof(wchar_t *));
	for (i = 0; i < aNumOfFilterPatterns; i++)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aFilterPatterns[i]);
		else lTmpWChar = tinyfd_mbcsTo16(aFilterPatterns[i]);
		lFilterPatterns[i] = (wchar_t *)malloc((wcslen(lTmpWChar) + 1)*sizeof(wchar_t *));
      if (lFilterPatterns[i]) wcscpy(lFilterPatterns[i], lTmpWChar);
	}

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aDefaultPathAndFile)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDefaultPathAndFile);
		else lTmpWChar = tinyfd_mbcsTo16(aDefaultPathAndFile);
		wcscpy(lDefaultPathAndFile, lTmpWChar);
	}
	if (aSingleFilterDescription)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aSingleFilterDescription);
		else lTmpWChar = tinyfd_mbcsTo16(aSingleFilterDescription);
		wcscpy(lSingleFilterDescription, lTmpWChar);
	}

	lTmpWChar = tinyfd_openFileDialogW(
		lTitle,
		lDefaultPathAndFile,
		aNumOfFilterPatterns,
		(wchar_t const**) lFilterPatterns, /*stupid cast for gcc*/
		lSingleFilterDescription,
		aAllowMultipleSelects);

	for (i = 0; i < aNumOfFilterPatterns; i++)
	{
		free(lFilterPatterns[i]);
	}
	free(lFilterPatterns);

	if (!lTmpWChar) return NULL;

	if (tinyfd_winUtf8) lTmpChar = tinyfd_utf16to8(lTmpWChar);
	else lTmpChar = tinyfd_utf16toMbcs(lTmpWChar);
	(void)tinyfd_openFileDialogW(NULL, NULL, 0, NULL, NULL, -1);

	return lTmpChar;
}


static char * selectFolderDialogWinGui(
	char * aoBuff,
	char const * aTitle, /*  NULL or "" */
	char const * aDefaultPath) /* NULL or "" */
{
	wchar_t lTitle[128] = L"";
	wchar_t lDefaultPath[MAX_PATH_OR_CMD] = L"";
	wchar_t * lTmpWChar;
	char * lTmpChar;

	if (aTitle)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
		else lTmpWChar = tinyfd_mbcsTo16(aTitle);
		wcscpy(lTitle, lTmpWChar);
	}
	if (aDefaultPath)
	{
		if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDefaultPath);
		else lTmpWChar = tinyfd_mbcsTo16(aDefaultPath);
		wcscpy(lDefaultPath, lTmpWChar);
	}

	lTmpWChar = tinyfd_selectFolderDialogW(
		lTitle,
		lDefaultPath);

	if (!lTmpWChar)
	{
		return NULL;
	}

	if (tinyfd_winUtf8) lTmpChar = tinyfd_utf16to8(lTmpWChar);
	else lTmpChar = tinyfd_utf16toMbcs(lTmpWChar);
	strcpy(aoBuff, lTmpChar);

	return aoBuff;
}


static char * colorChooserWinGui(
        char const * aTitle, /* NULL or "" */
        char const * aDefaultHexRGB, /* NULL or "#FF0000"*/
        unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
        unsigned char aoResultRGB[3]) /* { 0 , 0 , 0 } */
{
        static char lResultHexRGB[8];

        wchar_t lTitle[128];
        wchar_t lDefaultHexRGB[16];
        wchar_t * lTmpWChar;
        char * lTmpChar;

		if (aTitle)
		{
			if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aTitle);
			else lTmpWChar = tinyfd_mbcsTo16(aTitle);
			wcscpy(lTitle, lTmpWChar);
		}
		if (aDefaultHexRGB)
		{
			if (tinyfd_winUtf8) lTmpWChar = tinyfd_utf8to16(aDefaultHexRGB);
			else lTmpWChar = tinyfd_mbcsTo16(aDefaultHexRGB);
			wcscpy(lDefaultHexRGB, lTmpWChar);
		}

        lTmpWChar = tinyfd_colorChooserW(
                lTitle,
                lDefaultHexRGB,
                aDefaultRGB,
                aoResultRGB );

        if (!lTmpWChar)
        {
                return NULL;
        }

		if (tinyfd_winUtf8) lTmpChar = tinyfd_utf16to8(lTmpWChar);
		else lTmpChar = tinyfd_utf16toMbcs(lTmpWChar);
		strcpy(lResultHexRGB, lTmpChar);

        return lResultHexRGB;
}


static int dialogPresent(void)
{
        static int lDialogPresent = -1 ;
        char lBuff[MAX_PATH_OR_CMD] ;
        FILE * lIn ;
        char const * lString = "dialog.exe";
		if (!tinyfd_allowCursesDialogs) return 0;
		if (lDialogPresent < 0)
        {
                if (!(lIn = _popen("where dialog.exe","r")))
                {
                        lDialogPresent = 0 ;
                        return 0 ;
                }
                while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
                {}
                _pclose( lIn ) ;
                if ( lBuff[strlen( lBuff ) -1] == '\n' )
                {
                        lBuff[strlen( lBuff ) -1] = '\0' ;
                }
                if ( strcmp(lBuff+strlen(lBuff)-strlen(lString),lString) )
                {
                        lDialogPresent = 0 ;
                }
                else
                {
                        lDialogPresent = 1 ;
                }
        }
		return lDialogPresent;
}


static int messageBoxWinConsole(
    char const * aTitle , /* NULL or "" */
    char const * aMessage , /* NULL or ""  may contain \n and \t */
    char const * aDialogType , /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const * aIconType , /* "info" "warning" "error" "question" */
    int aDefaultButton ) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
        char lDialogString[MAX_PATH_OR_CMD];
        char lDialogFile[MAX_PATH_OR_CMD];
        FILE * lIn;
        char lBuff[MAX_PATH_OR_CMD] = "";

		strcpy(lDialogString, "dialog ");
		if (aTitle && strlen(aTitle))
        {
                strcat(lDialogString, "--title \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
        }

        if ( aDialogType && ( !strcmp( "okcancel" , aDialogType )
                || !strcmp("yesno", aDialogType) || !strcmp("yesnocancel", aDialogType) ) )
        {
                strcat(lDialogString, "--backtitle \"") ;
                strcat(lDialogString, "tab: move focus") ;
                strcat(lDialogString, "\" ") ;
        }

        if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
        {
                if ( ! aDefaultButton )
                {
                        strcat( lDialogString , "--defaultno " ) ;
                }
                strcat( lDialogString ,
                                "--yes-label \"Ok\" --no-label \"Cancel\" --yesno " ) ;
        }
        else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
        {
                if ( ! aDefaultButton )
                {
                        strcat( lDialogString , "--defaultno " ) ;
                }
                strcat( lDialogString , "--yesno " ) ;
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
                if (!aDefaultButton)
                {
                        strcat(lDialogString, "--defaultno ");
                }
                strcat(lDialogString, "--menu ");
        }
        else
        {
                strcat( lDialogString , "--msgbox " ) ;
        }

        strcat( lDialogString , "\"" ) ;
        if ( aMessage && strlen(aMessage) )
        {
                tfd_replaceSubStr( aMessage , "\n" , "\\n" , lBuff ) ;
                strcat(lDialogString, lBuff) ;
                lBuff[0]='\0';
        }
        strcat(lDialogString, "\" ");

        if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
                strcat(lDialogString, "0 60 0 Yes \"\" No \"\"");
                strcat(lDialogString, "2>>");
        }
        else
        {
                strcat(lDialogString, "10 60");
                strcat(lDialogString, " && echo 1 > ");
        }

        strcpy(lDialogFile, getenv("TEMP"));
        strcat(lDialogFile, "\\tinyfd.txt");
        strcat(lDialogString, lDialogFile);

        /*if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;*/
        system( lDialogString ) ;

        if (!(lIn = fopen(lDialogFile, "r")))
        {
                remove(lDialogFile);
                return 0 ;
        }
		while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
        {}
        fclose(lIn);
        remove(lDialogFile);
    if ( lBuff[strlen( lBuff ) -1] == '\n' )
    {
        lBuff[strlen( lBuff ) -1] = '\0' ;
    }

        /* if (tinyfd_verbose) printf("lBuff: %s\n", lBuff); */
        if ( ! strlen(lBuff) )
        {
                return 0;
        }

        if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
                if (lBuff[0] == 'Y') return 1;
                else return 2;
        }

        return 1;
}


static int inputBoxWinConsole(
        char * aoBuff ,
        char const * aTitle , /* NULL or "" */
        char const * aMessage , /* NULL or "" may NOT contain \n nor \t */
        char const * aDefaultInput ) /* "" , if NULL it's a passwordBox */
{
        char lDialogString[MAX_PATH_OR_CMD];
        char lDialogFile[MAX_PATH_OR_CMD];
        FILE * lIn;
        int lResult;

        strcpy(lDialogFile, getenv("TEMP"));
        strcat(lDialogFile, "\\tinyfd.txt");
        strcpy(lDialogString , "echo|set /p=1 >" ) ;
        strcat(lDialogString, lDialogFile);
        strcat( lDialogString , " & " ) ;

        strcat( lDialogString , "dialog " ) ;
        if ( aTitle && strlen(aTitle) )
        {
                strcat(lDialogString, "--title \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
        }

        strcat(lDialogString, "--backtitle \"") ;
        strcat(lDialogString, "tab: move focus") ;
        if ( ! aDefaultInput )
        {
                strcat(lDialogString, " (sometimes nothing, no blink nor star, is shown in text field)") ;
        }

        strcat(lDialogString, "\" ") ;

        if ( ! aDefaultInput )
        {
                strcat( lDialogString , "--insecure --passwordbox" ) ;
        }
        else
        {
                strcat( lDialogString , "--inputbox" ) ;
        }
        strcat( lDialogString , " \"" ) ;
        if ( aMessage && strlen(aMessage) )
        {
                strcat(lDialogString, aMessage) ;
        }
        strcat(lDialogString,"\" 10 60 ") ;
        if ( aDefaultInput && strlen(aDefaultInput) )
        {
                strcat(lDialogString, "\"") ;
                strcat(lDialogString, aDefaultInput) ;
                strcat(lDialogString, "\" ") ;
        }

        strcat(lDialogString, "2>>");
        strcpy(lDialogFile, getenv("TEMP"));
        strcat(lDialogFile, "\\tinyfd.txt");
        strcat(lDialogString, lDialogFile);
        strcat(lDialogString, " || echo 0 > ");
        strcat(lDialogString, lDialogFile);

        /* printf( "lDialogString: %s\n" , lDialogString ) ; */
        system( lDialogString ) ;

        if (!(lIn = fopen(lDialogFile, "r")))
        {
                remove(lDialogFile);
				aoBuff[0] = '\0';
				return 0;
        }
        while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
        {}
        fclose(lIn);

        wipefile(lDialogFile);
        remove(lDialogFile);
    if ( aoBuff[strlen( aoBuff ) -1] == '\n' )
    {
        aoBuff[strlen( aoBuff ) -1] = '\0' ;
    }
        /* printf( "aoBuff: %s\n" , aoBuff ) ; */

        /* printf( "aoBuff: %s len: %lu \n" , aoBuff , strlen(aoBuff) ) ; */
    lResult =  strncmp( aoBuff , "1" , 1) ? 0 : 1 ;
        /* printf( "lResult: %d \n" , lResult ) ; */
	if ( ! lResult )
	{
		aoBuff[0] = '\0';
		return 0 ;
	}
	/* printf( "aoBuff+1: %s\n" , aoBuff+1 ) ; */
	strcpy(aoBuff, aoBuff+3);
	return 1;
}


static char * saveFileDialogWinConsole(
        char * aoBuff ,
        char const * aTitle , /* NULL or "" */
        char const * aDefaultPathAndFile ) /* NULL or "" */
{
        char lDialogString[MAX_PATH_OR_CMD];
        char lPathAndFile[MAX_PATH_OR_CMD] = "";
        FILE * lIn;

        strcpy( lDialogString , "dialog " ) ;
        if ( aTitle && strlen(aTitle) )
        {
                strcat(lDialogString, "--title \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
        }

        strcat(lDialogString, "--backtitle \"") ;
        strcat(lDialogString,
                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
        strcat(lDialogString, "\" ") ;

        strcat( lDialogString , "--fselect \"" ) ;
        if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
        {
                /* dialog.exe uses unix separators even on windows */
                strcpy(lPathAndFile, aDefaultPathAndFile);
                replaceChr( lPathAndFile , '\\' , '/' ) ;
        }

        /* dialog.exe needs at least one separator */
        if ( ! strchr(lPathAndFile, '/') )
        {
                strcat(lDialogString, "./") ;
        }
        strcat(lDialogString, lPathAndFile) ;
        strcat(lDialogString, "\" 0 60 2>");
        strcpy(lPathAndFile, getenv("TEMP"));
        strcat(lPathAndFile, "\\tinyfd.txt");
        strcat(lDialogString, lPathAndFile);

        /* printf( "lDialogString: %s\n" , lDialogString ) ; */
        system( lDialogString ) ;

        if (!(lIn = fopen(lPathAndFile, "r")))
        {
                remove(lPathAndFile);
                return NULL;
        }
        while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
        {}
        fclose(lIn);
        remove(lPathAndFile);
        replaceChr( aoBuff , '/' , '\\' ) ;
        /* printf( "aoBuff: %s\n" , aoBuff ) ; */
        getLastName(lDialogString,aoBuff);
        if ( ! strlen(lDialogString) )
        {
                return NULL;
        }
        return aoBuff;
}


static char * openFileDialogWinConsole(
        char const * aTitle , /*  NULL or "" */
        char const * aDefaultPathAndFile ) /*  NULL or "" */
{
        char lFilterPatterns[MAX_PATH_OR_CMD] = "";
        char lDialogString[MAX_PATH_OR_CMD] ;
        FILE * lIn;

		static char aoBuff[MAX_PATH_OR_CMD];

        strcpy( lDialogString , "dialog " ) ;
        if ( aTitle && strlen(aTitle) )
        {
                strcat(lDialogString, "--title \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
        }

        strcat(lDialogString, "--backtitle \"") ;
        strcat(lDialogString,
                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
        strcat(lDialogString, "\" ") ;

        strcat( lDialogString , "--fselect \"" ) ;
        if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
        {
                /* dialog.exe uses unix separators even on windows */
                strcpy(lFilterPatterns, aDefaultPathAndFile);
                replaceChr( lFilterPatterns , '\\' , '/' ) ;
        }

        /* dialog.exe needs at least one separator */
        if ( ! strchr(lFilterPatterns, '/') )
        {
                strcat(lDialogString, "./") ;
        }
        strcat(lDialogString, lFilterPatterns) ;
        strcat(lDialogString, "\" 0 60 2>");
        strcpy(lFilterPatterns, getenv("TEMP"));
        strcat(lFilterPatterns, "\\tinyfd.txt");
        strcat(lDialogString, lFilterPatterns);

        /* printf( "lDialogString: %s\n" , lDialogString ) ; */
        system( lDialogString ) ;

        if (!(lIn = fopen(lFilterPatterns, "r")))
        {
                remove(lFilterPatterns);
                return NULL;
        }
        while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
        {}
        fclose(lIn);
        remove(lFilterPatterns);
        replaceChr( aoBuff , '/' , '\\' ) ;
        /* printf( "aoBuff: %s\n" , aoBuff ) ; */
        return aoBuff;
}


static char * selectFolderDialogWinConsole(
        char * aoBuff ,
        char const * aTitle , /*  NULL or "" */
        char const * aDefaultPath ) /* NULL or "" */
{
        char lDialogString[MAX_PATH_OR_CMD] ;
        char lString[MAX_PATH_OR_CMD] ;
        FILE * lIn ;

        strcpy( lDialogString , "dialog " ) ;
        if ( aTitle && strlen(aTitle) )
        {
                strcat(lDialogString, "--title \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
        }

        strcat(lDialogString, "--backtitle \"") ;
        strcat(lDialogString,
                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
        strcat(lDialogString, "\" ") ;

        strcat( lDialogString , "--dselect \"" ) ;
        if ( aDefaultPath && strlen(aDefaultPath) )
        {
                /* dialog.exe uses unix separators even on windows */
                strcpy(lString, aDefaultPath) ;
                ensureFinalSlash(lString);
                replaceChr( lString , '\\' , '/' ) ;
                strcat(lDialogString, lString) ;
        }
        else
        {
                /* dialog.exe needs at least one separator */
                strcat(lDialogString, "./") ;
        }
        strcat(lDialogString, "\" 0 60 2>");
        strcpy(lString, getenv("TEMP"));
        strcat(lString, "\\tinyfd.txt");
        strcat(lDialogString, lString);

        /* printf( "lDialogString: %s\n" , lDialogString ) ; */
        system( lDialogString ) ;

        if (!(lIn = fopen(lString, "r")))
        {
                remove(lString);
                return NULL;
        }
        while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
        {}
        fclose(lIn);
        remove(lString);
        replaceChr( aoBuff , '/' , '\\' ) ;
        /* printf( "aoBuff: %s\n" , aoBuff ) ; */
        return aoBuff;
}

static void writeUtf8( char const * aUtf8String )
{
	unsigned long lNum;
	void * lConsoleHandle;
	wchar_t * lTmpWChar;

	lConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	lTmpWChar = tinyfd_utf8to16(aUtf8String);
	(void)WriteConsoleW(lConsoleHandle, lTmpWChar, (DWORD) wcslen(lTmpWChar), &lNum, NULL);
}


int tinyfd_messageBox(
	char const * aTitle, /* NULL or "" */
	char const * aMessage, /* NULL or ""  may contain \n and \t */
	char const * aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
	char const * aIconType, /* "info" "warning" "error" "question" */
	int aDefaultButton) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
	char lChar;
	UINT lOriginalCP = 0;
	UINT lOriginalOutputCP = 0;

	if (tfd_quoteDetected(aTitle)) return tinyfd_messageBox("INVALID TITLE WITH QUOTES", aMessage, aDialogType, aIconType, aDefaultButton);
	if (tfd_quoteDetected(aMessage)) return tinyfd_messageBox(aTitle, "INVALID MESSAGE WITH QUOTES", aDialogType, aIconType, aDefaultButton);

	if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent()))
		&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
	{
		if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "windows"); return 1; }
		return messageBoxWinGui(aTitle, aMessage, aDialogType, aIconType, aDefaultButton);
	}
	else if (dialogPresent())
	{
		if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "dialog"); return 0; }
		return messageBoxWinConsole(
			aTitle, aMessage, aDialogType, aIconType, aDefaultButton);
	}
	else
	{
		if (!tinyfd_winUtf8)
		{
			lOriginalCP = GetConsoleCP();
			lOriginalOutputCP = GetConsoleOutputCP();
			(void)SetConsoleCP(GetACP());
			(void)SetConsoleOutputCP(GetACP());
		}

		if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "basicinput"); return 0; }
		if (!gWarningDisplayed && !tinyfd_forceConsole)
		{
			gWarningDisplayed = 1;
			printf("\n\n%s\n", gTitle);
			printf("%s\n\n", tinyfd_needs);
		}

		if (aTitle && strlen(aTitle))
		{
			printf("\n");
			if (tinyfd_winUtf8) writeUtf8(aTitle);
			else printf("%s", aTitle);
			printf("\n\n");
		}
		if (aDialogType && !strcmp("yesno", aDialogType))
		{
			do
			{
				if (aMessage && strlen(aMessage))
				{
					if (tinyfd_winUtf8) writeUtf8(aMessage);
					else printf("%s", aMessage);
					printf("\n");
				}
				printf("y/n: ");
				lChar = (char)tolower(_getch());
				printf("\n\n");
			} while (lChar != 'y' && lChar != 'n');
			if (!tinyfd_winUtf8) { (void)SetConsoleCP(lOriginalCP); (void)SetConsoleOutputCP(lOriginalOutputCP); }
			return lChar == 'y' ? 1 : 0;
		}
		else if (aDialogType && !strcmp("okcancel", aDialogType))
		{
			do
			{
				if (aMessage && strlen(aMessage))
				{
					if (tinyfd_winUtf8) writeUtf8(aMessage);
					else printf("%s", aMessage);
					printf("\n");
				}
				printf("[O]kay/[C]ancel: ");
				lChar = (char)tolower(_getch());
				printf("\n\n");
			} while (lChar != 'o' && lChar != 'c');
			if (!tinyfd_winUtf8) { (void)SetConsoleCP(lOriginalCP); (void)SetConsoleOutputCP(lOriginalOutputCP); }
			return lChar == 'o' ? 1 : 0;
		}
		else if (aDialogType && !strcmp("yesnocancel", aDialogType))
		{
			do
			{
				if (aMessage && strlen(aMessage))
				{
					if (tinyfd_winUtf8) writeUtf8(aMessage);
					else printf("%s", aMessage);
					printf("\n");
				}
				printf("[Y]es/[N]o/[C]ancel: ");
				lChar = (char)tolower(_getch());
				printf("\n\n");
			} while (lChar != 'y' && lChar != 'n' && lChar != 'c');
			if (!tinyfd_winUtf8) { (void)SetConsoleCP(lOriginalCP); (void)SetConsoleOutputCP(lOriginalOutputCP); }
			return (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0;
		}
		else
		{
			if (aMessage && strlen(aMessage))
			{
				if (tinyfd_winUtf8) writeUtf8(aMessage);
				else printf("%s", aMessage);
				printf("\n\n");
			}
			printf("press enter to continue ");
			lChar = (char)_getch();
			printf("\n\n");
			if (!tinyfd_winUtf8) { (void)SetConsoleCP(lOriginalCP); (void)SetConsoleOutputCP(lOriginalOutputCP); }
			return 1;
		}
	}
}


/* return has only meaning for tinyfd_query */
int tinyfd_notifyPopup(
        char const * aTitle, /* NULL or "" */
        char const * aMessage , /* NULL or "" may contain \n \t */
        char const * aIconType ) /* "info" "warning" "error" */
{
	if (tfd_quoteDetected(aTitle)) return tinyfd_notifyPopup("INVALID TITLE WITH QUOTES", aMessage, aIconType);
	if (tfd_quoteDetected(aMessage)) return tinyfd_notifyPopup(aTitle, "INVALID MESSAGE WITH QUOTES", aIconType);

    if ( powershellPresent() && (!tinyfd_forceConsole || !(
            GetConsoleWindow() ||
            dialogPresent()))
			&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
    {
            if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return 1;}
            return notifyWinGui(aTitle, aMessage, aIconType);
    }
    else
	    return tinyfd_messageBox(aTitle, aMessage, "ok" , aIconType, 0);
}


/* returns NULL on cancel */
char * tinyfd_inputBox(
        char const * aTitle , /* NULL or "" */
        char const * aMessage , /* NULL or "" (\n and \t have no effect) */
        char const * aDefaultInput ) /* "" , if NULL it's a passwordBox */
{
	static char lBuff[MAX_PATH_OR_CMD] = "";
	char * lEOF;

	DWORD mode = 0;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	unsigned long lNum;
	void * lConsoleHandle;
	char * lTmpChar;
	wchar_t lBuffW[1024];

	UINT lOriginalCP = 0;
	UINT lOriginalOutputCP = 0;

	if (!aTitle && !aMessage && !aDefaultInput) return lBuff; /* now I can fill lBuff from outside */

	if (tfd_quoteDetected(aTitle)) return tinyfd_inputBox("INVALID TITLE WITH QUOTES", aMessage, aDefaultInput);
	if (tfd_quoteDetected(aMessage)) return tinyfd_inputBox(aTitle, "INVALID MESSAGE WITH QUOTES", aDefaultInput);
	if (tfd_quoteDetected(aDefaultInput)) return tinyfd_inputBox(aTitle, aMessage, "INVALID DEFAULT_INPUT WITH QUOTES");

    mode = 0;
    hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if ((!tinyfd_forceConsole || !(
            GetConsoleWindow() ||
            dialogPresent()))
			&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
    {
        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return (char *)1;}
        lBuff[0]='\0';
		if (inputBoxWinGui(lBuff, aTitle, aMessage, aDefaultInput)) return lBuff;
		else return NULL;
	}
    else if ( dialogPresent() )
    {
        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
        lBuff[0]='\0';
		if (inputBoxWinConsole(lBuff, aTitle, aMessage, aDefaultInput) ) return lBuff;
		else return NULL;
	}
    else
    {
      if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"basicinput");return (char *)0;}
      lBuff[0]='\0';
      if (!gWarningDisplayed && !tinyfd_forceConsole)
      {
          gWarningDisplayed = 1 ;
          printf("\n\n%s\n", gTitle);
          printf("%s\n\n", tinyfd_needs);
      }

	  if (!tinyfd_winUtf8)
	  {
		  lOriginalCP = GetConsoleCP();
		  lOriginalOutputCP = GetConsoleOutputCP();
		  (void)SetConsoleCP(GetACP());
		  (void)SetConsoleOutputCP(GetACP());
	  }

	  if (aTitle && strlen(aTitle))
      {
		printf("\n");
		if (tinyfd_winUtf8) writeUtf8(aTitle);
		else printf("%s", aTitle);
		printf("\n\n");
	  }
      if ( aMessage && strlen(aMessage) )
      {
		if (tinyfd_winUtf8) writeUtf8(aMessage);
		else printf("%s", aMessage);
		printf("\n");
      }
      printf("(ctrl-Z + enter to cancel): ");
      if ( ! aDefaultInput )
      {
		  (void) GetConsoleMode(hStdin, &mode);
		  (void) SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
      }
	  if (tinyfd_winUtf8)
	  {
		lConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
		(void) ReadConsoleW(lConsoleHandle, lBuffW, MAX_PATH_OR_CMD, &lNum, NULL);
		if (!aDefaultInput)
		{
			(void)SetConsoleMode(hStdin, mode);
			printf("\n");
		}
		lBuffW[lNum] = '\0';
		if (lBuffW[wcslen(lBuffW) - 1] == '\n') lBuffW[wcslen(lBuffW) - 1] = '\0';
		if (lBuffW[wcslen(lBuffW) - 1] == '\r') lBuffW[wcslen(lBuffW) - 1] = '\0';
		lTmpChar = tinyfd_utf16to8(lBuffW);
		if (lTmpChar)
		{
			strcpy(lBuff, lTmpChar);
			return lBuff;
		}
		else
			return NULL;
	  }
	  else
	  {
		  lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
		  if (!aDefaultInput)
		  {
			  (void)SetConsoleMode(hStdin, mode);
			  printf("\n");
		  }

		  if (!tinyfd_winUtf8)
		  {
			  (void)SetConsoleCP(lOriginalCP);
			  (void)SetConsoleOutputCP(lOriginalOutputCP);
		  }

		  if (!lEOF)
		  {
			  return NULL;
		  }
		  printf("\n");
		  if (strchr(lBuff, 27))
		  {
			  return NULL;
		  }
		  if (lBuff[strlen(lBuff) - 1] == '\n')
		  {
			  lBuff[strlen(lBuff) - 1] = '\0';
		  }
		  return lBuff;
		}
	}
}


char * tinyfd_saveFileDialog(
        char const * aTitle , /* NULL or "" */
        char const * aDefaultPathAndFile , /* NULL or "" */
        int aNumOfFilterPatterns , /* 0 */
        char const * const * aFilterPatterns , /* NULL or {"*.jpg","*.png"} */
        char const * aSingleFilterDescription ) /* NULL or "image files" */
{
        static char lBuff[MAX_PATH_OR_CMD] ;
        char lString[MAX_PATH_OR_CMD] ;
        char * p ;
		char * lPointerInputBox;
		int i;

        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_saveFileDialog("INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (tfd_quoteDetected(aDefaultPathAndFile)) return tinyfd_saveFileDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (tfd_quoteDetected(aSingleFilterDescription)) return tinyfd_saveFileDialog(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, "INVALID FILTER_DESCRIPTION WITH QUOTES");
		for (i = 0; i < aNumOfFilterPatterns; i++)
		{
			if (tfd_quoteDetected(aFilterPatterns[i])) return tinyfd_saveFileDialog("INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL);
		}


		if ( ( !tinyfd_forceConsole || !( GetConsoleWindow() || dialogPresent() ) )
			&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
        {
            if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return (char *)1;}
            p = saveFileDialogWinGui(lBuff,
				aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, (char const * const *)aFilterPatterns, aSingleFilterDescription);
        }
		else if (dialogPresent())
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "dialog"); return (char *)0; }
			p = saveFileDialogWinConsole(lBuff, aTitle, aDefaultPathAndFile);
		}
		else
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "basicinput"); return (char *)0; }
			strcpy(lBuff, "Save file in ");
			strcat(lBuff, getCurDir());

			lPointerInputBox = tinyfd_inputBox(NULL,NULL,NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
			if (lPointerInputBox) strcpy(lString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
			p = tinyfd_inputBox(aTitle, lBuff, "");
			if (p) strcpy(lBuff, p); else lBuff[0] = '\0';
			if (lPointerInputBox) strcpy(lPointerInputBox, lString); /* restore its previous content to tinyfd_inputBox */
			p = lBuff;
		}

        if ( ! p || ! strlen( p )  )
        {
                return NULL;
        }
        getPathWithoutFinalSlash( lString , p ) ;
        if ( strlen( lString ) && ! dirExists( lString ) )
        {
                return NULL ;
        }
        getLastName(lString,p);
        if ( ! filenameValid(lString) )
        {
                return NULL;
        }
        return p ;
}


/* in case of multiple files, the separator is | */
char * tinyfd_openFileDialog(
    char const * aTitle , /* NULL or "" */
	char const * aDefaultPathAndFile, /* NULL or "" */
    int aNumOfFilterPatterns , /* 0 */
	char const * const * aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
	char const * aSingleFilterDescription, /* NULL or "image files" */
    int aAllowMultipleSelects ) /* 0 or 1 */
{
    static char lBuff[MAX_PATH_OR_CMD];
    char lString[MAX_PATH_OR_CMD];
	char * p;
	char * lPointerInputBox;
	int i;

	if (tfd_quoteDetected(aTitle)) return tinyfd_openFileDialog("INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
	if (tfd_quoteDetected(aDefaultPathAndFile)) return tinyfd_openFileDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
	if (tfd_quoteDetected(aSingleFilterDescription)) return tinyfd_openFileDialog(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, "INVALID FILTER_DESCRIPTION WITH QUOTES", aAllowMultipleSelects);
	for (i = 0; i < aNumOfFilterPatterns; i++)
	{
		if (tfd_quoteDetected(aFilterPatterns[i])) return tinyfd_openFileDialog("INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL, aAllowMultipleSelects);
	}

    if ( ( !tinyfd_forceConsole || !( GetConsoleWindow() || dialogPresent() ) )
		&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return (char *)1;}
                p = openFileDialogWinGui( aTitle, aDefaultPathAndFile, aNumOfFilterPatterns,
					(char const * const *)aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
        }
		else if (dialogPresent())
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "dialog"); return (char *)0; }
			p = openFileDialogWinConsole(aTitle, aDefaultPathAndFile);
		}
		else
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "basicinput"); return (char *)0; }
			strcpy(lBuff, "Open file from ");
			strcat(lBuff, getCurDir());
			lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
			if (lPointerInputBox) strcpy(lString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
			p = tinyfd_inputBox(aTitle, lBuff, "");
			if (p) strcpy(lBuff, p); else lBuff[0] = '\0';
			if (lPointerInputBox) strcpy(lPointerInputBox, lString); /* restore its previous content to tinyfd_inputBox */
			p = lBuff;
		}

        if ( ! p || ! strlen( p )  )
        {
                return NULL;
        }
        if ( aAllowMultipleSelects && strchr(p, '|') )
        {
                p = ensureFilesExist( (char *) p , p ) ;
        }
        else if ( ! fileExists(p) )
        {
                return NULL ;
        }
        /* printf( "lBuff3: %s\n" , p ) ; */
        return p ;
}


char * tinyfd_selectFolderDialog(
        char const * aTitle , /* NULL or "" */
        char const * aDefaultPath ) /* NULL or "" */
{
	static char lBuff[MAX_PATH_OR_CMD];
	char * p;
	char * lPointerInputBox;
	char lString[MAX_PATH_OR_CMD];

	if (tfd_quoteDetected(aTitle)) return tinyfd_selectFolderDialog("INVALID TITLE WITH QUOTES", aDefaultPath);
	if (tfd_quoteDetected(aDefaultPath)) return tinyfd_selectFolderDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES");

    if ( ( !tinyfd_forceConsole || !( GetConsoleWindow() || dialogPresent() ) )
		&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return (char *)1;}
                p = selectFolderDialogWinGui(lBuff, aTitle, aDefaultPath);
        }
		else
		if (dialogPresent())
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "dialog"); return (char *)0; }
			p = selectFolderDialogWinConsole(lBuff, aTitle, aDefaultPath);
		}
		else
		{
			if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "basicinput"); return (char *)0; }
			strcpy(lBuff, "Select folder from ");
			strcat(lBuff, getCurDir());
			lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
			if (lPointerInputBox) strcpy(lString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
			p = tinyfd_inputBox(aTitle, lBuff, "");
			if (p) strcpy(lBuff, p); else lBuff[0] = '\0';
			if (lPointerInputBox) strcpy(lPointerInputBox, lString); /* restore its previous content to tinyfd_inputBox */
			p = lBuff;
		}

        if ( ! p || ! strlen( p ) || ! dirExists( p ) )
        {
                return NULL ;
        }
        return p ;
}


/* returns the hexcolor as a string "#FF0000" */
/* aoResultRGB also contains the result */
/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
/* aDefaultRGB and aoResultRGB can be the same array */
char * tinyfd_colorChooser(
        char const * aTitle, /* NULL or "" */
        char const * aDefaultHexRGB, /* NULL or "#FF0000"*/
        unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
        unsigned char aoResultRGB[3]) /* { 0 , 0 , 0 } */
{
	static char lDefaultHexRGB[16];
    int i;
    char * p ;
	char * lPointerInputBox;
	char lString[MAX_PATH_OR_CMD];

	lDefaultHexRGB[0] = '\0';

	if (tfd_quoteDetected(aTitle)) return tinyfd_colorChooser("INVALID TITLE WITH QUOTES", aDefaultHexRGB, aDefaultRGB, aoResultRGB);
	if (tfd_quoteDetected(aDefaultHexRGB)) return tinyfd_colorChooser(aTitle, "INVALID DEFAULT_HEX_RGB WITH QUOTES", aDefaultRGB, aoResultRGB);

    if ( (!tinyfd_forceConsole || !( GetConsoleWindow() || dialogPresent()) )
		&& (!getenv("SSH_CLIENT") || getenvDISPLAY()))
    {
		if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"windows");return (char *)1;}
		p = colorChooserWinGui(aTitle, aDefaultHexRGB, aDefaultRGB, aoResultRGB);
        if (p)
        {
            strcpy(lDefaultHexRGB, p);
            return lDefaultHexRGB;
        }
        return NULL;
    }
	else if (dialogPresent())
	{
		if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "dialog"); return (char *)0; }
	}
	else
	{
		if (aTitle&&!strcmp(aTitle, "tinyfd_query")){ strcpy(tinyfd_response, "basicinput"); return (char *)0; }
	}

	if (aDefaultHexRGB)
	{
		strncpy(lDefaultHexRGB, aDefaultHexRGB,7);
		lDefaultHexRGB[7]='\0';
	}
	else
	{
		RGB2Hex(aDefaultRGB, lDefaultHexRGB);
	}

	lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
	if (lPointerInputBox) strcpy(lString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
	p = tinyfd_inputBox(aTitle, "Enter hex rgb color (i.e. #f5ca20)", lDefaultHexRGB);

    if ( !p || (strlen(p) != 7) || (p[0] != '#') )
    {
            return NULL ;
    }
    for ( i = 1 ; i < 7 ; i ++ )
    {
            if ( ! isxdigit( (int) p[i] ) )
            {
                    return NULL ;
            }
    }
    Hex2RGB(p,aoResultRGB);

	strcpy(lDefaultHexRGB, p);

	if (lPointerInputBox) strcpy(lPointerInputBox, lString); /* restore its previous content to tinyfd_inputBox */

	return lDefaultHexRGB;
}


#else /* unix */

static char gPython2Name[16];
static char gPython3Name[16];
static char gPythonName[16];

int tfd_isDarwin(void)
{
        static int lsIsDarwin = -1 ;
        struct utsname lUtsname ;
        if ( lsIsDarwin < 0 )
        {
                lsIsDarwin = !uname(&lUtsname) && !strcmp(lUtsname.sysname,"Darwin") ;
        }
        return lsIsDarwin ;
}


static int dirExists( char const * aDirPath )
{
        DIR * lDir ;
        if ( ! aDirPath || ! strlen( aDirPath ) )
                return 0 ;
        lDir = opendir( aDirPath ) ;
        if ( ! lDir )
        {
            return 0 ;
        }
        closedir( lDir ) ;
        return 1 ;
}


static int detectPresence( char const * aExecutable )
{
   char lBuff[MAX_PATH_OR_CMD] ;
   char lTestedString[MAX_PATH_OR_CMD] = "which " ;
   FILE * lIn ;
#ifdef _GNU_SOURCE
   char* lAllocatedCharString;
   int lSubstringUndetected;
#endif

   strcat( lTestedString , aExecutable ) ;
   strcat( lTestedString, " 2>/dev/null ");
   lIn = popen( lTestedString , "r" ) ;
   if ( ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
    && ( ! strchr( lBuff , ':' ) ) && ( strncmp(lBuff, "no ", 3) ) )
   {   /* present */
      pclose( lIn ) ;

#ifdef _GNU_SOURCE /*to bypass this, just comment out "#define _GNU_SOURCE" at the top of the file*/
      if ( lBuff[strlen( lBuff ) -1] == '\n' ) lBuff[strlen( lBuff ) -1] = '\0' ;
      lAllocatedCharString = realpath(lBuff,NULL); /*same as canonicalize_file_name*/
      lSubstringUndetected = ! strstr(lAllocatedCharString, aExecutable);
      free(lAllocatedCharString);
      if (lSubstringUndetected)
      {
         if (tinyfd_verbose) printf("detectPresence %s %d\n", aExecutable, 0);
         return 0;
      }
#endif /*_GNU_SOURCE*/

      if (tinyfd_verbose) printf("detectPresence %s %d\n", aExecutable, 1);
      return 1 ;
   }
   else
   {
      pclose( lIn ) ;
      if (tinyfd_verbose) printf("detectPresence %s %d\n", aExecutable, 0);
      return 0 ;
   }
}


static char * getVersion( char const * aExecutable ) /*version must be first numeral*/
{
	static char lBuff[MAX_PATH_OR_CMD] ;
	char lTestedString[MAX_PATH_OR_CMD] ;
	FILE * lIn ;
	char * lTmp ;

    strcpy( lTestedString , aExecutable ) ;
    strcat( lTestedString , " --version" ) ;

    lIn = popen( lTestedString , "r" ) ;
        lTmp = fgets( lBuff , sizeof( lBuff ) , lIn ) ;
        pclose( lIn ) ;

	lTmp += strcspn(lTmp,"0123456789");
	/* printf("lTmp:%s\n", lTmp); */
	return lTmp ;
}


static int * getMajorMinorPatch( char const * aExecutable )
{
	static int lArray[3] ;
	char * lTmp ;

	lTmp = (char *) getVersion(aExecutable);
	lArray[0] = atoi( strtok(lTmp," ,.-") ) ;
	/* printf("lArray0 %d\n", lArray[0]); */
	lArray[1] = atoi( strtok(0," ,.-") ) ;
	/* printf("lArray1 %d\n", lArray[1]); */
	lArray[2] = atoi( strtok(0," ,.-") ) ;
	/* printf("lArray2 %d\n", lArray[2]); */

	if ( !lArray[0] && !lArray[1] && !lArray[2] ) return NULL;
	return lArray ;
}


static int tryCommand( char const * aCommand )
{
        char lBuff[MAX_PATH_OR_CMD] ;
        FILE * lIn ;

        lIn = popen( aCommand , "r" ) ;
        if ( fgets( lBuff , sizeof( lBuff ) , lIn ) == NULL )
        {       /* present */
                pclose( lIn ) ;
                return 1 ;
        }
        else
        {
                pclose( lIn ) ;
                return 0 ;
        }

}


static int isTerminalRunning(void)
{
	static int lIsTerminalRunning = -1 ;
	if ( lIsTerminalRunning < 0 )
	{
		lIsTerminalRunning = isatty(1);
		if (tinyfd_verbose) printf("isTerminalRunning %d\n", lIsTerminalRunning );
	}
	return lIsTerminalRunning;
}


static char * dialogNameOnly(void)
{
	static char lDialogName[128] = "*" ;
	if ( lDialogName[0] == '*' )
	{
		if (!tinyfd_allowCursesDialogs)
		{
			strcpy(lDialogName , "" );
		}
		else if ( tfd_isDarwin() && * strcpy(lDialogName , "/opt/local/bin/dialog" )
			&& detectPresence( lDialogName ) )
		{}
		else if ( * strcpy(lDialogName , "dialog" )
			&& detectPresence( lDialogName ) )
		{}
		else
		{
			strcpy(lDialogName , "" );
		}
	}
	return lDialogName ;
}


int isDialogVersionBetter09b(void)
{
        char const * lDialogName ;
        char * lVersion ;
        int lMajor ;
        int lMinor ;
        int lDate ;
        int lResult ;
        char * lMinorP ;
        char * lLetter ;
        char lBuff[128] ;

        /*char lTest[128] = " 0.9b-20031126" ;*/

        lDialogName = dialogNameOnly() ;
        if ( ! strlen(lDialogName) || !(lVersion = (char *) getVersion(lDialogName)) ) return 0 ;
        /*lVersion = lTest ;*/
        /*printf("lVersion %s\n", lVersion);*/
        strcpy(lBuff,lVersion);
        lMajor = atoi( strtok(lVersion," ,.-") ) ;
        /*printf("lMajor %d\n", lMajor);*/
        lMinorP = strtok(0," ,.-abcdefghijklmnopqrstuvxyz");
        lMinor = atoi( lMinorP ) ;
        /*printf("lMinor %d\n", lMinor );*/
        lDate = atoi( strtok(0," ,.-") ) ;
        if (lDate<0) lDate = - lDate;
        /*printf("lDate %d\n", lDate);*/
        lLetter = lMinorP + strlen(lMinorP) ;
        strcpy(lVersion,lBuff);
        strtok(lLetter," ,.-");
        /*printf("lLetter %s\n", lLetter);*/
        lResult = (lMajor > 0) || ( ( lMinor == 9 ) && (*lLetter == 'b') && (lDate >= 20031126) );
        /*printf("lResult %d\n", lResult);*/
        return lResult;
}


static int whiptailPresentOnly(void)
{
        static int lWhiptailPresent = -1 ;
		if (!tinyfd_allowCursesDialogs) return 0;
        if ( lWhiptailPresent < 0 )
        {
                lWhiptailPresent = detectPresence( "whiptail" ) ;
        }
        return lWhiptailPresent ;
}


static char * terminalName(void)
{
        static char lTerminalName[128] = "*" ;
        char lShellName[64] = "*" ;
        int * lArray;

        if ( lTerminalName[0] == '*' )
        {
                if ( detectPresence( "bash" ) )
                {
                        strcpy(lShellName , "bash -c " ) ; /*good for basic input*/
                }
				else if ( strlen(dialogNameOnly()) || whiptailPresentOnly() )
				{
						strcpy(lShellName , "sh -c " ) ; /*good enough for dialog & whiptail*/
				}
				else
				{
					strcpy(lTerminalName , "" ) ;
					return NULL ;
				}

                if ( tfd_isDarwin() )
                {
					if ( * strcpy(lTerminalName , "/opt/X11/bin/xterm" )
                      && detectPresence( lTerminalName ) )
                        {
                                strcat(lTerminalName , " -fa 'DejaVu Sans Mono' -fs 10 -title tinyfiledialogs -e " ) ;
                                strcat(lTerminalName , lShellName ) ;
                        }
                        else
                        {
                                strcpy(lTerminalName , "" ) ;
                        }
                }
                else if ( * strcpy(lTerminalName,"xterm") /*good (small without parameters)*/
                        && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -fa 'DejaVu Sans Mono' -fs 10 -title tinyfiledialogs -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"terminator") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -x " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"lxterminal") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"konsole") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"kterm") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"tilix") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"xfce4-terminal") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -x " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"mate-terminal") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -x " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"Eterm") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"evilvte") /*good*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else if ( * strcpy(lTerminalName,"pterm") /*good (only letters)*/
                          && detectPresence(lTerminalName) )
                {
                        strcat(lTerminalName , " -e " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
				else if ( * strcpy(lTerminalName,"gnome-terminal")
                && detectPresence(lTerminalName) && (lArray = getMajorMinorPatch(lTerminalName))
				&& ((lArray[0]<3) || (lArray[0]==3 && lArray[1]<=6)) )
                {
                        strcat(lTerminalName , " --disable-factory -x " ) ;
                        strcat(lTerminalName , lShellName ) ;
                }
                else
                {
                        strcpy(lTerminalName , "" ) ;
                }
                /* bad: koi rxterm guake tilda vala-terminal qterminal
                aterm Terminal terminology sakura lilyterm weston-terminal
                roxterm termit xvt rxvt mrxvt urxvt */
        }
        if ( strlen(lTerminalName) )
        {
                return lTerminalName ;
        }
        else
        {
                return NULL ;
        }
}


static char * dialogName(void)
{
    char * lDialogName ;
    lDialogName = dialogNameOnly( ) ;
        if ( strlen(lDialogName) && ( isTerminalRunning() || terminalName() ) )
        {
                return lDialogName ;
        }
        else
        {
                return NULL ;
        }
}


static int whiptailPresent(void)
{
        int lWhiptailPresent ;
    lWhiptailPresent = whiptailPresentOnly( ) ;
        if ( lWhiptailPresent && ( isTerminalRunning() || terminalName() ) )
        {
                return lWhiptailPresent ;
        }
        else
        {
                return 0 ;
        }
}



static int graphicMode(void)
{
        return !( tinyfd_forceConsole && (isTerminalRunning() || terminalName()) )
			&& ( getenvDISPLAY()
			|| (tfd_isDarwin() && (!getenv("SSH_TTY") || getenvDISPLAY() ) ) ) ;
}


static int pactlPresent(void)
{
        static int lPactlPresent = -1 ;
        if ( lPactlPresent < 0 )
        {
                lPactlPresent = detectPresence("pactl") ;
        }
        return lPactlPresent ;
}


static int speakertestPresent(void)
{
        static int lSpeakertestPresent = -1 ;
        if ( lSpeakertestPresent < 0 )
        {
                lSpeakertestPresent = detectPresence("speaker-test") ;
        }
        return lSpeakertestPresent ;
}


static int playPresent()
{
   static int lPlayPresent = -1;
   if (lPlayPresent < 0)
   {
      lPlayPresent = detectPresence("sox"); /*if sox is present, play is ready*/
   }
   return lPlayPresent;
}


static int beepexePresent()
{
   static int lBeepexePresent = -1;
   if (lBeepexePresent < 0)
   {
      lBeepexePresent = detectPresence("beep.exe");
   }
   return lBeepexePresent;
}


static int beepPresent(void)
{
        static int lBeepPresent = -1 ;
        if ( lBeepPresent < 0 )
        {
                lBeepPresent = detectPresence("beep") ;
        }
        return lBeepPresent ;
}


static int xmessagePresent(void)
{
        static int lXmessagePresent = -1 ;
        if ( lXmessagePresent < 0 )
        {
                lXmessagePresent = detectPresence("xmessage");/*if not tty,not on osxpath*/
        }
        return lXmessagePresent && graphicMode( ) ;
}


static int gxmessagePresent(void)
{
    static int lGxmessagePresent = -1 ;
    if ( lGxmessagePresent < 0 )
    {
        lGxmessagePresent = detectPresence("gxmessage") ;
    }
    return lGxmessagePresent && graphicMode( ) ;
}


static int gmessagePresent(void)
{
        static int lGmessagePresent = -1 ;
        if ( lGmessagePresent < 0 )
        {
                lGmessagePresent = detectPresence("gmessage") ;
        }
        return lGmessagePresent && graphicMode( ) ;
}


static int notifysendPresent(void)
{
    static int lNotifysendPresent = -1 ;
    if ( lNotifysendPresent < 0 )
    {
        lNotifysendPresent = detectPresence("notify-send") ;
    }
    return lNotifysendPresent && graphicMode( ) ;
}


static int perlPresent(void)
{
   static int lPerlPresent = -1 ;
   char lBuff[MAX_PATH_OR_CMD] ;
   FILE * lIn ;

   if ( lPerlPresent < 0 )
   {
      lPerlPresent = detectPresence("perl") ;
      if (lPerlPresent)
      {
         lIn = popen("perl -MNet::DBus -e \"Net::DBus->session->get_service('org.freedesktop.Notifications')\" 2>&1", "r");
         if (fgets(lBuff, sizeof(lBuff), lIn) == NULL)
         {
            lPerlPresent = 2;
         }
         pclose(lIn);
         if (tinyfd_verbose) printf("perl-dbus %d\n", lPerlPresent);
      }
   }
   return graphicMode() ? lPerlPresent : 0 ;
}


static int afplayPresent(void)
{
        static int lAfplayPresent = -1 ;
        char lBuff[MAX_PATH_OR_CMD] ;
        FILE * lIn ;

        if ( lAfplayPresent < 0 )
        {
                lAfplayPresent = detectPresence("afplay") ;
                if ( lAfplayPresent )
                {
                        lIn = popen( "test -e /System/Library/Sounds/Ping.aiff || echo Ping" , "r" ) ;
                        if ( fgets( lBuff , sizeof( lBuff ) , lIn ) == NULL )
                        {
                                lAfplayPresent = 2 ;
                        }
                        pclose( lIn ) ;
                        if (tinyfd_verbose) printf("afplay %d\n", lAfplayPresent);
                }
        }
        return graphicMode() ? lAfplayPresent : 0 ;
}


static int xdialogPresent(void)
{
    static int lXdialogPresent = -1 ;
    if ( lXdialogPresent < 0 )
    {
        lXdialogPresent = detectPresence("Xdialog") ;
    }
    return lXdialogPresent && graphicMode( ) ;
}


static int gdialogPresent(void)
{
    static int lGdialoglPresent = -1 ;
    if ( lGdialoglPresent < 0 )
    {
        lGdialoglPresent = detectPresence( "gdialog" ) ;
    }
    return lGdialoglPresent && graphicMode( ) ;
}


static int osascriptPresent(void)
{
    static int lOsascriptPresent = -1 ;
    if ( lOsascriptPresent < 0 )
    {
                gWarningDisplayed |= !!getenv("SSH_TTY");
                lOsascriptPresent = detectPresence( "osascript" ) ;
    }
        return lOsascriptPresent && graphicMode() && !getenv("SSH_TTY") ;
}


int tfd_qarmaPresent(void)
{
        static int lQarmaPresent = -1 ;
        if ( lQarmaPresent < 0 )
        {
                lQarmaPresent = detectPresence("qarma") ;
        }
        return lQarmaPresent && graphicMode( ) ;
}


int tfd_matedialogPresent(void)
{
        static int lMatedialogPresent = -1 ;
        if ( lMatedialogPresent < 0 )
        {
                lMatedialogPresent = detectPresence("matedialog") ;
        }
        return lMatedialogPresent && graphicMode( ) ;
}


int tfd_shellementaryPresent(void)
{
        static int lShellementaryPresent = -1 ;
        if ( lShellementaryPresent < 0 )
        {
                lShellementaryPresent = 0 ; /*detectPresence("shellementary"); shellementary is not ready yet */
        }
        return lShellementaryPresent && graphicMode( ) ;
}


int tfd_xpropPresent(void)
{
	static int lXpropPresent = -1 ;
	if ( lXpropPresent < 0 )
	{
		lXpropPresent = detectPresence("xprop") ;
	}
	return lXpropPresent && graphicMode( ) ;
}


int tfd_zenityPresent(void)
{
        static int lZenityPresent = -1 ;
        if ( lZenityPresent < 0 )
        {
                lZenityPresent = detectPresence("zenity") ;
        }
        return lZenityPresent && graphicMode( ) ;
}


int tfd_yadPresent(void)
{
   static int lYadPresent = -1;
   if (lYadPresent < 0)
   {
      lYadPresent = detectPresence("yad");
   }
   return lYadPresent && graphicMode();
}


int tfd_zenity3Present(void)
{
        static int lZenity3Present = -1 ;
        char lBuff[MAX_PATH_OR_CMD] ;
        FILE * lIn ;
		int lIntTmp ;

        if ( lZenity3Present < 0 )
        {
                lZenity3Present = 0 ;
                if ( tfd_zenityPresent() )
                {
                        lIn = popen( "zenity --version" , "r" ) ;
                        if ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
                        {
                                if ( atoi(lBuff) >= 3 )
                                {
                                        lZenity3Present = 3 ;
										lIntTmp = atoi(strtok(lBuff,".")+2 ) ;
										if ( lIntTmp >= 18 )
										{
											lZenity3Present = 5 ;
										}
										else if ( lIntTmp >= 10 )
										{
											lZenity3Present = 4 ;
										}
								}
                                else if ( ( atoi(lBuff) == 2 ) && ( atoi(strtok(lBuff,".")+2 ) >= 32 ) )
                                {
                                        lZenity3Present = 2 ;
                                }
                                if (tinyfd_verbose) printf("zenity type %d\n", lZenity3Present);
                        }
                        pclose( lIn ) ;
                }
        }
        return graphicMode() ? lZenity3Present : 0 ;
}


int tfd_kdialogPresent(void)
{
	static int lKdialogPresent = -1 ;
	char lBuff[MAX_PATH_OR_CMD] ;
	FILE * lIn ;
	char * lDesktop;

	if ( lKdialogPresent < 0 )
	{
		if ( tfd_zenityPresent() )
		{
			lDesktop = getenv("XDG_SESSION_DESKTOP");
			if ( !lDesktop  || ( strcmp(lDesktop, "KDE") && strcmp(lDesktop, "lxqt") ) )
			{
				lKdialogPresent = 0 ;
				return lKdialogPresent ;
			}
		}

		lKdialogPresent = detectPresence("kdialog") ;
		if ( lKdialogPresent && !getenv("SSH_TTY") )
		{
			lIn = popen( "kdialog --attach 2>&1" , "r" ) ;
			if ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
			{
				if ( ! strstr( "Unknown" , lBuff ) )
				{
					lKdialogPresent = 2 ;
					if (tinyfd_verbose) printf("kdialog-attach %d\n", lKdialogPresent);
				}
			}
			pclose( lIn ) ;

			if (lKdialogPresent == 2)
			{
				lKdialogPresent = 1 ;
				lIn = popen( "kdialog --passivepopup 2>&1" , "r" ) ;
				if ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
				{
					if ( ! strstr( "Unknown" , lBuff ) )
					{
						lKdialogPresent = 2 ;
						if (tinyfd_verbose) printf("kdialog-popup %d\n", lKdialogPresent);
					}
				}
				pclose( lIn ) ;
			}
		}
	}
	return graphicMode() ? lKdialogPresent : 0 ;
}


static int osx9orBetter(void)
{
        static int lOsx9orBetter = -1 ;
        char lBuff[MAX_PATH_OR_CMD] ;
        FILE * lIn ;
        int V,v;

        if ( lOsx9orBetter < 0 )
        {
                lOsx9orBetter = 0 ;
                lIn = popen( "osascript -e 'set osver to system version of (system info)'" , "r" ) ;
                if ( ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
                        && ( 2 == sscanf(lBuff, "%d.%d", &V, &v) ) )
                {
                        V = V * 100 + v;
                        if ( V >= 1009 )
                        {
                                lOsx9orBetter = 1 ;
                        }
                }
                pclose( lIn ) ;
                if (tinyfd_verbose) printf("Osx10 = %d, %d = %s\n", lOsx9orBetter, V, lBuff) ;
        }
        return lOsx9orBetter ;
}


static int python3Present(void)
{
        static int lPython3Present = -1 ;
        int i;

        if ( lPython3Present < 0 )
        {
                lPython3Present = 0 ;
                strcpy(gPython3Name , "python3" ) ;
                if ( detectPresence(gPython3Name) ) lPython3Present = 1;
                else
                {
                        for ( i = 9 ; i >= 0 ; i -- )
                        {
                                sprintf( gPython3Name , "python3.%d" , i ) ;
                                if ( detectPresence(gPython3Name) )
                                {
                                        lPython3Present = 1;
                                        break;
                                }
                        }
                }
                if (tinyfd_verbose) printf("lPython3Present %d\n", lPython3Present) ;
                if (tinyfd_verbose) printf("gPython3Name %s\n", gPython3Name) ;
        }
		return lPython3Present ;
}


static int python2Present(void)
{
	static int lPython2Present = -1 ;
	int i;

	if ( lPython2Present < 0 )
	{
		lPython2Present = 0 ;
		strcpy(gPython2Name , "python2" ) ;
		if ( detectPresence(gPython2Name) ) lPython2Present = 1;
		else
		{
			for ( i = 9 ; i >= 0 ; i -- )
			{
				sprintf( gPython2Name , "python2.%d" , i ) ;
				if ( detectPresence(gPython2Name) )
				{
					lPython2Present = 1;
					break;
				}
			}
		}
		if (tinyfd_verbose) printf("lPython2Present %d\n", lPython2Present) ;
		if (tinyfd_verbose) printf("gPython2Name %s\n", gPython2Name) ;
	}
	return lPython2Present ;
}


static int tkinter3Present(void)
{
        static int lTkinter3Present = -1 ;
        char lPythonCommand[256];
        char lPythonParams[128] =
                "-S -c \"try:\n\timport tkinter;\nexcept:\n\tprint(0);\"";

        if ( lTkinter3Present < 0 )
        {
                lTkinter3Present = 0 ;
                if ( python3Present() )
                {
                        sprintf( lPythonCommand , "%s %s" , gPython3Name , lPythonParams ) ;
                        lTkinter3Present = tryCommand(lPythonCommand) ;
                }
                if (tinyfd_verbose) printf("lTkinter3Present %d\n", lTkinter3Present) ;
        }
		return lTkinter3Present && graphicMode() && !(tfd_isDarwin() && getenv("SSH_TTY") );
}


static int tkinter2Present(void)
{
	static int lTkinter2Present = -1 ;
	char lPythonCommand[256];
	char lPythonParams[128] =
		"-S -c \"try:\n\timport Tkinter;\nexcept:\n\tprint 0;\"";

	if ( lTkinter2Present < 0 )
	{
		lTkinter2Present = 0 ;
		if ( python2Present() )
		{
			sprintf( lPythonCommand , "%s %s" , gPython2Name , lPythonParams ) ;
			lTkinter2Present = tryCommand(lPythonCommand) ;
		}
		if (tinyfd_verbose) printf("lTkinter2Present %d\n", lTkinter2Present) ;
	}
	return lTkinter2Present && graphicMode() && !(tfd_isDarwin() && getenv("SSH_TTY") );
}


static int pythonDbusPresent(void)
{
    static int lPythonDbusPresent = -1 ;
        char lPythonCommand[384];
        char lPythonParams[256] =
"-c \"try:\n\timport dbus;bus=dbus.SessionBus();\
notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');\
notify=dbus.Interface(notif,'org.freedesktop.Notifications');\nexcept:\n\tprint(0);\"";

        if (lPythonDbusPresent < 0 )
        {
           lPythonDbusPresent = 0 ;
                if ( python2Present() )
                {
                        strcpy(gPythonName , gPython2Name ) ;
                        sprintf( lPythonCommand , "%s %s" , gPythonName , lPythonParams ) ;
                        lPythonDbusPresent = tryCommand(lPythonCommand) ;
                }

                if ( !lPythonDbusPresent && python3Present() )
                {
                        strcpy(gPythonName , gPython3Name ) ;
                        sprintf( lPythonCommand , "%s %s" , gPythonName , lPythonParams ) ;
                        lPythonDbusPresent = tryCommand(lPythonCommand) ;
                }

                if (tinyfd_verbose) printf("lPythonDbusPresent %d\n", lPythonDbusPresent) ;
                if (tinyfd_verbose) printf("gPythonName %s\n", gPythonName) ;
        }
        return lPythonDbusPresent && graphicMode() && !(tfd_isDarwin() && getenv("SSH_TTY") );
}


static void sigHandler(int signum)
{
        FILE * lIn ;
        if ( ( lIn = popen( "pactl unload-module module-sine" , "r" ) ) )
        {
                pclose( lIn ) ;
        }
		if (tinyfd_verbose) printf("tinyfiledialogs caught signal %d\n", signum);
}

void tinyfd_beep(void)
{
        char lDialogString[256] ;
        FILE * lIn ;

        if ( osascriptPresent() )
        {
                if ( afplayPresent() >= 2 )
                {
                        strcpy( lDialogString , "afplay /System/Library/Sounds/Ping.aiff") ;
                }
                else
                {
                        strcpy( lDialogString , "osascript -e 'tell application \"System Events\" to beep'") ;
                }
        }
        else if ( pactlPresent() )
        {
                signal(SIGINT, sigHandler);
                /*strcpy( lDialogString , "pactl load-module module-sine frequency=440;sleep .3;pactl unload-module module-sine" ) ;*/
                strcpy( lDialogString , "thnum=$(pactl load-module module-sine frequency=440);sleep .3;pactl unload-module $thnum" ) ;
        }
        else if ( speakertestPresent() )
        {
                /*strcpy( lDialogString , "timeout -k .3 .3 speaker-test --frequency 440 --test sine > /dev/tty" ) ;*/
                strcpy( lDialogString , "( speaker-test -t sine -f 440 > /dev/tty )& pid=$!;sleep .4; kill -9 $pid" ) ; /*.3 was too short for mac g3*/
        }
        else if (beepexePresent())
        {
                strcpy(lDialogString, "beep.exe 440 300");
        }
        else if (playPresent()) /* play is part of sox */
        {
                strcpy(lDialogString, "play -q -n synth .3 sine 440");
        }
        else if ( beepPresent() )
        {
                strcpy( lDialogString , "beep -f 440 -l 300" ) ;
        }
        else
        {
                strcpy( lDialogString , "printf '\a' > /dev/tty" ) ;
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;

        if ( ( lIn = popen( lDialogString , "r" ) ) )
        {
                pclose( lIn ) ;
        }

        if ( pactlPresent() )
        {
                signal(SIGINT, SIG_DFL);
        }
}


int tinyfd_messageBox(
        char const * aTitle , /* NULL or "" */
        char const * aMessage , /* NULL or ""  may contain \n and \t */
        char const * aDialogType , /* "ok" "okcancel" "yesno" "yesnocancel" */
        char const * aIconType , /* "info" "warning" "error" "question" */
        int aDefaultButton ) /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
        char lBuff[MAX_PATH_OR_CMD] ;
        char * lDialogString = NULL ;
        char * lpDialogString;
        FILE * lIn ;
        int lWasGraphicDialog = 0 ;
        int lWasXterm = 0 ;
        int lResult ;
        char lChar ;
        struct termios infoOri;
        struct termios info;
        size_t lTitleLen ;
        size_t lMessageLen ;

        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_messageBox("INVALID TITLE WITH QUOTES", aMessage, aDialogType, aIconType, aDefaultButton);
		if (tfd_quoteDetected(aMessage)) return tinyfd_messageBox(aTitle, "INVALID MESSAGE WITH QUOTES", aDialogType, aIconType, aDefaultButton);

        lTitleLen =  aTitle ? strlen(aTitle) : 0 ;
        lMessageLen =  aMessage ? strlen(aMessage) : 0 ;
        if ( !aTitle || strcmp(aTitle,"tinyfd_query") )
        {
                lDialogString = (char *) malloc( MAX_PATH_OR_CMD + lTitleLen + lMessageLen );
        }

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return 1;}

                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e 'set {vButton} to {button returned} of ( display dialog \"") ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage) ;
                }
                strcat(lDialogString, "\" ") ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "with title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }
                strcat(lDialogString, "with icon ") ;
                if ( aIconType && ! strcmp( "error" , aIconType ) )
                {
                        strcat(lDialogString, "stop " ) ;
                }
                else if ( aIconType && ! strcmp( "warning" , aIconType ) )
                {
                        strcat(lDialogString, "caution " ) ;
                }
                else /* question or info */
                {
                        strcat(lDialogString, "note " ) ;
                }
                if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
                {
                        if ( ! aDefaultButton )
                        {
                                strcat( lDialogString ,"default button \"Cancel\" " ) ;
                        }
                }
                else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
                {
                        strcat( lDialogString ,"buttons {\"No\", \"Yes\"} " ) ;
                        if (aDefaultButton)
                        {
                                strcat( lDialogString ,"default button \"Yes\" " ) ;
                        }
                        else
                        {
                                strcat( lDialogString ,"default button \"No\" " ) ;
                        }
                        strcat( lDialogString ,"cancel button \"No\"" ) ;
                }
                else if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat( lDialogString ,"buttons {\"No\", \"Yes\", \"Cancel\"} " ) ;
                        switch (aDefaultButton)
                        {
                                case 1: strcat( lDialogString ,"default button \"Yes\" " ) ; break;
                                case 2: strcat( lDialogString ,"default button \"No\" " ) ; break;
                                case 0: strcat( lDialogString ,"default button \"Cancel\" " ) ; break;
                        }
                        strcat( lDialogString ,"cancel button \"Cancel\"" ) ;
                }
                else
                {
                        strcat( lDialogString ,"buttons {\"OK\"} " ) ;
                        strcat( lDialogString ,"default button \"OK\" " ) ;
                }
                strcat( lDialogString, ")' ") ;

                strcat( lDialogString,
"-e 'if vButton is \"Yes\" then' -e 'return 1'\
 -e 'else if vButton is \"OK\" then' -e 'return 1'\
 -e 'else if vButton is \"No\" then' -e 'return 2'\
 -e 'else' -e 'return 0' -e 'end if' " );

                strcat( lDialogString, "-e 'on error number -128' " ) ;
                strcat( lDialogString, "-e '0' " );

                strcat( lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return 1;}

                strcpy( lDialogString , "kdialog" ) ;
				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }

                strcat( lDialogString , " --" ) ;
                if ( aDialogType && ( ! strcmp( "okcancel" , aDialogType )
                        || ! strcmp( "yesno" , aDialogType ) || ! strcmp( "yesnocancel" , aDialogType ) ) )
                {
                        if ( aIconType && ( ! strcmp( "warning" , aIconType )
                                || ! strcmp( "error" , aIconType ) ) )
                        {
                                strcat( lDialogString , "warning" ) ;
                        }
                        if ( ! strcmp( "yesnocancel" , aDialogType ) )
                        {
                                strcat( lDialogString , "yesnocancel" ) ;
                        }
                        else
                        {
                                strcat( lDialogString , "yesno" ) ;
                        }
                }
                else if ( aIconType && ! strcmp( "error" , aIconType ) )
                {
                        strcat( lDialogString , "error" ) ;
                }
                else if ( aIconType && ! strcmp( "warning" , aIconType ) )
                {
                        strcat( lDialogString , "sorry" ) ;
                }
                else
                {
                        strcat( lDialogString , "msgbox" ) ;
                }
                strcat( lDialogString , " \"" ) ;
                if ( aMessage )
                {
                        strcat( lDialogString , aMessage ) ;
                }
                strcat( lDialogString , "\"" ) ;
                if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
                {
                        strcat( lDialogString ,
                                " --yes-label Ok --no-label Cancel" ) ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }

                if ( ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat( lDialogString , "; x=$? ;if [ $x = 0 ] ;then echo 1;elif [ $x = 1 ] ;then echo 2;else echo 0;fi");
                }
                else
                {
                        strcat( lDialogString , ";if [ $? = 0 ];then echo 1;else echo 0;fi");
                }
        }
        else if ( tfd_zenityPresent() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                if ( tfd_zenityPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return 1;}
                        strcpy( lDialogString , "szAnswer=$(zenity" ) ;
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return 1;}
                        strcpy( lDialogString , "szAnswer=$(matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return 1;}
                        strcpy( lDialogString , "szAnswer=$(shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return 1;}
                        strcpy( lDialogString , "szAnswer=$(qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat(lDialogString, " --");

                if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
                {
                                strcat( lDialogString ,
                                                "question --ok-label=Ok --cancel-label=Cancel" ) ;
                }
                else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
                {
                                strcat( lDialogString , "question" ) ;
                }
                else if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat( lDialogString , "list --column \"\" --hide-header \"Yes\" \"No\"" ) ;
                }
                else if ( aIconType && ! strcmp( "error" , aIconType ) )
                {
                    strcat( lDialogString , "error" ) ;
                }
                else if ( aIconType && ! strcmp( "warning" , aIconType ) )
                {
                    strcat( lDialogString , "warning" ) ;
                }
                else
                {
                    strcat( lDialogString , "info" ) ;
                }

                strcat(lDialogString, " --title=\"");
                if ( aTitle && strlen(aTitle) ) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"");

                if (strcmp("yesnocancel", aDialogType)) strcat(lDialogString, " --no-wrap");

                strcat(lDialogString, " --text=\"") ;
                if (aMessage && strlen(aMessage)) strcat(lDialogString, aMessage) ;
                strcat(lDialogString, "\"") ;

                if ( (tfd_zenity3Present() >= 3) || (!tfd_zenityPresent() && (tfd_shellementaryPresent() || tfd_qarmaPresent()) ) )
                {
                        strcat( lDialogString , " --icon-name=dialog-" ) ;
                        if ( aIconType && (! strcmp( "question" , aIconType )
                          || ! strcmp( "error" , aIconType )
                          || ! strcmp( "warning" , aIconType ) ) )
                        {
                                strcat( lDialogString , aIconType ) ;
                        }
                        else
                        {
                                strcat( lDialogString , "information" ) ;
                        }
                }

                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");

                if ( ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat( lDialogString ,
");if [ $? = 1 ];then echo 0;elif [ $szAnswer = \"No\" ];then echo 2;else echo 1;fi");
                }
                else
                {
                        strcat( lDialogString , ");if [ $? = 0 ];then echo 1;else echo 0;fi");
                }
      }

      else if (tfd_yadPresent())
      {
         if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return 1; }
         strcpy(lDialogString, "szAnswer=$(yad --");
         if (aDialogType && !strcmp("ok", aDialogType))
         {
            strcat(lDialogString,"button=Ok:1");
         }
         else if (aDialogType && !strcmp("okcancel", aDialogType))
         {
            strcat(lDialogString,"button=Ok:1 --button=Cancel:0");
         }
         else if (aDialogType && !strcmp("yesno", aDialogType))
         {
            strcat(lDialogString, "button=Yes:1 --button=No:0");
         }
         else if (aDialogType && !strcmp("yesnocancel", aDialogType))
         {
            strcat(lDialogString, "button=Yes:1 --button=No:2 --button=Cancel:0");
         }
         else if (aIconType && !strcmp("error", aIconType))
         {
            strcat(lDialogString, "error");
         }
         else if (aIconType && !strcmp("warning", aIconType))
         {
            strcat(lDialogString, "warning");
         }
         else
         {
            strcat(lDialogString, "info");
         }
         if (aTitle && strlen(aTitle))
         {
            strcat(lDialogString, " --title=\"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
         }
         if (aMessage && strlen(aMessage))
         {
            strcat(lDialogString, " --text=\"");
            strcat(lDialogString, aMessage);
            strcat(lDialogString, "\"");
         }

         strcat(lDialogString, " --icon-name=dialog-");
         if (aIconType && (!strcmp("question", aIconType)
            || !strcmp("error", aIconType)
            || !strcmp("warning", aIconType)))
         {
            strcat(lDialogString, aIconType);
         }
         else
         {
            strcat(lDialogString, "information");
         }

         if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
         strcat(lDialogString,");echo $?");
      }

      else if ( !gxmessagePresent() && !gmessagePresent() && !gdialogPresent() && !xdialogPresent() && tkinter3Present() )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return 1;}

			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter;from tkinter import messagebox;root=tkinter.Tk();root.withdraw();");

			strcat( lDialogString ,"res=messagebox." ) ;
			if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
			{
				strcat( lDialogString , "askokcancel(" ) ;
				if ( aDefaultButton )
				{
					strcat( lDialogString , "default=messagebox.OK," ) ;
				}
				else
				{
					strcat( lDialogString , "default=messagebox.CANCEL," ) ;
				}
			}
			else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
			{
				strcat( lDialogString , "askyesno(" ) ;
				if ( aDefaultButton )
				{
					strcat( lDialogString , "default=messagebox.YES," ) ;
				}
				else
				{
					strcat( lDialogString , "default=messagebox.NO," ) ;
				}
			}
			else if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
			{
				strcat( lDialogString , "askyesnocancel(" ) ;
				switch ( aDefaultButton )
				{
				case 1: strcat( lDialogString , "default=messagebox.YES," ); break;
				case 2: strcat( lDialogString , "default=messagebox.NO," ); break;
				case 0: strcat( lDialogString , "default=messagebox.CANCEL," ); break;
				}
			}
			else
			{
				strcat( lDialogString , "showinfo(" ) ;
			}

			strcat( lDialogString , "icon='" ) ;
			if ( aIconType && (! strcmp( "question" , aIconType )
				|| ! strcmp( "error" , aIconType )
				|| ! strcmp( "warning" , aIconType ) ) )
			{
				strcat( lDialogString , aIconType ) ;
			}
			else
			{
				strcat( lDialogString , "info" ) ;
			}

			strcat(lDialogString, "',") ;
			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, "title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "',") ;
			}
			if ( aMessage && strlen(aMessage) )
			{
				strcat(lDialogString, "message='") ;
				lpDialogString = lDialogString + strlen(lDialogString);
				tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
				strcat(lDialogString, "'") ;
			}

			if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
			{
				strcat(lDialogString, ");\n\
if res is None :\n\tprint(0)\n\
elif res is False :\n\tprint(2)\n\
else :\n\tprint (1)\n\"" ) ;
			}
			else
			{
				strcat(lDialogString, ");\n\
if res is False :\n\tprint(0)\n\
else :\n\tprint(1)\n\"" ) ;
			}
		}
		else if ( !gxmessagePresent() && !gmessagePresent() && !gdialogPresent() && !xdialogPresent() && tkinter2Present() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return 1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
				strcat( lDialogString , gPython2Name ) ;
				if ( ! isTerminalRunning( ) && tfd_isDarwin( ) )
                {
                        strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }

                strcat( lDialogString ,
" -S -c \"import Tkinter,tkMessageBox;root=Tkinter.Tk();root.withdraw();");

                if ( tfd_isDarwin( ) )
                {
                        strcat( lDialogString ,
"import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
                }

                strcat( lDialogString ,"res=tkMessageBox." ) ;
                if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
                {
                  strcat( lDialogString , "askokcancel(" ) ;
                  if ( aDefaultButton )
                        {
                                strcat( lDialogString , "default=tkMessageBox.OK," ) ;
                        }
                        else
                        {
                                strcat( lDialogString , "default=tkMessageBox.CANCEL," ) ;
                        }
                }
                else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
                {
                        strcat( lDialogString , "askyesno(" ) ;
                        if ( aDefaultButton )
                        {
                                strcat( lDialogString , "default=tkMessageBox.YES," ) ;
                        }
                        else
                        {
                                strcat( lDialogString , "default=tkMessageBox.NO," ) ;
                        }
                }
                else if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat( lDialogString , "askyesnocancel(" ) ;
                        switch ( aDefaultButton )
                        {
                                case 1: strcat( lDialogString , "default=tkMessageBox.YES," ); break;
                                case 2: strcat( lDialogString , "default=tkMessageBox.NO," ); break;
                                case 0: strcat( lDialogString , "default=tkMessageBox.CANCEL," ); break;
                        }
                }
                else
                {
                                strcat( lDialogString , "showinfo(" ) ;
                }

                strcat( lDialogString , "icon='" ) ;
                if ( aIconType && (! strcmp( "question" , aIconType )
                  || ! strcmp( "error" , aIconType )
                  || ! strcmp( "warning" , aIconType ) ) )
                {
                                strcat( lDialogString , aIconType ) ;
                }
                else
                {
                                strcat( lDialogString , "info" ) ;
                }

                strcat(lDialogString, "',") ;
                if ( aTitle && strlen(aTitle) )
                {
                                strcat(lDialogString, "title='") ;
                                strcat(lDialogString, aTitle) ;
                                strcat(lDialogString, "',") ;
                }
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, "message='") ;
                        lpDialogString = lDialogString + strlen(lDialogString);
                        tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
                        strcat(lDialogString, "'") ;
                }

                if ( aDialogType && ! strcmp( "yesnocancel" , aDialogType ) )
                {
                        strcat(lDialogString, ");\n\
if res is None :\n\tprint 0\n\
elif res is False :\n\tprint 2\n\
else :\n\tprint 1\n\"" ) ;
                }
                else
                {
                        strcat(lDialogString, ");\n\
if res is False :\n\tprint 0\n\
else :\n\tprint 1\n\"" ) ;
                }
    }
        else if ( gxmessagePresent() || gmessagePresent() || (!gdialogPresent() && !xdialogPresent() && xmessagePresent()) )
        {
                if ( gxmessagePresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gxmessage");return 1;}
                        strcpy( lDialogString , "gxmessage");
                }
                else if ( gmessagePresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gmessage");return 1;}
                        strcpy( lDialogString , "gmessage");
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xmessage");return 1;}
                        strcpy( lDialogString , "xmessage");
                }

                if ( aDialogType && ! strcmp("okcancel" , aDialogType) )
                {
                        strcat( lDialogString , " -buttons Ok:1,Cancel:0");
                        switch ( aDefaultButton )
                        {
                                case 1: strcat( lDialogString , " -default Ok"); break;
                                case 0: strcat( lDialogString , " -default Cancel"); break;
                        }
                }
                else if ( aDialogType && ! strcmp("yesno" , aDialogType) )
                {
                        strcat( lDialogString , " -buttons Yes:1,No:0");
                        switch ( aDefaultButton )
                        {
                                case 1: strcat( lDialogString , " -default Yes"); break;
                                case 0: strcat( lDialogString , " -default No"); break;
                        }
                }
                else if ( aDialogType && ! strcmp("yesnocancel" , aDialogType) )
                {
                        strcat( lDialogString , " -buttons Yes:1,No:2,Cancel:0");
                        switch ( aDefaultButton )
                        {
                                case 1: strcat( lDialogString , " -default Yes"); break;
                                case 2: strcat( lDialogString , " -default No"); break;
                                case 0: strcat( lDialogString , " -default Cancel"); break;
                        }
                }
                else
                {
                        strcat( lDialogString , " -buttons Ok:1");
                        strcat( lDialogString , " -default Ok");
                }

                strcat( lDialogString , " -center \"");
                if ( aMessage && strlen(aMessage) )
                {
                        strcat( lDialogString , aMessage ) ;
                }
                strcat(lDialogString, "\"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat( lDialogString , " -title  \"");
                        strcat( lDialogString , aTitle ) ;
                        strcat( lDialogString, "\"" ) ;
                }
                strcat( lDialogString , " ; echo $? ");
        }
        else if ( xdialogPresent() || gdialogPresent() || dialogName() || whiptailPresent() )
        {
                if ( gdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gdialog");return 1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(gdialog " ) ;
                }
                else if ( xdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return 1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(Xdialog " ) ;
                }
                else if ( dialogName( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return 0;}
                        if ( isTerminalRunning( ) )
                        {
                                strcpy( lDialogString , "(dialog " ) ;
                        }
                        else
                        {
                                lWasXterm = 1 ;
                                strcpy( lDialogString , terminalName() ) ;
                                strcat( lDialogString , "'(" ) ;
                                strcat( lDialogString , dialogName() ) ;
                                strcat( lDialogString , " " ) ;
                        }
                }
                else if ( isTerminalRunning( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"whiptail");return 0;}
                        strcpy( lDialogString , "(whiptail " ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"whiptail");return 0;}
                        lWasXterm = 1 ;
                        strcpy( lDialogString , terminalName() ) ;
                        strcat( lDialogString , "'(whiptail " ) ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "--title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                if ( !xdialogPresent() && !gdialogPresent() )
                {
                        if ( aDialogType && ( !strcmp( "okcancel" , aDialogType ) || !strcmp( "yesno" , aDialogType )
                                || !strcmp( "yesnocancel" , aDialogType ) ) )
                        {
                                strcat(lDialogString, "--backtitle \"") ;
                                strcat(lDialogString, "tab: move focus") ;
                                strcat(lDialogString, "\" ") ;
                        }
                }

                if ( aDialogType && ! strcmp( "okcancel" , aDialogType ) )
                {
                        if ( ! aDefaultButton )
                        {
                                strcat( lDialogString , "--defaultno " ) ;
                        }
                        strcat( lDialogString ,
                                        "--yes-label \"Ok\" --no-label \"Cancel\" --yesno " ) ;
                }
                else if ( aDialogType && ! strcmp( "yesno" , aDialogType ) )
                {
                        if ( ! aDefaultButton )
                        {
                                strcat( lDialogString , "--defaultno " ) ;
                        }
                        strcat( lDialogString , "--yesno " ) ;
                }
                else if (aDialogType && !strcmp("yesnocancel", aDialogType))
                {
                        if (!aDefaultButton)
                        {
                                strcat(lDialogString, "--defaultno ");
                        }
                        strcat(lDialogString, "--menu ");
                }
                else
                {
                        strcat( lDialogString , "--msgbox " ) ;

                }
                strcat( lDialogString , "\"" ) ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage) ;
                }
                strcat(lDialogString, "\" ");

                if ( lWasGraphicDialog )
                {
                        if (aDialogType && !strcmp("yesnocancel", aDialogType))
                        {
                                strcat(lDialogString,"0 60 0 Yes \"\" No \"\") 2>/tmp/tinyfd.txt;\
if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes") ;
                        }
                        else
                        {
                                strcat(lDialogString,
                                   "10 60 ) 2>&1;if [ $? = 0 ];then echo 1;else echo 0;fi");
                        }
                }
                else
                {
                        if (aDialogType && !strcmp("yesnocancel", aDialogType))
                        {
                                strcat(lDialogString,"0 60 0 Yes \"\" No \"\" >/dev/tty ) 2>/tmp/tinyfd.txt;\
                if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
                tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes") ;

                                if ( lWasXterm )
                                {
                                        strcat(lDialogString," >/tmp/tinyfd0.txt';cat /tmp/tinyfd0.txt");
                                }
                                else
                                {
                                        strcat(lDialogString, "; clear >/dev/tty") ;
                                }
                        }
                        else
                        {
                                strcat(lDialogString, "10 60 >/dev/tty) 2>&1;if [ $? = 0 ];");
                                if ( lWasXterm )
                                {
                                        strcat( lDialogString ,
"then\n\techo 1\nelse\n\techo 0\nfi >/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
                                }
                                else
                                {
                                   strcat(lDialogString,
                                                  "then echo 1;else echo 0;fi;clear >/dev/tty");
                                }
                        }
                }
        }
        else if (  !isTerminalRunning() && terminalName() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"basicinput");return 0;}
                strcpy( lDialogString , terminalName() ) ;
                strcat( lDialogString , "'" ) ;
                if ( !gWarningDisplayed && !tinyfd_forceConsole)
                {
                        gWarningDisplayed = 1 ;
                        strcat( lDialogString , "echo \"" ) ;
                        strcat( lDialogString, gTitle) ;
                        strcat( lDialogString , "\";" ) ;
                        strcat( lDialogString , "echo \"" ) ;
                        strcat( lDialogString, tinyfd_needs) ;
                        strcat( lDialogString , "\";echo;echo;" ) ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat( lDialogString , "echo \"" ) ;
                        strcat( lDialogString, aTitle) ;
                        strcat( lDialogString , "\";echo;" ) ;
                }
                if ( aMessage && strlen(aMessage) )
                {
                        strcat( lDialogString , "echo \"" ) ;
                        strcat( lDialogString, aMessage) ;
                        strcat( lDialogString , "\"; " ) ;
                }
                if ( aDialogType && !strcmp("yesno",aDialogType) )
                {
                        strcat( lDialogString , "echo -n \"y/n: \"; " ) ;
                        strcat( lDialogString , "stty sane -echo;" ) ;
                        strcat( lDialogString ,
                                "answer=$( while ! head -c 1 | grep -i [ny];do true ;done);");
                        strcat( lDialogString ,
                                "if echo \"$answer\" | grep -iq \"^y\";then\n");
                        strcat( lDialogString , "\techo 1\nelse\n\techo 0\nfi" ) ;
                }
                else if ( aDialogType && !strcmp("okcancel",aDialogType) )
                {
                        strcat( lDialogString , "echo -n \"[O]kay/[C]ancel: \"; " ) ;
                        strcat( lDialogString , "stty sane -echo;" ) ;
                        strcat( lDialogString ,
                                "answer=$( while ! head -c 1 | grep -i [oc];do true ;done);");
                        strcat( lDialogString ,
                                "if echo \"$answer\" | grep -iq \"^o\";then\n");
                        strcat( lDialogString , "\techo 1\nelse\n\techo 0\nfi" ) ;
                }
                else if ( aDialogType && !strcmp("yesnocancel",aDialogType) )
                {
                        strcat( lDialogString , "echo -n \"[Y]es/[N]o/[C]ancel: \"; " ) ;
                        strcat( lDialogString , "stty sane -echo;" ) ;
                        strcat( lDialogString ,
                                "answer=$( while ! head -c 1 | grep -i [nyc];do true ;done);");
                        strcat( lDialogString ,
                                "if echo \"$answer\" | grep -iq \"^y\";then\n\techo 1\n");
                        strcat( lDialogString , "elif echo \"$answer\" | grep -iq \"^n\";then\n\techo 2\n" ) ;
                        strcat( lDialogString , "else\n\techo 0\nfi" ) ;
                }
                else
                {
                        strcat(lDialogString , "echo -n \"press enter to continue \"; ");
                        strcat( lDialogString , "stty sane -echo;" ) ;
                        strcat( lDialogString ,
                                "answer=$( while ! head -c 1;do true ;done);echo 1");
                }
                strcat( lDialogString ,
                        " >/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
        }
        else if ( !isTerminalRunning() && pythonDbusPresent() && !strcmp("ok" , aDialogType) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python-dbus");return 1;}
                strcpy( lDialogString , gPythonName ) ;
                strcat( lDialogString ," -c \"import dbus;bus=dbus.SessionBus();");
                strcat( lDialogString ,"notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');" ) ;
                strcat( lDialogString ,"notify=dbus.Interface(notif,'org.freedesktop.Notifications');" ) ;
                strcat( lDialogString ,"notify.Notify('',0,'" ) ;
                if ( aIconType && strlen(aIconType) )
                {
                        strcat( lDialogString , aIconType ) ;
                }
                strcat(lDialogString, "','") ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                }
                strcat(lDialogString, "','") ;
                if ( aMessage && strlen(aMessage) )
                {
                        lpDialogString = lDialogString + strlen(lDialogString);
                        tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
                }
                strcat(lDialogString, "','','',5000)\"") ;
        }
        else if ( !isTerminalRunning() && (perlPresent() >= 2)  && !strcmp("ok" , aDialogType) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"perl-dbus");return 1;}

				strcpy( lDialogString ,  "perl -e \"use Net::DBus;\
my \\$sessionBus = Net::DBus->session;\
my \\$notificationsService = \\$sessionBus->get_service('org.freedesktop.Notifications');\
my \\$notificationsObject = \\$notificationsService->get_object('/org/freedesktop/Notifications',\
'org.freedesktop.Notifications');");

				sprintf( lDialogString + strlen(lDialogString),
"my \\$notificationId;\\$notificationId = \\$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);\" ",
							aIconType?aIconType:"", aTitle?aTitle:"", aMessage?aMessage:"" ) ;
        }
        else if ( !isTerminalRunning() && notifysendPresent() && !strcmp("ok" , aDialogType) )
        {

                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"notifysend");return 1;}
                strcpy( lDialogString , "notify-send" ) ;
                if ( aIconType && strlen(aIconType) )
                {
                        strcat( lDialogString , " -i '" ) ;
                        strcat( lDialogString , aIconType ) ;
                        strcat( lDialogString , "'" ) ;
                }
        strcat( lDialogString , " \"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                        strcat( lDialogString , " | " ) ;
                }
                if ( aMessage && strlen(aMessage) )
                {
            tfd_replaceSubStr( aMessage , "\n\t" , " |  " , lBuff ) ;
            tfd_replaceSubStr( aMessage , "\n" , " | " , lBuff ) ;
            tfd_replaceSubStr( aMessage , "\t" , "  " , lBuff ) ;
                        strcat(lDialogString, lBuff) ;
                }
                strcat( lDialogString , "\"" ) ;
        }
        else
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"basicinput");return 0;}
                if ( !gWarningDisplayed && !tinyfd_forceConsole)
                {
                        gWarningDisplayed = 1 ;
                        printf("\n\n%s\n", gTitle);
                        printf("%s\n\n", tinyfd_needs);
                }
                if ( aTitle && strlen(aTitle) )
                {
                        printf("\n%s\n", aTitle);
                }

                tcgetattr(0, &infoOri);
                tcgetattr(0, &info);
                info.c_lflag &= ~ICANON;
                info.c_cc[VMIN] = 1;
                info.c_cc[VTIME] = 0;
                tcsetattr(0, TCSANOW, &info);
                if ( aDialogType && !strcmp("yesno",aDialogType) )
                {
                        do
                        {
                                if ( aMessage && strlen(aMessage) )
                                {
                                        printf("\n%s\n",aMessage);
                                }
                                printf("y/n: "); fflush(stdout);
                                lChar = tolower( getchar() ) ;
                                printf("\n\n");
                        }
                        while ( lChar != 'y' && lChar != 'n' );
                        lResult = lChar == 'y' ? 1 : 0 ;
                }
                else if ( aDialogType && !strcmp("okcancel",aDialogType) )
                {
                        do
                        {
                                if ( aMessage && strlen(aMessage) )
                                {
                                        printf("\n%s\n",aMessage);
                                }
                                printf("[O]kay/[C]ancel: "); fflush(stdout);
                                lChar = tolower( getchar() ) ;
                                printf("\n\n");
                        }
                        while ( lChar != 'o' && lChar != 'c' );
                        lResult = lChar == 'o' ? 1 : 0 ;
                }
                else if ( aDialogType && !strcmp("yesnocancel",aDialogType) )
                {
                        do
                        {
                                if ( aMessage && strlen(aMessage) )
                                {
                                        printf("\n%s\n",aMessage);
                                }
                                printf("[Y]es/[N]o/[C]ancel: "); fflush(stdout);
                                lChar = tolower( getchar() ) ;
                                printf("\n\n");
                        }
                        while ( lChar != 'y' && lChar != 'n' && lChar != 'c' );
                        lResult = (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0 ;
                }
                else
                {
                        if ( aMessage && strlen(aMessage) )
                        {
                                printf("\n%s\n\n",aMessage);
                        }
                        printf("press enter to continue "); fflush(stdout);
                        getchar() ;
                        printf("\n\n");
                        lResult = 1 ;
                }
                tcsetattr(0, TCSANOW, &infoOri);
                free(lDialogString);
                return lResult ;
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;

        if ( ! ( lIn = popen( lDialogString , "r" ) ) )
        {
                free(lDialogString);
                return 0 ;
        }
        while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
        {}

        pclose( lIn ) ;

        /* printf( "lBuff: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */
        if ( lBuff[strlen( lBuff ) -1] == '\n' )
        {
                lBuff[strlen( lBuff ) -1] = '\0' ;
        }
        /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */

        if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
                if ( lBuff[0]=='1' )
                {
                        if ( !strcmp( lBuff+1 , "Yes" )) strcpy(lBuff,"1");
                        else if ( !strcmp( lBuff+1 , "No" )) strcpy(lBuff,"2");
                }
        }
        /* printf( "lBuff2: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */

        lResult =  !strcmp( lBuff , "2" ) ? 2 : !strcmp( lBuff , "1" ) ? 1 : 0;

        /* printf( "lResult: %d\n" , lResult ) ; */
        free(lDialogString);
        return lResult ;
}


/* return has only meaning for tinyfd_query */
int tinyfd_notifyPopup(
        char const * aTitle , /* NULL or "" */
        char const * aMessage , /* NULL or ""  may contain \n and \t */
        char const * aIconType ) /* "info" "warning" "error" */
{
    char lBuff[MAX_PATH_OR_CMD];
        char * lDialogString = NULL ;
    char * lpDialogString ;
        FILE * lIn ;
        size_t lTitleLen ;
        size_t lMessageLen ;

		if (tfd_quoteDetected(aTitle)) return tinyfd_notifyPopup("INVALID TITLE WITH QUOTES", aMessage, aIconType);
		if (tfd_quoteDetected(aMessage)) return tinyfd_notifyPopup(aTitle, "INVALID MESSAGE WITH QUOTES", aIconType);

        if ( getenv("SSH_TTY") )
        {
                return tinyfd_messageBox(aTitle, aMessage, "ok", aIconType, 0);
        }

        lTitleLen =  aTitle ? strlen(aTitle) : 0 ;
        lMessageLen =  aMessage ? strlen(aMessage) : 0 ;
        if ( !aTitle || strcmp(aTitle,"tinyfd_query") )
        {
                lDialogString = (char *) malloc( MAX_PATH_OR_CMD + lTitleLen + lMessageLen );
        }

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return 1;}

                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e 'display notification \"") ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage) ;
                }
                strcat(lDialogString, " \" ") ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "with title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                strcat( lDialogString, "' -e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return 1;}
                strcpy( lDialogString , "kdialog" ) ;

                if ( aIconType && strlen(aIconType) )
                {
                        strcat( lDialogString , " --icon '" ) ;
                        strcat( lDialogString , aIconType ) ;
                        strcat( lDialogString , "'" ) ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat( lDialogString , " --title \"" ) ;
                        strcat( lDialogString , aTitle ) ;
                        strcat( lDialogString , "\"" ) ;
                }

                strcat( lDialogString , " --passivepopup" ) ;
                strcat( lDialogString , " \"" ) ;
                if ( aMessage )
                {
                        strcat( lDialogString , aMessage ) ;
                }
                strcat( lDialogString , " \" 5" ) ;
        }
        else if ( (tfd_zenity3Present()>=5) )
        {
                /* zenity 2.32 & 3.14 has the notification but with a bug: it doesnt return from it */
                /* zenity 3.8 show the notification as an alert ok cancel box */
                if ( tfd_zenity3Present()>=5 )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return 1;}
                        strcpy( lDialogString , "zenity" ) ;
                }

                strcat( lDialogString , " --notification");

                if ( aIconType && strlen( aIconType ) )
                {
                        strcat( lDialogString , " --window-icon '");
                        strcat( lDialogString , aIconType ) ;
                        strcat( lDialogString , "'" ) ;
                }

                strcat( lDialogString , " --text \"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\n") ;
                }
                if ( aMessage && strlen( aMessage ) )
                {
                        strcat( lDialogString , aMessage ) ;
                }
                strcat( lDialogString , " \"" ) ;
        }
        else if ( perlPresent() >= 2 )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"perl-dbus");return 1;}

				strcpy( lDialogString , "perl -e \"use Net::DBus;\
my \\$sessionBus = Net::DBus->session;\
my \\$notificationsService = \\$sessionBus->get_service('org.freedesktop.Notifications');\
my \\$notificationsObject = \\$notificationsService->get_object('/org/freedesktop/Notifications',\
'org.freedesktop.Notifications');");

				sprintf( lDialogString + strlen(lDialogString) ,
"my \\$notificationId;\\$notificationId = \\$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);\" ",
aIconType?aIconType:"", aTitle?aTitle:"", aMessage?aMessage:"" ) ;
        }
        else if ( pythonDbusPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python-dbus");return 1;}
                strcpy( lDialogString , gPythonName ) ;
                strcat( lDialogString ," -c \"import dbus;bus=dbus.SessionBus();");
                strcat( lDialogString ,"notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');" ) ;
                strcat( lDialogString ,"notify=dbus.Interface(notif,'org.freedesktop.Notifications');" ) ;
                strcat( lDialogString ,"notify.Notify('',0,'" ) ;
                if ( aIconType && strlen(aIconType) )
                {
                        strcat( lDialogString , aIconType ) ;
                }
                strcat(lDialogString, "','") ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                }
                strcat(lDialogString, "','") ;
                if ( aMessage && strlen(aMessage) )
                {
                        lpDialogString = lDialogString + strlen(lDialogString);
                        tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
                }
                strcat(lDialogString, "','','',5000)\"") ;
        }
        else if ( notifysendPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"notifysend");return 1;}
                strcpy( lDialogString , "notify-send" ) ;
                if ( aIconType && strlen(aIconType) )
                {
                        strcat( lDialogString , " -i '" ) ;
                        strcat( lDialogString , aIconType ) ;
                        strcat( lDialogString , "'" ) ;
                }
        strcat( lDialogString , " \"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                        strcat( lDialogString , " | " ) ;
                }
                if ( aMessage && strlen(aMessage) )
                {
            tfd_replaceSubStr( aMessage , "\n\t" , " |  " , lBuff ) ;
            tfd_replaceSubStr( aMessage , "\n" , " | " , lBuff ) ;
            tfd_replaceSubStr( aMessage , "\t" , "  " , lBuff ) ;
                        strcat(lDialogString, lBuff) ;
                }
                strcat( lDialogString , "\"" ) ;
        }
        else
        {
                return tinyfd_messageBox(aTitle, aMessage, "ok", aIconType, 0);
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;

        if ( ! ( lIn = popen( lDialogString , "r" ) ) )
        {
                free(lDialogString);
                return 0 ;
        }

        pclose( lIn ) ;
        free(lDialogString);
        return 1;
}


/* returns NULL on cancel */
char * tinyfd_inputBox(
        char const * aTitle , /* NULL or "" */
        char const * aMessage , /* NULL or "" (\n and \t have no effect) */
        char const * aDefaultInput ) /* "" , if NULL it's a passwordBox */
{
        static char lBuff[MAX_PATH_OR_CMD];
        char * lDialogString = NULL;
        char * lpDialogString;
        FILE * lIn ;
        int lResult ;
        int lWasGdialog = 0 ;
        int lWasGraphicDialog = 0 ;
        int lWasXterm = 0 ;
        int lWasBasicXterm = 0 ;
        struct termios oldt ;
        struct termios newt ;
        char * lEOF;
        size_t lTitleLen ;
        size_t lMessageLen ;

		if (!aTitle && !aMessage && !aDefaultInput) return lBuff; /* now I can fill lBuff from outside */

        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_inputBox("INVALID TITLE WITH QUOTES", aMessage, aDefaultInput);
		if (tfd_quoteDetected(aMessage)) return tinyfd_inputBox(aTitle, "INVALID MESSAGE WITH QUOTES", aDefaultInput);
		if (tfd_quoteDetected(aDefaultInput)) return tinyfd_inputBox(aTitle, aMessage, "INVALID DEFAULT_INPUT WITH QUOTES");

        lTitleLen =  aTitle ? strlen(aTitle) : 0 ;
        lMessageLen =  aMessage ? strlen(aMessage) : 0 ;
        if ( !aTitle || strcmp(aTitle,"tinyfd_query") )
        {
                lDialogString = (char *) malloc( MAX_PATH_OR_CMD + lTitleLen + lMessageLen );
        }

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return (char *)1;}
                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e 'display dialog \"") ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage) ;
                }
                strcat(lDialogString, "\" ") ;
                strcat(lDialogString, "default answer \"") ;
                if ( aDefaultInput && strlen(aDefaultInput) )
                {
                        strcat(lDialogString, aDefaultInput) ;
                }
                strcat(lDialogString, "\" ") ;
                if ( ! aDefaultInput )
                {
                        strcat(lDialogString, "hidden answer true ") ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "with title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }
                strcat(lDialogString, "with icon note' ") ;
                strcat(lDialogString, "-e '\"1\" & text returned of result' " );
                strcat(lDialogString, "-e 'on error number -128' " ) ;
                strcat(lDialogString, "-e '0' " );
                strcat(lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat(lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return (char *)1;}
                strcpy( lDialogString , "szAnswer=$(kdialog" ) ;

				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }

                if ( ! aDefaultInput )
                {
                        strcat(lDialogString, " --password ") ;
                }
                else
                {
                        strcat(lDialogString, " --inputbox ") ;

                }
                strcat(lDialogString, "\"") ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage ) ;
                }
                strcat(lDialogString , "\" \"" ) ;
                if ( aDefaultInput && strlen(aDefaultInput) )
                {
                        strcat(lDialogString, aDefaultInput ) ;
                }
                strcat(lDialogString , "\"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }
                strcat( lDialogString ,
                        ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
        }
        else if ( tfd_zenityPresent() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                if ( tfd_zenityPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return (char *)1;}
                        strcpy( lDialogString , "szAnswer=$(zenity" ) ;
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat( lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return (char *)1;}
                        strcpy( lDialogString ,  "szAnswer=$(matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return (char *)1;}
                        strcpy( lDialogString , "szAnswer=$(shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return (char *)1;}
                        strcpy( lDialogString ,  "szAnswer=$(qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat( lDialogString ," --entry" ) ;

                strcat(lDialogString, " --title=\"") ;
                if (aTitle && strlen(aTitle)) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"") ;

                strcat(lDialogString, " --text=\"") ;
                if (aMessage && strlen(aMessage)) strcat(lDialogString, aMessage) ;
                strcat(lDialogString, "\"") ;

                if ( aDefaultInput && strlen(aDefaultInput) )
                {
                        strcat(lDialogString, " --entry-text=\"") ;
                        strcat(lDialogString, aDefaultInput) ;
                        strcat(lDialogString, "\"") ;
                }
                else
                {
                        strcat(lDialogString, " --hide-text") ;
                }
                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");
                strcat( lDialogString ,
                                ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
        }
        else if (tfd_yadPresent())
        {
           if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return (char*)1; }
           strcpy(lDialogString, "szAnswer=$(yad --entry");
           if (aTitle && strlen(aTitle))
           {
              strcat(lDialogString, " --title=\"");
              strcat(lDialogString, aTitle);
              strcat(lDialogString, "\"");
           }
           if (aMessage && strlen(aMessage))
           {
              strcat(lDialogString, " --text=\"");
              strcat(lDialogString, aMessage);
              strcat(lDialogString, "\"");
           }
           if (aDefaultInput && strlen(aDefaultInput))
           {
              strcat(lDialogString, " --entry-text=\"");
              strcat(lDialogString, aDefaultInput);
              strcat(lDialogString, "\"");
           }
           else
           {
              strcat(lDialogString, " --hide-text");
           }
           if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
           strcat(lDialogString,
              ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
        }
        else if ( gxmessagePresent() || gmessagePresent() )
        {
                if ( gxmessagePresent() ) {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gxmessage");return (char *)1;}
                        strcpy( lDialogString , "szAnswer=$(gxmessage -buttons Ok:1,Cancel:0 -center \"");
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gmessage");return (char *)1;}
                        strcpy( lDialogString , "szAnswer=$(gmessage -buttons Ok:1,Cancel:0 -center \"");
                }

                if ( aMessage && strlen(aMessage) )
                {
                        strcat( lDialogString , aMessage ) ;
                }
                strcat(lDialogString, "\"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat( lDialogString , " -title  \"");
                        strcat( lDialogString , aTitle ) ;
                        strcat(lDialogString, "\" " ) ;
                }
                strcat(lDialogString, " -entrytext \"" ) ;
                if ( aDefaultInput && strlen(aDefaultInput) )
                {
                        strcat( lDialogString , aDefaultInput ) ;
                }
                strcat(lDialogString, "\"" ) ;
                strcat( lDialogString , ");echo $?$szAnswer");
        }
		else if ( !gdialogPresent() && !xdialogPresent() && tkinter3Present( ) )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return (char *)1;}
			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter; from tkinter import simpledialog;root=tkinter.Tk();root.withdraw();");
			strcat( lDialogString ,"res=simpledialog.askstring(" ) ;
			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, "title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "',") ;
			}
			if ( aMessage && strlen(aMessage) )
			{

				strcat(lDialogString, "prompt='") ;
				lpDialogString = lDialogString + strlen(lDialogString);
				tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
				strcat(lDialogString, "',") ;
			}
			if ( aDefaultInput )
			{
				if ( strlen(aDefaultInput) )
				{
					strcat(lDialogString, "initialvalue='") ;
					strcat(lDialogString, aDefaultInput) ;
					strcat(lDialogString, "',") ;
				}
			}
			else
			{
				strcat(lDialogString, "show='*'") ;
			}
			strcat(lDialogString, ");\nif res is None :\n\tprint(0)");
			strcat(lDialogString, "\nelse :\n\tprint('1'+res)\n\"" ) ;
		}
		else if ( !gdialogPresent() && !xdialogPresent() && tkinter2Present( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return (char *)1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
				strcat( lDialogString , gPython2Name ) ;
				if ( ! isTerminalRunning( ) && tfd_isDarwin( ) )
                {
                strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }

				strcat( lDialogString ,
					" -S -c \"import Tkinter,tkSimpleDialog;root=Tkinter.Tk();root.withdraw();");

                if ( tfd_isDarwin( ) )
                {
                        strcat( lDialogString ,
"import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
                }

                strcat( lDialogString ,"res=tkSimpleDialog.askstring(" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "title='") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "',") ;
                }
                if ( aMessage && strlen(aMessage) )
                {

                        strcat(lDialogString, "prompt='") ;
                        lpDialogString = lDialogString + strlen(lDialogString);
                        tfd_replaceSubStr( aMessage , "\n" , "\\n" , lpDialogString ) ;
                        strcat(lDialogString, "',") ;
                }
                if ( aDefaultInput )
                {
                        if ( strlen(aDefaultInput) )
                        {
                                strcat(lDialogString, "initialvalue='") ;
                                strcat(lDialogString, aDefaultInput) ;
                                strcat(lDialogString, "',") ;
                        }
                }
                else
                {
                        strcat(lDialogString, "show='*'") ;
                }
                strcat(lDialogString, ");\nif res is None :\n\tprint 0");
                strcat(lDialogString, "\nelse :\n\tprint '1'+res\n\"" ) ;
        }
        else if ( gdialogPresent() || xdialogPresent() || dialogName() || whiptailPresent() )
        {
                if ( gdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"gdialog");return (char *)1;}
                        lWasGraphicDialog = 1 ;
                        lWasGdialog = 1 ;
                        strcpy( lDialogString , "(gdialog " ) ;
                }
                else if ( xdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return (char *)1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(Xdialog " ) ;
                }
                else if ( dialogName( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        if ( isTerminalRunning( ) )
                        {
                                strcpy( lDialogString , "(dialog " ) ;
                        }
                        else
                        {
                                lWasXterm = 1 ;
                                strcpy( lDialogString , terminalName() ) ;
                                strcat( lDialogString , "'(" ) ;
                                strcat( lDialogString , dialogName() ) ;
                                strcat( lDialogString , " " ) ;
                        }
                }
                else if ( isTerminalRunning( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"whiptail");return (char *)0;}
                        strcpy( lDialogString , "(whiptail " ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"whiptail");return (char *)0;}
                        lWasXterm = 1 ;
                        strcpy( lDialogString , terminalName() ) ;
                        strcat( lDialogString , "'(whiptail " ) ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "--title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                if ( !xdialogPresent() && !gdialogPresent() )
                {
                        strcat(lDialogString, "--backtitle \"") ;
                        strcat(lDialogString, "tab: move focus") ;
                        if ( ! aDefaultInput && !lWasGdialog )
                        {
                                strcat(lDialogString, " (sometimes nothing, no blink nor star, is shown in text field)") ;
                        }
                        strcat(lDialogString, "\" ") ;
                }

                if ( aDefaultInput || lWasGdialog )
                {
                        strcat( lDialogString , "--inputbox" ) ;
                }
                else
                {
                        if ( !lWasGraphicDialog && dialogName() && isDialogVersionBetter09b() )
                        {
                                strcat( lDialogString , "--insecure " ) ;
                        }
                        strcat( lDialogString , "--passwordbox" ) ;
                }
                strcat( lDialogString , " \"" ) ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat(lDialogString, aMessage) ;
                }
                strcat(lDialogString,"\" 10 60 ") ;
                if ( aDefaultInput && strlen(aDefaultInput) )
                {
                        strcat(lDialogString, "\"") ;
                        strcat(lDialogString, aDefaultInput) ;
                        strcat(lDialogString, "\" ") ;
                }
                if ( lWasGraphicDialog )
                {
                        strcat(lDialogString,") 2>/tmp/tinyfd.txt;\
        if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
        tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes") ;
                }
                else
                {
                        strcat(lDialogString,">/dev/tty ) 2>/tmp/tinyfd.txt;\
        if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
        tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes") ;

                        if ( lWasXterm )
                        {
                strcat(lDialogString," >/tmp/tinyfd0.txt';cat /tmp/tinyfd0.txt");
                        }
                        else
                        {
                                strcat(lDialogString, "; clear >/dev/tty") ;
                        }
                }
        }
        else if ( ! isTerminalRunning( ) && terminalName() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"basicinput");return (char *)0;}
                lWasBasicXterm = 1 ;
                strcpy( lDialogString , terminalName() ) ;
                strcat( lDialogString , "'" ) ;
                if ( !gWarningDisplayed && !tinyfd_forceConsole)
                {
					gWarningDisplayed = 1 ;
					tinyfd_messageBox(gTitle,tinyfd_needs,"ok","warning",0);
                }
                if ( aTitle && strlen(aTitle) && !tinyfd_forceConsole)
                {
                        strcat( lDialogString , "echo \"" ) ;
                        strcat( lDialogString, aTitle) ;
                        strcat( lDialogString , "\";echo;" ) ;
                }

                strcat( lDialogString , "echo \"" ) ;
                if ( aMessage && strlen(aMessage) )
                {
                        strcat( lDialogString, aMessage) ;
                }
                strcat( lDialogString , "\";read " ) ;
                if ( ! aDefaultInput )
                {
                        strcat( lDialogString , "-s " ) ;
                }
                strcat( lDialogString , "-p \"" ) ;
                strcat( lDialogString , "(esc+enter to cancel): \" ANSWER " ) ;
                strcat( lDialogString , ";echo 1$ANSWER >/tmp/tinyfd.txt';" ) ;
                strcat( lDialogString , "cat -v /tmp/tinyfd.txt");
        }
        else if ( !gWarningDisplayed && ! isTerminalRunning( ) && ! terminalName() ) {
			gWarningDisplayed = 1 ;
			tinyfd_messageBox(gTitle,tinyfd_needs,"ok","warning",0);
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"no_solution");return (char *)0;}
			free(lDialogString);
			return NULL;
        }
        else
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"basicinput");return (char *)0;}
                if ( !gWarningDisplayed && !tinyfd_forceConsole)
                {
                        gWarningDisplayed = 1 ;
                        tinyfd_messageBox(gTitle,tinyfd_needs,"ok","warning",0);
                }
                if ( aTitle && strlen(aTitle) )
                {
                        printf("\n%s\n", aTitle);
                }
                if ( aMessage && strlen(aMessage) )
                {
                        printf("\n%s\n",aMessage);
                }
                printf("(esc+enter to cancel): "); fflush(stdout);
                if ( ! aDefaultInput )
                {
                        tcgetattr(STDIN_FILENO, & oldt) ;
                        newt = oldt ;
                        newt.c_lflag &= ~ECHO ;
                        tcsetattr(STDIN_FILENO, TCSANOW, & newt);
                }

                lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
                /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
                if ( ! lEOF  || (lBuff[0] == '\0') )
                {
                        free(lDialogString);
                        return NULL;
                }

                if ( lBuff[0] == '\n' )
                {
                        lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
                        /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
                        if ( ! lEOF  || (lBuff[0] == '\0') )
                        {
                                free(lDialogString);
                                return NULL;
                        }
                }

                if ( ! aDefaultInput )
                {
                        tcsetattr(STDIN_FILENO, TCSANOW, & oldt);
                        printf("\n");
                }
                printf("\n");
                if ( strchr(lBuff,27) )
                {
                        free(lDialogString);
                        return NULL ;
                }
                if ( lBuff[strlen( lBuff ) -1] == '\n' )
                {
                        lBuff[strlen( lBuff ) -1] = '\0' ;
                }
                free(lDialogString);
                return lBuff ;
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;
        lIn = popen( lDialogString , "r" );
        if ( ! lIn  )
        {
                if ( fileExists("/tmp/tinyfd.txt") )
                {
                        wipefile("/tmp/tinyfd.txt");
                        remove("/tmp/tinyfd.txt");
                }
                if ( fileExists("/tmp/tinyfd0.txt") )
                {
                        wipefile("/tmp/tinyfd0.txt");
                        remove("/tmp/tinyfd0.txt");
                }
                free(lDialogString);
                return NULL ;
        }
        while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
        {}

        pclose( lIn ) ;

        if ( fileExists("/tmp/tinyfd.txt") )
        {
                wipefile("/tmp/tinyfd.txt");
                remove("/tmp/tinyfd.txt");
        }
        if ( fileExists("/tmp/tinyfd0.txt") )
        {
                wipefile("/tmp/tinyfd0.txt");
                remove("/tmp/tinyfd0.txt");
        }

        /* printf( "len Buff: %lu\n" , strlen(lBuff) ) ; */
        /* printf( "lBuff0: %s\n" , lBuff ) ; */
        if ( lBuff[strlen( lBuff ) -1] == '\n' )
        {
                lBuff[strlen( lBuff ) -1] = '\0' ;
        }
        /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */
        if ( lWasBasicXterm )
        {
                if ( strstr(lBuff,"^[") ) /* esc was pressed */
                {
                        free(lDialogString);
                        return NULL ;
                }
        }

        lResult =  strncmp( lBuff , "1" , 1) ? 0 : 1 ;
        /* printf( "lResult: %d \n" , lResult ) ; */
        if ( ! lResult )
        {
                free(lDialogString);
                return NULL ;
        }

        /* printf( "lBuff+1: %s\n" , lBuff+1 ) ; */
        free(lDialogString);
        return lBuff+1 ;
}


char * tinyfd_saveFileDialog(
    char const * aTitle , /* NULL or "" */
    char const * aDefaultPathAndFile , /* NULL or "" */
    int aNumOfFilterPatterns , /* 0 */
    char const * const * aFilterPatterns , /* NULL or {"*.jpg","*.png"} */
    char const * aSingleFilterDescription ) /* NULL or "image files" */
{
        static char lBuff[MAX_PATH_OR_CMD] ;
        char lDialogString[MAX_PATH_OR_CMD] ;
        char lString[MAX_PATH_OR_CMD] ;
        int i ;
        int lWasGraphicDialog = 0 ;
        int lWasXterm = 0 ;
        char * p ;
		char * lPointerInputBox ;
        FILE * lIn ;
        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_saveFileDialog("INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (tfd_quoteDetected(aDefaultPathAndFile)) return tinyfd_saveFileDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
		if (tfd_quoteDetected(aSingleFilterDescription)) return tinyfd_saveFileDialog(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, "INVALID FILTER_DESCRIPTION WITH QUOTES");
		for (i = 0; i < aNumOfFilterPatterns; i++)
		{
			if (tfd_quoteDetected(aFilterPatterns[i])) return tinyfd_saveFileDialog("INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL);
		}

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return (char *)1;}
                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"Finder\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e 'POSIX path of ( choose file name " );
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "with prompt \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }
                getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
                if ( strlen(lString) )
                {
                        strcat(lDialogString, "default location \"") ;
                        strcat(lDialogString, lString ) ;
                        strcat(lDialogString , "\" " ) ;
                }
                getLastName( lString , aDefaultPathAndFile ) ;
                if ( strlen(lString) )
                {
                        strcat(lDialogString, "default name \"") ;
                        strcat(lDialogString, lString ) ;
                        strcat(lDialogString , "\" " ) ;
                }
                strcat( lDialogString , ")' " ) ;
                strcat(lDialogString, "-e 'on error number -128' " ) ;
                strcat(lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return (char *)1;}

                strcpy( lDialogString , "kdialog" ) ;
				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }
                strcat( lDialogString , " --getsavefilename " ) ;

                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        if ( aDefaultPathAndFile[0] != '/' )
                        {
                                strcat(lDialogString, "$PWD/") ;
                        }
                        strcat(lDialogString, "\"") ;
                        strcat(lDialogString, aDefaultPathAndFile ) ;
                        strcat(lDialogString , "\"" ) ;
                }
                else
                {
                        strcat(lDialogString, "$PWD/") ;
                }

                if ( aNumOfFilterPatterns > 0 )
                {
                        strcat(lDialogString , " \"" ) ;
						strcat( lDialogString , aFilterPatterns[0] ) ;
                        for ( i = 1 ; i < aNumOfFilterPatterns ; i ++ )
                        {
							strcat( lDialogString , " " ) ;
							strcat( lDialogString , aFilterPatterns[i] ) ;
                        }
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , " | " ) ;
                                strcat( lDialogString , aSingleFilterDescription ) ;
                        }
                        strcat( lDialogString , "\"" ) ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }
        }
        else if ( tfd_zenityPresent() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                if ( tfd_zenityPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return (char *)1;}
                        strcpy( lDialogString , "zenity" ) ;
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat( lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return (char *)1;}
                        strcpy( lDialogString , "matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return (char *)1;}
                        strcpy( lDialogString , "shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return (char *)1;}
                        strcpy( lDialogString , "qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat(lDialogString, " --file-selection --save --confirm-overwrite" ) ;

                strcat(lDialogString, " --title=\"") ;
                if (aTitle && strlen(aTitle)) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"") ;

                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        strcat(lDialogString, " --filename=\"") ;
                        strcat(lDialogString, aDefaultPathAndFile) ;
                        strcat(lDialogString, "\"") ;
                }
                if ( aNumOfFilterPatterns > 0 )
                {
                        strcat( lDialogString , " --file-filter='" ) ;
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , aSingleFilterDescription ) ;
                                strcat( lDialogString , " |" ) ;
                        }
                        for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
                        {
							strcat( lDialogString , " " ) ;
							strcat( lDialogString , aFilterPatterns[i] ) ;
                        }
                        strcat( lDialogString , "' --file-filter='All files | *'" ) ;
                }
                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");
        }
        else if (tfd_yadPresent())
        {
           if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return (char*)1; }
           strcpy(lDialogString, "yad --file-selection --save --confirm-overwrite");
           if (aTitle && strlen(aTitle))
           {
              strcat(lDialogString, " --title=\"");
              strcat(lDialogString, aTitle);
              strcat(lDialogString, "\"");
           }
           if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
           {
              strcat(lDialogString, " --filename=\"");
              strcat(lDialogString, aDefaultPathAndFile);
              strcat(lDialogString, "\"");
           }
           if (aNumOfFilterPatterns > 0)
           {
              strcat(lDialogString, " --file-filter='");
              if (aSingleFilterDescription && strlen(aSingleFilterDescription))
              {
                 strcat(lDialogString, aSingleFilterDescription);
                 strcat(lDialogString, " |");
              }
              for (i = 0; i < aNumOfFilterPatterns; i++)
              {
                 strcat(lDialogString, " ");
                 strcat(lDialogString, aFilterPatterns[i]);
              }
              strcat(lDialogString, "' --file-filter='All files | *'");
           }
           if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
      }
      else if ( !xdialogPresent() && tkinter3Present( ) )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return (char *)1;}
			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
			strcat( lDialogString , "res=filedialog.asksaveasfilename(");
			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, "title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "',") ;
			}
			if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
			{
				getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
				if ( strlen(lString) )
				{
					strcat(lDialogString, "initialdir='") ;
					strcat(lDialogString, lString ) ;
					strcat(lDialogString , "'," ) ;
				}
				getLastName( lString , aDefaultPathAndFile ) ;
				if ( strlen(lString) )
				{
					strcat(lDialogString, "initialfile='") ;
					strcat(lDialogString, lString ) ;
					strcat(lDialogString , "'," ) ;
				}
			}
			if ( ( aNumOfFilterPatterns > 1 )
				|| ( (aNumOfFilterPatterns == 1) /* test because poor osx behaviour */
				&& ( aFilterPatterns[0][strlen(aFilterPatterns[0])-1] != '*' ) ) )
			{
				strcat(lDialogString , "filetypes=(" ) ;
				strcat( lDialogString , "('" ) ;
				if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
				{
					strcat( lDialogString , aSingleFilterDescription ) ;
				}
				strcat( lDialogString , "',(" ) ;
				for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
				{
					strcat( lDialogString , "'" ) ;
					strcat( lDialogString , aFilterPatterns[i] ) ;
					strcat( lDialogString , "'," ) ;
				}
				strcat( lDialogString , "))," ) ;
				strcat( lDialogString , "('All files','*'))" ) ;
			}
			strcat( lDialogString, ");\nif not isinstance(res, tuple):\n\tprint(res)\n\"" ) ;
		}
		else if ( !xdialogPresent() && tkinter2Present( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return (char *)1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
				strcat( lDialogString , gPython2Name ) ;
				if ( ! isTerminalRunning( ) && tfd_isDarwin( ))
                {
                strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }
            strcat( lDialogString ,
" -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if ( tfd_isDarwin( ) )
        {
                        strcat( lDialogString ,
"import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set\
 frontmost of process \\\"Python\\\" to true' ''');");
                }

                strcat( lDialogString , "res=tkFileDialog.asksaveasfilename(");
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "title='") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "',") ;
                }
            if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
            {
                        getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
                        if ( strlen(lString) )
                        {
                                strcat(lDialogString, "initialdir='") ;
                                strcat(lDialogString, lString ) ;
                                strcat(lDialogString , "'," ) ;
                        }
                        getLastName( lString , aDefaultPathAndFile ) ;
                        if ( strlen(lString) )
                        {
                                strcat(lDialogString, "initialfile='") ;
                                strcat(lDialogString, lString ) ;
                                strcat(lDialogString , "'," ) ;
                        }
                }
            if ( ( aNumOfFilterPatterns > 1 )
                  || ( (aNumOfFilterPatterns == 1) /* test because poor osx behaviour */
                        && ( aFilterPatterns[0][strlen(aFilterPatterns[0])-1] != '*' ) ) )
            {
                        strcat(lDialogString , "filetypes=(" ) ;
                        strcat( lDialogString , "('" ) ;
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , aSingleFilterDescription ) ;
                        }
                        strcat( lDialogString , "',(" ) ;
                        for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
                        {
                                strcat( lDialogString , "'" ) ;
                                strcat( lDialogString , aFilterPatterns[i] ) ;
                                strcat( lDialogString , "'," ) ;
                        }
                        strcat( lDialogString , "))," ) ;
                        strcat( lDialogString , "('All files','*'))" ) ;
            }
			strcat( lDialogString, ");\nif not isinstance(res, tuple):\n\tprint res \n\"" ) ;
		}
        else if ( xdialogPresent() || dialogName() )
        {
                if ( xdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return (char *)1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(Xdialog " ) ;
                }
                else if ( isTerminalRunning( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        strcpy( lDialogString , "(dialog " ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        lWasXterm = 1 ;
                        strcpy( lDialogString , terminalName() ) ;
                        strcat( lDialogString , "'(" ) ;
                        strcat( lDialogString , dialogName() ) ;
                        strcat( lDialogString , " " ) ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "--title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                if ( !xdialogPresent() && !gdialogPresent() )
                {
                        strcat(lDialogString, "--backtitle \"") ;
                        strcat(lDialogString,
                                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
                        strcat(lDialogString, "\" ") ;
                }

                strcat( lDialogString , "--fselect \"" ) ;
                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        if ( ! strchr(aDefaultPathAndFile, '/') )
                        {
                                strcat(lDialogString, "./") ;
                        }
                        strcat(lDialogString, aDefaultPathAndFile) ;
                }
                else if ( ! isTerminalRunning( ) && !lWasGraphicDialog )
                {
                        strcat(lDialogString, getenv("HOME")) ;
                        strcat(lDialogString, "/") ;
                }
                else
                {
                        strcat(lDialogString, "./") ;
                }

                if ( lWasGraphicDialog )
                {
                        strcat(lDialogString, "\" 0 60 ) 2>&1 ") ;
                }
                else
                {
                        strcat(lDialogString, "\" 0 60  >/dev/tty) ") ;
                        if ( lWasXterm )
                        {
                          strcat( lDialogString ,
                                "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
                        }
                        else
                        {
                                strcat(lDialogString, "2>&1 ; clear >/dev/tty") ;
                        }
                }
        }
        else
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){return tinyfd_inputBox(aTitle,NULL,NULL);}
				strcpy(lBuff, "Save file in ");
				strcat(lBuff, getCurDir());
				lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
				if (lPointerInputBox) strcpy(lString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
				p = tinyfd_inputBox(aTitle, lBuff, "");
				if (p) strcpy(lBuff, p); else lBuff[0] = '\0';
				if (lPointerInputBox) strcpy(lPointerInputBox, lString); /* restore its previous content to tinyfd_inputBox */
				p = lBuff;

				getPathWithoutFinalSlash( lString , p ) ;
                if ( strlen( lString ) && ! dirExists( lString ) )
                {
                        return NULL ;
                }
                getLastName(lString,p);
                if ( ! strlen(lString) )
                {
                        return NULL;
                }
                return p ;
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;
    if ( ! ( lIn = popen( lDialogString , "r" ) ) )
    {
        return NULL ;
    }
    while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
    {}
    pclose( lIn ) ;
    if ( lBuff[strlen( lBuff ) -1] == '\n' )
    {
        lBuff[strlen( lBuff ) -1] = '\0' ;
    }
        /* printf( "lBuff: %s\n" , lBuff ) ; */
        if ( ! strlen(lBuff) )
        {
                return NULL;
        }
    getPathWithoutFinalSlash( lString , lBuff ) ;
    if ( strlen( lString ) && ! dirExists( lString ) )
    {
        return NULL ;
    }
        getLastName(lString,lBuff);
        if ( ! filenameValid(lString) )
        {
                return NULL;
        }
    return lBuff ;
}


/* in case of multiple files, the separator is | */
char * tinyfd_openFileDialog(
	char const * aTitle , /* NULL or "" */
	char const * aDefaultPathAndFile , /* NULL or "" */
    int aNumOfFilterPatterns , /* 0 */
	char const * const * aFilterPatterns , /* NULL or {"*.jpg","*.png"} */
    char const * aSingleFilterDescription , /* NULL or "image files" */
    int aAllowMultipleSelects ) /* 0 or 1 */
{
      char lDialogString[MAX_PATH_OR_CMD] ;
      char lString[MAX_PATH_OR_CMD] ;
      int i ;
      FILE * lIn ;
      char * p ;
      char * lPointerInputBox ;
      int lWasKdialog = 0 ;
      int lWasGraphicDialog = 0 ;
      int lWasXterm = 0 ;
      size_t lFullBuffLen ;
      static char * lBuff = NULL;

		if (tfd_quoteDetected(aTitle)) return tinyfd_openFileDialog("INVALID TITLE WITH QUOTES", aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
		if (tfd_quoteDetected(aDefaultPathAndFile)) return tinyfd_openFileDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES", aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
		if (tfd_quoteDetected(aSingleFilterDescription)) return tinyfd_openFileDialog(aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, "INVALID FILTER_DESCRIPTION WITH QUOTES", aAllowMultipleSelects);
		for (i = 0; i < aNumOfFilterPatterns; i++)
		{
			if (tfd_quoteDetected(aFilterPatterns[i])) return tinyfd_openFileDialog("INVALID FILTER_PATTERN WITH QUOTES", aDefaultPathAndFile, 0, NULL, NULL, aAllowMultipleSelects);
		}

		free(lBuff);
		if (aTitle&&!strcmp(aTitle,"tinyfd_query"))
		{
			lBuff = NULL;
		}
		else
		{
			if (aAllowMultipleSelects)
			{
				lFullBuffLen = MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD + 1;
				lBuff = (char *)(malloc(lFullBuffLen * sizeof(char)));
				if (!lBuff)
				{
					lFullBuffLen = LOW_MULTIPLE_FILES * MAX_PATH_OR_CMD + 1;
					lBuff = (char *)( malloc( lFullBuffLen * sizeof(char)));
				}
			}
			else
			{
				lFullBuffLen = MAX_PATH_OR_CMD + 1;
				lBuff = (char *)(malloc(lFullBuffLen * sizeof(char)));
			}
			if (!lBuff) return NULL;
			lBuff[0]='\0';
		}

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return (char *)1;}
                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e '" );
    if ( ! aAllowMultipleSelects )
    {


                        strcat( lDialogString , "POSIX path of ( " );
                }
                else
                {
                        strcat( lDialogString , "set mylist to " );
                }
                strcat( lDialogString , "choose file " );
            if ( aTitle && strlen(aTitle) )
            {
                        strcat(lDialogString, "with prompt \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
            }
                getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
                if ( strlen(lString) )
                {
                        strcat(lDialogString, "default location \"") ;
                        strcat(lDialogString, lString ) ;
                        strcat(lDialogString , "\" " ) ;
                }
                if ( aNumOfFilterPatterns > 0 )
                {
                        strcat(lDialogString , "of type {\"" );
                        strcat( lDialogString , aFilterPatterns[0] + 2 ) ;
                        strcat( lDialogString , "\"" ) ;
                        for ( i = 1 ; i < aNumOfFilterPatterns ; i ++ )
                        {
                                strcat( lDialogString , ",\"" ) ;
                                strcat( lDialogString , aFilterPatterns[i] + 2) ;
                                strcat( lDialogString , "\"" ) ;
                        }
                        strcat( lDialogString , "} " ) ;
                }
                if ( aAllowMultipleSelects )
                {
                        strcat( lDialogString , "multiple selections allowed true ' " ) ;
                        strcat( lDialogString ,
                                        "-e 'set mystring to POSIX path of item 1 of mylist' " );
                        strcat( lDialogString ,
                                        "-e 'repeat with  i from 2 to the count of mylist' " );
                        strcat( lDialogString , "-e 'set mystring to mystring & \"|\"' " );
                        strcat( lDialogString ,
                        "-e 'set mystring to mystring & POSIX path of item i of mylist' " );
                        strcat( lDialogString , "-e 'end repeat' " );
                        strcat( lDialogString , "-e 'mystring' " );
                }
                else
                {
                        strcat( lDialogString , ")' " ) ;
                }
                strcat(lDialogString, "-e 'on error number -128' " ) ;
                strcat(lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return (char *)1;}
                lWasKdialog = 1 ;

                strcpy( lDialogString , "kdialog" ) ;
				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }
                strcat( lDialogString , " --getopenfilename " ) ;

                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        if ( aDefaultPathAndFile[0] != '/' )
                        {
                                strcat(lDialogString, "$PWD/") ;
                        }
                        strcat(lDialogString, "\"") ;
                        strcat(lDialogString, aDefaultPathAndFile ) ;
                        strcat(lDialogString , "\"" ) ;
                }
                else
                {
                        strcat(lDialogString, "$PWD/") ;
                }

                if ( aNumOfFilterPatterns > 0 )
                {
                        strcat(lDialogString , " \"" ) ;
						strcat( lDialogString , aFilterPatterns[0] ) ;
                        for ( i = 1 ; i < aNumOfFilterPatterns ; i ++ )
                        {
							strcat( lDialogString , " " ) ;
							strcat( lDialogString , aFilterPatterns[i] ) ;
                        }
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , " | " ) ;
                                strcat( lDialogString , aSingleFilterDescription ) ;
                        }
                        strcat( lDialogString , "\"" ) ;
                }
                if ( aAllowMultipleSelects )
                {
                        strcat( lDialogString , " --multiple --separate-output" ) ;
                }
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }
        }
        else if ( tfd_zenityPresent() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                if ( tfd_zenityPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return (char *)1;}
                        strcpy( lDialogString , "zenity" ) ;
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat( lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return (char *)1;}
                        strcpy( lDialogString , "matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return (char *)1;}
                        strcpy( lDialogString , "shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return (char *)1;}
                        strcpy( lDialogString , "qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat( lDialogString , " --file-selection" ) ;

                if ( aAllowMultipleSelects )
                {
                        strcat( lDialogString , " --multiple" ) ;
                }
                
                strcat(lDialogString, " --title=\"") ;
                if (aTitle && strlen(aTitle)) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"") ;

                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        strcat(lDialogString, " --filename=\"") ;
                        strcat(lDialogString, aDefaultPathAndFile) ;
                        strcat(lDialogString, "\"") ;
                }
                if ( aNumOfFilterPatterns > 0 )
                {
                        strcat( lDialogString , " --file-filter='" ) ;
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , aSingleFilterDescription ) ;
                                strcat( lDialogString , " |" ) ;
                        }
                        for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
                        {
							strcat( lDialogString , " " ) ;
							strcat( lDialogString , aFilterPatterns[i] ) ;
                        }
                        strcat( lDialogString , "' --file-filter='All files | *'" ) ;
                }
                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");
        }
        else if (tfd_yadPresent())
        {
           if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return (char*)1; }
           strcpy(lDialogString, "yad --file-selection");
           if (aAllowMultipleSelects)
           {
              strcat(lDialogString, " --multiple");
           }
           if (aTitle && strlen(aTitle))
           {
              strcat(lDialogString, " --title=\"");
              strcat(lDialogString, aTitle);
              strcat(lDialogString, "\"");
           }
           if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
           {
              strcat(lDialogString, " --filename=\"");
              strcat(lDialogString, aDefaultPathAndFile);
              strcat(lDialogString, "\"");
           }
           if (aNumOfFilterPatterns > 0)
           {
              strcat(lDialogString, " --file-filter='");
              if (aSingleFilterDescription && strlen(aSingleFilterDescription))
              {
                 strcat(lDialogString, aSingleFilterDescription);
                 strcat(lDialogString, " |");
              }
              for (i = 0; i < aNumOfFilterPatterns; i++)
              {
                 strcat(lDialogString, " ");
                 strcat(lDialogString, aFilterPatterns[i]);
              }
              strcat(lDialogString, "' --file-filter='All files | *'");
           }
           if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
      }
      else if ( tkinter3Present( ) )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return (char *)1;}
			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
			strcat( lDialogString , "lFiles=filedialog.askopenfilename(");
			if ( aAllowMultipleSelects )
			{
				strcat( lDialogString , "multiple=1," ) ;
			}
			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, "title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "',") ;
			}
			if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
			{
				getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
				if ( strlen(lString) )
				{
					strcat(lDialogString, "initialdir='") ;
					strcat(lDialogString, lString ) ;
					strcat(lDialogString , "'," ) ;
				}
				getLastName( lString , aDefaultPathAndFile ) ;
				if ( strlen(lString) )
				{
					strcat(lDialogString, "initialfile='") ;
					strcat(lDialogString, lString ) ;
					strcat(lDialogString , "'," ) ;
				}
			}
			if ( ( aNumOfFilterPatterns > 1 )
				|| ( ( aNumOfFilterPatterns == 1 ) /*test because poor osx behaviour*/
				&& ( aFilterPatterns[0][strlen(aFilterPatterns[0])-1] != '*' ) ) )
			{
				strcat(lDialogString , "filetypes=(" ) ;
				strcat( lDialogString , "('" ) ;
				if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
				{
					strcat( lDialogString , aSingleFilterDescription ) ;
				}
				strcat( lDialogString , "',(" ) ;
				for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
				{
					strcat( lDialogString , "'" ) ;
					strcat( lDialogString , aFilterPatterns[i] ) ;
					strcat( lDialogString , "'," ) ;
				}
				strcat( lDialogString , "))," ) ;
				strcat( lDialogString , "('All files','*'))" ) ;
			}
			strcat( lDialogString , ");\
\nif not isinstance(lFiles, tuple):\n\tprint(lFiles)\nelse:\
\n\tlFilesString=''\n\tfor lFile in lFiles:\n\t\tlFilesString+=str(lFile)+'|'\
\n\tprint(lFilesString[:-1])\n\"" ) ;
		}
		else if ( tkinter2Present( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return (char *)1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
				strcat( lDialogString , gPython2Name ) ;
				if ( ! isTerminalRunning( ) && tfd_isDarwin( ) )
                {
                strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }
                strcat( lDialogString ,
" -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if ( tfd_isDarwin( ) )
        {
                        strcat( lDialogString ,
"import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
                }
                strcat( lDialogString , "lFiles=tkFileDialog.askopenfilename(");
    if ( aAllowMultipleSelects )
    {
                        strcat( lDialogString , "multiple=1," ) ;
    }
    if ( aTitle && strlen(aTitle) )
    {
                        strcat(lDialogString, "title='") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "',") ;
    }
    if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
    {
                        getPathWithoutFinalSlash( lString , aDefaultPathAndFile ) ;
                        if ( strlen(lString) )
                        {
                                strcat(lDialogString, "initialdir='") ;
                                strcat(lDialogString, lString ) ;
                                strcat(lDialogString , "'," ) ;
                        }
                        getLastName( lString , aDefaultPathAndFile ) ;
                        if ( strlen(lString) )
                        {
                                strcat(lDialogString, "initialfile='") ;
                                strcat(lDialogString, lString ) ;
                                strcat(lDialogString , "'," ) ;
                        }
                }
                if ( ( aNumOfFilterPatterns > 1 )
                        || ( ( aNumOfFilterPatterns == 1 ) /*test because poor osx behaviour*/
                                && ( aFilterPatterns[0][strlen(aFilterPatterns[0])-1] != '*' ) ) )
                {
                        strcat(lDialogString , "filetypes=(" ) ;
                        strcat( lDialogString , "('" ) ;
                        if ( aSingleFilterDescription && strlen(aSingleFilterDescription) )
                        {
                                strcat( lDialogString , aSingleFilterDescription ) ;
                        }
                        strcat( lDialogString , "',(" ) ;
                        for ( i = 0 ; i < aNumOfFilterPatterns ; i ++ )
                        {
                                strcat( lDialogString , "'" ) ;
                                strcat( lDialogString , aFilterPatterns[i] ) ;
                                strcat( lDialogString , "'," ) ;
                        }
                        strcat( lDialogString , "))," ) ;
                        strcat( lDialogString , "('All files','*'))" ) ;
                }
                strcat( lDialogString , ");\
\nif not isinstance(lFiles, tuple):\n\tprint lFiles\nelse:\
\n\tlFilesString=''\n\tfor lFile in lFiles:\n\t\tlFilesString+=str(lFile)+'|'\
\n\tprint lFilesString[:-1]\n\"" ) ;
        }
        else if ( xdialogPresent() || dialogName() )
        {
                if ( xdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return (char *)1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(Xdialog " ) ;
                }
                else if ( isTerminalRunning( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        strcpy( lDialogString , "(dialog " ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        lWasXterm = 1 ;
                        strcpy( lDialogString , terminalName() ) ;
                        strcat( lDialogString , "'(" ) ;
                        strcat( lDialogString , dialogName() ) ;
                        strcat( lDialogString , " " ) ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "--title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                if ( !xdialogPresent() && !gdialogPresent() )
                {
                        strcat(lDialogString, "--backtitle \"") ;
                        strcat(lDialogString,
                                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
                        strcat(lDialogString, "\" ") ;
                }

                strcat( lDialogString , "--fselect \"" ) ;
                if ( aDefaultPathAndFile && strlen(aDefaultPathAndFile) )
                {
                        if ( ! strchr(aDefaultPathAndFile, '/') )
                        {
                                strcat(lDialogString, "./") ;
                        }
                        strcat(lDialogString, aDefaultPathAndFile) ;
                }
                else if ( ! isTerminalRunning( ) && !lWasGraphicDialog )
                {
                        strcat(lDialogString, getenv("HOME")) ;
                        strcat(lDialogString, "/");
                }
                else
                {
                        strcat(lDialogString, "./") ;
                }

                if ( lWasGraphicDialog )
                {
                        strcat(lDialogString, "\" 0 60 ) 2>&1 ") ;
                }
                else
                {
                        strcat(lDialogString, "\" 0 60  >/dev/tty) ") ;
                        if ( lWasXterm )
                        {
                                strcat( lDialogString ,
                                "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
                        }
                        else
                        {
                                strcat(lDialogString, "2>&1 ; clear >/dev/tty") ;
                        }
                }
        }
        else
        {
            if (aTitle&&!strcmp(aTitle,"tinyfd_query")){return tinyfd_inputBox(aTitle,NULL,NULL);}
				strcpy(lBuff, "Open file from ");
				strcat(lBuff, getCurDir());
				lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
				if (lPointerInputBox) strcpy(lDialogString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
				p = tinyfd_inputBox(aTitle, lBuff, "");
				if ( p ) strcpy(lBuff, p); else lBuff[0] = '\0';
				if (lPointerInputBox) strcpy(lPointerInputBox, lDialogString); /* restore its previous content to tinyfd_inputBox */
            if ( ! fileExists(lBuff) )
            {
					free(lBuff);
					lBuff = NULL;
            }
				else
				{
					lBuff = (char *)( realloc( lBuff, (strlen(lBuff)+1) * sizeof(char)));
				}
				return lBuff ;
        }

    if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;
    if ( ! ( lIn = popen( lDialogString , "r" ) ) )
    {
		free(lBuff);
		lBuff = NULL;
		return NULL ;
    }
        lBuff[0]='\0';
        p = lBuff;
        while ( fgets( p , sizeof( lBuff ) , lIn ) != NULL )
        {
                p += strlen( p );
        }
    pclose( lIn ) ;
    if ( lBuff[strlen( lBuff ) -1] == '\n' )
    {
        lBuff[strlen( lBuff ) -1] = '\0' ;
    }
    /* printf( "lBuff: %s\n" , lBuff ) ; */
        if ( lWasKdialog && aAllowMultipleSelects )
        {
                p = lBuff ;
                while ( ( p = strchr( p , '\n' ) ) )
                        * p = '|' ;
        }
        /* printf( "lBuff2: %s\n" , lBuff ) ; */
        if ( ! strlen( lBuff )  )
        {
			free(lBuff);
			lBuff = NULL;
			return NULL;
        }
        if ( aAllowMultipleSelects && strchr(lBuff, '|') )
        {
			if( ! ensureFilesExist( lBuff , lBuff ) )
			{
				free(lBuff);
				lBuff = NULL;
				return NULL;
			}
        }
        else if ( !fileExists(lBuff) )
        {
			free(lBuff);
			lBuff = NULL;
			return NULL;
		}

		lBuff = (char *)( realloc( lBuff, (strlen(lBuff)+1) * sizeof(char)));

        /*printf( "lBuff3: %s\n" , lBuff ) ; */
		return lBuff ;
}


char * tinyfd_selectFolderDialog(
        char const * aTitle , /* "" */
        char const * aDefaultPath ) /* "" */
{
        static char lBuff[MAX_PATH_OR_CMD] ;
        char lDialogString[MAX_PATH_OR_CMD] ;
        FILE * lIn ;
        char * p ;
		char * lPointerInputBox ;
        int lWasGraphicDialog = 0 ;
        int lWasXterm = 0 ;
        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_selectFolderDialog("INVALID TITLE WITH QUOTES", aDefaultPath);
		if (tfd_quoteDetected(aDefaultPath)) return tinyfd_selectFolderDialog(aTitle, "INVALID DEFAULT_PATH WITH QUOTES");

        if ( osascriptPresent( ))
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return (char *)1;}
                strcpy( lDialogString , "osascript ");
                if ( ! osx9orBetter() ) strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                strcat( lDialogString , " -e 'try' -e 'POSIX path of ( choose folder ");
                if ( aTitle && strlen(aTitle) )
                {
                strcat(lDialogString, "with prompt \"") ;
                strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\" ") ;
                }
                if ( aDefaultPath && strlen(aDefaultPath) )
                {
                        strcat(lDialogString, "default location \"") ;
                        strcat(lDialogString, aDefaultPath ) ;
                        strcat(lDialogString , "\" " ) ;
                }
                strcat( lDialogString , ")' " ) ;
                strcat(lDialogString, "-e 'on error number -128' " ) ;
                strcat(lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return (char *)1;}
                strcpy( lDialogString , "kdialog" ) ;
				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }
                strcat( lDialogString , " --getexistingdirectory " ) ;

                if ( aDefaultPath && strlen(aDefaultPath) )
                {
                        if ( aDefaultPath[0] != '/' )
                        {
                                strcat(lDialogString, "$PWD/") ;
                        }
                        strcat(lDialogString, "\"") ;
                        strcat(lDialogString, aDefaultPath ) ;
                        strcat(lDialogString , "\"" ) ;
                }
                else
                {
                        strcat(lDialogString, "$PWD/") ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }
        }
        else if ( tfd_zenityPresent() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                if ( tfd_zenityPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity");return (char *)1;}
                        strcpy( lDialogString , "zenity" ) ;
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat( lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return (char *)1;}
                        strcpy( lDialogString , "matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return (char *)1;}
                        strcpy( lDialogString , "shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return (char *)1;}
                        strcpy( lDialogString , "qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat( lDialogString , " --file-selection --directory" ) ;

                strcat(lDialogString, " --title=\"") ;
                if (aTitle && strlen(aTitle)) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"") ;

                if ( aDefaultPath && strlen(aDefaultPath) )
                {
                        strcat(lDialogString, " --filename=\"") ;
                        strcat(lDialogString, aDefaultPath) ;
                        strcat(lDialogString, "\"") ;
                }
                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");
        }
        else if (tfd_yadPresent())
        {
           if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return (char*)1; }
           strcpy(lDialogString, "yad --file-selection --directory");
           if (aTitle && strlen(aTitle))
           {
              strcat(lDialogString, " --title=\"");
              strcat(lDialogString, aTitle);
              strcat(lDialogString, "\"");
           }
           if (aDefaultPath && strlen(aDefaultPath))
           {
              strcat(lDialogString, " --filename=\"");
              strcat(lDialogString, aDefaultPath);
              strcat(lDialogString, "\"");
           }
           if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
      }
      else if ( !xdialogPresent() && tkinter3Present( ) )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return (char *)1;}
			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
			strcat( lDialogString , "res=filedialog.askdirectory(");
			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, "title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "',") ;
			}
			if ( aDefaultPath && strlen(aDefaultPath) )
			{
				strcat(lDialogString, "initialdir='") ;
				strcat(lDialogString, aDefaultPath ) ;
				strcat(lDialogString , "'" ) ;
			}
			strcat( lDialogString, ");\nif not isinstance(res, tuple):\n\tprint(res)\n\"" ) ;
		}
		else if ( !xdialogPresent() && tkinter2Present( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return (char *)1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
                strcat( lDialogString , gPython2Name ) ;
                if ( ! isTerminalRunning( ) && tfd_isDarwin( ) )
                {
                strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }
        strcat( lDialogString ,
" -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if ( tfd_isDarwin( ) )
        {
                        strcat( lDialogString ,
"import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
                }

                strcat( lDialogString , "print tkFileDialog.askdirectory(");
            if ( aTitle && strlen(aTitle) )
            {
                        strcat(lDialogString, "title='") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "',") ;
            }
        if ( aDefaultPath && strlen(aDefaultPath) )
        {
                                strcat(lDialogString, "initialdir='") ;
                                strcat(lDialogString, aDefaultPath ) ;
                                strcat(lDialogString , "'" ) ;
                }
                strcat( lDialogString , ")\"" ) ;
        }
        else if ( xdialogPresent() || dialogName() )
        {
                if ( xdialogPresent( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return (char *)1;}
                        lWasGraphicDialog = 1 ;
                        strcpy( lDialogString , "(Xdialog " ) ;
                }
                else if ( isTerminalRunning( ) )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        strcpy( lDialogString , "(dialog " ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"dialog");return (char *)0;}
                        lWasXterm = 1 ;
                        strcpy( lDialogString , terminalName() ) ;
                        strcat( lDialogString , "'(" ) ;
                        strcat( lDialogString , dialogName() ) ;
                        strcat( lDialogString , " " ) ;
                }

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, "--title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\" ") ;
                }

                if ( !xdialogPresent() && !gdialogPresent() )
                {
                        strcat(lDialogString, "--backtitle \"") ;
                        strcat(lDialogString,
                                "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY") ;
                        strcat(lDialogString, "\" ") ;
                }

                strcat( lDialogString , "--dselect \"" ) ;
                if ( aDefaultPath && strlen(aDefaultPath) )
                {
                        strcat(lDialogString, aDefaultPath) ;
                        ensureFinalSlash(lDialogString);
                }
                else if ( ! isTerminalRunning( ) && !lWasGraphicDialog )
                {
                        strcat(lDialogString, getenv("HOME")) ;
                        strcat(lDialogString, "/");
                }
                else
                {
                        strcat(lDialogString, "./") ;
                }

                if ( lWasGraphicDialog )
                {
                        strcat(lDialogString, "\" 0 60 ) 2>&1 ") ;
                }
                else
                {
                        strcat(lDialogString, "\" 0 60  >/dev/tty) ") ;
                        if ( lWasXterm )
                        {
                          strcat( lDialogString ,
                                "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
                        }
                        else
                        {
                                strcat(lDialogString, "2>&1 ; clear >/dev/tty") ;
                        }
                }
        }
        else
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){return tinyfd_inputBox(aTitle,NULL,NULL);}
				strcpy(lBuff, "Select folder from ");
				strcat(lBuff, getCurDir());
				lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
				if (lPointerInputBox) strcpy(lDialogString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
				p = tinyfd_inputBox(aTitle, lBuff, "");
				if (p) strcpy(lBuff, p); else lBuff[0] = '\0';
				if (lPointerInputBox) strcpy(lPointerInputBox, lDialogString); /* restore its previous content to tinyfd_inputBox */
				p = lBuff;

                if ( !p || ! strlen( p ) || ! dirExists( p ) )
                {
                        return NULL ;
                }
                return p ;
        }
    if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;
    if ( ! ( lIn = popen( lDialogString , "r" ) ) )
    {
        return NULL ;
    }
        while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
        {}
        pclose( lIn ) ;
    if ( lBuff[strlen( lBuff ) -1] == '\n' )
    {
        lBuff[strlen( lBuff ) -1] = '\0' ;
    }
        /* printf( "lBuff: %s\n" , lBuff ) ; */
        if ( ! strlen( lBuff ) || ! dirExists( lBuff ) )
        {
                return NULL ;
        }
        return lBuff ;
}


/* returns the hexcolor as a string "#FF0000" */
/* aoResultRGB also contains the result */
/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
/* aDefaultRGB and aoResultRGB can be the same array */
char * tinyfd_colorChooser(
        char const * aTitle , /* NULL or "" */
        char const * aDefaultHexRGB , /* NULL or "#FF0000"*/
        unsigned char const aDefaultRGB[3] , /* { 0 , 255 , 255 } */
        unsigned char aoResultRGB[3] ) /* { 0 , 0 , 0 } */
{
	static char lDefaultHexRGB[16];
	char lBuff[128] ;

        char lTmp[128] ;
#if !((defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__))
		char * lTmp2 ;
#endif
        char lDialogString[MAX_PATH_OR_CMD] ;
        unsigned char lDefaultRGB[3];
        char * p;
		char * lPointerInputBox;
        FILE * lIn ;
        int i ;
        int lWasZenity3 = 0 ;
        int lWasOsascript = 0 ;
        int lWasXdialog = 0 ;
        lBuff[0]='\0';

		if (tfd_quoteDetected(aTitle)) return tinyfd_colorChooser("INVALID TITLE WITH QUOTES", aDefaultHexRGB, aDefaultRGB, aoResultRGB);
		if (tfd_quoteDetected(aDefaultHexRGB)) return tinyfd_colorChooser(aTitle, "INVALID DEFAULT_HEX_RGB WITH QUOTES", aDefaultRGB, aoResultRGB);

		if (aDefaultHexRGB)
		{
			Hex2RGB(aDefaultHexRGB, lDefaultRGB);
         strcpy(lDefaultHexRGB, aDefaultHexRGB);
		}
		else
		{
			lDefaultRGB[0] = aDefaultRGB[0];
			lDefaultRGB[1] = aDefaultRGB[1];
			lDefaultRGB[2] = aDefaultRGB[2];
         RGB2Hex(aDefaultRGB, lDefaultHexRGB);
		}

        if ( osascriptPresent( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"applescript");return (char *)1;}
                lWasOsascript = 1 ;
                strcpy( lDialogString , "osascript");

                if ( ! osx9orBetter() )
                {
                        strcat( lDialogString , " -e 'tell application \"System Events\"' -e 'Activate'");
                        strcat( lDialogString , " -e 'try' -e 'set mycolor to choose color default color {");
                }
                else
                {
                        strcat( lDialogString ,
" -e 'try' -e 'tell app (path to frontmost application as Unicode text) \
to set mycolor to choose color default color {");
                }

                sprintf(lTmp, "%d", 256 * lDefaultRGB[0] ) ;
                strcat(lDialogString, lTmp ) ;
                strcat(lDialogString, "," ) ;
                sprintf(lTmp, "%d", 256 * lDefaultRGB[1] ) ;
                strcat(lDialogString, lTmp ) ;
                strcat(lDialogString, "," ) ;
                sprintf(lTmp, "%d", 256 * lDefaultRGB[2] ) ;
                strcat(lDialogString, lTmp ) ;
                strcat(lDialogString, "}' " ) ;
                strcat( lDialogString ,
"-e 'set mystring to ((item 1 of mycolor) div 256 as integer) as string' " );
                strcat( lDialogString ,
"-e 'repeat with i from 2 to the count of mycolor' " );
                strcat( lDialogString ,
"-e 'set mystring to mystring & \" \" & ((item i of mycolor) div 256 as integer) as string' " );
                strcat( lDialogString , "-e 'end repeat' " );
                strcat( lDialogString , "-e 'mystring' ");
                strcat(lDialogString, "-e 'on error number -128' " ) ;
                strcat(lDialogString, "-e 'end try'") ;
                if ( ! osx9orBetter() ) strcat( lDialogString, " -e 'end tell'") ;
        }
        else if ( tfd_kdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"kdialog");return (char *)1;}
                strcpy( lDialogString , "kdialog" ) ;
				if ( (tfd_kdialogPresent() == 2) && tfd_xpropPresent() )
                {
                        strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                }
                sprintf( lDialogString + strlen(lDialogString) , " --getcolor --default '%s'" , lDefaultHexRGB ) ;

                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, " --title \"") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "\"") ;
                }
        }
        else if ( tfd_zenity3Present() || tfd_matedialogPresent() || tfd_shellementaryPresent() || tfd_qarmaPresent() )
        {
                lWasZenity3 = 1 ;
                if ( tfd_zenity3Present() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"zenity3");return (char *)1;}
                        strcpy( lDialogString , "zenity" );
						if ( (tfd_zenity3Present() >= 4) && !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat( lDialogString, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                else if ( tfd_matedialogPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"matedialog");return (char *)1;}
                        strcpy( lDialogString , "matedialog" ) ;
                }
                else if ( tfd_shellementaryPresent() )
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"shellementary");return (char *)1;}
                        strcpy( lDialogString , "shellementary" ) ;
                }
                else
                {
                        if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"qarma");return (char *)1;}
                        strcpy( lDialogString , "qarma" ) ;
						if ( !getenv("SSH_TTY") && tfd_xpropPresent() )
                        {
                                strcat(lDialogString, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
                        }
                }
                strcat( lDialogString , " --color-selection --show-palette" ) ;
                sprintf( lDialogString + strlen(lDialogString), " --color=%s" , lDefaultHexRGB ) ;

                strcat(lDialogString, " --title=\"") ;
                if (aTitle && strlen(aTitle)) strcat(lDialogString, aTitle) ;
                strcat(lDialogString, "\"") ;

                if (tinyfd_silent) strcat( lDialogString , " 2>/dev/null ");
        }
        else if (tfd_yadPresent())
        {
           if (aTitle && !strcmp(aTitle, "tinyfd_query")) { strcpy(tinyfd_response, "yad"); return (char*)1; }
           strcpy(lDialogString, "yad --color");
           sprintf(lDialogString + strlen(lDialogString), " --init-color=%s", lDefaultHexRGB);
           if (aTitle && strlen(aTitle))
           {
              strcat(lDialogString, " --title=\"");
              strcat(lDialogString, aTitle);
              strcat(lDialogString, "\"");
           }
           if (tinyfd_silent) strcat(lDialogString, " 2>/dev/null ");
        }
        else if ( xdialogPresent() )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"xdialog");return (char *)1;}
                lWasXdialog = 1 ;
                strcpy( lDialogString , "Xdialog --colorsel \"" ) ;
                if ( aTitle && strlen(aTitle) )
                {
                        strcat(lDialogString, aTitle) ;
                }
                strcat(lDialogString, "\" 0 60 ") ;
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
				sprintf(lTmp,"%hhu %hhu %hhu",lDefaultRGB[0],lDefaultRGB[1],lDefaultRGB[2]);
#else
                sprintf(lTmp,"%hu %hu %hu",lDefaultRGB[0],lDefaultRGB[1],lDefaultRGB[2]);
#endif
                strcat(lDialogString, lTmp) ;
                strcat(lDialogString, " 2>&1");
        }
		else if ( tkinter3Present( ) )
		{
			if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python3-tkinter");return (char *)1;}
			strcpy( lDialogString , gPython3Name ) ;
			strcat( lDialogString ,
				" -S -c \"import tkinter;from tkinter import colorchooser;root=tkinter.Tk();root.withdraw();");
			strcat( lDialogString , "res=colorchooser.askcolor(color='" ) ;
			strcat(lDialogString, lDefaultHexRGB ) ;
			strcat(lDialogString, "'") ;

			if ( aTitle && strlen(aTitle) )
			{
				strcat(lDialogString, ",title='") ;
				strcat(lDialogString, aTitle) ;
				strcat(lDialogString, "'") ;
			}
			strcat( lDialogString , ");\
\nif res[1] is not None:\n\tprint(res[1])\"" ) ;
		}
		else if ( tkinter2Present( ) )
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){strcpy(tinyfd_response,"python2-tkinter");return (char *)1;}
				strcpy( lDialogString , "export PYTHONIOENCODING=utf-8;" ) ;
				strcat( lDialogString , gPython2Name ) ;
				if ( ! isTerminalRunning( ) && tfd_isDarwin( ) )
                {
                strcat( lDialogString , " -i" ) ;  /* for osx without console */
                }

                strcat( lDialogString ,
" -S -c \"import Tkinter,tkColorChooser;root=Tkinter.Tk();root.withdraw();");

                if ( tfd_isDarwin( ) )
                {
                        strcat( lDialogString ,
"import os;os.system('''osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
                }

                strcat( lDialogString , "res=tkColorChooser.askcolor(color='" ) ;
                strcat(lDialogString, lDefaultHexRGB ) ;
                strcat(lDialogString, "'") ;


            if ( aTitle && strlen(aTitle) )
            {
                        strcat(lDialogString, ",title='") ;
                        strcat(lDialogString, aTitle) ;
                        strcat(lDialogString, "'") ;
            }
                strcat( lDialogString , ");\
\nif res[1] is not None:\n\tprint res[1]\"" ) ;
        }
        else
        {
                if (aTitle&&!strcmp(aTitle,"tinyfd_query")){return tinyfd_inputBox(aTitle,NULL,NULL);}
				lPointerInputBox = tinyfd_inputBox(NULL, NULL, NULL); /* obtain a pointer on the current content of tinyfd_inputBox */
				if (lPointerInputBox) strcpy(lDialogString, lPointerInputBox); /* preserve the current content of tinyfd_inputBox */
				p = tinyfd_inputBox(aTitle, "Enter hex rgb color (i.e. #f5ca20)", lDefaultHexRGB);

                if ( !p || (strlen(p) != 7) || (p[0] != '#') )
                {
                        return NULL ;
                }
                for ( i = 1 ; i < 7 ; i ++ )
                {
                        if ( ! isxdigit( (int) p[i] ) )
                        {
                                return NULL ;
                        }
                }
				Hex2RGB(p,aoResultRGB);
				strcpy(lDefaultHexRGB, p);
				if (lPointerInputBox) strcpy(lPointerInputBox, lDialogString); /* restore its previous content to tinyfd_inputBox */
				return lDefaultHexRGB;
        }

        if (tinyfd_verbose) printf( "lDialogString: %s\n" , lDialogString ) ;
        if ( ! ( lIn = popen( lDialogString , "r" ) ) )
        {
                return NULL ;
    }
        while ( fgets( lBuff , sizeof( lBuff ) , lIn ) != NULL )
        {
        }
        pclose( lIn ) ;
    if ( ! strlen( lBuff ) )
    {
        return NULL ;
    }
        /* printf( "len Buff: %lu\n" , strlen(lBuff) ) ; */
        /* printf( "lBuff0: %s\n" , lBuff ) ; */
    if ( lBuff[strlen( lBuff ) -1] == '\n' )
    {
        lBuff[strlen( lBuff ) -1] = '\0' ;
    }

        if ( lWasZenity3 )
    {
                if ( lBuff[0] == '#' )
                {
                        if ( strlen(lBuff)>7 )
                        {
                                lBuff[3]=lBuff[5];
                                lBuff[4]=lBuff[6];
                                lBuff[5]=lBuff[9];
                                lBuff[6]=lBuff[10];
                                lBuff[7]='\0';
                        }
                Hex2RGB(lBuff,aoResultRGB);
                }
                else if ( lBuff[3] == '(' ) {
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
    sscanf(lBuff,"rgb(%hhu,%hhu,%hhu", & aoResultRGB[0], & aoResultRGB[1],& aoResultRGB[2]);
#else
    aoResultRGB[0] = strtol(lBuff+4, & lTmp2, 10 );
    aoResultRGB[1] = strtol(lTmp2+1, & lTmp2, 10 );
    aoResultRGB[2] = strtol(lTmp2+1, NULL, 10 );
#endif
    RGB2Hex(aoResultRGB,lBuff);
                }
                else if ( lBuff[4] == '(' ) {
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
    sscanf(lBuff,"rgba(%hhu,%hhu,%hhu",  & aoResultRGB[0], & aoResultRGB[1],& aoResultRGB[2]);
#else
    aoResultRGB[0] = strtol(lBuff+5, & lTmp2, 10 );
    aoResultRGB[1] = strtol(lTmp2+1, & lTmp2, 10 );
    aoResultRGB[2] = strtol(lTmp2+1, NULL, 10 );
#endif
    RGB2Hex(aoResultRGB,lBuff);
                }
    }
    else if ( lWasOsascript || lWasXdialog )
    {
                /* printf( "lBuff: %s\n" , lBuff ) ; */
#if (defined(__cplusplus ) && __cplusplus >= 201103L) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__clang__)
    sscanf(lBuff,"%hhu %hhu %hhu", & aoResultRGB[0], & aoResultRGB[1],& aoResultRGB[2]);
#else
    aoResultRGB[0] = strtol(lBuff, & lTmp2, 10 );
    aoResultRGB[1] = strtol(lTmp2+1, & lTmp2, 10 );
    aoResultRGB[2] = strtol(lTmp2+1, NULL, 10 );
#endif
    RGB2Hex(aoResultRGB,lBuff);
    }
    else
    {
                Hex2RGB(lBuff,aoResultRGB);
    }
    /* printf("%d %d %d\n", aoResultRGB[0],aoResultRGB[1],aoResultRGB[2]); */
    /* printf( "lBuff: %s\n" , lBuff ) ; */

	strcpy(lDefaultHexRGB,lBuff);
	return lDefaultHexRGB ;
}

#endif /* _WIN32 */


/*
int main( int argc , char * argv[] )
{
char const * lTmp;
char const * lTheSaveFileName;
char const * lTheOpenFileName;
char const * lTheSelectFolderName;
char const * lTheHexColor;
char const * lWillBeGraphicMode;
unsigned char lRgbColor[3];
FILE * lIn;
char lBuffer[1024];
char lString[1024];
char const * lFilterPatterns[2] = { "*.txt", "*.text" };

tinyfd_verbose = argc - 1;
tinyfd_silent = 1;

lWillBeGraphicMode = tinyfd_inputBox("tinyfd_query", NULL, NULL);

strcpy(lBuffer, "v");
strcat(lBuffer, tinyfd_version);
if (lWillBeGraphicMode)
{
    strcat(lBuffer, "\ngraphic mode: ");
}
else
{
    strcat(lBuffer, "\nconsole mode: ");
}
strcat(lBuffer, tinyfd_response);
strcat(lBuffer, "\n");
strcat(lBuffer, tinyfd_needs+78);
strcpy(lString, "tinyfiledialogs");
tinyfd_messageBox(lString, lBuffer, "ok", "info", 0);

tinyfd_notifyPopup("the title", "the message\n\tfrom outer-space", "info");

if (lWillBeGraphicMode && !tinyfd_forceConsole)
{
        tinyfd_forceConsole = ! tinyfd_messageBox("Hello World",
                "graphic dialogs [yes] / console mode [no]?",
                "yesno", "question", 1);
}

lTmp = tinyfd_inputBox(
        "a password box", "your password will be revealed", NULL);

if (!lTmp) return 1;

strcpy(lString, lTmp);

lTheSaveFileName = tinyfd_saveFileDialog(
        "let us save this password",
        "passwordFile.txt",
        2,
        lFilterPatterns,
        NULL);

if (!lTheSaveFileName)
{
        tinyfd_messageBox(
                "Error",
                "Save file name is NULL",
                "ok",
                "error",
                1);
        return 1;
}

lIn = fopen(lTheSaveFileName, "w");
if (!lIn)
{
        tinyfd_messageBox(
                "Error",
                "Can not open this file in write mode",
                "ok",
                "error",
                1);
        return 1;
}
fputs(lString, lIn);
fclose(lIn);

lTheOpenFileName = tinyfd_openFileDialog(
        "let us read the password back",
        "",
        2,
        lFilterPatterns,
        NULL,
        0);

if (!lTheOpenFileName)
{
        tinyfd_messageBox(
                "Error",
                "Open file name is NULL",
                "ok",
                "error",
                1);
        return 1;
}

lIn = fopen(lTheOpenFileName, "r");

if (!lIn)
{
        tinyfd_messageBox(
                "Error",
                "Can not open this file in read mode",
                "ok",
                "error",
                1);
        return(1);
}
lBuffer[0] = '\0';
fgets(lBuffer, sizeof(lBuffer), lIn);
fclose(lIn);

tinyfd_messageBox("your password is",
        lBuffer, "ok", "info", 1);

lTheSelectFolderName = tinyfd_selectFolderDialog(
        "let us just select a directory", NULL);

if (!lTheSelectFolderName)
{
        tinyfd_messageBox(
                "Error",
                "Select folder name is NULL",
                "ok",
                "error",
                1);
        return 1;
}

tinyfd_messageBox("The selected folder is",
        lTheSelectFolderName, "ok", "info", 1);

lTheHexColor = tinyfd_colorChooser(
        "choose a nice color",
        "#FF0077",
        lRgbColor,
        lRgbColor);

if (!lTheHexColor)
{
        tinyfd_messageBox(
                "Error",
                "hexcolor is NULL",
                "ok",
                "error",
                1);
        return 1;
}

tinyfd_messageBox("The selected hexcolor is",
        lTheHexColor, "ok", "info", 1);

        tinyfd_beep();

        return 0;
}
*/

#ifdef _MSC_VER
#pragma warning(default:4996)
#pragma warning(default:4100)
#pragma warning(default:4706)
#endif
