#ifndef APPLICATION_TIMER_H
#define APPLICATION_TIMER_H

#define MAX_FRAMES 6

#include <chrono>

#include "text.h"

#include "glcore.h"

class TP;

class ApplicationTimer
{
public:
	ApplicationTimer() : m_main_app(nullptr) {}
	ApplicationTimer(TP* main_app);

	void prerender();
	void postrender();

private:
	TP* m_main_app;

	std::chrono::high_resolution_clock::time_point m_cpu_start;
	std::chrono::high_resolution_clock::time_point m_cpu_stop;

	GLuint m_time_query[MAX_FRAMES];
	GLint64 m_frame_time;
	int m_frame;

	Text m_console;
}; 

#endif 