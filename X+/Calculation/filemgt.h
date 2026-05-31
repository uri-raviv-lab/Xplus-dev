#ifndef __FILEMGT_H
#define __FILEMGT_H

#include <vector>

#include "globalsettings.h"

EXPORTED int CheckSizeOfFile(const wchar_t *filename);

EXPORTED void ReadDataFile(const wchar_t *filename,
				           std::vector<double>& x, 
						   std::vector<double>& y);

EXPORTED void Read1DDataFile(const wchar_t *filename,
				             std::vector<double>& x);

EXPORTED void WriteDataFile(const wchar_t *filename, std::vector<double>& x, 
                   std::vector<double>& y);

EXPORTED void WriteDataFileWHeader(const wchar_t *filename, std::vector<double>& x, 
				std::vector<double>& y, std::stringstream& header);

EXPORTED void Write3ColDataFile(const wchar_t *filename, std::vector<double>& x, 
							std::vector<double>& y, std::vector<double>& err);

EXPORTED void Write1DDataFile(const wchar_t *filename, std::vector<double>& x);

EXPORTED void GetDirectory(const wchar_t *file, wchar_t *result, int n = 260);
EXPORTED void GetBasename(const wchar_t *file, wchar_t *result, int n = 260);

/*
EXPORTED std::string GetDirectory(const std::string file);
EXPORTED std::string GetBasename(const std::string file) ;
*/

#endif
