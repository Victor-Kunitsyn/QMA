#include "stdafx.h"
#include "CURLDownloaderLight.h"

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <sstream>

CURLDownloaderLight::CURLDownloaderLight()
{
    mHeadersList = NULL;
    mResponseHttpHeaders = NULL;
    
    curl_global_init(CURL_GLOBAL_ALL);
    ClearURL();

    ::InitializeCriticalSection( &mRunningMutex );
    ::InitializeCriticalSection( &mCancelMutex );
}

CURLDownloaderLight::~CURLDownloaderLight()
{
    // cancel any fetch and wait for getData() to end
    CancelURL();
    curl_global_cleanup();

    ::DeleteCriticalSection( &mRunningMutex );
    ::DeleteCriticalSection( &mCancelMutex );
}

void CURLDownloaderLight::ClearURL(void)
{
    mURLString = "";
}

///  Set or modify the URL to use
///  Treated as an arg
///    If the string starts with "?" and the current url contains a "?"  a "&" is appended with the rest of the string
///   Otherwise If the string starts with "?" string is appended
///  Treated as a full URL
///   Otherwise If the string contains "://" it sets the url
///  Treated as a protocol
///   Otherwise If the string ends with : it sets the url
///  Treated as data to append
///   Otherwise the string is appended.  
/// @returns true on success and false on failure
bool CURLDownloaderLight::SetURL(const char *url)
{
    mCancelRequest = false;
    // The order that these checks are performed is important!
    if(*url == '?')
    {
        if(mURLString.find('?') != std::string::npos)
        {
            mURLString.append("&");
            mURLString.append(url+1);
            return true;
        }
        else
        {
            mURLString.append(url);
            return true;
        }
    }

    if( strstr(url,"://") != NULL)
    {
        mURLString = url;
        return true;
    }

    if( *url && *(url + strlen(url) - 1) == ':' )
    {
        mURLString = url;
        return true;
    }

    mURLString.append(url);
    return true;
}

void CURLDownloaderLight::UseProxy(const char *proxy, const char *proxyPort)
{
    mProxyPort = proxyPort ? proxyPort : "";
    mProxy = proxy ? proxy : "";
}

void CURLDownloaderLight::SetHttpHeaders(const HttpHeaders& httpHeaders)
{
    mHttpHeaders = httpHeaders;
}

void CURLDownloaderLight::SetResponseHttpHeadersOutput(HttpHeaders* httpHeaders)
{
    mResponseHttpHeaders = httpHeaders;
}
void CURLDownloaderLight::CancelURL()
{
    ::EnterCriticalSection(&mCancelMutex);
    mCancelRequest = true;
    ::LeaveCriticalSection(&mCancelMutex);

    ::EnterCriticalSection(&mRunningMutex);// wait for getData to cancel 
    ::LeaveCriticalSection(&mRunningMutex);
}
static size_t HeadersFunction(void *ptr, size_t size, size_t nmemb, void *string)
{
    ((std::string *) string)->append((char*)ptr, size * nmemb);

    return size * nmemb;
}

std::string ansi2utf8(const std::string& s)
{
	USES_CONVERSION;
	_acp = CP_ACP;
	wchar_t* pw = A2W(s.c_str());

	_acp = CP_UTF8;
	return W2A(pw);
}

std::string ReplaceSpaces(const std::string& s)
{
	std::string ret = "";
	
	for (int i = 0; i < s.size(); i++)
	{
		ret += ( (s[i] == ' ') ? '+' : s[i] );
	}
	
	return ret;
}

CURLDownloaderLight::IDownloaderError CURLDownloaderLight::GetData( void (*pParseFunc)(void *, const char *, int), void *param, unsigned long connect_timeout_ms /*= 3000*/, unsigned long data_timeout_ms /*= 5000*/,
                                                                    long* p_http_response /*= NULL*/, const char* post_params /*= NULL*/)
{
    ::EnterCriticalSection(&mRunningMutex);

    CURLMcode result(CURLM_OK);
    mParseFunc = pParseFunc;
    mParseFuncParam = param;
    IDownloaderError ret_status = UR_NO_ERROR;
    //initialize section: init & setup curl
    ::EnterCriticalSection(&mCancelMutex);

    do // breakable block start
    {
        if(mCancelRequest)
        {
            ret_status = UR_FAILED;
            break;
        }
        mHandle = curl_multi_init();
        if(!mHandle)
        {
            //LOG_ERROR(10, ("LibCURL multi init failed"));
            ret_status = UR_FAILED;
            break;
        }
        mReader = curl_easy_init();
        if(!mReader)
        {
            //LOG_ERROR(10, ("LibCURL easy init failed"));
            finalize();
            ret_status = UR_FAILED;
            break;
        }
        curl_multi_setopt(mHandle, CURLMOPT_MAXCONNECTS, 10);
        if(mHttpHeaders.size() > 0)
        {
            for(HttpHeaders::const_iterator iter = mHttpHeaders.begin(); iter != mHttpHeaders.end(); ++iter)
            {
                mHeadersList = curl_slist_append(mHeadersList, (iter->first + ": " + iter->second).c_str());
            }

            curl_easy_setopt(mReader, CURLOPT_HTTPHEADER, mHeadersList);
        }

        if(mResponseHttpHeaders)
        {
            curl_easy_setopt(mReader, CURLOPT_HEADERFUNCTION, HeadersFunction);
            curl_easy_setopt(mReader, CURLOPT_HEADERDATA, &mResponseHttpHeadersBuffer);
        }

		std::string conv_url = ansi2utf8(mURLString);
		conv_url = ReplaceSpaces(conv_url);
		
		curl_easy_setopt(mReader, CURLOPT_URL, conv_url.c_str());

		curl_easy_setopt(mReader, CURLOPT_WRITEFUNCTION, &onResponseWrapper);
        curl_easy_setopt(mReader, CURLOPT_WRITEDATA, this);

        if(post_params)
        {
            curl_easy_setopt(mReader, CURLOPT_POST, true);
            curl_easy_setopt(mReader, CURLOPT_POSTFIELDS, post_params);
        }

        curl_easy_setopt(mReader, CURLOPT_SSL_VERIFYPEER, 0L); //tell curl not to verify the host
        curl_easy_setopt(mReader, CURLOPT_SSL_VERIFYHOST, 0L); //tell curl not to verify the host
        curl_easy_setopt(mReader, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3); //tell curl not to verify the host

        curl_easy_setopt(mReader, CURLOPT_TIMEOUT_MS, data_timeout_ms); //data timeout
        curl_easy_setopt(mReader, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms); //connection timeout 

        if(!mProxy.empty() && !mProxyPort.empty())
        {
            curl_easy_setopt(mReader,CURLOPT_PROXY,mProxy.c_str());
            curl_easy_setopt(mReader,CURLOPT_PROXYPORT,atol(mProxyPort.c_str()));
            curl_easy_setopt(mReader,CURLOPT_PROXYTYPE,CURLPROXY_SOCKS4A);
        }

        result = curl_multi_add_handle(mHandle, mReader);
        if(result != CURLM_OK)
        {
            //LOG_ERROR(10, ("LibCURL add handle failed: %d(%s) url=[%s]", result, curl_multi_strerror(result), mURLString.c_str()));
            finalize();
            ret_status = UR_FAILED;
            break;
        }

    }while(false);// breakable block end

    ::LeaveCriticalSection(&mCancelMutex);

    if(ret_status != UR_NO_ERROR)
    {
        ::LeaveCriticalSection(&mRunningMutex);
        return ret_status;
    }

    //active transfer section  
    while(true)
    {
        int runningCount(0);
        result = curl_multi_perform(mHandle, &runningCount);
        if(result != CURLM_OK || !runningCount)
        {
            break;
        }

        ::EnterCriticalSection(&mCancelMutex);

        if(mCancelRequest)
        {
            ::LeaveCriticalSection(&mCancelMutex);
            break;
        }
        else
            ::LeaveCriticalSection(&mCancelMutex);

        int numFds = 0;
        result = curl_multi_wait(mHandle, NULL, 0, 30000, &numFds);

        if(result != CURLM_OK)
        {
            break;
        }

        ::EnterCriticalSection(&mCancelMutex);

        if(mCancelRequest)
        {
            ::LeaveCriticalSection(&mCancelMutex);
            break;
        }
        else
            ::LeaveCriticalSection(&mCancelMutex);
    }

    //finalize section
    do // breakable block start
    {
        ::EnterCriticalSection(&mCancelMutex);

        if(mCancelRequest)
        {
            ::LeaveCriticalSection(&mCancelMutex);
            ret_status = UR_FAILED;
            break;
        }
        else
            ::LeaveCriticalSection(&mCancelMutex);

        long http_response;
        curl_easy_getinfo(mReader, CURLINFO_RESPONSE_CODE, &http_response);
        
        if (http_response != 200L)
            ret_status = UR_NO_CONNECTION;

        if(p_http_response)
            *p_http_response = http_response;
        
        if(result != CURLM_OK)
        {
            //LOG_ERROR(10, ("LibCURL perform/wait failed: %d(%s) url=[%s]", result, curl_multi_strerror(result), mURLString.c_str()));
            ret_status = UR_FAILED;
            break;
        }

        result = curl_multi_remove_handle(mHandle, mReader);

        if(result != CURLM_OK)
        {
            //LOG_ERROR(10, ("LibCURL remove handle failed: %d(%s) url=[%s]", result, curl_multi_strerror(result), mURLString.c_str()));
            ret_status = UR_FAILED;
            break;
        }
        ParseResponseHeaders();
    }
    while(0);// breakable block end

    finalize();
    ::LeaveCriticalSection(&mRunningMutex);

    return ret_status;//UR_NO_ERROR;
}

void CURLDownloaderLight::ParseResponseHeaders()
{
    if( mResponseHttpHeaders == NULL )
        return;

    std::stringstream stream(mResponseHttpHeadersBuffer);
    std::string line;

    while (std::getline(stream, line, '\n'))
    {
        size_t end_pos = line.find_last_not_of("\r");
        
        if( end_pos != std::string::npos )
            line = line.substr(0, end_pos+1);

        size_t idx = line.find(": ");
        
        if( idx != std::string::npos )
        {
            std::string key = line.substr(0, idx);
            std::string value = line.substr(idx + 2);

            mResponseHttpHeaders->insert(HttpHeaders::value_type(key, value));
        }
    }
}
void CURLDownloaderLight::finalize()
{
    if(mHandle)
    {
        curl_multi_cleanup(mHandle);
        mHandle = NULL;
    }

    if(mReader)
    {
        curl_easy_cleanup(mReader);
        mReader = NULL;
    }
    if(mHeadersList)
    {
        curl_slist_free_all(mHeadersList);
        mHeadersList = NULL;
    }
}

size_t CURLDownloaderLight::onResponseWrapper(char* buffer, size_t size, size_t nmemb, void* pCURLReader)
{
    (*(CURLDownloaderLight *)pCURLReader).onResponse(buffer, size * nmemb );
    return size * nmemb; //tell curl how many bytes we handled
}

void CURLDownloaderLight::onResponse(const char * readBuffer, int sizeRead)
{
    if(mCancelRequest)
    {
        return;
    }
    (mParseFunc)(mParseFuncParam, readBuffer, sizeRead);
    return;
}

