// Buhlmann ZHL-16C model
// CC mode single set point
// No helium penalty
// Assume ascent at 9 m/min
// Assume 3 m stops and last stop at 3 m
// Assume no prior dives
// Assume sea level

#include "ZHL16C.h"
#include <math.h>
#include <string.h>

// ZHL-16C Algorithm Constants
static const float kHT[16] = {  // N2 half-times (minutes)
    5.0f, 8.0f, 12.5f, 18.5f, 27.0f, 38.3f, 54.3f, 77.0f,
    109.0f, 146.0f, 187.0f, 239.0f, 305.0f, 390.0f, 498.0f, 635.0f
};
static const float kA[16] = {   // N2 a-values
    1.1696f, 1.0000f, 0.8618f, 0.7562f, 0.6491f, 0.5316f, 0.4681f, 0.4301f,
    0.4049f, 0.3719f, 0.3447f, 0.3176f, 0.2828f, 0.2716f, 0.2523f, 0.2327f
};
static const float kB[16] = {   // N2 b-values
    0.5578f, 0.6514f, 0.7222f, 0.7825f, 0.8126f, 0.8434f, 0.8693f, 0.8910f,
    0.9092f, 0.9222f, 0.9319f, 0.9403f, 0.9477f, 0.9544f, 0.9602f, 0.9653f
};

// Model Constants
static constexpr float ASCENT_RATE = 9.0f;  // Ascent rate m/min

// Physics Constants
static constexpr float LN2 = 0.693147f;              // Natural log of 2
static constexpr float GRAVITY = 9.81f;              // Gravity m/s²
static constexpr float ATM_PRESSURE_PA = 101325.0f;  // Pa per atm
static constexpr float N2_FRAC_AIR = 0.7902f;        // FiN2 in air
static constexpr float WATER_VAPOR = 0.0627f;        // Alveolar water vapour pressure
static constexpr float SURFACE_ATM = 1.0f;           // 1 atm at surface
static constexpr float SEAWATER_DENSITY = 1020.0f;   // EN13319 density kg/m³
static constexpr float METERS_PER_ATM = ATM_PRESSURE_PA / (SEAWATER_DENSITY * GRAVITY);

// Current PPN2 in compartments
static float N2[16];

// Default model parameters
static float gfLow = 0.60f;       // GF LOW 60
static float gfHigh = 0.85f;      // GF HIGH 85
static float po2Setpoint = 1.2f;  // Setpoint 1.2

// Gradient factor enabled/disabled
static bool gfEnabled = true;

// Convert atm to depth in meters
static inline float depthFromPressureAtm(float pressureAtm) {
    const float depthM = (pressureAtm - SURFACE_ATM) * METERS_PER_ATM;
    return (depthM < 0.0f) ? 0.0f : depthM;
}

// Convert depth in meters to atm
static inline float pressureAtmFromDepth(float depthM) {
    return SURFACE_ATM + depthM / METERS_PER_ATM;
}

// Alveolar PPN2
static inline float ppN2AlvFromAmb(float ambientAtm) {
    const float p = ambientAtm - po2Setpoint - WATER_VAPOR;
    return (p < 0.0f) ? 0.0f : p;
}

// Tissue update from ambient pressure
static void tickTissuesAmbient(float n2[], float ambientAtm, float dtMin) {
    const float pAlv = ppN2AlvFromAmb(ambientAtm);
    for (int i = 0; i < 16; i++) {
        const float k = LN2 / kHT[i];
        n2[i] += (pAlv - n2[i]) * (1.0f - expf(-k * dtMin));
    }
}

// Tissue update from depth for ascent simulation
static void tickTissuesDepth(float n2[], float depthM, float dtMin) {
    tickTissuesAmbient(n2, pressureAtmFromDepth(depthM), dtMin);
}

// Ceiling
static float ceilDepth(const float n2[], float gf) {
    float maxAtm = 0.0f;
    for (int i = 0; i < 16; i++) {
        const float p = n2[i];
        if (p <= 0.0f) continue;
        const float denom = gf / kB[i] + 1.0f - gf;
        if (denom <= 0.0f) continue;
        const float c = (p - kA[i] * gf) / denom;
        if (c > maxAtm) maxAtm = c;
    }
    const float d = (maxAtm - SURFACE_ATM) * METERS_PER_ATM;
    return (d < 0.0f) ? 0.0f : d;
}

// GF interpolation
static float gfAt(float depth, float dFirst) {
    if (dFirst <= 0.0f) return gfHigh;
    const float gf = gfHigh + (gfLow - gfHigh) * depth / dFirst;
    if (gf < gfLow) return gfLow;
    if (gf > gfHigh) return gfHigh;
    return gf;
}

// Surface GF
static float surfaceGfPercent(const float n2[]) {
    float maxGf = 0.0f;
    for (int i = 0; i < 16; i++) {
        const float p = n2[i];
        if (p <= 0.0f) continue;
        const float denom = kA[i] + SURFACE_ATM / kB[i] - SURFACE_ATM;
        if (denom <= 0.0f) continue;
        const float gf = (p - SURFACE_ATM) / denom;
        if (gf > maxGf) maxGf = gf;
    }
    if (maxGf < 0.0f) maxGf = 0.0f;
    return maxGf * 100.0f;
}

// Simulate ascent for TTS calculation
static void simAscent(float n2[], float from, float to, uint16_t *totalMin) {
    if (from <= to) return;
    const float avg = (from + to) * 0.5f;
    const float travelMin = (from - to) / ASCENT_RATE;
    tickTissuesDepth(n2, avg, travelMin);
    *totalMin += static_cast<uint16_t>(ceilf(travelMin));
}

// Round to the next deeper 3 m stop
static int roundUpToNext3mStop(float depthM) {
    if (depthM <= 0.0f) return 0;
    constexpr float STOP_STEP_M = 3.0f;
    constexpr float EPS = 1e-6f;
    const float stopIndex = ceilf((depthM / STOP_STEP_M) - EPS);
    int stopDepth = static_cast<int>(stopIndex * STOP_STEP_M);
    if (static_cast<float>(stopDepth) < depthM) {
        stopDepth += static_cast<int>(STOP_STEP_M);
    }
    return stopDepth;
}


// ----- Public API ----- //

// Setup model parameters
// Input: GF Low (%), GF High (%), CCR setpoint (PPO2 ata)
// Output: return false if invalid config: 0 < GF Low < GF High <= 100, setpoint > 0
bool decoSetup(uint8_t gfLowPercent, uint8_t gfHighPercent, float po2InputSetpoint) {
    if (gfLowPercent == 0) return false;
    if (gfLowPercent >= gfHighPercent) return false;
    if (gfHighPercent > 100) return false;
    if (po2InputSetpoint <= 0.0f) return false;
    gfLow = static_cast<float>(gfLowPercent) / 100.0f;
    gfHigh = static_cast<float>(gfHighPercent) / 100.0f;
    po2Setpoint = po2InputSetpoint;
    return true;
}

// Disable/enable gradient factor
// Input: enabled = true to disable GF settings and force 100% GF
void ripNtear(bool enabled) {
    gfEnabled = !enabled;
}

// Initialise tissue compartments
void decoInit() {
    const float ppN2Surf = (SURFACE_ATM - WATER_VAPOR) * N2_FRAC_AIR;
    for (int i = 0; i < 16; i++) {
        N2[i] = ppN2Surf;
    }
}

// Update tissue compartments
// Input: current ambient pressure (atm), current dive time (min)
void decoUpdate(float pressureAtm, float dtMin) {
    if (dtMin > 0.0f) {
        tickTissuesAmbient(N2, pressureAtm, dtMin);
    }
}

// Compute decompression stops
// Input: current ambient pressure (atm)
// Output: deco required, surface GF (%), next stop depth (m), next stop time (min), time to surface (min)
DecoResult decoCompute(float currentPressureAtm) {
    const float currentDepthM = depthFromPressureAtm(currentPressureAtm);
    float n2[16];
    memcpy(n2, N2, sizeof(n2));

    // Calculate surface GF
    const uint16_t surfGF = static_cast<uint16_t>(lroundf(surfaceGfPercent(n2)));

    // Find the deepest stop
    const float firstCeilGf = gfEnabled ? gfLow : 1.0f;
    const float rawCeil = ceilDepth(n2, firstCeilGf);
    uint16_t dFirst = 0;
    if (rawCeil > 0.1f) {
        dFirst = static_cast<uint16_t>(roundUpToNext3mStop(rawCeil));
    }

    // NDL
    if (dFirst <= 0) {
        return {false, 0, 0, static_cast<uint16_t>(ceilf(currentDepthM / ASCENT_RATE)), surfGF};
    }

    // Simulate ascent from current depth to first stop
    uint16_t totalMin = 0;
    if (currentDepthM > static_cast<float>(dFirst) + 0.1f) {
        simAscent(n2, currentDepthM, static_cast<float>(dFirst), &totalMin);
    }

    // 3 meter stops
    bool firstRecorded = false;
    uint16_t firstStopMin = 0;
    float dStop = static_cast<float>(dFirst);

    while (dStop > 0.0f) {
        const float dNext = (dStop <= 3.0f) ? 0.0f : dStop - 3.0f;
        const float gfNext = gfEnabled ? gfAt(dNext, static_cast<float>(dFirst)) : 1.0f;

        // Wait at stop until ceiling clear to next stop
        uint16_t stopMin = 0;
        while (ceilDepth(n2, gfNext) > dNext) {
            tickTissuesDepth(n2, dStop, 1.0f);
            stopMin++;
            totalMin++;
        }
        if (!firstRecorded) {
            firstStopMin = stopMin;
            firstRecorded = true;
        }

        // Final stop to surface
        if (dNext <= 0.0f) {
            totalMin += static_cast<uint16_t>(ceilf(dStop / ASCENT_RATE));
            break;
        }

        simAscent(n2, dStop, dNext, &totalMin);
        dStop = dNext;
    }
    if (firstStopMin == 0) firstStopMin = 1;  // Minimum stop 1min
    return {true, dFirst, firstStopMin, totalMin, surfGF};
}