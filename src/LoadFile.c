/*
* Tilengine - The 2D retro graphics engine with raster effects
* Copyright (C) 2015-2019 Marc Palacios Domenech <mailto:megamarc@hotmail.com>
* All rights reserved
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LoadFile.h"
#include "ResPack.h"

#define SLASH	  '/'
#define BACKSLASH '\\'
#define MAX_PATH	300
#define MAX_ASSETS	8

static char localpath[MAX_PATH] = ".";
static ResPack respack = NULL;
struct
{
	ResAsset asset;
	FILE* pf;
}
static assets[MAX_ASSETS] = { 0 };

/*!
 * \brief
 * Sets base path for TLN_LoadXXX functions.
 * 
 * \param path
 * Base path. Files will load at path/filename. Can be NULL
 */
void TLN_SetLoadPath (const char* path)
{
	size_t trailing;

	if (path)
		strncpy (localpath, path, MAX_PATH);
	else
		strncpy (localpath, ".", MAX_PATH);

	localpath[MAX_PATH - 1] = '\0';

	/* cut trailing separator */
	trailing = strlen (localpath) - 1;
	if (trailing > 0 && (localpath[trailing] == SLASH || localpath[trailing] == BACKSLASH))
		localpath[trailing] = 0;
}

/*!
 * \brief Open the resource package with optional aes-128 key and binds it
 * \param filename file with the resource package (.dat extension)
 * \param key optional null-terminated ASCII string with aes decryption key
 * \return true if package opened and made current, or false if error
 * \remarks
 * When the package is opened, it's globally bind to all TLN_LoadXXX functions. 
 * The assets inside the package are indexed with their original path/file as when 
 * they were plain files. As long as the structure used to build the package
 * matches the original structure of the assets, the TLN_SetLoadPath() and the TLN_LoadXXX
 * functions will work transparently, easing the migration with minimal changes.
 * \sa TLN_CloseResourcePack
 */
bool TLN_OpenResourcePack(const char* filename, const char* key)
{
	respack = ResPack_Open(filename, key);
	return respack != NULL;
}

/*!
 * \brief Closes current resource package and unbinds it 
 * \sa TLN_OpenResourcePack
 */
void TLN_CloseResourcePack(void)
{
	if (respack != NULL)
		ResPack_Close(respack);
	respack = NULL;
}

/* open file/packed asset */
FILE* FileOpen (const char* filename)
{
	FILE* pf;
	char path[MAX_PATH + 1];
	char oldchar, newchar;
	char* p;
	ResAsset asset;

#if (_MSC_VER) && (_MSC_VER < 1900)
	sprintf (path, "%s/%s", localpath, filename);
#else
	snprintf (path, sizeof(path), "%s/%s", localpath, filename);
#endif

	/* replace correct path separator */
	p = path;
#if defined (_MSC_VER)
	oldchar = SLASH;
	newchar = BACKSLASH;
#else
	oldchar = BACKSLASH;
	newchar = SLASH;
#endif
	while (*p != 0)
	{
		if (*p == oldchar)
			*p = newchar;
		p++;
	}

	/* asset pack active? */
	if (respack != NULL)
	{
		/* search free slot */
		int c;
		for (c = 0; c < MAX_ASSETS; c++)
		{
			if (assets[c].asset == NULL)
				break;
		}
		if (c == MAX_ASSETS)
			return NULL;

		/* open */
		asset = ResPack_OpenAsset(respack, path);
		pf = ResPack_GetAssetFile(asset);
		if (pf != NULL)
		{
			assets[c].asset = asset;
			assets[c].pf = pf;
		}
	}
	else
		pf = fopen (path, "rb");

	return pf;
}

/* closes file/packed asset */
void FileClose(FILE* pf)
{
	if (pf == NULL)
		return;

	/* asset pack active? */
	if (respack != NULL)
	{
		int c;
		for (c = 0; c < MAX_ASSETS; c++)
		{
			if (assets[c].pf == pf)
			{
				ResPack_CloseAsset(assets[c].asset);
				assets[c].asset = NULL;
				assets[c].pf = NULL;
				return;
			}
		}
	}
	else
		fclose(pf);
}

/* generic load file into RAM buffer */
void* LoadFile (const char* filename, ssize_t* out_size)
{
	size_t size;
	FILE* pf;
	uint8_t* data;

	/* abre */
	pf = FileOpen (filename);
	if (!pf)
	{
		*out_size = 0;
		return NULL;
	}

	/* carga */
	fseek (pf, 0, SEEK_END);
	size = ftell (pf);
	fseek (pf, 0, SEEK_SET);
	data = malloc (size + 1);
	if (data)
	{
		fread (data, size, 1, pf);
		data[size] = 0;
		*out_size = size;
	}
	else
		*out_size = -1;

	FileClose (pf);
	return (void*)data;
}

/* check if file exists */
bool CheckFile (const char* filename)
{
	FILE* pf;

	pf = FileOpen (filename);
	if (!pf)
		return false;

	FileClose (pf);
	return true;
}
