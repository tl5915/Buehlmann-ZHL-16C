#include <Arduino.h>
#include <ZHL16C.h>

static constexpr float surfaceAtm = 1.0f;
static constexpr float metersPerAtm = 10.13f;

static float pressureAtmFromDepth(float depthMeters) {
  return surfaceAtm + (depthMeters / metersPerAtm);
}


void setup() {
  Serial.begin(115200);
  delay(10);

  // Configure model parameters: GF Low, GF High, CCR setpoint
  if (!decoSetup(60, 85, 1.2f)) {
    Serial.println("Invalid parameters, default values will be used");
  }

  // Initialise model
  decoInit();
  Serial.println("Dive Started");
}


void loop() {
  // Example profile
  float depthMeters = 18.0f;  // Current depth reading from pressure sensor
  float pressureAtm = pressureAtmFromDepth(depthMeters);  // Convert depth to atm for model input
  float dtMin = 16.5;  // Dive time in minutes

  // Update model with current depth and time
  decoUpdate(pressureAtm, dtMin);

  // Optional: disable gradient factors, this will force GF to 100% and remove conservatism
  // ripNtear(true);

  // Query model output
  DecoResult result = decoCompute(pressureAtm);

  // Variables for model output
  bool inDeco = result.inDeco;
  u_int16_t nextStopDepth = result.nextStopDepth;
  u_int16_t stopTime = result.stopTime;
  u_int16_t timeToSurface = result.timeToSurface;
  u_int16_t surfGF = result.surfGF;

  // Print results
  Serial.print("Dive Time: ");
  Serial.print(dtMin);
  Serial.println(" min");

  Serial.print("Depth: ");
  Serial.print(depthMeters);
  Serial.println(" m");

  if (inDeco) {
    Serial.println("DECO");
  } else {
    Serial.println("NDL");
  }
  
  Serial.print("Stop: ");
  Serial.print(nextStopDepth);
  Serial.println(" m");

  Serial.print("Time: ");
  Serial.print(stopTime);
  Serial.println(" min");

  Serial.print("TTS: ");
  Serial.print(timeToSurface);
  Serial.println(" min");

  Serial.print("Surface GF: ");
  Serial.print(surfGF);
  Serial.println("%");

  delay(1000);
}