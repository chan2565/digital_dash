#include "raylib/src/raylib.h"
#include "raylib/src/raymath.h"
#include "obd_reader.h"
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 700
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)
#define GAUGE_RADIUS 150
#define MAX_RPM 8000
#define MIN_RPM 0
#define REDLINE_RPM 7000
#define MAX_SPEED 200
#define MAX_TEMP 120

// OBD mode toggle
typedef enum {
    MODE_SIMULATION,
    MODE_OBD
} TachMode;

typedef struct {
    float currentRPM;
    float targetRPM;
    float currentSpeed;
    float targetSpeed;
    float currentTemp;
    float targetTemp;
    TachMode mode;
    OBDConnection obd;
    bool obdThreadRunning;
    pthread_t obdThread;
    pthread_mutex_t dataMutex;
} Tachometer;

// Thread function to read OBD data
void* OBDReadThread(void* arg) {
    Tachometer* tach = (Tachometer*)arg;

    while (tach->obdThreadRunning) {
        int rpm = OBD_ReadRPM(&tach->obd);
        int speed = OBD_ReadSpeed(&tach->obd);
        int temp = OBD_ReadCoolantTemp(&tach->obd);

        pthread_mutex_lock(&tach->dataMutex);
        if (rpm >= 0) tach->targetRPM = (float)rpm;
        if (speed >= 0) tach->targetSpeed = (float)speed;
        if (temp >= -40) tach->targetTemp = (float)temp;
        pthread_mutex_unlock(&tach->dataMutex);

        usleep(100000);  // Read every 100ms
    }

    return NULL;
}

void DrawTachometerGauge(Vector2 center, float radius) {
    // Draw outer circle
    DrawCircleV(center, radius + 10, BLACK);
    DrawCircleV(center, radius + 5, DARKGRAY);
    DrawCircleV(center, radius, (Color){20, 20, 30, 255});

    // Draw RPM markings and labels
    float startAngle = 135.0f;
    float endAngle = 405.0f;
    int rpmSteps[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int numSteps = 9;

    for (int i = 0; i < numSteps; i++) {
        float angle = startAngle + (endAngle - startAngle) * i / (numSteps - 1);
        float angleRad = angle * DEG2RAD;

        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 30),
            center.y + sinf(angleRad) * (radius - 30)
        };

        Color tickColor = WHITE;
        if (rpmSteps[i] >= 7) tickColor = RED;
        else if (rpmSteps[i] >= 6) tickColor = YELLOW;

        DrawLineEx(outerPoint, innerPoint, 3.0f, tickColor);

        Vector2 labelPos = {
            center.x + cosf(angleRad) * (radius - 60),
            center.y + sinf(angleRad) * (radius - 60)
        };

        char label[8];
        sprintf(label, "%d", rpmSteps[i]);
        int labelWidth = MeasureText(label, 20);
        DrawText(label, labelPos.x - labelWidth / 2, labelPos.y - 10, 20, tickColor);
    }

    // Draw minor tick marks
    for (int i = 0; i < (numSteps - 1) * 5; i++) {
        if (i % 5 == 0) continue;

        float angle = startAngle + (endAngle - startAngle) * i / ((numSteps - 1) * 5.0f);
        float angleRad = angle * DEG2RAD;

        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 20),
            center.y + sinf(angleRad) * (radius - 20)
        };

        DrawLineEx(outerPoint, innerPoint, 1.5f, GRAY);
    }

    // Draw color zones
    for (int angle = startAngle; angle < endAngle; angle++) {
        float angleRad = angle * DEG2RAD;
        float rpm = (angle - startAngle) / (endAngle - startAngle) * MAX_RPM;

        Color zoneColor;
        if (rpm >= REDLINE_RPM) zoneColor = (Color){255, 0, 0, 40};
        else if (rpm >= 6000) zoneColor = (Color){255, 255, 0, 40};
        else zoneColor = (Color){0, 255, 0, 20};

        Vector2 p1 = {
            center.x + cosf(angleRad) * (radius - 35),
            center.y + sinf(angleRad) * (radius - 35)
        };
        Vector2 p2 = {
            center.x + cosf(angleRad) * (radius - 5),
            center.y + sinf(angleRad) * (radius - 5)
        };

        DrawLineEx(p1, p2, 2.0f, zoneColor);
    }

    DrawText("RPM x1000", center.x - 50, center.y - 90, 15, LIGHTGRAY);
}

void DrawNeedle(Vector2 center, float rpm, float maxRPM) {
    float startAngle = 135.0f;
    float endAngle = 405.0f;
    float currentAngle = startAngle + (endAngle - startAngle) * (rpm / maxRPM);
    float angleRad = currentAngle * DEG2RAD;

    Vector2 needleTip = {
        center.x + cosf(angleRad) * (GAUGE_RADIUS - 40),
        center.y + sinf(angleRad) * (GAUGE_RADIUS - 40)
    };

    float baseAngle1 = (currentAngle - 90) * DEG2RAD;
    float baseAngle2 = (currentAngle + 90) * DEG2RAD;

    Vector2 basePoint1 = {
        center.x + cosf(baseAngle1) * 8,
        center.y + sinf(baseAngle1) * 8
    };

    Vector2 basePoint2 = {
        center.x + cosf(baseAngle2) * 8,
        center.y + sinf(baseAngle2) * 8
    };

    DrawTriangle(
        (Vector2){needleTip.x + 2, needleTip.y + 2},
        (Vector2){basePoint1.x + 2, basePoint1.y + 2},
        (Vector2){basePoint2.x + 2, basePoint2.y + 2},
        (Color){0, 0, 0, 100}
    );

    Color needleColor = RED;
    if (rpm < REDLINE_RPM) needleColor = ORANGE;

    DrawTriangle(needleTip, basePoint1, basePoint2, needleColor);
    DrawTriangle(needleTip, basePoint2, basePoint1, needleColor);

    DrawCircleV(center, 12, BLACK);
    DrawCircleV(center, 10, DARKGRAY);
    DrawCircleV(center, 6, needleColor);
}

void DrawDigitalReadout(Vector2 center, float rpm) {
    char rpmText[16];
    sprintf(rpmText, "%04d", (int)rpm);

    Rectangle box = {center.x - 60, center.y + 20, 120, 50};
    DrawRectangleRounded(box, 0.2f, 8, BLACK);
    DrawRectangleRoundedLines(box, 0.2f, 8, DARKGRAY);

    int textWidth = MeasureText(rpmText, 35);
    DrawText(rpmText, center.x - textWidth / 2, center.y + 30, 35, LIME);
}

void DrawSpeedometer(Vector2 center, float radius, float speed) {
    // Draw outer circle
    DrawCircleV(center, radius + 10, BLACK);
    DrawCircleV(center, radius + 5, DARKGRAY);
    DrawCircleV(center, radius, (Color){20, 20, 30, 255});

    // Draw speed markings
    float startAngle = 135.0f;
    float endAngle = 405.0f;
    int speedSteps[] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200};
    int numSteps = 11;

    for (int i = 0; i < numSteps; i++) {
        float angle = startAngle + (endAngle - startAngle) * i / (numSteps - 1);
        float angleRad = angle * DEG2RAD;

        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 25),
            center.y + sinf(angleRad) * (radius - 25)
        };

        DrawLineEx(outerPoint, innerPoint, 2.5f, WHITE);

        if (i % 2 == 0) {  // Label every other mark
            Vector2 labelPos = {
                center.x + cosf(angleRad) * (radius - 50),
                center.y + sinf(angleRad) * (radius - 50)
            };

            char label[8];
            sprintf(label, "%d", speedSteps[i]);
            int labelWidth = MeasureText(label, 16);
            DrawText(label, labelPos.x - labelWidth / 2, labelPos.y - 8, 16, WHITE);
        }
    }

    // Draw minor tick marks
    for (int i = 0; i < (numSteps - 1) * 2; i++) {
        if (i % 2 == 0) continue;

        float angle = startAngle + (endAngle - startAngle) * i / ((numSteps - 1) * 2.0f);
        float angleRad = angle * DEG2RAD;

        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 18),
            center.y + sinf(angleRad) * (radius - 18)
        };

        DrawLineEx(outerPoint, innerPoint, 1.2f, GRAY);
    }

    DrawText("km/h", center.x - 22, center.y - 70, 14, LIGHTGRAY);

    // Draw needle for speed
    float currentAngle = startAngle + (endAngle - startAngle) * (speed / MAX_SPEED);
    if (speed > MAX_SPEED) currentAngle = startAngle + (endAngle - startAngle);
    float angleRad = currentAngle * DEG2RAD;

    Vector2 needleTip = {
        center.x + cosf(angleRad) * (radius - 35),
        center.y + sinf(angleRad) * (radius - 35)
    };

    float baseAngle1 = (currentAngle - 90) * DEG2RAD;
    float baseAngle2 = (currentAngle + 90) * DEG2RAD;

    Vector2 basePoint1 = {
        center.x + cosf(baseAngle1) * 6,
        center.y + sinf(baseAngle1) * 6
    };

    Vector2 basePoint2 = {
        center.x + cosf(baseAngle2) * 6,
        center.y + sinf(baseAngle2) * 6
    };

    DrawTriangle(needleTip, basePoint1, basePoint2, SKYBLUE);
    DrawTriangle(needleTip, basePoint2, basePoint1, SKYBLUE);

    DrawCircleV(center, 10, BLACK);
    DrawCircleV(center, 8, DARKGRAY);
    DrawCircleV(center, 5, SKYBLUE);

    // Digital speed readout
    char speedText[16];
    sprintf(speedText, "%03d", (int)speed);
    Rectangle box = {center.x - 40, center.y + 15, 80, 35};
    DrawRectangleRounded(box, 0.2f, 8, BLACK);
    DrawRectangleRoundedLines(box, 0.2f, 8, DARKGRAY);
    int textWidth = MeasureText(speedText, 25);
    DrawText(speedText, center.x - textWidth / 2, center.y + 20, 25, SKYBLUE);
}

void DrawTemperatureGauge(Vector2 center, float radius, float temp) {
    // Draw outer circle
    DrawCircleV(center, radius + 10, BLACK);
    DrawCircleV(center, radius + 5, DARKGRAY);
    DrawCircleV(center, radius, (Color){20, 20, 30, 255});

    // Draw temperature arc (bottom half + some sides)
    float startAngle = 135.0f;
    float endAngle = 405.0f;
    int tempSteps[] = {0, 20, 40, 60, 80, 100, 120};
    int numSteps = 7;

    for (int i = 0; i < numSteps; i++) {
        float angle = startAngle + (endAngle - startAngle) * i / (numSteps - 1);
        float angleRad = angle * DEG2RAD;

        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 25),
            center.y + sinf(angleRad) * (radius - 25)
        };

        Color tickColor = WHITE;
        if (tempSteps[i] >= 100) tickColor = RED;
        else if (tempSteps[i] >= 90) tickColor = YELLOW;

        DrawLineEx(outerPoint, innerPoint, 2.5f, tickColor);

        Vector2 labelPos = {
            center.x + cosf(angleRad) * (radius - 45),
            center.y + sinf(angleRad) * (radius - 45)
        };

        char label[8];
        sprintf(label, "%d", tempSteps[i]);
        int labelWidth = MeasureText(label, 14);
        DrawText(label, labelPos.x - labelWidth / 2, labelPos.y - 7, 14, tickColor);
    }

    DrawText("COOLANT °C", center.x - 45, center.y - 60, 13, LIGHTGRAY);

    // Draw needle for temperature
    float clampedTemp = temp;
    if (clampedTemp < 0) clampedTemp = 0;
    if (clampedTemp > MAX_TEMP) clampedTemp = MAX_TEMP;

    float currentAngle = startAngle + (endAngle - startAngle) * (clampedTemp / MAX_TEMP);
    float angleRad = currentAngle * DEG2RAD;

    Vector2 needleTip = {
        center.x + cosf(angleRad) * (radius - 30),
        center.y + sinf(angleRad) * (radius - 30)
    };

    float baseAngle1 = (currentAngle - 90) * DEG2RAD;
    float baseAngle2 = (currentAngle + 90) * DEG2RAD;

    Vector2 basePoint1 = {
        center.x + cosf(baseAngle1) * 6,
        center.y + sinf(baseAngle1) * 6
    };

    Vector2 basePoint2 = {
        center.x + cosf(baseAngle2) * 6,
        center.y + sinf(baseAngle2) * 6
    };

    Color tempNeedleColor = LIME;
    if (temp >= 100) tempNeedleColor = RED;
    else if (temp >= 90) tempNeedleColor = YELLOW;

    DrawTriangle(needleTip, basePoint1, basePoint2, tempNeedleColor);
    DrawTriangle(needleTip, basePoint2, basePoint1, tempNeedleColor);

    DrawCircleV(center, 10, BLACK);
    DrawCircleV(center, 8, DARKGRAY);
    DrawCircleV(center, 5, tempNeedleColor);

    // Digital temperature readout
    char tempText[16];
    sprintf(tempText, "%d°C", (int)temp);
    Rectangle box = {center.x - 35, center.y + 15, 70, 35};
    DrawRectangleRounded(box, 0.2f, 8, BLACK);
    DrawRectangleRoundedLines(box, 0.2f, 8, DARKGRAY);
    int textWidth = MeasureText(tempText, 22);
    DrawText(tempText, center.x - textWidth / 2, center.y + 22, 22, tempNeedleColor);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Car Tachometer - OBD Mode");
    SetTargetFPS(60);

    Tachometer tach = {0};
    tach.currentRPM = 0.0f;
    tach.targetRPM = 0.0f;
    tach.currentSpeed = 0.0f;
    tach.targetSpeed = 0.0f;
    tach.currentTemp = 20.0f;
    tach.targetTemp = 20.0f;
    tach.mode = MODE_SIMULATION;
    tach.obdThreadRunning = false;
    pthread_mutex_init(&tach.dataMutex, NULL);

    // Gauge positions
    Vector2 tachCenter = {280, 280};
    Vector2 speedCenter = {680, 280};
    Vector2 tempCenter = {1000, 500};

    while (!WindowShouldClose()) {
        // Handle mode switching
        if (IsKeyPressed(KEY_O)) {
            if (tach.mode == MODE_SIMULATION) {
                // Try to connect to OBD
                // Change this path to your actual device
                if (OBD_Init(&tach.obd, "/dev/tty.OBD-II-Port")) {
                    tach.mode = MODE_OBD;
                    tach.obdThreadRunning = true;
                    pthread_create(&tach.obdThread, NULL, OBDReadThread, &tach);
                }
            } else {
                // Switch back to simulation
                tach.obdThreadRunning = false;
                pthread_join(tach.obdThread, NULL);
                OBD_Close(&tach.obd);
                tach.mode = MODE_SIMULATION;
            }
        }

        // Update values based on mode
        if (tach.mode == MODE_SIMULATION) {
            // RPM controls
            if (IsKeyDown(KEY_UP)) {
                tach.targetRPM += 50.0f;
                if (tach.targetRPM > MAX_RPM) tach.targetRPM = MAX_RPM;
                // Simulate temp increase with RPM
                tach.targetTemp = 50.0f + (tach.targetRPM / MAX_RPM) * 50.0f;
            }
            if (IsKeyDown(KEY_DOWN)) {
                tach.targetRPM -= 50.0f;
                if (tach.targetRPM < MIN_RPM) tach.targetRPM = MIN_RPM;
                tach.targetTemp = 50.0f + (tach.targetRPM / MAX_RPM) * 50.0f;
            }
            // Speed controls
            if (IsKeyDown(KEY_RIGHT)) {
                tach.targetSpeed += 2.0f;
                if (tach.targetSpeed > MAX_SPEED) tach.targetSpeed = MAX_SPEED;
            }
            if (IsKeyDown(KEY_LEFT)) {
                tach.targetSpeed -= 2.0f;
                if (tach.targetSpeed < 0) tach.targetSpeed = 0;
            }
        } else {
            pthread_mutex_lock(&tach.dataMutex);
            // Values are updated by OBD thread
            pthread_mutex_unlock(&tach.dataMutex);
        }

        // Smooth transitions
        float rpmDiff = tach.targetRPM - tach.currentRPM;
        tach.currentRPM += rpmDiff * 0.1f;

        float speedDiff = tach.targetSpeed - tach.currentSpeed;
        tach.currentSpeed += speedDiff * 0.1f;

        float tempDiff = tach.targetTemp - tach.currentTemp;
        tach.currentTemp += tempDiff * 0.05f;

        // Draw
        BeginDrawing();
        ClearBackground((Color){15, 15, 25, 255});

        // Draw all three gauges
        DrawTachometerGauge(tachCenter, GAUGE_RADIUS);
        DrawNeedle(tachCenter, tach.currentRPM, MAX_RPM);
        DrawDigitalReadout(tachCenter, tach.currentRPM);

        DrawSpeedometer(speedCenter, GAUGE_RADIUS, tach.currentSpeed);

        DrawTemperatureGauge(tempCenter, 120, tach.currentTemp);

        // Draw mode indicator
        const char* modeText = (tach.mode == MODE_OBD) ? "OBD MODE" : "SIMULATION";
        Color modeColor = (tach.mode == MODE_OBD) ? GREEN : YELLOW;
        DrawText(modeText, 20, 20, 20, modeColor);

        // Draw instructions
        if (tach.mode == MODE_SIMULATION) {
            DrawText("UP/DOWN: RPM | LEFT/RIGHT: Speed | O: Connect OBD", 20, 50, 18, WHITE);
        } else {
            DrawText("Reading from vehicle... | O: Disconnect", 20, 50, 18, WHITE);
        }

        // Redline warning
        if (tach.currentRPM >= REDLINE_RPM) {
            DrawText("REDLINE!", tachCenter.x - 70, tachCenter.y + 100, 30, RED);
        }

        EndDrawing();
    }

    // Cleanup
    if (tach.mode == MODE_OBD) {
        tach.obdThreadRunning = false;
        pthread_join(tach.obdThread, NULL);
        OBD_Close(&tach.obd);
    }
    pthread_mutex_destroy(&tach.dataMutex);

    CloseWindow();
    return 0;
}
