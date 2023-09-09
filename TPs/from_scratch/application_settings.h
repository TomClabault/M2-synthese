#ifndef APPLICATION_SETTINGS_H
#define APPLICATION_SETTINGS_H

#include "color.h"

struct ApplicationSettings
{
	bool use_ambient = true;

	//1 for cubemap, 0 for skysphere
	int cubemap_or_skysphere = 1;

	Color ambient_color = Color(0.1, 0.1, 0.1, 0);
};

#endif