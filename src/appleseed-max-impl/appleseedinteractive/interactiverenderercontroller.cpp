
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
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

// Interface header.
#include "interactiverenderercontroller.h"

// 3ds Max headers.
#include <interactiverender.h>

// Standard headers.
#include <utility>

namespace asr = renderer;

InteractiveRendererController::InteractiveRendererController()
  : m_status(ContinueRendering)
{
}

void InteractiveRendererController::on_rendering_begin()
{
    for (auto& updater : m_scheduled_actions)
        updater->update();
    
    m_scheduled_actions.clear();
    m_status = ContinueRendering;
}

asr::IRendererController::Status InteractiveRendererController::get_status() const
{
    return m_status;
}

void InteractiveRendererController::set_status(const Status status)
{
    m_status = status;
}

void InteractiveRendererController::schedule_update(std::unique_ptr<ScheduledAction> updater)
{
    m_scheduled_actions.push_back(std::move(updater));
}
