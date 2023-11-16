#pragma once

#include <fstream>

#include "../utils.hpp"

namespace octave
{
struct octave
{
	octave(const char* filename) : file{filename}
	{
		file << "1;\n";
		file << "hold on;\n";
		file << "axis('equal');\n";
		file << "axis([0, 1000000, 0, 1000000]);\n"; // Set axis limits
		file << "plot([0, 1000000], [500000, 500000]);"; // Plot section lines
		file << "plot([500000, 500000], [0, 1000000]);";
	}

	void draw_plot(const std::vector<utils::state>& points, const double linewidth, const int time_step)
	{
		std::string plot_name{"plot_" + std::to_string(index)};
		file << plot_name << "=[";
		index++;

		long int prev_added{};
		for (const auto& p : points)
		{
			if (p.time - prev_added > time_step * 1000)
			{
				file << p.position.x << "," << p.position.y << ";";
				prev_added = p.time;
			}
		}
		file << "];\n";
		file << "plot(" << plot_name << "(:,1), " << plot_name << "(:,2), " << "'linewidth', " << linewidth << ")\n";
	}

	void close()
	{
		file.close();
	}

private:
	int index{};
	std::ofstream file;
};
}
