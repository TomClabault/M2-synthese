#ifndef APPLICATION_STATE_H
#define APPLICATION_STATE_H

struct ApplicationState
{
	bool currently_recomputing_irradiance = false;

	//Whether or not an irradiance map has freshly been recomputed and the OpenGl texture needs to be updated
	bool irradiance_map_freshly_recomputed = false;
};

#endif