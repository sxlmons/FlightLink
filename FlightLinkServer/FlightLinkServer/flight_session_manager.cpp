/**
 * @file flight_session_manager.cpp
 * @brief Implementation of flight session management and statistics tracking
 */

#include "flight_session_manager.h"
#include "sqlite3.h"

// flight_session_manager.cpp

/** @brief CSV log file path for persistent storage */
static const char* CSV_FILE = "flight_log.csv";

/** @brief Shared per-aircraft aggregate statistics store */
static std::unordered_map<uint32_t, AircraftStats> aircraft_map;

/** @brief Mutex protecting access to the shared aircraft_map */
static std::mutex aircraft_mutex;
static std::mutex db_mutex;
static sqlite3* g_db = nullptr;

bool init_database() {
	int rc = sqlite3_open("flight_logs.db", &g_db);
	if (rc != SQLITE_OK) {
		std::cerr << "Failed to open db: " << sqlite3_errmsg(g_db) << std::endl;
		return false;
	}

	sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

	sqlite3_exec(g_db,
		"CREATE TABLE IF NOT EXISTS flight_sessions ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"plane_id INTEGER NOT NULL,"
		"fuel_sum REAL,"
		"current_avg REAL,"
		"packet_count INTEGER,"
		"status TEXT DEFAULT 'active'"
		");",
		nullptr, nullptr, nullptr);

	sqlite3_exec(g_db,
		"CREATE TABLE IF NOT EXISTS aircraft_stats ("
		"plane_id INTEGER PRIMARY KEY,"
		"cumulative_avg REAL,"
		"flight_count INTEGER NOT NULL"
		");",
		nullptr, nullptr, nullptr);

	return true;
}

void close_database() {
	if (g_db) {
		sqlite3_close(g_db);
		g_db = nullptr;
	}
}

FlightSession start_session(uint32_t plane_id) {
	FlightSession session{};
	session.plane_id	 = plane_id;
	session.prev_fuel	 = 0.0;
	session.fuel_sum	 = 0.0;
	session.packet_count = 0;
	session.has_baseline = false;

	{
		std::lock_guard<std::mutex> lock(db_mutex);

		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(g_db,
			"INSERT INTO flight_sessions (plane_id, fuel_sum, current_avg, packet_count, status) "
			"VALUES (?, 0.0, 0.0, 0, 'active');",
			-1, &stmt, nullptr);
		sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(session.plane_id));
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);

		session.db_row_id = sqlite3_last_insert_rowid(g_db);
	}

	std::cout << "[flight] Session started | plane_id: " << plane_id << std::endl;
	return session;
}

void process_telemetry(FlightSession& session, double fuel_remaining) {
	if (!session.has_baseline) {
		// First packet establishes the baseline lol - no delta
		session.prev_fuel = fuel_remaining;
		session.has_baseline = true;
		session.packet_count = 1;
		return;
	}

	double delta = session.prev_fuel - fuel_remaining;
	session.fuel_sum += delta;
	session.packet_count++;
	session.prev_fuel = fuel_remaining;
	session.current_avg = session.fuel_sum / (session.packet_count - 1);  
	
	if (session.packet_count % 100 == 0) 
	{
		std::lock_guard<std::mutex> lock(db_mutex);

		sqlite3_stmt* stmt = nullptr;

		sqlite3_prepare_v2(g_db,
			"UPDATE flight_sessions SET "
			"fuel_sum = ?, current_avg = ?, packet_count = ? WHERE id = ?;",
			-1, &stmt, nullptr);

		sqlite3_bind_double(stmt, 1, session.fuel_sum);
		sqlite3_bind_double(stmt, 2, session.current_avg);
		sqlite3_bind_int(stmt, 3, session.packet_count);
		sqlite3_bind_int64(stmt, 4, session.db_row_id);

		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}


// Shared finalization logic. The two distinct session endings
// (graceful FLIGHT_END and mid-flight interrupt) differ only in
// the status string persisted to the DB and the label logged.
void end_session(FlightSession& session, const char* status, const char* log_label)
{
	if (session.finalized) 
		return;

	if (session.packet_count < 2) {
		std::cout << "[flight] Session ended | plane_id: " << session.plane_id
			<< " | not enough data for average" << std::endl;
		return;
	}

	session.finalized = true;

	const double avg_consumption = session.fuel_sum / (session.packet_count - 1);
	double final_cumulative_avg;
	int    final_flight_count;

	std::cout << "[flight] " << log_label << " | plane_id: " << session.plane_id
		<< " | avg consumption: " << avg_consumption
		<< " | total consumed: " << session.fuel_sum
		<< " | readings: " << session.packet_count
		<< std::endl;

	{
		std::lock_guard<std::mutex> lock(aircraft_mutex);
		AircraftStats& stats = aircraft_map[session.plane_id];
		stats.cumulative_avg = (stats.cumulative_avg * stats.flight_count + avg_consumption) / (stats.flight_count + 1);
		stats.flight_count++;
		final_cumulative_avg = stats.cumulative_avg;
		final_flight_count = stats.flight_count;

		std::cout << "[fleet]  Aircraft " << session.plane_id
			<< " | cumulative avg: " << stats.cumulative_avg
			<< " | flights: " << stats.flight_count
			<< std::endl;
	}

	{
		std::lock_guard<std::mutex> lock(db_mutex);
		std::string update_sql =
			"UPDATE flight_sessions SET "
			"fuel_sum = ?, current_avg = ?, packet_count = ?, status = '";
		update_sql += status;
		update_sql += "' WHERE id = ?;";

		sqlite3_stmt* stmt = nullptr;
		sqlite3_prepare_v2(g_db, update_sql.c_str(), -1, &stmt, nullptr);
		sqlite3_bind_double(stmt, 1, session.fuel_sum);
		sqlite3_bind_double(stmt, 2, avg_consumption);
		sqlite3_bind_int(stmt, 3, session.packet_count);
		sqlite3_bind_int64(stmt, 4, session.db_row_id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);

		sqlite3_stmt* stats_stmt = nullptr;
		sqlite3_prepare_v2(g_db,
			"INSERT OR REPLACE INTO aircraft_stats "
			"(plane_id, cumulative_avg, flight_count) VALUES (?, ?, ?);",
			-1, &stats_stmt, nullptr);
		sqlite3_bind_int64(stats_stmt, 1, static_cast<int64_t>(session.plane_id));
		sqlite3_bind_double(stats_stmt, 2, final_cumulative_avg);
		sqlite3_bind_int(stats_stmt, 3, final_flight_count);
		sqlite3_step(stats_stmt);
		sqlite3_finalize(stats_stmt);
	}
}

void finalize_session(FlightSession& session) {
	end_session(session, "completed", "Completed");
}

void interrupt_session(FlightSession& session) {
	end_session(session, "interrupted", "Interrupted");
}