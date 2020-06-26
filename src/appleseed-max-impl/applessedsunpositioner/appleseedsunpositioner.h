#pragma once


//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2020 João Marcos Costa, The appleseedhq Organization
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

// appleseed-max headers.
#include "resource.h"
#include "renderer/utility/solarpositionalgorithm.h"


// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <iparamm2.h>
#include "appleseed-max-common/_endmaxheaders.h"


class SunPositionerWrapper
{
public:
    SunPositionerWrapper();
    ~SunPositionerWrapper();

    void update(IParamBlock2* const pblock, TimeValue& t, Interval& params_validity);

    float get_phi() const;
    float get_theta() const;
   
private:

    foundation::auto_release_ptr<renderer::SunPositioner>   m_sun_positioner;
    int                                                     m_hour;
    int                                                     m_minute;
    int                                                     m_second;
    int                                                     m_month;
    int                                                     m_day;
    int                                                     m_year;
    int                                                     m_timezone;
    float                                                   m_north;
    float                                                   m_latitude;
    float                                                   m_longitude;
 };