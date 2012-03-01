/*
Copyright (c) 2012, Rene Sugar (Twitter: @renesugar)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
  in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <windows.h>

#include <map>
#include <string>
using namespace std;

#if defined(__MINGW32__) || defined(__MINGW64__)

class PathTranslationMap :
    public std::map<std::string,std::string>
{
};

static PathTranslationMap * FstabMap;

static PathTranslationMap * UUIDMap;

static PathTranslationMap * VolumeLabelMap;

static PathTranslationMap * Mingw2Win32Map;

// Returns true if string starts with another string
static bool StringStartsWith(const char* str1, const char* str2)
{
	if (!str1 || !str2)
	{
		return false;
	}
	size_t len1 = strlen(str1), len2 = strlen(str2);
	return len1 >= len2 && !strncmp(str1, str2, len2) ? true : false;
}

// Returns true if string equals another string
static bool StringEquals(const char* str1, const char* str2)
{
	if (!str1 && !str2)
	{
		return true;
	}
	
	if (!str1 || !str2)
	{
		return false;
	}

	return !strcmp(str1, str2) ? true : false;
}

static bool ConvertUnixToWindowsPathSeparator(const char * readLine, char * writeLine, bool addSeparator)
{
	const char * readLineStart = readLine;
	
	if (!readLine || !writeLine)
	{
		return false;
	}
	
	while(*readLine)
	{
		if ('/' == *readLine)
		{
			*writeLine++ = '\\';
		}
		else
		{
			*writeLine++ = *readLine;
		}

		readLine++;
	}

	// add trailing path separator if not present
	
	if (addSeparator && (readLine > readLineStart))
	{
		readLine--;
		
		if ('\\' != *readLine)
		{
			*writeLine++ = '\\';
		}
	}
	
	*writeLine++ = 0;
	
	return true;
}

static bool AppendUnixPathSeparator(char * readLine)
{
	char * readLineStart = readLine;
	
	if (!readLine)
	{
		return false;
	}
	
	while(*readLine)
	{
		readLine++;
	}

	// add trailing path separator if not present
	
	if (readLine > readLineStart)
	{
		if ('/' != *(readLine-1))
		{
			*readLine++ = '/';
		}
	}
	
	*readLine++ = 0;
	
	return true;
}

static bool FstabToWin32(const char *mountpoint, char *win32_path)
{
	PathTranslationMap::iterator i = FstabMap->find(mountpoint);

	win32_path[0] = 0;

	if (!mountpoint)
	{
		false;
	}
	
	if (':' == mountpoint[1])
	{
		return ConvertUnixToWindowsPathSeparator(mountpoint, win32_path, false);
	}

	if (('/' == mountpoint[0]) && ('/' == mountpoint[1]))
	{
		return ConvertUnixToWindowsPathSeparator(mountpoint, win32_path, false);
	}
	
	if (i != FstabMap->end())
	{
		strncpy(win32_path, i->second.c_str(), _MAX_PATH);
		// win32_path will only be null-terminated by strncpy if the length of the C string in i->second.c_str() is less than _MAX_PATH
		win32_path[_MAX_PATH] = 0;
	}
	
	return win32_path[0] != 0;
}

// Convert MinGW path to Windows path
static bool mingw_conv_to_win32_path(const char *path, char *win32_path)
{
	char   Path[_MAX_PATH+2];
	char * startPath    = Path;
	char * currentPath  = 0;
	char * endPath      = 0;
	char   saveChar     = 0;
	int	   lenPath      = 0;
	int    lenWin32Path = 0;
	bool   found        = false;
	bool   hasSeparator = false;
	
	if (!path || !win32_path)
	{
		return false;
	}

	lenPath = strlen(path);

	win32_path[0] = 0;
		
	if (0 == lenPath)
	{
		return true;
	}
	
	if (lenPath > _MAX_PATH)
	{
		return false;
	}

	// Make a local writable copy of path
	strncpy(Path, path, _MAX_PATH);
	Path[_MAX_PATH]   = 0;
	Path[_MAX_PATH+1] = 0;	// extra character for appended path separator
	
	lenPath = strlen(Path);
	
	currentPath = startPath + lenPath - 1;

	if ('/' == *currentPath)
	{
		hasSeparator = true;
	}

	AppendUnixPathSeparator(Path);

	lenPath = strlen(Path);
	
	currentPath = startPath + lenPath - 1;

	endPath = currentPath;
	
	while (!found && (currentPath >= startPath))
	{
		if ('/' == *currentPath)
		{
			saveChar = (*(currentPath + 1));
			*(currentPath + 1) = 0;
			win32_path[0] = 0;
			
			if (FstabToWin32(startPath, win32_path))
			{
				*(currentPath + 1) = saveChar;
				
				lenWin32Path = strlen(win32_path);
				
				if (!hasSeparator && ('/' == *endPath))
				{
					*endPath = 0;
				}

				strncpy(win32_path + lenWin32Path, (currentPath + 1), _MAX_PATH - lenWin32Path);

				// win32_path will only be null-terminated by strncpy if the length of the C string in (currentPath + 1) is less than (_MAX_PATH - lenWin32Path)
				win32_path[_MAX_PATH] = 0;

				ConvertUnixToWindowsPathSeparator(win32_path, win32_path, false);
				
				lenWin32Path = strlen(win32_path);
				
				if (!hasSeparator && (lenWin32Path > 2) && ('\\' == win32_path[lenWin32Path-1]) && (':' != win32_path[lenWin32Path-2]))
				{
					win32_path[lenWin32Path-1] = 0;
				}

				found = true;
			}
			
			*(currentPath + 1) = saveChar;
		}
		
		currentPath--;
	}
	
	return win32_path[0] != 0;
}

static bool VolumeLabelToWin32(const char *label, char *win32_path)
{
	PathTranslationMap::iterator i = VolumeLabelMap->find(label);

	win32_path[0] = 0;
	
	if (i != VolumeLabelMap->end())
	{
		strncpy(win32_path, i->second.c_str(), _MAX_PATH);
		// win32_path will only be null-terminated by strncpy if the length of the C string in i->second.c_str() is less than _MAX_PATH
		win32_path[_MAX_PATH] = 0;
	}
	
	return win32_path[0] != 0;
}

static bool UUIDToWin32(const char *uuid, char *win32_path)
{
	PathTranslationMap::iterator i = UUIDMap->find(uuid);

	win32_path[0] = 0;
	
	if (i != UUIDMap->end())
	{
		strncpy(win32_path, i->second.c_str(), _MAX_PATH);
		// win32_path will only be null-terminated by strncpy if the length of the C string in i->second.c_str() is less than _MAX_PATH
		win32_path[_MAX_PATH] = 0;
	}
	
	return win32_path[0] != 0;
}

static bool PathMingwToWin32(const char *path, char *win32_path)
{
	PathTranslationMap::iterator i = Mingw2Win32Map->find(path);

	win32_path[0] = 0;
	
	if (!path)
	{
		return false;
	}
	
	if ((0 != path[0]) && (':' == path[1]))
	{
		// path already in Windows format
		return false;
	}

	if (('\\' == path[0]) && ('\\' == path[1]))
	{
		// path already in Windows format
		return false;
	}	

	if (i != Mingw2Win32Map->end())
	{
		strncpy(win32_path, i->second.c_str(), _MAX_PATH);
		// win32_path will only be null-terminated by strncpy if the length of the C string in i->second.c_str() is less than _MAX_PATH
		win32_path[_MAX_PATH] = 0;
	}
	else
	{
		if(mingw_conv_to_win32_path(path, win32_path))
		{
			Mingw2Win32Map->insert(make_pair(std::string(path), std::string(win32_path)));
		}
	}
	
	return win32_path[0] != 0;
}

#define FSTAB_MAX_LINE 2048

typedef enum
{
	FSTAB_LINE_BLANK = 0,
	FSTAB_LINE_COMMENT,
	FSTAB_LINE_OK
} FSTAB_LINE_TYPE;

static void ParseFstabWhitespace(char ** preadLine)
{
	char * readLine  = *preadLine;
	
	// skip whitespace

	while((*readLine) && (( '\t' == *readLine) || (' ' == *readLine)))
	{
		if ('\n' == *readLine)
		{
			break;
		}

		readLine++;
	}

	*preadLine  = readLine;
}

static int ParseFstabInteger(char ** preadLine, char ** pwriteLine, int maxDigits)
{
	char * readLine  = *preadLine;
	char * writeLine = *pwriteLine;
	int    state     = 0;
	int    value     = 0;
	int    digits    = maxDigits;

	// Deterministic finite automaton to recognize natural numbers
	
	while((*readLine) && (( '\t' != *readLine) && (' ' != *readLine) && ('\n' != *readLine)) && (digits > 0))
	{
		switch(state)
		{
			case 0:
				if ((*readLine >= '0') && (*readLine <= '9'))
				{
					value *= 10;
					value += ((*readLine) - '0');
					digits--;
					
					*writeLine++ = *readLine;
				}
				else
				{
					// invalid character
					value  = 0;
					digits = 0;
				}
				break;
			default:
				// invalid character
				state = 0;
		}

		if ('\n' != *readLine)
		{
			readLine++;
		}
	}

	*writeLine++ = 0;	
	
	*preadLine  = readLine;
	*pwriteLine = writeLine;

	return value;
}

static void ParseFstabString(char ** preadLine, char ** pwriteLine)
{
	char * readLine  = *preadLine;
	char * writeLine = *pwriteLine;
	int    state     = 0;
	int    value     = 0;

	// Deterministic finite automaton to recognize strings containing escaped characters, and octal and hex constants

	while((*readLine) && (( '\t' != *readLine) && (' ' != *readLine) && ('\n' != *readLine)))
	{
		switch(state)
		{
			case 0:
				if (*readLine == '\\')
				{
					// escaped character
					state = 1;
				}
				else
				{
					*writeLine++ = *readLine;
				}
				break;
			case 1:
				if (*readLine == '0')
				{
					// read octal or hex constant
					state = 2;
				}
				else
				{
					// write escaped character
					*writeLine++ = *readLine;
					state = 0;
				}
				break;
			case 2:
				if (*readLine == 'x')
				{
					// hex constant
					state = 3;
				}
				else
				{
					// octal constant
					if ((*readLine >= '0') && (*readLine <= '7'))
					{
						// first octal digit
						value = 0;
						value += ((*readLine) - '0');

						readLine++;

						if ((*readLine >= '0') && (*readLine <= '7'))
						{
							// second octal digit
							value <<= 3;
							value += ((*readLine) - '0');
							*writeLine++ = (char)value;
							state = 0;
						}
						else
						{
							*writeLine++ = (char)value;

							state = 0;

							readLine--;
						}

					}
					else
					{
						// invalid constant
						state = 0;
					}
				}
				break;
			case 3:
				if (((*readLine >= '0') && (*readLine <= '9')) || ((*readLine >= 'a') && (*readLine <= 'f')) || ((*readLine >= 'A') && (*readLine <= 'F')))
				{
					// first hex digit
					value = 0;

					if      ((*readLine >= '0') && (*readLine <= '9'))
					{
						value += ((*readLine) - '0');
					}
					else if ((*readLine >= 'a') && (*readLine <= 'f'))
					{
						value += ((*readLine) - 'a');
					}
					else if ((*readLine >= 'A') && (*readLine <= 'F'))
					{
						value += ((*readLine) - 'A');
					}

					if (('\n' != *readLine) && (0 != *readLine))
					{
						readLine++;

						// second hex digit

						if      ((*readLine >= '0') && (*readLine <= '9'))
						{
							value <<= 4;
							value += ((*readLine) - '0');
							*writeLine++ = (char)value;
						}
						else if ((*readLine >= 'a') && (*readLine <= 'f'))
						{
							value <<= 4;
							value += ((*readLine) - 'a');
							*writeLine++ = (char)value;
						}
						else if ((*readLine >= 'A') && (*readLine <= 'F'))
						{
							value <<= 4;
							value += ((*readLine) - 'A');
							*writeLine++ = (char)value;
						}
						else
						{
							*writeLine++ = (char)value;

							readLine--;
						}
					}

					state = 0;
				}
				else
				{
					// invalid hex constant
					state = 0;
				}

				break;
			default:
				// invalid character
				state = 0;
		}

		if ('\n' != *readLine)
		{
			readLine++;
		}
	}

	*writeLine++ = 0;	// add room to append an extra character to string
	*writeLine++ = 0;
	
	*preadLine  = readLine;
	*pwriteLine = writeLine;
}

static FSTAB_LINE_TYPE fstab_readline(char ** preadLine, char ** pwriteLine, char ** pfs_spec, char ** pfs_mountpoint, char ** pfs_type, char ** pfs_options, int * pfs_dump, int * pfs_pass)
{
	char *fs_spec       = *pfs_spec;
	char *fs_mountpoint = *pfs_mountpoint;
	char *fs_type       = *pfs_type;
	char *fs_options    = *pfs_options;
	int   fs_dump       = *pfs_dump;
	int   fs_pass       = *pfs_pass;

	char *readLineStart = *preadLine;
	char *readLine      = *preadLine;
	char *writeLine     = *pwriteLine;
	int   lenLine       = 0;
	bool  eol           = false;
	
	fs_spec       = NULL;
	fs_mountpoint = NULL;
	fs_type       = NULL;
	fs_options    = NULL;
	fs_dump       = 0;
	fs_pass       = 0;	

	lenLine       = strlen(readLine);

	// skip whitespace
	
	ParseFstabWhitespace(&readLine);
	
	// skip comment line

	if ('#' == *readLine)
	{
		readLineStart[0] = 0;
		return FSTAB_LINE_COMMENT;
	}

	// skip blank line

	if ('\n' == *readLine)
	{
		readLineStart[0] = 0;
		return FSTAB_LINE_BLANK;
	}

	// get file system path

	fs_spec = writeLine;

	ParseFstabString(&readLine, &writeLine);

	// get mountpoint

	if ('\n' != *readLine)
	{
		// skip whitespace
	
		ParseFstabWhitespace(&readLine);

		fs_mountpoint = writeLine;
		
		ParseFstabString(&readLine, &writeLine);			
	}

	// get file system type

	if ('\n' != *readLine)
	{
		// skip whitespace
	
		ParseFstabWhitespace(&readLine);

		fs_type = writeLine;
		
		ParseFstabString(&readLine, &writeLine);					
	}

	// get file system options

	if ('\n' != *readLine)
	{
		// skip whitespace
	
		ParseFstabWhitespace(&readLine);

		fs_options = writeLine;
		
		ParseFstabString(&readLine, &writeLine);					
	}
	
	// get dump

	if ('\n' != *readLine)
	{
		// skip whitespace
	
		ParseFstabWhitespace(&readLine);
	
		fs_dump = ParseFstabInteger(&readLine, &writeLine, 1);
	}

	// get pass

	if ('\n' != *readLine)
	{
		// skip whitespace
	
		ParseFstabWhitespace(&readLine);
	
		fs_pass = ParseFstabInteger(&readLine, &writeLine, 1);		
	}

	*pfs_spec       = fs_spec;
	*pfs_mountpoint = fs_mountpoint;
	*pfs_type       = fs_type;
	*pfs_options    = fs_options;
	*pfs_dump       = fs_dump;
	*pfs_pass       = fs_pass;
	
	return FSTAB_LINE_OK;
}

static bool fstab_init(char * fstabPath)
{
	unsigned long uDriveMask = _getdrives();
	char drive = 'a';
    char driveUpper = 'A';		
	char Drive[] = "c:\\";		// template drive specifier
	char MsysDrive[] = "/c/";	// template MSYS drive specifier

	bool  bFlag = false;
	char  VolumePathName[_MAX_PATH+1];
	DWORD VolumePathNameSize = sizeof(VolumePathName);	
	char  VolumeName[_MAX_PATH+1];
	DWORD VolumeNameSize = sizeof(VolumeName);
	DWORD VolumeSerialNumber = 0;
	DWORD MaximumComponentLength = 0;
	DWORD FileSystemFlags = 0;
	CHAR  FileSystemName[_MAX_PATH+1];
	DWORD FileSystemNameSize = sizeof(FileSystemName);
	char  MsysRootName[_MAX_PATH+1];
	char  MsysName[_MAX_PATH+1];
	char  HomeName[_MAX_PATH+1];	
	char  EtcName[_MAX_PATH+1];
	char  UUIDName[51];
	
	char *home = NULL;

	if (!fstabPath)
	{
		return false;
	}
	
	fstabPath[0] = 0;
	
	// get $HOME environment variable

	home = getenv("HOME");
	ConvertUnixToWindowsPathSeparator(home, home, true);

	if (home)
	{
		strncpy(HomeName, home, _MAX_PATH);
		HomeName[_MAX_PATH] = 0;
		
		// get MSYS root drive path
		
		bFlag = GetVolumePathName(
			home,
			MsysRootName,
			sizeof(MsysRootName));

		if (bFlag != 0)
		{
			int i = 0;
			char *srcPath = HomeName + strlen(MsysRootName);
			char *dstPath = MsysName + strlen(MsysRootName);

			// check that returned path is a prefix of the original path
			if (0 == strncmp(HomeName, MsysRootName, strlen(MsysRootName)))
			{
				// create path to MSYS
				
				strcpy(MsysName, MsysRootName);
				
				for(i = 0; (i <= _MAX_PATH ) && (*srcPath) && ( '\\' != *srcPath ) && ( '/' != *srcPath ); i++ )
				{
					dstPath[i] = (char)*srcPath++;
				}
				
				if (i <= _MAX_PATH)
				{
					dstPath[i] = 0;
				}
				else
				{
					dstPath[_MAX_PATH] = 0;		
				}

				ConvertUnixToWindowsPathSeparator(dstPath, dstPath, true);
					
				// construct Windows path to /etc

				strcpy(EtcName, MsysName);
				strcpy(EtcName + strlen(MsysName), "etc");
			
				ConvertUnixToWindowsPathSeparator(EtcName, EtcName, true);
							
				// construct Windows path to /etc/fstab

				strcpy(fstabPath, MsysName);
				strcpy(fstabPath + strlen(MsysName), "etc\\fstab");
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		
		FstabMap = new PathTranslationMap;
		
		if (!FstabMap)
		{
			return false;
		}

		UUIDMap = new PathTranslationMap;
		
		if (!UUIDMap)
		{
			return false;
		}
		
		VolumeLabelMap = new PathTranslationMap;
		
		if (!VolumeLabelMap)
		{
			return false;
		}
		
		Mingw2Win32Map = new PathTranslationMap;
		
		if (!Mingw2Win32Map)
		{
			return false;
		}
		
		// add fstab entries for each drive letter (e.g. /c/ /d/ /e/ ...)

		while (uDriveMask) {
			if (uDriveMask & 1)
			{
				Drive[0] = driveUpper;
				MsysDrive[1] = driveUpper;
				
				// add /C/ entry to map
				FstabMap->insert(std::make_pair(std::string(MsysDrive), std::string(Drive)));

				Drive[0] = drive;
				MsysDrive[1] = drive;
				
				// add /c/ entry to map
				FstabMap->insert(std::make_pair(std::string(MsysDrive), std::string(Drive)));

				VolumePathName[0] = 0;
				
				bFlag = GetVolumeNameForVolumeMountPointA(
						 Drive,  // input volume mount point or directory
						 VolumePathName,    // output volume name buffer
						 VolumePathNameSize // size of volume name buffer
					  );

				if (bFlag != 0) 
				{
					if ('{' == VolumePathName[10])
					{
						strncpy(UUIDName, &VolumePathName[11], 36);
						UUIDName[36] = 0;

						// add \\?\Volume{UUID}\ to UUID map
						UUIDMap->insert(make_pair(std::string(UUIDName), std::string(Drive)));						
					}
					
					bFlag = GetVolumeInformation(
					  Drive,
					  VolumeName,
					  VolumeNameSize,
					  &VolumeSerialNumber,
					  &MaximumComponentLength,
					  &FileSystemFlags,
					  FileSystemName,
					  FileSystemNameSize
					);	  

					if (bFlag != 0) 
					{
						// add LABEL (volume name) to LABEL map
						VolumeLabelMap->insert(make_pair(std::string(VolumeName), std::string(Drive)));												
					}			  
				}
			}
			
			++drive;
			uDriveMask >>= 1;
		}

		// add fstab entries for /, ~/, /etc/
		
		// add / entry to map
		FstabMap->insert(make_pair(std::string("/"), std::string(MsysName)));

		// add ~/ entry to map
		FstabMap->insert(make_pair(std::string("~/"), std::string(HomeName)));

		// add /etc/ entry to map
		FstabMap->insert(make_pair(std::string("/etc/"), std::string(EtcName)));		
	}
	else
	{
		return false;
	}
	
	return true;
}

static bool fstab_load(char * fstabFileName)
{
	int  lenLine;
	char readLineBuffer[FSTAB_MAX_LINE];
	char writeLineBuffer[FSTAB_MAX_LINE+4];	// add room for an extra character appended to each string
	char path[_MAX_PATH+1];
	char *readLineStart = readLineBuffer;
	char *readLine = readLineBuffer;
	char *writeLine = writeLineBuffer;
	bool eol = false;
	FILE *fp;

	char *fs_spec       = NULL;
	char *fs_mountpoint = NULL;
	char *fs_type       = NULL;
	char *fs_options    = NULL;
	int   fs_dump       = 0;
	int   fs_pass       = 0;

	FSTAB_LINE_TYPE line_type = FSTAB_LINE_OK;

	// open /etc/fstab file for reading
	fp = fopen (fstabFileName, "rt");

	if (NULL == fp)
	{
		// could not open /etc/fstab file
		return false;
	}

	readLineBuffer[0] = 0;

	while(fgets(readLineBuffer, sizeof(readLineBuffer), fp) != NULL)
	{
		fs_spec       = NULL;
		fs_mountpoint = NULL;
		fs_type       = NULL;
		fs_options    = NULL;
		fs_dump       = 0;
		fs_pass       = 0;

		eol        = false;
		lenLine    = strlen(readLineBuffer);
		readLine   = readLineBuffer;
		writeLine  = writeLineBuffer;
		*writeLine = 0;

		line_type = fstab_readline(&readLine, &writeLine, &fs_spec, &fs_mountpoint, &fs_type, &fs_options, &fs_dump, &fs_pass);

		// skip comment line

		if (FSTAB_LINE_COMMENT == line_type)
		{
			readLineBuffer[0] = 0;
			continue;
		}

		// skip blank line

		if (FSTAB_LINE_BLANK == line_type)
		{
			readLineBuffer[0] = 0;
			continue;
		}
		
		// add fstab entry to path translation map

		// if fs_spec begins with UUID=, add  "\\?\Volume{UUID}\"
		if (StringStartsWith(fs_spec, "UUID="))
		{
			if (UUIDToWin32(&fs_spec[5], path))
			{
				AppendUnixPathSeparator(fs_mountpoint);
				FstabMap->insert(make_pair(std::string(fs_mountpoint), std::string(path)));
			}		
		}
		// else if fs_spec begins with LABEL= lookup path by volume label and add path
		else if (StringStartsWith(fs_spec, "LABEL="))
		{
			if (VolumeLabelToWin32(&fs_spec[6], path))
			{
				AppendUnixPathSeparator(fs_mountpoint);			
				FstabMap->insert(make_pair(std::string(fs_mountpoint), std::string(path)));
			}
		}
		// else if fs_spec equals "none", do not add line
		else if (StringEquals(fs_spec, "none"))
		{
			; // do nothing
		}
		// else, add line to path translation map
		else
		{
			ConvertUnixToWindowsPathSeparator(fs_spec, fs_spec, true);
			AppendUnixPathSeparator(fs_mountpoint);	
			FstabMap->insert(make_pair(std::string(fs_mountpoint), std::string(fs_spec)));		
		}
		
		readLineBuffer[0] = 0;
	}

	// close /etc/fstab file
	fclose(fp);

	return true;
}

#endif
   
int main(int argc, char *argv[])
{
	char  FstabName[_MAX_PATH+1];
	char  Win32PathName[_MAX_PATH+1];
	bool  isMSYS = false;

	if (2 != argc)
	{
		printf("usage: fstab paths.txt\n");
		exit(1);
	}

	isMSYS = fstab_init(FstabName);
	
	if (isMSYS)
	{
		isMSYS = fstab_load(FstabName);
		
		if (isMSYS)
		{
			char readLineBuffer[FSTAB_MAX_LINE];
			FILE *fp;

			fp = fopen (argv[1], "rt");

			if (NULL == fp)
			{
				printf("could not open '%s'\n", argv[1]);
				exit(1);
			}

			readLineBuffer[0] = 0;

			while(fgets(readLineBuffer, sizeof(readLineBuffer), fp) != NULL)
			{
				if ('\n' == readLineBuffer[strlen(readLineBuffer)-1])
				{
					readLineBuffer[strlen(readLineBuffer)-1] = 0;
				}
				
				Win32PathName[0] = 0;
				
				if (PathMingwToWin32(readLineBuffer, Win32PathName))
				{
					printf("MSYS: '%s' to '%s'\n", readLineBuffer, Win32PathName);
				}
				else
				{
					printf("MSYS: could not convert path '%s'\n", readLineBuffer);
				}
			}
			
			fclose(fp);
		}
		else
		{
			printf("MSYS: could not load fstab\n");
		}
	}
	else
	{
		printf("Windows: not running MSYS\n");
	}
}