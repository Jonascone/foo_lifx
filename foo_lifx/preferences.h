#pragma once

#include "stdafx.h"

/*{225f833e - 9d1e - 44ba - 8fa1 - 0411f0e58303}
{99eae587 - 3410 - 4bce - b436 - c30c66980551}
{6d64f9b5 - c9ab - 439e-9255 - b221685dd306}
{faa86ed9 - b276 - 44d5 - ad70 - bf338fd4bdc8}
{dede418a - eeaf - 4083 - 965a - d51819615070}
{3089f941 - 2974 - 4328 - a047 - a34bab74c56f}*/

static const GUID guid_cfg_lifx_enabled = { 0x57873171, 0x2285, 0x4fa3,{ 0xa5, 0xc3, 0x30, 0xe7, 0xc4, 0xd5, 0x73, 0xab } };
static const GUID guid_cfg_lifx_cycle_speed = { 0x8f338bca, 0x76f4, 0x49ff,{ 0xaa, 0x44, 0xe6, 0xa2, 0xae, 0x98, 0x88, 0x99 } };
static const GUID guid_cfg_lifx_brightness = { 0xf3343d66, 0x12db, 0x4bf8, {0x97, 0x92, 0x5d, 0x4d, 0x15, 0x40, 0x2c, 0x93} };

extern cfg_bool cfg_lifx_enabled;
extern cfg_uint cfg_lifx_cycle_speed;
extern cfg_uint cfg_lifx_brightness;