#include "stdafx.h"

#include "DropBoxAccessor.h"

#include <QFile>
#include <QBuffer>

#include "dropbox/files/FilesRoutes.h"

static const char access_token[] = "ya1yb9RlhpAAAAAAAAAAk0961HXE4sgZBB1B6T6IBhPECb4iNFtraIROftk2SNmT";  //< access token

CDropBoxAccessor::CDropBoxAccessor() : cli(access_token)
{
}

CDropBoxAccessor::~CDropBoxAccessor()
{
}

bool CDropBoxAccessor::ReadFile(const char* file, memStream& mem_stream, bool& file_not_exist)
{
	bool ret = false;

	memStreamCleanup(&mem_stream);

	file_not_exist = !(cli.fileExists(file));

	if ( !file_not_exist )
	{
		ret = true;
		
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		buffer.open(QIODevice::WriteOnly);

		files::DownloadArg d(file);

		try
		{			
			cli.getFiles()->download(d, &buffer);
		}
		catch (DropboxException& )
		{
			ret = false;
		}

		if (ret)
		{
			memStreamWrite(byteArray.constData(), 1, byteArray.size(), &mem_stream);

			if (mem_stream.data && (mem_stream.size > 0))
			{
				if (mem_stream.data[mem_stream.size - 1] != 0x0)
				{
					memStreamWrite("\x0", 1, 1, &mem_stream);
				}
			}
		}
	}

	return ret;
}

bool CDropBoxAccessor::PutFile(const char* file, const memStream& mem_stream)
{
	bool ret = false;
	
	if ( mem_stream.data )
	{
		ret = true;
		
		QByteArray byteArray(mem_stream.data, mem_stream.size);
		QBuffer buffer(&byteArray);
		buffer.open(QIODevice::ReadOnly);

		files::CommitInfo arg(file);
		arg.setMode(files::WriteMode::WriteMode_OVERWRITE);

		try
		{
			cli.getFiles()->upload(arg, &buffer);
		}
		catch (DropboxException& )
		{
			ret = false;
		}
	}

	return ret;	
}


