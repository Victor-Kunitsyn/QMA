#ifndef CURL_DOWNLOADER_LIGHT_H
#define CURL_DOWNLOADER_LIGHT_H

#include <string>
#include <map>

#include "curl.h"

class CBaseMutexAbstract;

typedef std::map< std::string, std::string > HttpHeaders;

/// Class used to construct a URL and to open, read and process the data from the url.
class CURLDownloaderLight
{
public:
    enum IDownloaderError
    {
        UR_NO_ERROR,
        UR_FAILED,
        UR_NO_CONNECTION
    };

public:
    CURLDownloaderLight();
    ~CURLDownloaderLight();

    /// Clear the URL
    void ClearURL();

    /// wrapper to mURL->UseProxy
    void UseProxy(const char *proxy, const char *proxyPort);

    void SetHttpHeaders(const HttpHeaders& httpHeaders);

    void SetResponseHttpHeadersOutput(HttpHeaders* httpHeaders);

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
    bool SetURL(const char *url);

    /// Cancel getting data
    /// Requests a cancel and Blocks until any running getData ends
    ///
    void CancelURL();

    /// Read and process data from a url
    ///
    /// The method takes a function to use to parse the data. The parse function should expect to be passed the
    ///    param plus a non null terminated buffer and a length.
    /// @param[in] ParseFunc
    ///   The address of the function to use to process each buffer of data
    /// @param[in] param
    ///    A void pointer to pass to the function as the first argument of the function.
    /// @returns one of the IDownloaderError enums
    IDownloaderError GetData( void (*ParseFunc)(void *, const char *, int), void *param, unsigned long connect_timeout_ms = 3000, unsigned long data_timeout_ms = 5000, 
                              long* p_http_response = NULL, const char* post_params = NULL );

private:
    void finalize();
    void ParseResponseHeaders();
    static size_t onResponseWrapper(char* buffer, size_t size, size_t nmemb, void* pCURLReader);
    // static int debugOutputCallback(CURL * handle, curl_infotype type, char * msg, size_t size, void * param);
    void onResponse(const char * buffer, int len);
    
    CRITICAL_SECTION mRunningMutex;      // Mutex set while data is being fetched
    CRITICAL_SECTION mCancelMutex;       // Mutex to protect mCancelRequest
    bool mCancelRequest;      // Flag to say if the fetch should be cancelled
    CURLM* mHandle;
    CURL*  mReader;
    struct curl_slist *mHeadersList;
    void (*mParseFunc)(void *,const char *,int);
    void* mParseFuncParam;
    std::string mURLString;   // URL string that was passed in
    std::string mProxy;       // Name of the proxy to use if not empty string
    std::string mProxyPort;   // The proxyPort to use if not empty string
    HttpHeaders mHttpHeaders; // HTTP headers to use with connection
    HttpHeaders* mResponseHttpHeaders;
    std::string mResponseHttpHeadersBuffer;

};

#endif // CURL_DOWNLOADER_H
