# Bühlmann ZHL-16C Arduino Library

This library provides a compact Bühlmann ZHL-16C decompression model API for DIY dive computers.

## Model Assumptions

- Supports both closed circuit (fixed PPO2 setpoint) and open circuit (fixed FiO2)
- Air diluent, not considering helium penalty
- Ascent rate 9 m/min
- Stops in 3 m intervals, last stop at 3 m
- No prior dives, single dive day
- Diving from sea level
- Stop time and TTS are rounded up to full minute

## Data Input Format

- Everything is metric
- Input **ambient pressure in atm** (float) from pressure sensor reading
- Input dive time in **minutes** (float)

## Functions

### `bool decoSetupCC(uint8_t gfLowPercent, uint8_t gfHighPercent, float po2Setpoint)`

Configures gradient factors and the CCR oxygen setpoint.- `gfLowPercent`: integer percent, must be `> 0` and `< gfHighPercent`.

- `gfHighPercent`: integer percent, must be `<= 100`
- `po2Setpoint`: float, must be `> 0`
- Returns `true` when the configuration is accepted
- If no valid input, model defaults to **GF 60/85** and **setpoint 1.2**

### `bool decoSetupOC(uint8_t gfLowPercent, uint8_t gfHighPercent, float fiO2)`

Configures gradient factors and open-circuit gas.

- `gfLowPercent`: integer percent, must be `> 0` and `< gfHighPercent`
- `gfHighPercent`: integer percent, must be `<= 100`
- `fiO2`: oxygen fraction as float, must be `>= 0` and `<= 1`
- Returns `true` when the configuration is accepted
- If no valid input, model defaults to **GF 60/85** and **FiO2 0.2098 (air)**

### `void decoInit()`

Initialises all tissue compartments to surface equilibrium.
Call after `decoSetup()` at startup or at the begining of the dive.

### `void decoUpdate(float pressureAtm, float dtMin)`

Updates tissue compartments for dive time at ambient pressure.

- `pressureAtm`: ambient pressure in **atm**
- `dtMin`: dive time in **minutes**

Call this regularly in control loop, e.g. every second.

### `DecoResult decoCompute(float currentPressureAtm)`

Computes current decompression output.

- `currentPressureAtm`: ambient pressure in **atm**
- Returns `DecoResult`:
  - `bool inDeco`: `true`= mandatory decompression, `false`= in NDL
  - `u_int16_t nextStopDepth`: depth of next stop in meters, round to deeper 3 m interval
  - `u_int16_t stopTime`: time at next stop in minutes, round up to full minute
  - `u_int16_t timeToSurface`: total time to surface in minutes, including ascent time and stops
  - `u_int16_t surfGF`: gradient factor in percent expected if instantaneously surfaced

Call this when decompression needs to be calculated, e.g. every 10 seconds.

### `ripNtear(bool enabled)`

Disable or enable gradient factor.

- `false`: normal decompression behavior, with conservatism by pre-set gradient factors
- `true`: ignore GF settings and force 100% GF (removes all conservatism, raw Bühlmann ZHL-16C algorithm)

Default to false. Can be called in the middle of a dive if you find deco boring and want to ride the M-value train

### `bool setPo2Setpoint(float po2)`

Sets a new CCR setpoint (`> 0`) and switches gas model to CCR mode.

### `bool setFiO2(float fiO2)`

Sets a new OC gas FiO2 (`>= 0` and `<= 1`) and switches gas model to OC mode.

### `float getPo2Setpoint()` / `float getFiO2()` / `bool isOCmode()`

Query current gas model settings.

## Typical Usage

1. Call `decoSetupCC(gfLowPercent, gfHighPercent, po2Setpoint)` for CCR or `decoSetupOC(gfLowPercent, gfHighPercent, fiO2)` for OC in `setup()`
2. Call `decoInit()` once right after
3. Call `decoUpdate(pressureAtm, dtMin)` in `loop()`
4. Call `decoCompute(pressureAtm)` at your display/log interval
5. Use fields in `DecoResult` to get Deco, Stop, Time, TTS, surfGF
6. Toggle `ripNtear(true)` to see decompression result at raw M-value