
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016-2017 Francois Beaune, The appleseedhq Organization
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
#include "logtarget.h"

// appleseed-max headers.
#include "utilities.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/string.h"

// 3ds Max headers.
#include <log.h>
#include <max.h>

// Standard headers.
#include <vector>
#include <string>

namespace asf = foundation;

namespace
{
    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;

    struct MessageData
    {
        std::vector<std::string> lines;
        DWORD type;
    };
   
    void write_log_lines(const std::vector<std::string>& lines, const int type)
    {
        for (auto line : lines)
        {
            GetCOREInterface()->Log()->LogEntry(
                type,
                FALSE,
                _T("appleseed"),
                _T("[appleseed] %s"),
                utf8_to_wide(line).c_str());
        }
    }

    void ui_log_writer(UINT_PTR param_ptr)
    {
        MessageData* data_ptr = reinterpret_cast<MessageData*>(param_ptr);

        write_log_lines(data_ptr->lines, data_ptr->type);

        delete data_ptr;
    }
}

void LogTarget::release()
{
    delete this;
}

void LogTarget::write(
    const asf::LogMessage::Category category,
    const char*                     file,
    const size_t                    line,
    const char*                     header,
    const char*                     message)
{
    DWORD type;
    switch (category)
    {
      case asf::LogMessage::Debug: type = SYSLOG_DEBUG; break;
      case asf::LogMessage::Info: type = SYSLOG_INFO; break;
      case asf::LogMessage::Warning: type = SYSLOG_WARN; break;
      case asf::LogMessage::Error:
      case asf::LogMessage::Fatal:
      default:
        type = SYSLOG_ERROR;
        break;
    }

#if MAX_RELEASE > MAX_RELEASE_R17
    auto is_ui_thread = GetCOREInterface15()->GetMainThreadID() == GetCurrentThreadId();
#else
    auto is_ui_thread = reinterpret_cast<Interface15*>(GetCOREInterface()->GetInterface(Interface15::kInterface15InterfaceID))->GetMainThreadID() == GetCurrentThreadId();
#endif

    if (is_ui_thread)
    {
        std::vector<std::string> lines;
        asf::split(message, "\n", lines);
        write_log_lines(lines, type);
    }
    else
    {
        MessageData* data = new MessageData();
        asf::split(message, "\n", data->lines);
        data->type = type;

        PostMessage(GetCOREInterface()->GetMAXHWnd(), WM_TRIGGER_CALLBACK, (UINT_PTR)ui_log_writer, (UINT_PTR)data);
    }
}
