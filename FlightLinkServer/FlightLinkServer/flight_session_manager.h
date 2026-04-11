#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <iostream>

// flight_session_manager.h
// 
// Manages per flight state and per aircraft aggregate data
// 
// FlightSession is owned by each worker thread
// AircraftStats lives in a shared mutex guarded hash map

// Per flight in-progress state (not shared between threads)
struct FlightSession {
    int64_t  db_row_id;
    uint32_t plane_id;
    double   prev_fuel;
    double   fuel_sum;
    double   current_avg;
    int      packet_count;
    bool     has_baseline;
};

// Per-aircraft cumulative stats (shared)
struct AircraftStats {
    double cumulative_avg;
    int    flight_count;
};

bool init_database();
void close_database();
FlightSession start_session(uint32_t plane_id);
void process_telemetry(FlightSession& session, double fuel_remianing);
void finalize_session(FlightSession& session);
