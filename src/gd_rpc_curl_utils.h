#pragma once
#define CURL_STATICLIB
#include <curl/curl.h>
#include <string>

namespace rpc
{
    static size_t write_function(
        void* ptr,
        size_t size,
        size_t nmemb,
        std::string* data)
    {
        data->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    static std::string _unsafe_post_http_request(
        const char* url,
        const char* fields)
    {
        std::string response_string;
        auto curl = curl_easy_init();

        if (!curl) return response_string;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: */*");
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "User-Agent: ");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        std::string header_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

        char* _url;
        long response_code;
        double elapsed;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &_url);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        return response_string;
    }

    static std::string post_http_request(
        const std::string& url,
        const std::string& fields)
    {
        try
        {
            return _unsafe_post_http_request(url.c_str(), fields.c_str());
        }
        catch (...)
        {
            return std::string();
        }
    }
}