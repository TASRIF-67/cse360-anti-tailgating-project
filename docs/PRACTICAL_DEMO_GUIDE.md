# Practical Testing and Final Demonstration Guide

Run every isolated sketch in [`TESTING_ORDER.md`](TESTING_ORDER.md) before integrated testing.

## Safety and readiness

- Share ground between UNO, ESP32-CAM, sensors, and servo supply.
- Do not connect the separate servo +5 V rail to UNO +5 V.
- Power RC522 and CD4050 at 3.3 V and follow [`PIN_MAP.md`](PIN_MAP.md).
- Ensure gates move freely, Gate A presses its CLOSED switch, and the load platform does not touch the chamber walls or floor.
- Keep the emergency button and power disconnect reachable.
- Use a lightweight model; never use this shoebox prototype as a human gate.

Before integration, enter measured I2C addresses, RFID UID/PIN, IR polarity, servo angles, HX711 factor/load thresholds, and Wi-Fi credentials. Set `LOAD_CELL_CONFIGURED = true` only after calibration.

## Upload and observation setup

1. Upload both sketches in `firmware/` to their respective controllers.
2. Disconnect UNO A2/A3 while flashing the ESP32 if its programmer uses GPIO1/GPIO3; reconnect after normal boot.
3. Open the UNO Serial Monitor at 9600 baud.
4. Open the ESP32 IP address and check `/status`, `/capture`, and `/last-photo`.
5. Start each scenario empty and clear, with gates in safe positions. Press `#` only when requested for recovery.

## Practical pass/fail scenarios

| ID | Action | Expected result | Pass condition |
|---|---|---|---|
| D1 | Power on with an empty chamber. | B stays closed; A reaches CLOSED/A1; LCD shows `System Ready / Scan RFID`; UNO sends `READY`. | Ready appears without a fault or unintended opening. |
| D2 | Present an unknown card. | Gates stay closed; LCD shows `ACCESS DENIED`; `AUTH_DENIED`; `/last-photo` updates. | No gate movement and denial evidence is visible. |
| D3 | Use a known card, wrong PIN, then `#`. | Gates stay closed; `RFID_OK`, then `AUTH_DENIED`; photo updates. | Wrong PIN never opens a gate. |
| D4 | Use a known card/correct PIN. Move one calibrated model through IR-A onto the platform, then through IR-B and off it. | `AUTH_OK`, `ENTRY_DETECTED`, `GATE_A_CLOSED`, `CHAMBER_OK`, `EXIT_DETECTED`, `PASSAGE_COMPLETE`, `READY`. | A1 is verified before B opens; gates are never open together normally. |
| D5 | Repeat entry with load above `LOAD_EXPECTED_MAX_GRAMS`. | B stays closed; alarm; `POSSIBLE / TAILGATING`; A opens for return; `POSSIBLE_TAILGATING`; photo updates. | B never opens; clear chamber and press `#` to recover. |
| D6 | Safely prevent A1 confirming CLOSED, without touching a moving gate. | `SYSTEM FAULT / A NOT CLOSED`; `FAULT`; B stays closed and A opens for return. | A1 failure cannot lead to B opening. |
| D7 | Authenticate but do not enter before timeout. | Safe fault/return path; `FAULT`; B stays closed. | Timeout leaves no unsafe open path. |
| D8 | Press emergency from idle and, if safe, during a cycle. | Both gates open; alarm; `EMERGENCY / # after clear`; photo updates. | Emergency overrides normal state; release, clear, and press `#` to recover. |
| D9 | Power down only ESP32 and repeat an UNO denial/safe cycle. | UNO safety continues; web monitoring is unavailable. | ESP32 loss cannot command a gate or defeat UNO safety. |

Never put fingers or objects in a moving gate. Simulate sensor conditions electrically or with the lightweight model.

## What to show the evaluator

1. Architecture/pin map: UNO controls safety; ESP32 only monitors.
2. Power-on and ready screen.
3. Unknown-card or wrong-PIN denial.
4. Successful passage, emphasizing that B waits for A1 and acceptable load.
5. Possible-tailgating response with a calibrated heavier load.
6. Emergency release and deliberate recovery.
7. ESP32 status, current image, and stored alert photo.
8. Recorded calibration values and completed results below.

Say **possible tailgating** or **unexpected chamber condition**. Weight alone cannot prove two people are present.

## Test record

| Field | Result |
|---|---|
| Date, tester, commit | |
| I2C addresses, authorized UID | |
| IR polarity, servo angles | |
| HX711 factor/load thresholds | |
| D1 through D9 | Pass / Fail for each |
| Notes, timings, corrective actions | |

Do not claim verification until every safety-critical scenario passes repeatedly with the actual demonstration model.
