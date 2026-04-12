/**
 * @file flight_session_manager.h
 * @brief Flight session state management and fuel consumption tracking
 *
 * Manages per-flight state for active connections and maintains
 * per-aircraft aggregate statistics across all flights.
 */

#pragma once

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <iostream>

 /**
  * @struct FlightSession
  * @brief Per-flight in-progress state (not shared between threads)
  *
  * Each worker thread maintains its own FlightSession instance for
  * the client it is currently processing. This structure tracks the
  * fuel consumption calculations for a single flight.
  */

struct FlightSession {
    uint32_t plane_id;
    double   prev_fuel;
    double   fuel_sum;
    double   current_avg;
    int      packet_count;
    bool     has_baseline;
};

/**
 * @struct AircraftStats
 * @brief Per-aircraft cumulative statistics (shared across threads)
 *
 * These statistics are maintained across all flights for each aircraft
 * and are protected by a mutex for thread-safe access.
 */
struct AircraftStats {
    double cumulative_avg;
    int    flight_count;
};

/**
 * @brief Initializes a new flight session for an aircraft
 *
 * Creates and returns a new FlightSession structure with initial values.
 *
 * @param plane_id Unique identifier for the aircraft
 * @return Initialized FlightSession structure
 */
FlightSession start_session(uint32_t plane_id);

/**
 * @brief Processes a telemetry reading and updates flight statistics
 *
 * Calculates the fuel consumption delta from the previous reading and
 * updates the running average for the current flight.
 *
 * @param session Reference to the current flight session
 * @param fuel_remaining Current fuel reading from telemetry packet
 */
void process_telemetry(FlightSession& session, double fuel_remianing);

/**
 * @brief Finalizes a flight session and updates aggregate statistics
 *
 * Calculates final average fuel consumption, updates the shared
 * per-aircraft statistics, and persists results to the flight log CSV.
 *
 * @param session Reference to the flight session to finalize
 */
void finalize_session(FlightSession& session);
