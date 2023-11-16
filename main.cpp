#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <cassert>
#include <fstream>

#include "sqlite.hpp"
#include "utils.hpp"
#include "visualize/octave.hpp"

namespace generator
{
std::string random_string()
{
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define the character set for the string
	const std::string charset = "0123456789abcdefghijklmnopqrstuvwxyz";
	const int length = 32;

	// Create a distribution for the indices in the character set
	std::uniform_int_distribution<> distribution(0, charset.size() - 1);
	std::uniform_int_distribution<> distribution_chars(0, charset.size() - 11);

	std::stringstream ss;
	ss << charset.substr(10)[distribution_chars(gen)]; // Start from index 10 to exclude digits

	for (int i = 1; i < length; ++i) {
		ss << charset[distribution(gen)];
	}

	assert(!std::isdigit(ss.str()[0]));
	return ss.str();
}

std::string generate_random_hex_data(const int size)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distribution(0, 255);

	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (int i = 0; i < size; ++i) {
		ss << std::setw(2) << distribution(gen);
	}

	return ss.str();
}

utils::vector2 random_point_in_map()
{
	constexpr double world_size{1000000.0};
	return {.x = utils::random_double(0.0, world_size), .y = utils::random_double(0.0, world_size)};
}

utils::vector2 random_point_in_area(const utils::vector2& initial, const double min, const double max)
{
	utils::vector2 point;

	do {point = random_point_in_map();}
	while (!utils::is_within_range((point - initial).length(), min, max)); // Brute force a valid destination

	return point;
}

char which_section(utils::vector2 point)
{
	if (point.y < 500000)
	{
		return point.x < 500000 ? 'A' : 'B';
	}
	else
	{
		return point.x < 500000 ? 'C' : 'D';
	}
}

utils::vector2 quadratic_bezier(const utils::vector2& point_0, const utils::vector2& point_1, const utils::vector2& P2, const double t)
{
	return {
		.x = std::pow(1 - t, 2) * point_0.x + 2 * (1 - t) * t * point_1.x + std::pow(t, 2) * P2.x,
		.y = std::pow(1 - t, 2) * point_0.y + 2 * (1 - t) * t * point_1.y + std::pow(t, 2) * P2.y,
	};
}

std::pair<std::vector<utils::state>, std::map<char, std::vector<long int>>> bezier_trajectory_generator(const utils::vector2& initial, const utils::vector2& waypoint, const utils::vector2& destination, const double speed, const long int start_time, const long int sim_end_time)
{
	/*
	 * Generates a trajectory for the object.
	 * Also creates a std::map of timestamps when object crossed a section boundary and saves relevant section IDs,
	 * which can be used to simplify API calls on sections.
	 */
	std::vector<utils::state> trajectory;
	std::map<char, std::vector<long int>> entrance_exit_times;

	constexpr double sampling_time{0.15}; // seconds
	const double sampling_distance{speed * sampling_time};
	const double max_bezier_length{(initial - waypoint).length() + (destination - waypoint).length()};
	const double stepsize{sampling_distance / max_bezier_length / 20}; // Generates about 20 extra points to approximate the curve more accurately
	utils::vector2 current_point{initial};

	entrance_exit_times[which_section(current_point)].push_back(start_time); // Push entrance time
	trajectory.push_back({
		.time = start_time,
		.position = current_point,
		.heading = -1.0,
	});

	long int next_time{start_time};
	double distance{};

	for (double fraction{}; fraction <= 1.0; fraction += stepsize)
	{
		const utils::vector2 next_point{quadratic_bezier(initial, waypoint, destination, fraction)};
		distance += (next_point - current_point).length();

		if (distance >= sampling_distance)
		{
			if (next_time + 150 > sim_end_time)
			{
				break;
			}
			next_time += 150;

			const char current_section{which_section(trajectory.back().position)};
			const char next_section{which_section(next_point)};

			trajectory.push_back({
				.time = next_time,
				.position = next_point,
				.heading = utils::heading(next_point - trajectory.back().position),
			});

			if (current_section != next_section) // Object crosses some section line
			{
				entrance_exit_times[current_section].push_back(next_time - 150);
				entrance_exit_times[next_section].push_back(next_time);
			}

			distance = distance - sampling_distance; // Basicallly zeroes the distance, but preserves the error of previous approximation
		}

		current_point = next_point;
	}

	entrance_exit_times[which_section(trajectory.back().position)].push_back(next_time); // Push last exit time

	assert(next_time <= sim_end_time);
	assert(next_time - start_time < 10 * 3600 * 1000);

	for (const auto [key, val] : entrance_exit_times)
	{
		assert(!(val.size() % 2)); // Assume entrance and exit times are in pairs
	}

	for (auto [key, val] : entrance_exit_times)
	{
		for (auto it{val.begin()}; it != val.end(); it++)
		{
			assert(*it <= *std::next(it)); // Assume entrance time smaller than exit time, can be equal if only 1 point of trajectory is inside section
			it++;
		}
	}

	return {trajectory, entrance_exit_times};
}
}

int main()
{
	octave::octave octave{"visualize/octave_plot.m"}; // Octave visualizer
	sqlite::database db("drone_map.db");

	db.create_sections_data_tables();
	constexpr int number_of_trajectories{50};

	for (int i{1}; i <= 50; i++)
	{
		std::cout << i << "/" << number_of_trajectories << "\n";

		const utils::vector2 initial{generator::random_point_in_map()};
		const utils::vector2 waypoint{generator::random_point_in_area(initial, 100'000, 150'000)};
		const utils::vector2 destination{generator::random_point_in_area(initial, 150'000, 400'000)};

		assert((initial - destination).length() > 150'000);

		constexpr long int sim_time{10 * 3600 * 1000}; // 10h
		const long int start_time{std::chrono::duration_cast<std::chrono::milliseconds>(utils::start_time(2006, 12, 1, 13).time_since_epoch()).count()};
		const long int sim_end_time{start_time + sim_time};
		const long int sim_start_time{utils::random_int(start_time, sim_end_time)};

		const double speed{utils::random_double(10.0, 80.0)};
		const auto object_data{generator::bezier_trajectory_generator(initial, waypoint, destination, speed, sim_start_time, sim_end_time)};
		const std::string object_id{generator::random_string()};

		db.create_trajectory_table(object_id);
		db.create_object_data_table(object_id);

		const std::string payload{generator::generate_random_hex_data(100)};
		const long int expire_time{object_data.first.back().time};

		db.insert_object_data(object_id, speed, sim_start_time, expire_time, payload);
		db.insert_to_trajectory(object_id, object_data.first);
		db.insert_sections_data(object_id, object_data.second);

		octave.draw_plot(object_data.first, 2.0, 100); // Draws all generated lanes into an octave file
	}

	octave.close();

	return 0;
}
