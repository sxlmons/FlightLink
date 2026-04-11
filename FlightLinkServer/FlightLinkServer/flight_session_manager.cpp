#include "flight_session_manager.h"
#include "sqlite3.h"
#include <fstream>

// flight_session_manager.cpp

static const char* CSV_FILE = "flight_log.csv";

// shared per-aircraft aggregate store
static std::unordered_map<uint32_t, AircraftStats> aircraft_map;
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
}

void finalize_session(FlightSession& session) {
	// need at least 2 readings before computing an average
	if (session.packet_count < 2) {
		std::cout << "[flight] Session ended | plane_id: " << session.plane_id
			<< " | not enough data for average"
			<< std::endl;
		
		return;
	}

	double avg_consumption = session.fuel_sum / (session.packet_count - 1);

	std::cout << "[flight] Completed | plane_id: " << session.plane_id
		<< " | avg consumption: " << avg_consumption
		<< " | total consumed: " << session.fuel_sum
		<< " | readings: " << session.packet_count
		<< std::endl;

	// Update shared per-aircraft stats under lock
	{
		std::lock_guard<std::mutex> lock(aircraft_mutex);
		AircraftStats& stats = aircraft_map[session.plane_id];
		stats.cumulative_avg = (stats.cumulative_avg * stats.flight_count + avg_consumption) / (stats.flight_count + 1);
		stats.flight_count++;

		std::cout << "[fleet]  Aircraft "
			<< session.plane_id
			<< " | cumulative avg: " << stats.cumulative_avg
			<< " | flights: " << stats.flight_count
			<< std::endl;

		// Persist to CSV 
		std::ifstream check(CSV_FILE);
		bool file_exists = check.good();
		check.close();
		std::ofstream csv(CSV_FILE, std::ios::app);
		if (csv.is_open()) {
			if (!file_exists) {
				csv << "plane_id,avg_consumption,total_consumed,"
					<< "readings,cumulative_avg,flight_count\n";
			}
			csv << session.plane_id
				<< "," << avg_consumption
				<< "," << session.fuel_sum
				<< "," << session.packet_count
				<< "," << stats.cumulative_avg
				<< "," << stats.flight_count
				<< "\n";
		}

	}
}