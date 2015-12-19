
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "updatechecker.h"

// RapidJSON headers.
#include "3rdparty/rapidjson/document.h"

// Windows headers.
#include <Windows.h>
#include <wininet.h>
#include <tchar.h>

// Standard headers.
#include <cstddef>
#include <ctime>
#include <exception>
#include <vector>

namespace json = rapidjson;

namespace
{
    //
    // Code review:
    // http://codereview.stackexchange.com/questions/112330/date-of-latest-github-project-release
    //

    class HInternet
    {
      public:
        HInternet(const HINTERNET handle)
          : m_handle(handle) {}

        ~HInternet()
        {
            if (m_handle != nullptr)
                InternetCloseHandle(m_handle);
        }

        operator HINTERNET() const
        {
            return m_handle;
        }

      private:
        const HINTERNET m_handle;
    };

    DWORD query_release_information(std::string& response)
    {
        static const TCHAR* Host = _T("api.github.com");
        static const TCHAR* Path = _T("/repos/appleseedhq/appleseed-max/releases");

        response.clear();

        const HInternet session =
            InternetOpen(
                _T("appleseed"),
                INTERNET_OPEN_TYPE_DIRECT,
                nullptr,
                nullptr,
                0);
        if (!session)
            return GetLastError();

        const HInternet connection =
            InternetConnect(
                session,
                Host,
                INTERNET_DEFAULT_HTTPS_PORT,
                nullptr,
                nullptr,
                INTERNET_SERVICE_HTTP,
                0,
                0);
        if (!connection)
            return GetLastError();

        static const TCHAR* AcceptTypes[] = { _T("application/json"), nullptr };
        const HInternet request =
            HttpOpenRequest(
                connection,
                _T("GET"),
                Path,
                nullptr,
                nullptr,
                AcceptTypes,
                INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD,
                0);
        if (!request)
            return GetLastError();

        if (!HttpSendRequest(request, nullptr, 0, nullptr, 0))
            return GetLastError();

        std::vector<char> buffer(4096);

        while (true)
        {
            DWORD bytes_read;
            if (!InternetReadFile(request, &buffer[0], static_cast<DWORD>(buffer.size()), &bytes_read))
            {
                const DWORD last_error = GetLastError();
                if (last_error == ERROR_INSUFFICIENT_BUFFER)
                    buffer.resize(buffer.size() * 2);
                else return last_error;
            }

            if (bytes_read > 0)
                response.append(&buffer[0], bytes_read);
            else return ERROR_SUCCESS;
        }
    }

    struct JSONMemberNotFound : public std::exception {};

    const json::Value& get_member(const json::Value& parent, const json::Value::Ch* member)
    {
        if (!parent.HasMember(member))
            throw JSONMemberNotFound();
        return parent[member];
    }

    const json::Value& get_index(const json::Value& parent, const json::SizeType index)
    {
        if (!parent.IsArray() || index >= parent.Size())
            throw JSONMemberNotFound();
        return parent[index];
    }
}

bool check_for_update(
    std::string&    version_string,
    std::string&    publication_date,
    std::string&    download_url)
{
    std::string json;
    if (query_release_information(json) != ERROR_SUCCESS)
        return false;

    json::Document doc;
    if (doc.Parse(json.c_str()).HasParseError())
        return false;

    try
    {
        const auto& release = get_index(doc, 0);
        const auto& tag_name = get_member(release, "tag_name");
        const auto& published_date = get_member(release, "published_at");
        const auto& assets = get_member(release, "assets");
        const auto& asset = get_index(assets, 0);
        const auto& asset_download_url = get_member(asset, "browser_download_url");

        int year, month, day, hours, minutes, seconds;
        std::sscanf(
            published_date.GetString(),
            "%d-%d-%dT%d:%d:%d:%dZ",
            &year, &month, &day, &hours, &minutes, &seconds);

        std::tm time;
        time.tm_year = year - 1900;
        time.tm_mon = month - 1;
        time.tm_mday = day;
        time.tm_hour = hours;
        time.tm_min = minutes;
        time.tm_sec = seconds;

        const std::time_t t = std::mktime(&time);
        const std::tm* local_time = std::localtime(&t);

        char buffer[1024];
        std::strftime(buffer, sizeof(buffer), "%b %d %Y", local_time);

        version_string = tag_name.GetString();
        publication_date = std::string(buffer);
        download_url = asset_download_url.GetString();

        return true;
    }
    catch (const JSONMemberNotFound&)
    {
        return false;
    }
}
