#pragma once
#include <stdlib.h>
#define NUM_LED_PROPS 8
enum ledProp
{
	pid,
	fps,
	anim_hz,
	hue,
	sat,
	val,
	delta_hue,
	delta_val,
	delta_sat
};