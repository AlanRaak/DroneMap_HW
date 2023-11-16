#pragma once

#include <string>
#include <random>
#include <chrono>

namespace utils
{
struct vector2
{
	double x;
	double y;

	double length() const
	{
		return std::sqrt(x * x + y * y);
	}

	vector2 operator-(const vector2& other) const
	{
		return {x - other.x, y - other.y};
	}

	vector2& operator=(const vector2& other) {
		x = other.x;
		y = other.y;
		return *this;
	}
};

struct state
{
	long int time;
	vector2 position;
	double heading;
};

auto start_time(const int year, const int mon, const int day, const int hour)
{
	std::tm time_info{};
	time_info.tm_year = year - 1900;  // Year since 1900
	time_info.tm_mon = mon - 1;       // Month
	time_info.tm_mday = day;          // Day of the month
	time_info.tm_hour = hour + 2;     // Hour (24-hour clock),  +2 to UTC

	return std::chrono::system_clock::from_time_t(std::mktime(&time_info));
}

double random_double(const double min, const double max)
{
	// Create a random number generator
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define the distribution for the specified range
	std::uniform_real_distribution<double> distribution(min, max);

	// Generate and return a random double
	return distribution(gen);
}

long int random_int(const long int min, const long int max)
{
	// Create a random number generator
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define the distribution for the specified range
	std::uniform_int_distribution<long int> distribution(min, max);

	// Generate and return a random int
	return distribution(gen);
}

bool is_within_range(const double value, const double min, const double max)
{
	return (value >= min) && (value <= max);
}

double heading(const vector2& v)
{
	double angle{std::atan2(v.y, v.x)};

	if (angle < 0) {
		angle += 2 * M_PI;
	}

	return angle;
}
}
