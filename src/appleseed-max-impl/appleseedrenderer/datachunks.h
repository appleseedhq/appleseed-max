
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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

const USHORT ChunkFileFormatVersion                                 = 0x0001;

const USHORT ChunkSettings                                          = 0x1000;

const USHORT ChunkSettingsImageSampling                             = 0x1100;
const USHORT ChunkSettingsImageSamplingPixelSamples                 = 0x1110;
const USHORT ChunkSettingsImageSamplingPasses                       = 0x1120;
const USHORT ChunkSettingsImageSamplingTileSize                     = 0x1130;
const USHORT ChunkSettingsPixelFilter                               = 0x1140;
const USHORT ChunkSettingsPixelFilterSize                           = 0x1150;
const USHORT ChunkSettingsImageSamplerType                          = 0x1160;
const USHORT ChunkSettingsAdaptiveBatchSize                         = 0x1161;
const USHORT ChunkSettingsAdaptiveMinSamples                        = 0x1162;
const USHORT ChunkSettingsAdaptiveMaxSamples                        = 0x1163;
const USHORT ChunkSettingsAdaptiveNoiseThreshold                    = 0x1164;

const USHORT ChunkSettingsLighting                                  = 0x1200;
const USHORT ChunkSettingsLightingGI                                = 0x1210;
const USHORT ChunkSettingsLightingRRMinPathLength                   = 0x1211;
const USHORT ChunkSettingsLightingCaustics                          = 0x1215;
const USHORT ChunkSettingsLightingGlobalBounces                     = 0x1220;
const USHORT ChunkSettingsLightingDiffuseBounces                    = 0x1221;
const USHORT ChunkSettingsLightingDiffuseBouncesEnabled             = 0x1222;
const USHORT ChunkSettingsLightingGlossyBounces                     = 0x1223;
const USHORT ChunkSettingsLightingGlossyBouncesEnabled              = 0x1224;
const USHORT ChunkSettingsLightingSpecularBounces                   = 0x1225;
const USHORT ChunkSettingsLightingSpecularBouncesEnabled            = 0x1226;
const USHORT ChunkSettingsLightingVolumeBounces                     = 0x1227;
const USHORT ChunkSettingsLightingVolumeBouncesEnabled              = 0x1228;
const USHORT ChunkSettingsLightingVolumeDistanceSamples             = 0x1229;
const USHORT ChunkSettingsLightingMaxRayIntensitySet                = 0x1240;
const USHORT ChunkSettingsLightingMaxRayIntensity                   = 0x1250;
const USHORT ChunkSettingsLightingClampRougness                     = 0x1251;
const USHORT ChunkSettingsLightingBackgroundEmitsLight              = 0x1230;
const USHORT ChunkSettingsLightingDirectLightingEnabled             = 0x1231;
const USHORT ChunkSettingsLightingDirectLightingSamples             = 0x1232;
const USHORT ChunkSettingsLightingDirectLightingLowLightThreshold   = 0x1233;
const USHORT ChunkSettingsLightingImageBasedLightingSamples         = 0x1234;
const USHORT ChunkSettingsLightingBackgroundAlpha                   = 0x1260;
const USHORT ChunkSettingsLightingForceOffDefaultLights             = 0x1270;
const USHORT ChunkSettingsLightingOptimizeLightsOutsideVolumes      = 0x1271;

const USHORT ChunkSettingsOutput                                    = 0x1300;
const USHORT ChunkSettingsOutputMode                                = 0x1310;
const USHORT ChunkSettingsOutputProjectFilePath                     = 0x1320;
const USHORT ChunkSettingsOutputScaleMultiplier                     = 0x1330;

const USHORT ChunkSettingsSystem                                    = 0x1400;
const USHORT ChunkSettingsSystemRenderingThreads                    = 0x1410;
const USHORT ChunkSettingsSystemLowPriorityMode                     = 0x1420;
const USHORT ChunkSettingsSystemUseMaxProceduralMaps                = 0x1430;
const USHORT ChunkSettingsSystemEnableRenderStamp                   = 0x1440;
const USHORT ChunkSettingsSystemRenderStampString                   = 0x1450;
const USHORT ChunkSettingsSystemEnableEmbree                        = 0x1460;
