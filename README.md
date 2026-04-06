# Bühlmann ZHL-16C Arduino Library

This library provides a compact Bühlmann ZHL-16C decompression model API for DIY dive computers.

## Model Assumptions

- Closed circuit mode with fixed PPO2 setpoint (I might implement open circuit in the future)
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

### `bool decoSetup(uint8_t gfLowPercent, uint8_t gfHighPercent, float po2Setpoint)`

Configures gradient factors and the CCR oxygen setpoint.- `gfLowPercent`: integer percent, must be `> 0` and `< gfHighPercent`.

- `gfHighPercent`: integer percent, must be `<= 100`
- `po2Setpoint`: float, must be `> 0`
- Returns `true` when the configuration is accepted
- If no valid input, model defaults to **GF 60/85** and **setpoint 1.2**


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

### `void mad_man_mode(bool enabled)`

Disable or enable gradient factor.

- `false`: normal decompression behavior, with conservatism by pre-set gradient factors
- `true`: ignore GF settings and force 100% GF (removes all conservatism, raw Bühlmann ZHL-16C algorithm)

Can be called in the middle of a dive if you find deco boring and want to ride the M-value train

## Typical Usage

1. Call `decoSetup(gfLowPercent, gfHighPercent, po2Setpoint)` in `setup()`
2. Call `decoInit()` once right after
3. Call `decoUpdate(pressureAtm, dtMin)` in `loop()`
4. Call `decoCompute(pressureAtm)` at your display/log interval
5. Use fields in `DecoResult` to get Deco, Stop, Time, TTS, surfGF
6. Toggle `mad_man_mode(true)` to see decompression result at raw M-value
