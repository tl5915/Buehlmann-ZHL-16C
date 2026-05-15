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
/*
gfLowPercent: GF Low (%), integer percent, must be > 0 and < gfHighPercent
gfHighPercent: GF High (%), integer percent, must be <= 100
po2Setpoint: CCR setpoint (PPO2 ata), must be > 0
Returns true if all input values are valid
If not set, default to GF 60/85, setpoint 1.2
*/
bool decoSetupCC(uint8_t gfLowPercent, uint8_t gfHighPercent, float po2Setpoint);

/*
gfLowPercent: GF Low (%), integer percent, must be > 0 and < gfHighPercent
gfHighPercent: GF High (%), integer percent, must be <= 100
fiO2: Open-circuit fixed oxygen fraction, must be >= 0 and <= 1
Returns true if all input values are valid
If not set, default to GF 60/85, FiO2 0.21
*/
bool decoSetupOC(uint8_t gfLowPercent, uint8_t gfHighPercent, float fiO2);

// ----- Initialise tissue compartments ----- //
void decoInit();


// ----- Update tissue compartments ----- //
/*
pressureAtm: ambient pressure in atm
dtMin: dive time in minutes
*/
void decoUpdate(float pressureAtm, float dtMin);


// ----- Compute decompression stops ----- //
/*
currentPressureAtm: ambient pressure in atm
*/
DecoResult decoCompute(float currentPressureAtm);


// ----- Change last-stop depth ----- //
/*
Default to false (disabled): last stop at 3m
If enabled: last stop at 6 m
*/
void setLastStop6m(bool enabled);

// ----- Change CC PO2 setpoint ----- //
/*
Change setpoint during the dive: returns true if valid setpoint (> 0) is applied
*/
bool setPo2Setpoint(float po2);

// ----- Change OC FiO2 ----- //
/*
Change gas FiO2 during the dive: returns true if valid FiO2 (>= 0 and <= 1) is applied
Switches gas model to open-circuit mode
*/
bool setFiO2(float fiO2);

// ----- Rip & Tear Mode ----- //
/*
Disable/enable gradient factor
Default to false (disabled): GF settings apply
Set to true (enabled): ignore GF settings and force 100% GF
(removes all conservatism, raw Bühlmann ZHL-16C algorithm)
Can be called in the middle of a dive if you find deco boring and want to ride the M-value train
*/
void ripNtear(bool enabled);

// ----- Query current settings ----- //
void getGradientFactors(float *gfLowPercent, float *gfHighPercent);  // Get current gradient factors
float getPo2Setpoint(void);  // Get current PO2 setpoint
float getFiO2(void);         // Get current gas FiO2
bool isOCmode(void);         // Get current mode OC/CC