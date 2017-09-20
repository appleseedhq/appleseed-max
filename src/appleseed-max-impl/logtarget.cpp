
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

// Boost headers.
#include "boost/thread/locks.hpp"
#include "boost/thread/mutex.hpp"

// 3ds Max headers.
#include <log.h>
#include <max.h>

// Standard headers.
#include <string>
#include <vector>

namespace asf = foundation;

namespace
{
    typedef std::vector<std::string> StringVec;

    void emit_message(const DWORD type, const StringVec& lines)
    {
        for (const auto& line : lines)
        {
            GetCOREInterface()->Log()->LogEntry(
                type,
                FALSE,
                L"appleseed",
                L"[appleseed] %s",
                utf8_to_wide(line).c_str());
        }
    }

    struct Message
    {
        DWORD               m_type;
        StringVec           m_lines;
    };

    boost::mutex            g_message_queue_mutex;
    std::vector<Message>    g_message_queue;

    void push_message(const DWORD type, const StringVec& lines)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        Message message;
        message.m_type = type;
        message.m_lines = lines;
        g_message_queue.push_back(message);
    }

    void emit_pending_messages()
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        for (const auto& message : g_message_queue)
            emit_message(message.m_type, message.m_lines);

        g_message_queue.clear();
    }

    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;

    bool is_main_thread()
    {
        // There does not appear to be a GetCOREInterface15() function in the 3ds Max 2015 SDK.
        Interface15* interface15 =
            reinterpret_cast<Interface15*>(GetCOREInterface14()->GetInterface(Interface15::kInterface15InterfaceID));

        return interface15->GetMainThreadID() == GetCurrentThreadId();
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

    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    if (is_main_thread())
        emit_message(type, lines);
    else
    {
        push_message(type, lines);
        PostMessage(
            GetCOREInterface()->GetMAXHWnd(),
            WM_TRIGGER_CALLBACK,
            reinterpret_cast<WPARAM>(emit_pending_messages),
            0);
    }
}
