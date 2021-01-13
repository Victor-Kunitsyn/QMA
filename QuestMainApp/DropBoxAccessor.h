#pragma once

#include <iostream>
#include "memStream.h"
#include "dropbox/DropboxClient.h"

using namespace dropboxQt;

class CDropBoxAccessor
{
public:
	CDropBoxAccessor();
	virtual ~CDropBoxAccessor();

	bool ReadFile(const char* file, memStream& mem_stream, bool& file_not_exist);
	bool PutFile(const char* file, const memStream& mem_stream);

private:
	DropboxClient cli;
};