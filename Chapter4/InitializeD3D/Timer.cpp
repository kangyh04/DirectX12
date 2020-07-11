#include <Windows.h>
#include "Timer.h"

Timer::Timer()
	: deltaTime(-1.0), baseTime(0), pausedTime(0),
	stopTime(0), previousTime(0), currentTime(0), stopped(false)
{
	__int64 countsPerSecond;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSecond);
	secondsPerCount = 1.0 / (double)countsPerSecond;
}

float Timer::TotalTime() const
{
// 	if (stopped)
// 	{
// 		return (float)(((stopTime - pausedTime) - baseTime) * secondsPerCount);
// 	}
// 	else
// 	{
// 		return (float)(((currentTime - pausedTime) - baseTime) * secondsPerCount);
// 	}

	auto targetTime = stopped ? stopTime : currentTime;
	return (float)(((targetTime - pausedTime) - baseTime) * secondsPerCount);
}

float Timer::DeltaTime() const
{
	return (float)deltaTime;
}

void Timer::Reset()
{
	__int64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

	baseTime = curTime;
	previousTime = curTime;
	stopTime = 0;
	stopped = false;
}

void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (stopped)
	{
		pausedTime += (startTime - stopTime);

		previousTime = startTime;
		stopTime = 0;
		stopped - false;
	}
}

void Timer::Stop()
{
	if (!stopped)
	{
		__int64 curTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&curTime);

		stopTime = curTime;
		stopped = true;
	}
}

void Timer::Tick()
{
	if (stopped)
	{
		deltaTime = 0.0f;
		return;
	}

	__int64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
	currentTime = curTime;

	deltaTime = (currentTime - previousTime) * secondsPerCount;

	previousTime = currentTime;

	if (deltaTime < 0.0)
	{
		deltaTime = 0.0;
	}
}