#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "utils.hpp"

namespace sqlite
{
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for(i = 0; i<argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

struct database
{
	database(const std::string& db_name)
	{
		sqlite3_open(db_name.c_str(), &db);

		if (rc)
		{
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		}
		else
		{
			fprintf(stderr, "Opened database successfully\n");
		}
	}

	void create_trajectory_table(const std::string& id)
	{
		const std::string sql_req{
			"CREATE TABLE " + id + "_trajectory ("
			"time INT PRIMARY KEY,"
			"x DECIMAL(8, 2),"
			"y DECIMAL(8, 2),"
			"heading DECIMAL(5, 2) NULL);"
		};

		const int rc{sqlite3_exec(db, sql_req.c_str(), sqlite::callback, 0, &zErrMsg)};

		if (rc != SQLITE_OK)
		{
			fprintf(stderr, "SQL create_trajectory_table error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	void create_object_data_table(const std::string& id)
	{
		const std::string sql_req{
			"CREATE TABLE " + id + "_data ("
			"speed DECIMAL(5, 2),"
			"created_time INT,"
			"expire_time INT,"
			"payload BLOB(100));"
		};
		const int rc{sqlite3_exec(db, sql_req.c_str(), sqlite::callback, 0, &zErrMsg)};

		if (rc != SQLITE_OK)
		{
			fprintf(stderr, "SQL create_object_data_table error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	void create_sections_data_tables()
	{
		std::vector<char> sections{'A', 'B', 'C', 'D'};

		for (const char section : sections)
		{
			const std::string sql_req{
				"CREATE TABLE " + std::string(1, section) + " ("
				"id_index TEXT PRIMARY KEY,"
				"start INT,"
				"end INT);"
			};

			const int rc{sqlite3_exec(db, sql_req.c_str(), sqlite::callback, 0, &zErrMsg)};

			if (rc != SQLITE_OK)
			{
				fprintf(stderr, "SQL create_sections_data_tables error: %s\n", zErrMsg);

				std::cout << "sql req: \n" << sql_req << "\n";
				sqlite3_free(zErrMsg);
			}
		}
	}

	void insert_to_trajectory(const std::string& id, const std::vector<utils::state>& states)
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << "INSERT INTO " << id << "_trajectory (time, x, y, heading) VALUES "; // TODO is precision necessary?

		for (auto it{states.begin()}; it != states.end(); it++)
		{
			stream << "(" << std::to_string(it->time) << ", " << std::to_string(it->position.x) << ", " << std::to_string(it->position.y) << ", " << std::to_string(it->heading) << "), ";
		}

		std::string sql_query(stream.str());
		sql_query.replace(sql_query.size() - 2, 2, ";");
		int rc{sqlite3_exec(db, sql_query.c_str(), sqlite::callback, 0, &zErrMsg)};

		if (rc != SQLITE_OK)
		{
			fprintf(stderr, "SQL insert_to_trajectory error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	void insert_sections_data(const std::string& id, const std::map<char, std::vector<long int>>& sections_data)
	{
		for (const auto& [key, val] : sections_data)
		{
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "INSERT INTO " << std::string(1, key) << " (id_index, start, end) VALUES ";

			for (auto it{val.begin()}; it != val.end(); it++)
			{
				stream << "('" << id << "_" << std::to_string(it-val.begin()) << "', " << std::to_string(*it) << ", " << std::to_string(*std::next(it)) << "), ";
				it++; // TODO
			}

			std::string sql_query(stream.str());
			sql_query.replace(sql_query.size() - 2, 2, ";");
			int rc{sqlite3_exec(db, sql_query.c_str(), sqlite::callback, 0, &zErrMsg)};

			if (rc != SQLITE_OK)
			{
				fprintf(stderr, "SQL insert_sections_data error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
		}
	}

	void insert_object_data(const std::string& id, const double speed, const long int start, const long int expire, const std::string& payload)
	{
		std::stringstream stream;
		stream << std::fixed << std::setprecision(2) << "INSERT INTO " << id << "_data (speed, created_time, expire_time, payload) VALUES ";
		stream << "(" << speed << ", " << start << ", " << expire << ", '" << payload << "');";

		int rc{sqlite3_exec(db, stream.str().c_str(), sqlite::callback, 0, &zErrMsg)};

		if (rc != SQLITE_OK)
		{
			fprintf(stderr, "SQL insert_object_data error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	}

	~database()
	{
		sqlite3_close(db);
	}

	sqlite3 *db;
private:
	char *zErrMsg = 0;
	int rc{};
	const char *sql;
};
}
