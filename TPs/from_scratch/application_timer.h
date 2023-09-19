#ifndef APPLICATION_TIMER_H
#define APPLICATION_TIMER_H

#define MAX_FRAMES 6

#include <chrono>

#include "text.h"

#include "glcore.h"

//TODO remove ?
class TP;
class TP2;

class ApplicationTimer
{
public:
    ApplicationTimer() : m_tp1(nullptr), m_tp2(nullptr) {}
    ApplicationTimer(TP* tp1);
    ApplicationTimer(TP2* tp2);

	void prerender();
	void postrender();

private:
    TP* m_tp1;
    TP2* m_tp2;

	std::chrono::high_resolution_clock::time_point m_cpu_start;
	std::chrono::high_resolution_clock::time_point m_cpu_stop;

	GLuint m_time_query[MAX_FRAMES];
	GLint64 m_frame_time;
	int m_frame;

	Text m_console;
}; 

#endif 
