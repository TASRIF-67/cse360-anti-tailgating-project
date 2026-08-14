/*
  ================================================================
  Arduino UNO Main Controller
  Anti-Tailgating Security Interlock Gate
  ================================================================

  This is the MAIN controller for:
    - RC522 RFID authentication
    - 4x4 keypad PIN entry through PCF8574
    - 16x2 I2C LCD
    - 1 kg load cell through HX711
    - IR-A and IR-B passage sensors
    - Gate A and Gate B SG90 servos
    - Gate A CLOSED limit switch
    - Emergency push button
    - Active buzzer
    - UART event messages to ESP32-AI-CAM

  IMPORTANT:
    Arduino sketches are compiled as C++, but this file is intentionally
    written in a simple C-like beginner style:
      - normal functions
      - clear if/else blocks
      - explicit curly braces
      - no advanced classes created by us
      - no one-line compressed logic

  ----------------------------------------------------------------
  FINAL UNO PIN MAP
  ----------------------------------------------------------------

  D0  = USB Serial RX only
  D1  = USB Serial TX only

  D2  = Emergency push button
  D3  = HX711 DOUT / DT
  D4  = HX711 SCK / CLK
  D5  = Gate A servo signal
  D6  = Gate B servo signal
  D7  = IR-A output
  D8  = IR-B output
  D9  = Active buzzer signal
  D10 = RC522 SS / SDA through CD4050
  D11 = RC522 MOSI through CD4050
  D12 = RC522 MISO directly to UNO
  D13 = RC522 SCK through CD4050

  A0  = RC522 RST through CD4050
  A1  = Gate A CLOSED limit switch
  A2  = UNO RX from ESP32 TX (direct)
  A3  = UNO TX to ESP32 RX (through CD4050)
  A4  = I2C SDA: LCD + PCF8574
  A5  = I2C SCL: LCD + PCF8574

  ----------------------------------------------------------------
  CD4050 CHANNELS
  ----------------------------------------------------------------

  UNO D10 -> CD4050 pin 3  -> pin 2  -> RC522 SS/SDA
  UNO D11 -> CD4050 pin 5  -> pin 4  -> RC522 MOSI
  UNO D13 -> CD4050 pin 7  -> pin 6  -> RC522 SCK
  UNO A0  -> CD4050 pin 9  -> pin 10 -> RC522 RST
  UNO A3  -> CD4050 pin 11 -> pin 12 -> ESP32 RX / GPIO3

  RC522 MISO -> UNO D12 directly.
  ESP32 TX / GPIO1 -> UNO A2 directly.

  ----------------------------------------------------------------
  BEFORE USING THE COMPLETE PROGRAM
  ----------------------------------------------------------------

  1. Run the I2C scanner and set:
       LCD_I2C_ADDRESS
       KEYPAD_PCF_ADDRESS

  2. Run the RFID UID reader and replace the example UIDs.

  3. Calibrate the load cell and set:
       LOAD_CELL_CONFIGURED = true
       LOAD_CALIBRATION_FACTOR
       LOAD_EMPTY_MAX_GRAMS
       LOAD_EXPECTED_MIN_GRAMS
       LOAD_EXPECTED_MAX_GRAMS

  4. Calibrate the four servo angles.

  5. Test whether your FC-51 sensors are active LOW or active HIGH.

  ================================================================
*/


#include <Wire.h>
#include <SPI.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>
#include <HX711.h>
#include <LiquidCrystal_I2C.h>


// ================================================================
// 1. PIN DEFINITIONS
// ================================================================

const byte PIN_EMERGENCY = 2;

const byte PIN_HX711_DOUT = 3;
const byte PIN_HX711_SCK = 4;

const byte PIN_SERVO_A = 5;
const byte PIN_SERVO_B = 6;

const byte PIN_IR_A = 7;
const byte PIN_IR_B = 8;

const byte PIN_BUZZER = 9;

const byte PIN_RFID_SS = 10;
const byte PIN_RFID_RST = A0;

const byte PIN_GATE_A_CLOSED = A1;

const byte PIN_ESP_RX = A2;
const byte PIN_ESP_TX = A3;


// ================================================================
// 2. I2C ADDRESSES
// ================================================================
//
// CHANGE THESE after running tests/01_i2c_scanner.
//
// Common examples are:
//   LCD backpack: 0x27
//   PCF8574:      0x20
//
// But YOUR modules may use different addresses.
//

const byte LCD_I2C_ADDRESS = 0x27;
const byte KEYPAD_PCF_ADDRESS = 0x20;


// ================================================================
// 3. HARDWARE OBJECTS
// ================================================================

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

HX711 scale;

Servo gateA;
Servo gateB;

// RX, TX
SoftwareSerial espSerial(PIN_ESP_RX, PIN_ESP_TX);


// ================================================================
// 4. RFID USER DATABASE
// ================================================================
//
// Replace the example UIDs with the UIDs printed by
// tests/04_rc522_uid_reader.
//
// Each user has:
//   - UID bytes
//   - UID length
//   - 4-digit PIN
//
// You can keep only one user if you want.
//

struct UserRecord
{
    byte uid[10];
    byte uidLength;
    char pin[5];
};


// EXAMPLE USER 1:
// Replace DE AD BE EF with your real card UID.
const UserRecord USERS[] =
{
    {
        {0xDE, 0xAD, 0xBE, 0xEF, 0, 0, 0, 0, 0, 0},
        4,
        "1234"
    },

    // OPTIONAL SECOND USER.
    // Replace this UID too, or delete this complete record.
    {
        {0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0, 0, 0},
        4,
        "5678"
    }
};

const byte USER_COUNT = sizeof(USERS) / sizeof(USERS[0]);


// ================================================================
// 5. KEYPAD LAYOUT
// ================================================================
//
// PCF8574:
//   P0 P1 P2 P3 = keypad rows R1 R2 R3 R4
//   P4 P5 P6 P7 = keypad columns C1 C2 C3 C4
//

const char KEYPAD_MAP[4][4] =
{
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};


// ================================================================
// 6. SERVO CALIBRATION
// ================================================================
//
// THESE ARE EXAMPLES.
// Replace them after running the servo calibration sketch.
//

const int GATE_A_CLOSED_ANGLE = 20;
const int GATE_A_OPEN_ANGLE = 90;

const int GATE_B_CLOSED_ANGLE = 20;
const int GATE_B_OPEN_ANGLE = 90;

const unsigned long GATE_MOVE_TIME_MS = 900;


// ================================================================
// 7. IR SENSOR SETTINGS
// ================================================================
//
// Many FC-51 modules are active LOW:
//   obstacle detected = LOW
//
// If your test shows the opposite, set this to false.
//

const bool IR_ACTIVE_LOW = true;


// ================================================================
// 8. BUZZER SETTINGS
// ================================================================
//
// This code assumes a normal 3-pin ACTIVE buzzer module.
//
// If your module sounds when SIG is HIGH, leave true.
// If it sounds when SIG is LOW, change it to false.
//
// If you are using a bare 2-pin buzzer with a transistor driver,
// adjust the hardware and logic separately.
//

const bool BUZZER_ACTIVE_HIGH = true;


// ================================================================
// 9. LOAD CELL CALIBRATION AND THRESHOLDS
// ================================================================
//
// IMPORTANT:
// The system should NOT make chamber decisions until the load cell
// has been calibrated.
//
// Run:
//   tests/11_hx711_calibration
//
// Then:
//   1. Put the real factor below.
//   2. Set LOAD_CELL_CONFIGURED = true.
//   3. Measure your actual model weights.
//   4. Replace the example thresholds.
//
// Example only:
//   empty chamber  = around 0 g
//   one model      = around 200 to 300 g
//   extra load     = above 400 g
//

const bool LOAD_CELL_CONFIGURED = false;

// EXAMPLE ONLY. Replace this.
const float LOAD_CALIBRATION_FACTOR = 1.0;

// EXAMPLE THRESHOLDS. Replace after calibration.
const float LOAD_EMPTY_MAX_GRAMS = 30.0;
const float LOAD_EXPECTED_MIN_GRAMS = 100.0;
const float LOAD_EXPECTED_MAX_GRAMS = 400.0;


// ================================================================
// 10. TIMEOUTS
// ================================================================

const unsigned long PIN_ENTRY_TIMEOUT_MS = 15000;
const unsigned long ENTRY_TIMEOUT_MS = 12000;
const unsigned long EXIT_TIMEOUT_MS = 12000;

const unsigned long MESSAGE_TIME_MS = 1800;


// ================================================================
// 11. SYSTEM STATES
// ================================================================

enum SystemState
{
    STATE_IDLE,
    STATE_WAIT_PIN,

    STATE_OPEN_GATE_A,
    STATE_WAIT_ENTRY,
    STATE_CLOSE_GATE_A,
    STATE_CHECK_CHAMBER,

    STATE_OPEN_GATE_B,
    STATE_WAIT_EXIT,
    STATE_CLOSE_GATE_B,

    STATE_ACCESS_DENIED,
    STATE_TAILGATING_ALERT,
    STATE_FAULT,
    STATE_EMERGENCY
};

SystemState currentState = STATE_IDLE;

unsigned long stateStartTime = 0;


// ================================================================
// 12. RUNTIME VARIABLES
// ================================================================

int currentUserIndex = -1;

char enteredPin[5];
byte enteredPinLength = 0;

bool entrySensorWasBlocked = false;
bool exitSensorWasBlocked = false;

bool espReady = false;

char espReceiveBuffer[64];
byte espReceiveIndex = 0;


// ================================================================
// 13. FUNCTION PROTOTYPES
// ================================================================

void initializePins();
void initializeDisplay();
void initializeRFID();
void initializeLoadCell();
void initializeServos();

void setState(SystemState newState);
void runCurrentState();

void handleIdleState();
void handleWaitPinState();
void handleOpenGateAState();
void handleWaitEntryState();
void handleCloseGateAState();
void handleCheckChamberState();
void handleOpenGateBState();
void handleWaitExitState();
void handleCloseGateBState();
void handleAccessDeniedState();
void handleTailgatingState();
void handleFaultState();
void handleEmergencyState();

void displayMessage(const char *line1, const char *line2);
void displayPinProgress();

bool emergencyButtonPressed();

bool irADetected();
bool irBDetected();

bool gateAClosedVerified();

void openGateA();
void closeGateA();
void openGateB();
void closeGateB();

void buzzerOn();
void buzzerOff();
void beepShort();

int readCardAndFindUser();
int findUserByUid(byte *uidBytes, byte uidLength);

char readKeypadKey();
char scanKeypadRaw();
void writePCF8574(byte value);
byte readPCF8574();

void clearEnteredPin();
void addPinDigit(char key);
bool enteredPinIsCorrect();

bool readWeightGrams(float &weightGrams);
bool chamberIsEmpty();
bool chamberHasExpectedLoad(float weightGrams);
bool chamberHasAbnormalHighLoad(float weightGrams);

void enterTailgatingAlert();
void enterFault(const char *reason);
void enterEmergency();

bool safeManualResetPossible();
void performManualReset();

void sendEspEvent(const char *message);
void processEspMessages();


// ================================================================
// 14. SETUP
// ================================================================

void setup()
{
    Serial.begin(9600);

    // UART to ESP32-AI-CAM.
    espSerial.begin(9600);

    Wire.begin();
    SPI.begin();

    initializePins();
    initializeDisplay();
    initializeRFID();
    initializeLoadCell();
    initializeServos();

    displayMessage("Starting...", "Please wait");

    delay(1200);

    // The Gate A CLOSED switch is our only physical gate-position
    // verification switch in the simplified build.
    if (gateAClosedVerified() == false)
    {
        enterFault("A NOT CLOSED");
        return;
    }

    if (LOAD_CELL_CONFIGURED == false)
    {
        displayMessage("Calibrate Load", "Then set TRUE");

        Serial.println("LOAD CELL CONFIGURATION IS NOT FINISHED.");
        Serial.println("Run the load-cell calibration sketch.");
        Serial.println("Then update the calibration factor and thresholds.");
        Serial.println("Finally set LOAD_CELL_CONFIGURED = true.");

        currentState = STATE_FAULT;
        stateStartTime = millis();

        buzzerOn();

        return;
    }

    setState(STATE_IDLE);
    sendEspEvent("READY");
}


// ================================================================
// 15. MAIN LOOP
// ================================================================

void loop()
{
    // Read messages from ESP32.
    // ESP32 is never allowed to control gate safety.
    processEspMessages();

    // Emergency input has the highest priority.
    if (emergencyButtonPressed() == true)
    {
        if (currentState != STATE_EMERGENCY)
        {
            enterEmergency();
        }
    }

    runCurrentState();
}


// ================================================================
// 16. INITIALIZATION FUNCTIONS
// ================================================================

void initializePins()
{
    pinMode(PIN_EMERGENCY, INPUT_PULLUP);

    pinMode(PIN_IR_A, INPUT);
    pinMode(PIN_IR_B, INPUT);

    pinMode(PIN_BUZZER, OUTPUT);

    pinMode(PIN_GATE_A_CLOSED, INPUT_PULLUP);

    buzzerOff();
}


void initializeDisplay()
{
    lcd.init();
    lcd.backlight();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Anti-Tailgate");
    lcd.setCursor(0, 1);
    lcd.print("Initializing");
}


void initializeRFID()
{
    rfid.PCD_Init();

    Serial.println("RC522 initialized.");
}


void initializeLoadCell()
{
    scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);

    if (LOAD_CELL_CONFIGURED == true)
    {
        scale.set_scale(LOAD_CALIBRATION_FACTOR);

        // The chamber MUST be empty at startup.
        if (scale.wait_ready_timeout(1500) == true)
        {
            scale.tare();

            Serial.println("HX711 ready. Empty chamber tared.");
        }
        else
        {
            Serial.println("HX711 did not respond during startup.");
        }
    }
}


void initializeServos()
{
    gateA.attach(PIN_SERVO_A);
    gateB.attach(PIN_SERVO_B);

    // Normal startup position = both gates closed.
    closeGateA();
    closeGateB();

    delay(GATE_MOVE_TIME_MS);
}


// ================================================================
// 17. STATE CONTROL
// ================================================================

void setState(SystemState newState)
{
    currentState = newState;
    stateStartTime = millis();

    if (newState == STATE_IDLE)
    {
        currentUserIndex = -1;

        clearEnteredPin();

        entrySensorWasBlocked = false;
        exitSensorWasBlocked = false;

        buzzerOff();

        displayMessage("System Ready", "Scan RFID");
    }

    else if (newState == STATE_WAIT_PIN)
    {
        clearEnteredPin();

        displayMessage("Enter PIN", "# = submit");
    }

    else if (newState == STATE_OPEN_GATE_A)
    {
        displayMessage("Access OK", "Opening Gate A");

        sendEspEvent("AUTH_OK");

        beepShort();

        openGateA();
    }

    else if (newState == STATE_WAIT_ENTRY)
    {
        entrySensorWasBlocked = false;

        displayMessage("Enter Chamber", "Gate A open");
    }

    else if (newState == STATE_CLOSE_GATE_A)
    {
        displayMessage("Closing Gate A", "Please wait");

        closeGateA();
    }

    else if (newState == STATE_CHECK_CHAMBER)
    {
        displayMessage("Checking", "Chamber load");
    }

    else if (newState == STATE_OPEN_GATE_B)
    {
        displayMessage("Chamber OK", "Opening Gate B");

        sendEspEvent("CHAMBER_OK");

        openGateB();
    }

    else if (newState == STATE_WAIT_EXIT)
    {
        exitSensorWasBlocked = false;

        displayMessage("Exit Now", "Through Gate B");
    }

    else if (newState == STATE_CLOSE_GATE_B)
    {
        displayMessage("Closing Gate B", "Please wait");

        closeGateB();
    }

    else if (newState == STATE_ACCESS_DENIED)
    {
        displayMessage("ACCESS DENIED", "Try again");

        sendEspEvent("AUTH_DENIED");

        buzzerOn();
    }

    else if (newState == STATE_TAILGATING_ALERT)
    {
        displayMessage("POSSIBLE", "TAILGATING");

        sendEspEvent("POSSIBLE_TAILGATING");

        // Keep Gate B locked.
        closeGateB();

        // Allow safe return through Gate A.
        openGateA();

        buzzerOn();
    }

    else if (newState == STATE_FAULT)
    {
        buzzerOn();
    }

    else if (newState == STATE_EMERGENCY)
    {
        displayMessage("EMERGENCY", "# after clear");

        sendEspEvent("EMERGENCY");

        // Emergency release opens BOTH gates.
        openGateA();
        openGateB();

        buzzerOn();
    }
}


void runCurrentState()
{
    if (currentState == STATE_IDLE)
    {
        handleIdleState();
    }

    else if (currentState == STATE_WAIT_PIN)
    {
        handleWaitPinState();
    }

    else if (currentState == STATE_OPEN_GATE_A)
    {
        handleOpenGateAState();
    }

    else if (currentState == STATE_WAIT_ENTRY)
    {
        handleWaitEntryState();
    }

    else if (currentState == STATE_CLOSE_GATE_A)
    {
        handleCloseGateAState();
    }

    else if (currentState == STATE_CHECK_CHAMBER)
    {
        handleCheckChamberState();
    }

    else if (currentState == STATE_OPEN_GATE_B)
    {
        handleOpenGateBState();
    }

    else if (currentState == STATE_WAIT_EXIT)
    {
        handleWaitExitState();
    }

    else if (currentState == STATE_CLOSE_GATE_B)
    {
        handleCloseGateBState();
    }

    else if (currentState == STATE_ACCESS_DENIED)
    {
        handleAccessDeniedState();
    }

    else if (currentState == STATE_TAILGATING_ALERT)
    {
        handleTailgatingState();
    }

    else if (currentState == STATE_FAULT)
    {
        handleFaultState();
    }

    else if (currentState == STATE_EMERGENCY)
    {
        handleEmergencyState();
    }
}


// ================================================================
// 18. IDLE / RFID
// ================================================================

void handleIdleState()
{
    int result = readCardAndFindUser();

    // -2 means no new card.
    if (result == -2)
    {
        return;
    }

    // -1 means a card was read but it is not authorized.
    if (result == -1)
    {
        setState(STATE_ACCESS_DENIED);

        return;
    }

    // 0 or greater is the authorized user index.
    currentUserIndex = result;

    sendEspEvent("RFID_OK");

    setState(STATE_WAIT_PIN);
}


// ================================================================
// 19. PIN ENTRY
// ================================================================

void handleWaitPinState()
{
    char key = readKeypadKey();

    if (key != 0)
    {
        if (key >= '0' && key <= '9')
        {
            addPinDigit(key);

            displayPinProgress();
        }

        else if (key == '*')
        {
            clearEnteredPin();

            displayPinProgress();
        }

        else if (key == 'D')
        {
            setState(STATE_ACCESS_DENIED);

            return;
        }

        else if (key == '#')
        {
            if (enteredPinIsCorrect() == true)
            {
                setState(STATE_OPEN_GATE_A);
            }
            else
            {
                setState(STATE_ACCESS_DENIED);
            }

            return;
        }
    }

    if (millis() - stateStartTime > PIN_ENTRY_TIMEOUT_MS)
    {
        setState(STATE_ACCESS_DENIED);
    }
}


// ================================================================
// 20. OPEN GATE A
// ================================================================

void handleOpenGateAState()
{
    if (millis() - stateStartTime >= GATE_MOVE_TIME_MS)
    {
        setState(STATE_WAIT_ENTRY);
    }
}


// ================================================================
// 21. WAIT FOR ENTRY
// ================================================================

void handleWaitEntryState()
{
    // Person/model blocks IR-A while crossing Gate A.
    if (irADetected() == true)
    {
        if (entrySensorWasBlocked == false)
        {
            entrySensorWasBlocked = true;

            sendEspEvent("ENTRY_DETECTED");
        }
    }

    // A complete crossing should normally produce:
    // CLEAR -> BLOCKED -> CLEAR.
    if (entrySensorWasBlocked == true)
    {
        if (irADetected() == false)
        {
            float weightGrams = 0.0;

            if (readWeightGrams(weightGrams) == false)
            {
                enterFault("HX711 ERROR");

                return;
            }

            Serial.print("Entry chamber weight: ");
            Serial.println(weightGrams);

            // If the IR sensor saw a crossing but the load cell still
            // sees an empty chamber, the sensors disagree.
            if (weightGrams < LOAD_EXPECTED_MIN_GRAMS)
            {
                enterFault("NO LOAD FOUND");

                return;
            }

            // If the weight is already too high before Gate A closes,
            // classify it conservatively as possible tailgating /
            // unexpected extra load.
            if (chamberHasAbnormalHighLoad(weightGrams) == true)
            {
                enterTailgatingAlert();

                return;
            }

            setState(STATE_CLOSE_GATE_A);

            return;
        }
    }

    if (millis() - stateStartTime > ENTRY_TIMEOUT_MS)
    {
        enterFault("ENTRY TIMEOUT");
    }
}


// ================================================================
// 22. CLOSE AND VERIFY GATE A
// ================================================================

void handleCloseGateAState()
{
    // If IR-A becomes blocked while Gate A is closing,
    // do not keep closing on an obstruction.
    if (irADetected() == true)
    {
        enterTailgatingAlert();

        return;
    }

    if (millis() - stateStartTime >= GATE_MOVE_TIME_MS)
    {
        if (gateAClosedVerified() == false)
        {
            enterFault("A NOT CLOSED");

            return;
        }

        sendEspEvent("GATE_A_CLOSED");

        setState(STATE_CHECK_CHAMBER);
    }
}


// ================================================================
// 23. CHAMBER CHECK
// ================================================================

void handleCheckChamberState()
{
    // Gate B must never open unless Gate A is physically confirmed closed.
    if (gateAClosedVerified() == false)
    {
        enterFault("A OPEN FAULT");

        return;
    }

    // Gate B exit sensor must not already be blocked.
    if (irBDetected() == true)
    {
        enterFault("IR-B BLOCKED");

        return;
    }

    float weightGrams = 0.0;

    if (readWeightGrams(weightGrams) == false)
    {
        enterFault("HX711 ERROR");

        return;
    }

    Serial.print("Chamber check weight: ");
    Serial.println(weightGrams);

    if (chamberHasAbnormalHighLoad(weightGrams) == true)
    {
        enterTailgatingAlert();

        return;
    }

    if (chamberHasExpectedLoad(weightGrams) == false)
    {
        enterFault("BAD LOAD");

        return;
    }

    setState(STATE_OPEN_GATE_B);
}


// ================================================================
// 24. OPEN GATE B
// ================================================================

void handleOpenGateBState()
{
    // Re-check the most important invariant while Gate B opens.
    if (gateAClosedVerified() == false)
    {
        closeGateB();

        enterFault("A OPEN FAULT");

        return;
    }

    if (millis() - stateStartTime >= GATE_MOVE_TIME_MS)
    {
        setState(STATE_WAIT_EXIT);
    }
}


// ================================================================
// 25. WAIT FOR EXIT
// ================================================================

void handleWaitExitState()
{
    if (irBDetected() == true)
    {
        if (exitSensorWasBlocked == false)
        {
            exitSensorWasBlocked = true;

            sendEspEvent("EXIT_DETECTED");
        }
    }

    if (exitSensorWasBlocked == true)
    {
        if (irBDetected() == false)
        {
            float weightGrams = 0.0;

            if (readWeightGrams(weightGrams) == false)
            {
                enterFault("HX711 ERROR");

                return;
            }

            Serial.print("After exit weight: ");
            Serial.println(weightGrams);

            // Do not finish the cycle while a significant load remains
            // in the security chamber.
            if (weightGrams <= LOAD_EMPTY_MAX_GRAMS)
            {
                setState(STATE_CLOSE_GATE_B);

                return;
            }
        }
    }

    if (millis() - stateStartTime > EXIT_TIMEOUT_MS)
    {
        enterFault("EXIT TIMEOUT");
    }
}


// ================================================================
// 26. CLOSE GATE B
// ================================================================

void handleCloseGateBState()
{
    // If something enters the Gate B sensor zone while the gate is
    // closing, reopen Gate B to avoid trapping the person/model.
    if (irBDetected() == true)
    {
        openGateB();

        stateStartTime = millis();

        currentState = STATE_WAIT_EXIT;

        displayMessage("Gate B blocked", "Exit safely");

        return;
    }

    if (millis() - stateStartTime >= GATE_MOVE_TIME_MS)
    {
        sendEspEvent("PASSAGE_COMPLETE");

        displayMessage("Passage Done", "System reset");

        delay(MESSAGE_TIME_MS);

        setState(STATE_IDLE);
    }
}


// ================================================================
// 27. ACCESS DENIED
// ================================================================

void handleAccessDeniedState()
{
    if (millis() - stateStartTime >= MESSAGE_TIME_MS)
    {
        buzzerOff();

        setState(STATE_IDLE);
    }
}


// ================================================================
// 28. POSSIBLE TAILGATING
// ================================================================

void handleTailgatingState()
{
    // Gate B remains closed.
    closeGateB();

    // Gate A remains open for safe return.
    openGateA();

    // To reset:
    // 1. Return everything out through Gate A.
    // 2. Make chamber empty.
    // 3. Make IR-A and IR-B clear.
    // 4. Press #.
    char key = readKeypadKey();

    if (key == '#')
    {
        if (safeManualResetPossible() == true)
        {
            performManualReset();
        }
        else
        {
            displayMessage("Clear chamber", "Then press #");
        }
    }
}


// ================================================================
// 29. FAULT
// ================================================================

void handleFaultState()
{
    // Simplified safe fault behavior:
    //   - Gate B closed
    //   - Gate A open for return
    //
    // NOTE:
    // A real access-control product would require more detailed
    // fail-safe mechanical design.
    closeGateB();
    openGateA();

    char key = readKeypadKey();

    if (key == '#')
    {
        if (safeManualResetPossible() == true)
        {
            performManualReset();
        }
        else
        {
            displayMessage("FAULT ACTIVE", "Clear + press #");
        }
    }
}


// ================================================================
// 30. EMERGENCY
// ================================================================

void handleEmergencyState()
{
    // Keep both gates open while the physical emergency button
    // is still pressed.
    openGateA();
    openGateB();

    if (emergencyButtonPressed() == true)
    {
        return;
    }

    // After emergency button is released, operator must press #
    // to deliberately recover.
    char key = readKeypadKey();

    if (key == '#')
    {
        if (safeManualResetPossible() == true)
        {
            performManualReset();
        }
        else
        {
            displayMessage("Clear system", "Then press #");
        }
    }
}


// ================================================================
// 31. LCD FUNCTIONS
// ================================================================

void displayMessage(const char *line1, const char *line2)
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);
}


void displayPinProgress()
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("PIN: ");

    byte i = 0;

    for (i = 0; i < enteredPinLength; i++)
    {
        lcd.print('*');
    }

    lcd.setCursor(0, 1);
    lcd.print("# submit * clear");
}


// ================================================================
// 32. BUTTON / SENSOR FUNCTIONS
// ================================================================

bool emergencyButtonPressed()
{
    if (digitalRead(PIN_EMERGENCY) == LOW)
    {
        return true;
    }

    return false;
}


bool irADetected()
{
    int value = digitalRead(PIN_IR_A);

    if (IR_ACTIVE_LOW == true)
    {
        if (value == LOW)
        {
            return true;
        }

        return false;
    }

    if (value == HIGH)
    {
        return true;
    }

    return false;
}


bool irBDetected()
{
    int value = digitalRead(PIN_IR_B);

    if (IR_ACTIVE_LOW == true)
    {
        if (value == LOW)
        {
            return true;
        }

        return false;
    }

    if (value == HIGH)
    {
        return true;
    }

    return false;
}


bool gateAClosedVerified()
{
    // A1 uses INPUT_PULLUP.
    // Switch COM -> GND.
    // Switch NO  -> A1.
    //
    // Fully closed:
    //   COM and NO connect -> A1 becomes LOW.
    if (digitalRead(PIN_GATE_A_CLOSED) == LOW)
    {
        return true;
    }

    return false;
}


// ================================================================
// 33. SERVO FUNCTIONS
// ================================================================

void openGateA()
{
    gateA.write(GATE_A_OPEN_ANGLE);
}


void closeGateA()
{
    gateA.write(GATE_A_CLOSED_ANGLE);
}


void openGateB()
{
    gateB.write(GATE_B_OPEN_ANGLE);
}


void closeGateB()
{
    gateB.write(GATE_B_CLOSED_ANGLE);
}


// ================================================================
// 34. BUZZER FUNCTIONS
// ================================================================

void buzzerOn()
{
    if (BUZZER_ACTIVE_HIGH == true)
    {
        digitalWrite(PIN_BUZZER, HIGH);
    }
    else
    {
        digitalWrite(PIN_BUZZER, LOW);
    }
}


void buzzerOff()
{
    if (BUZZER_ACTIVE_HIGH == true)
    {
        digitalWrite(PIN_BUZZER, LOW);
    }
    else
    {
        digitalWrite(PIN_BUZZER, HIGH);
    }
}


void beepShort()
{
    buzzerOn();

    delay(120);

    buzzerOff();
}


// ================================================================
// 35. RFID FUNCTIONS
// ================================================================

int readCardAndFindUser()
{
    if (rfid.PICC_IsNewCardPresent() == false)
    {
        return -2;
    }

    if (rfid.PICC_ReadCardSerial() == false)
    {
        return -2;
    }

    Serial.print("RFID UID: ");

    byte i = 0;

    for (i = 0; i < rfid.uid.size; i++)
    {
        if (rfid.uid.uidByte[i] < 0x10)
        {
            Serial.print('0');
        }

        Serial.print(rfid.uid.uidByte[i], HEX);

        if (i + 1 < rfid.uid.size)
        {
            Serial.print(' ');
        }
    }

    Serial.println();

    int userIndex = findUserByUid(rfid.uid.uidByte, rfid.uid.size);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return userIndex;
}


int findUserByUid(byte *uidBytes, byte uidLength)
{
    byte userIndex = 0;

    for (userIndex = 0; userIndex < USER_COUNT; userIndex++)
    {
        if (USERS[userIndex].uidLength != uidLength)
        {
            continue;
        }

        bool allBytesMatch = true;

        byte i = 0;

        for (i = 0; i < uidLength; i++)
        {
            if (USERS[userIndex].uid[i] != uidBytes[i])
            {
                allBytesMatch = false;

                break;
            }
        }

        if (allBytesMatch == true)
        {
            return userIndex;
        }
    }

    return -1;
}


// ================================================================
// 36. PIN BUFFER FUNCTIONS
// ================================================================

void clearEnteredPin()
{
    enteredPinLength = 0;

    enteredPin[0] = '\0';
}


void addPinDigit(char key)
{
    if (enteredPinLength >= 4)
    {
        return;
    }

    enteredPin[enteredPinLength] = key;

    enteredPinLength++;

    enteredPin[enteredPinLength] = '\0';
}


bool enteredPinIsCorrect()
{
    if (currentUserIndex < 0)
    {
        return false;
    }

    if (enteredPinLength != 4)
    {
        return false;
    }

    if (strcmp(enteredPin, USERS[currentUserIndex].pin) == 0)
    {
        return true;
    }

    return false;
}


// ================================================================
// 37. PCF8574 KEYPAD FUNCTIONS
// ================================================================

void writePCF8574(byte value)
{
    Wire.beginTransmission(KEYPAD_PCF_ADDRESS);

    Wire.write(value);

    Wire.endTransmission();
}


byte readPCF8574()
{
    byte value = 0xFF;

    Wire.requestFrom((int)KEYPAD_PCF_ADDRESS, 1);

    if (Wire.available() > 0)
    {
        value = Wire.read();
    }

    return value;
}


char scanKeypadRaw()
{
    byte row = 0;
    byte column = 0;

    for (row = 0; row < 4; row++)
    {
        // PCF8574 is quasi-bidirectional.
        //
        // Start with every line HIGH.
        byte outputState = 0xFF;

        // Pull ONE row LOW.
        bitClear(outputState, row);

        writePCF8574(outputState);

        delayMicroseconds(120);

        byte inputState = readPCF8574();

        for (column = 0; column < 4; column++)
        {
            byte columnBit = 4 + column;

            // Pressed key connects the selected LOW row to a column,
            // so that column reads LOW.
            if (bitRead(inputState, columnBit) == 0)
            {
                writePCF8574(0xFF);

                return KEYPAD_MAP[row][column];
            }
        }
    }

    // Release all PCF pins HIGH when scanning is finished.
    writePCF8574(0xFF);

    return 0;
}


char readKeypadKey()
{
    // Simple non-blocking debounce.
    static char previousRawKey = 0;
    static char stableKey = 0;
    static unsigned long lastChangeTime = 0;

    char rawKey = scanKeypadRaw();

    if (rawKey != previousRawKey)
    {
        previousRawKey = rawKey;

        lastChangeTime = millis();
    }

    if (millis() - lastChangeTime >= 40)
    {
        if (rawKey != stableKey)
        {
            stableKey = rawKey;

            // Only return a key when it becomes PRESSED.
            // When it becomes released, stableKey becomes 0
            // but we return nothing.
            if (stableKey != 0)
            {
                return stableKey;
            }
        }
    }

    return 0;
}


// ================================================================
// 38. LOAD CELL FUNCTIONS
// ================================================================

bool readWeightGrams(float &weightGrams)
{
    if (LOAD_CELL_CONFIGURED == false)
    {
        return false;
    }

    // Prevent a disconnected HX711 from blocking forever.
    if (scale.wait_ready_timeout(300) == false)
    {
        return false;
    }

    // Average three readings.
    // This is intentionally simple.
    weightGrams = scale.get_units(3);

    // Small negative values around zero are normal after tare.
    if (weightGrams < 0.0)
    {
        if (weightGrams > -LOAD_EMPTY_MAX_GRAMS)
        {
            weightGrams = 0.0;
        }
    }

    return true;
}


bool chamberIsEmpty()
{
    float weightGrams = 0.0;

    if (readWeightGrams(weightGrams) == false)
    {
        return false;
    }

    if (weightGrams <= LOAD_EMPTY_MAX_GRAMS)
    {
        return true;
    }

    return false;
}


bool chamberHasExpectedLoad(float weightGrams)
{
    if (weightGrams >= LOAD_EXPECTED_MIN_GRAMS)
    {
        if (weightGrams <= LOAD_EXPECTED_MAX_GRAMS)
        {
            return true;
        }
    }

    return false;
}


bool chamberHasAbnormalHighLoad(float weightGrams)
{
    if (weightGrams > LOAD_EXPECTED_MAX_GRAMS)
    {
        return true;
    }

    return false;
}


// ================================================================
// 39. ALERT / FAULT / EMERGENCY FUNCTIONS
// ================================================================

void enterTailgatingAlert()
{
    setState(STATE_TAILGATING_ALERT);
}


void enterFault(const char *reason)
{
    currentState = STATE_FAULT;
    stateStartTime = millis();

    closeGateB();
    openGateA();

    displayMessage("SYSTEM FAULT", reason);

    buzzerOn();

    sendEspEvent("FAULT");

    Serial.print("FAULT: ");
    Serial.println(reason);
}


void enterEmergency()
{
    setState(STATE_EMERGENCY);
}


// ================================================================
// 40. MANUAL SAFE RESET
// ================================================================

bool safeManualResetPossible()
{
    if (irADetected() == true)
    {
        return false;
    }

    if (irBDetected() == true)
    {
        return false;
    }

    if (chamberIsEmpty() == false)
    {
        return false;
    }

    return true;
}


void performManualReset()
{
    displayMessage("Recovering", "Closing gates");

    buzzerOff();

    // Close Gate B first.
    closeGateB();

    delay(GATE_MOVE_TIME_MS);

    // Then close Gate A.
    closeGateA();

    delay(GATE_MOVE_TIME_MS);

    if (gateAClosedVerified() == false)
    {
        enterFault("A RESET FAIL");

        return;
    }

    sendEspEvent("RECOVERED");

    setState(STATE_IDLE);
}


// ================================================================
// 41. ESP32 UART EVENT FUNCTIONS
// ================================================================

void sendEspEvent(const char *message)
{
    espSerial.println(message);

    Serial.print("UNO -> ESP32: ");
    Serial.println(message);
}


void processEspMessages()
{
    while (espSerial.available() > 0)
    {
        char incoming = espSerial.read();

        if (incoming == '\r')
        {
            continue;
        }

        if (incoming == '\n')
        {
            espReceiveBuffer[espReceiveIndex] = '\0';

            if (espReceiveIndex > 0)
            {
                Serial.print("ESP32 -> UNO: ");
                Serial.println(espReceiveBuffer);

                if (strcmp(espReceiveBuffer, "ESP_READY") == 0)
                {
                    espReady = true;
                }
            }

            espReceiveIndex = 0;

            continue;
        }

        if (espReceiveIndex < sizeof(espReceiveBuffer) - 1)
        {
            espReceiveBuffer[espReceiveIndex] = incoming;

            espReceiveIndex++;
        }
        else
        {
            // Buffer overflow protection.
            espReceiveIndex = 0;
        }
    }
}


/*
  ================================================================
  OPTIONAL / LATER FEATURES
  ================================================================

  The following features are intentionally NOT part of this file:

  1. RTC timestamping
  2. microSD logging
  3. More gate-position limit switches
  4. Remote gate-open commands from ESP32
  5. Face recognition / AI / ML

  If you later obtain more limit switches, add:
    - Gate A OPEN
    - Gate B OPEN
    - Gate B CLOSED

  Gate B should then be physically verified instead of relying only
  on servo angle + movement time.

  ESP32 is intentionally monitoring-only. Do NOT add a command that
  bypasses the UNO interlock and directly opens Gate B.
  ================================================================
*/
