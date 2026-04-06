#pragma once
#include <stdint.h>

struct DecoResult {
    bool inDeco;             // true if decompression required
    uint16_t nextStopDepth;  // depth (m) of the next step
    uint16_t stopTime;       // time (min) at the next stop
    uint16_t timeToSurface;  // total time to surface (min) including stops and ascent
    uint16_t surfGF;         // surface GF (%) - gradient factor expected if instantaneously surfaced
};

// ----- Configure model parameters ----- //
// gfLowPercent: GF Low (%), integer percent, must be > 0 and < gfHighPercent
// gfHighPercent: GF High (%), integer percent, must be <= 100
// po2Setpoint: CCR setpoint (PPO2 ata), must be > 0
bool decoSetup(uint8_t gfLowPercent, uint8_t gfHighPercent, float po2Setpoint);
// Returns true if all input values are valid
// If not set, default to GF 60/85, setpoint 1.2

// ----- Initialise tissue compartments ----- //
void decoInit();

// ----- Update tissue compartments ----- //
// pressureAtm: ambient pressure in atm
// dtMin: dive time in minutes
void decoUpdate(float pressureAtm, float dtMin);

// ----- Compute decompression stops ----- //
// currentPressureAtm: ambient pressure in atm.
DecoResult decoCompute(float currentPressureAtm);

// ----- Mad Man Mode ----- //
// Disable/enable gradient factor
// Default to false (disabled): GF settings apply
// Set to true (enabled): ignore GF settings and force 100% GF (removes all conservatism, raw Bühlmann ZHL-16C algorithm)
// Can be called in the middle of a dive if you find deco boring and want to ride the M-value train
void mad_man_mode(bool enabled);