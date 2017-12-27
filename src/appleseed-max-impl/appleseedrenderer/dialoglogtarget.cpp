
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Sergo Pogosyan, The appleseedhq Organization
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
#include "dialoglogtarget.h"

// appleseed-max headers.
#include "appleseedrenderer/resource.h"
#include "main.h"
#include "utilities.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/string.h"

// Boost headers.
#include "boost/thread/locks.hpp"
#include "boost/thread/mutex.hpp"

// 3ds Max headers.
#include <max.h>

// Standard headers.
#include <Richedit.h>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    boost::mutex                g_message_queue_mutex;
    std::vector<MessagePair>    g_message_queue;
    HWND                        g_log_dialog = nullptr;

    static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
          case WM_INITDIALOG:
            break;

          case WM_CLOSE:
          case WM_DESTROY:
            if (g_log_dialog != nullptr)
            {
                GetCOREInterface14()->UnRegisterModelessRenderWindow(g_log_dialog);
                g_log_dialog = nullptr;
                DestroyWindow(hwnd);
            }
            break;
          
          case WM_SIZE:
            {
                HWND edit_box = GetDlgItem(hwnd, IDC_EDIT_LOG);
                MoveWindow(
                    edit_box,
                    0,
                    0,
                    LOWORD(lparam),
                    HIWORD(lparam),
                    true);
                return TRUE;
            }
            break;

          default:
            return FALSE;
        }

        return TRUE;
    }

    void append_text(HWND edit_box, const wchar_t* text, COLORREF color)
    {
        const int text_length = GetWindowTextLength(edit_box);
        SendMessage(edit_box, EM_SETSEL, text_length, text_length);

        CHARFORMAT char_format;
        SendMessage(edit_box, EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));
        char_format.cbSize = sizeof(char_format);
        char_format.dwMask = CFM_COLOR | CFM_FACE;
        char_format.dwEffects = 0;
        char_format.crTextColor = color;
        wcscpy(char_format.szFaceName, L"Consolas");
        SendMessage(edit_box, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&char_format));

        SendMessage(edit_box, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
        SendMessage(edit_box, EM_LINESCROLL, 0, 1);
    }

    void print_message(const MessageType type, const StringVec& lines)
    {
        if (g_log_dialog == nullptr)
            return;

        for (const auto& line : lines)
        {
            COLORREF message_color;
            switch (type)
            {
              case MessageType::Error:
              case MessageType::Fatal:
                message_color = RGB(220, 10, 10);
                break;
              case MessageType::Warning:
                message_color = RGB(220, 100, 10);
                break;
              case MessageType::Debug:
                message_color = RGB(10, 150, 10);
                break;
              default:
                message_color = RGB(10, 10, 10);
                break;
            }

            append_text(
                GetDlgItem(g_log_dialog, IDC_EDIT_LOG),
                utf8_to_wide(line + "\n").c_str(),
                message_color);
        }
    }

    void push_message(const MessagePair& message)
    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);
        g_message_queue.push_back(message);
    }

    // Runs in UI thread.
    void emit_pending_messages()
    {
        if (g_log_dialog == nullptr)
        {
            g_log_dialog =
                CreateDialogParam(
                    g_module,
                    MAKEINTRESOURCE(IDD_DIALOG_LOG),
                    GetCOREInterface()->GetMAXHWnd(),
                    dialog_proc,
                    NULL);

            GetCOREInterface14()->RegisterModelessRenderWindow(g_log_dialog);
        }

        boost::mutex::scoped_lock lock(g_message_queue_mutex);

        for (const auto& message : g_message_queue)
            print_message(message.first, message.second);

        g_message_queue.clear();
    }

    // Runs in UI thread.
    void emit_saved_messages()
    {
        if (g_log_dialog != nullptr)
            SetDlgItemText(g_log_dialog, IDC_EDIT_LOG, L"");

        emit_pending_messages();
    }

    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;
}

DialogLogTarget::DialogLogTarget(const LogDialogMode open_mode)
  : m_log_mode(open_mode)
{
    m_session_messages.clear();
    g_message_queue.clear();
    asr::global_logger().add_target(this);
}

void DialogLogTarget::release()
{
    asr::global_logger().remove_target(this);
    delete this;
}

void DialogLogTarget::write(
    const MessageType       category,
    const char*             file,
    const size_t            line,
    const char*             header,
    const char*             message)
{
    std::vector<std::string> lines;
    asf::split(message, "\n", lines);

    m_session_messages.push_back(MessagePair(category, lines));

    push_message(MessagePair(category, lines));

    if (g_log_dialog)
        print_to_dialog();
    else
    {
        switch (category)
        {
          case asf::LogMessage::Category::Error:
          case asf::LogMessage::Category::Fatal:
          case asf::LogMessage::Category::Warning:
            if (m_log_mode != LogDialogMode::Never)
                print_to_dialog();
            break;
          case asf::LogMessage::Category::Debug:
          case asf::LogMessage::Category::Info:
            if (m_log_mode == LogDialogMode::Always)
                print_to_dialog();
            break;
        }
    }
}

void DialogLogTarget::show_last_session_messages()
{
    if (g_log_dialog)
        return;

    {
        boost::mutex::scoped_lock lock(g_message_queue_mutex);
        g_message_queue.clear();
        for (const auto& message : m_session_messages)
            g_message_queue.push_back(message);
    }

    PostMessage(
        GetCOREInterface()->GetMAXHWnd(),
        WM_TRIGGER_CALLBACK,
        reinterpret_cast<WPARAM>(emit_saved_messages),
        0);
}

void DialogLogTarget::print_to_dialog()
{
    PostMessage(
        GetCOREInterface()->GetMAXHWnd(),
        WM_TRIGGER_CALLBACK,
        reinterpret_cast<WPARAM>(emit_pending_messages),
        0);
}
