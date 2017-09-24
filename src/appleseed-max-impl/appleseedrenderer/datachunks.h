
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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
#include "foundation/platform/windows.h"

//
// Changing the values of these constants WILL break compatibility
// with 3ds Max files saved with older versions of the plugin.
//

const USHORT ChunkFileFormatVersion                     = 0x0001;

const USHORT ChunkSettings                              = 0x1000;

const USHORT ChunkSettingsImageSampling                 = 0x1100;
const USHORT ChunkSettingsImageSamplingPixelSamples     = 0x1110;
const USHORT ChunkSettingsImageSamplingPasses           = 0x1120;
const USHORT ChunkSettingsImageSamplingTileSize         = 0x1130;

const USHORT ChunkSettingsLighting                      = 0x1200;
const USHORT ChunkSettingsLightingGI                    = 0x1210;
const USHORT ChunkSettingsLightingCaustics              = 0x1215;
const USHORT ChunkSettingsLightingBounces               = 0x1220;
const USHORT ChunkSettingsLightingMaxRayIntensitySet    = 0x1240;
const USHORT ChunkSettingsLightingMaxRayIntensity       = 0x1250;
const USHORT ChunkSettingsLightingBackgroundEmitsLight  = 0x1230;
const USHORT ChunkSettingsLightingBackgroundAlpha       = 0x1260;

const USHORT ChunkSettingsOutput                        = 0x1300;
const USHORT ChunkSettingsOutputMode                    = 0x1310;
const USHORT ChunkSettingsOutputProjectFilePath         = 0x1320;
const USHORT ChunkSettingsOutputScaleMultiplier         = 0x1330;

const USHORT ChunkSettingsSystem                        = 0x1400;
const USHORT ChunkSettingsSystemRenderingThreads        = 0x1410;
const USHORT ChunkSettingsSystemLowPriorityMode         = 0x1420;
const USHORT ChunkSettingsSystemUseMaxProceduralMaps    = 0x1430;
