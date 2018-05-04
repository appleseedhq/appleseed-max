
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Sergo Pogosyan, The appleseedhq Organization
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

#pragma once

// appleseed.foundation headers.
#include "foundation/utility/log.h"

// Standard headers.
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <utility>

typedef std::vector<std::string> StringVec;
typedef foundation::LogMessage::Category MessageType;

struct MessageRecord
{
    MessageType m_type;
    std::string m_header;
    StringVec   m_lines;

    MessageRecord() {}
    MessageRecord(
        const MessageType   type,
        const std::string&  header,
        const StringVec&    lines)
      : m_type(type)
      , m_header(header)
      , m_lines(lines)
    {
    }
};

class DialogLogTarget
  : public foundation::ILogTarget
{
  public:
    enum class OpenMode
    {
        // Changing these value WILL break compatibility.
        Always  = 0,
        Never   = 1,
        Errors  = 2
    };

    explicit DialogLogTarget(
        const OpenMode          open_mode);

    void release() override;

    void write(
        const MessageType       category,
        const char*             file,
        const size_t            line,
        const char*             header,
        const char*             message) override;

    void show_last_session_messages();

  private:
    std::vector<MessageRecord>  m_session_messages;
    OpenMode                    m_open_mode;

    void print_to_dialog();
};
