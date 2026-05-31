#ifndef __INIMGT_H
#define __INIMGT_H

#pragma once

#include "globalsettings.h"

#include "modelfitting.h"
#include "structurefitting.h"
#include "bgfitting.h"

#include "simpleini/SimpleIni.h"


double GetIniDouble (const std::wstring& file, const std::string& object, const std::string& param);
void   SetIniDouble (const std::wstring& file, const std::string& object, const std::string& param,
						     double value, int precision = 6);

int  GetIniInt (const std::wstring& file, const std::string& object, const std::string& param);
void SetIniInt (const std::wstring& file, const std::string& object, const std::string& param,
					    int value);

char GetIniChar (const std::wstring& file, const std::string& object, const std::string& param);
void SetIniChar (const std::wstring& file, const std::string& object, const std::string& param,
						 char value);

void GetIniString(const std::wstring& file, const std::string& section, const std::string& key, 
						   std::string& result);
void SetIniString (const std::wstring& file, const std::string& object, const std::string& param,
						   const std::string& value);

EXPORTED void ReadParameters(const std::wstring &filename, std::string obj, paramStruct *p, void* ini);
EXPORTED void WriteParameters(const std::wstring &filename, std::string obj, paramStruct *p, void* ini);

EXPORTED void ReadPeaks(const std::wstring &filename, std::string obj, peakStruct *peaks, void* ini);
EXPORTED void WritePeaks(const std::wstring &filename, std::string obj, const peakStruct *peaks, void* ini);

EXPORTED void WriteBG(const std::wstring &filename, std::string obj, const bgStruct *BGs, void* ini);
EXPORTED void ReadBG(const std::wstring &filename, std::string obj, bgStruct *BGs, void* ini);

EXPORTED void WriteCaille(const std::wstring &filename, std::string obj, const graphTable *Cailles, const cailleParamStruct *Caillesdrawing, void* ini);
EXPORTED void ReadCaille(const std::wstring &filename, std::string obj, graphTable *Cailles, cailleParamStruct *Caillesdrawing, void* ini);

EXPORTED void WritePhases(const std::wstring &filename, std::string obj, const phaseStruct *ph, int pt, void* ini);
EXPORTED void ReadPhases(const std::wstring &filename, std::string obj, phaseStruct *ph, int *pt, void* ini);

EXPORTED bool IniHasModelType(const std::wstring& file, const std::string& object, void* ini);


EXPORTED double GetIniDouble (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED void   SetIniDouble (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
				     double value, int precision = 6);

EXPORTED int  GetIniNoOfParameters (const std::wstring& file, const std::string& object, const std::string& param, void* ini);

EXPORTED int  GetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED int  GetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini, int defval);
EXPORTED void SetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
			    int value);

EXPORTED char GetIniChar (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED void SetIniChar (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
				 char value);

EXPORTED void GetIniString(const std::wstring& file, const std::string& section, const std::string& key, 
				   std::string& result, void* ini);
EXPORTED void SetIniString (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
				   const std::string& value);
 
EXPORTED bool ReadableIni(const std::wstring& file, const std::string& object, void* ini);
EXPORTED bool WritableIni(const std::wstring& file, const std::string& object, void* ini);

// Instantiates ini as a CSimpleIniA
EXPORTED void *NewIniFile();
// Deletes ini
EXPORTED void CloseIniFile(void* ini);


#endif
