#pragma once
#include <stdlib.h>
#define NUM_LED_PROPS 10
enum ledProp
{
	pid,
	fps,
	anim_hz,
	brightness,
	hue,
	sat,
	val,
	delta_hue,
	delta_val,
	delta_sat
};