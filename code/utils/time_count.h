#pragma once
#include <windows.h>
#include <stdio.h>

class time_count{
	LARGE_INTEGER frequ;
	LARGE_INTEGER start_time;
	LARGE_INTEGER end_time;
	LARGE_INTEGER interval;

public:
	time_count(){
		QueryPerformanceFrequency( &frequ );
		interval.QuadPart = 0;
	}
	void start(){
		QueryPerformanceCounter( &start_time );	
	}
	void end(char *inf = NULL){
		QueryPerformanceCounter( &end_time );
		double t_s = ( (double)end_time.QuadPart - start_time.QuadPart ) / frequ.QuadPart;
		printf("%stime: %.3f\n", inf, t_s);
	}
	void pause(){
		QueryPerformanceCounter( &end_time );
		interval.QuadPart += (end_time.QuadPart - start_time.QuadPart);
	}
	void show(char *inf = NULL) {
		double t_s = (double)interval.QuadPart / frequ.QuadPart;
		printf("%s time: %.3f\n", inf, t_s);
	}
	double get_end_time() {
		QueryPerformanceCounter( &end_time );
		return ((double)(end_time.QuadPart - start_time.QuadPart) * 1000) / frequ.QuadPart;
	}
};