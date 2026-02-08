#include "raylib/src/raylib.h"
#include "raylib/src/raymath.h"
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)
#define GAUGE_RADIUS 200
#define MAX_RPM 8000
#define MIN_RPM 0
#define REDLINE_RPM 7000

typedef struct {
    float currentRPM;
    float targetRPM;
} Tachometer;

void DrawTachometerGauge(Vector2 center, float radius) {
    // Draw outer circle
    DrawCircleV(center, radius + 10, BLACK);
    DrawCircleV(center, radius + 5, DARKGRAY);
    DrawCircleV(center, radius, (Color){20, 20, 30, 255});

    // Draw RPM markings and labels
    float startAngle = 135.0f;  // Bottom left
    float endAngle = 405.0f;    // Bottom right (full sweep 270 degrees)
    int rpmSteps[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int numSteps = 9;

    for (int i = 0; i < numSteps; i++) {
        float angle = startAngle + (endAngle - startAngle) * i / (numSteps - 1);
        float angleRad = angle * DEG2RAD;

        // Major tick marks
        Vector2 outerPoint = {
            center.x + cosf(angleRad) * (radius - 10),
            center.y + sinf(angleRad) * (radius - 10)
        };
        Vector2 innerPoint = {
            center.x + cosf(angleRad) * (radius - 30),
            center.y + sinf(angleRad) * (radius - 30)
        };

        // Color code the tick marks
        Color tickColor = WHITE;
        if (rpmSteps[i] >= 7) tickColor = RED;
        else if (rpmSteps[i] >= 6) tickColor = YELLOW;

        DrawLineEx(outerPoint, innerPoint, 3.0f, tickColor);

        // Draw RPM labels
        Vector2 labelPos = {
            center.x + cosf(angleRad) * (radius - 60),
            center.y + sinf(angleRad) * (radius - 60)
        };

        char label[8];
        sprintf(label, "%d", rpmSteps[i]);
        int labelWidth = MeasureText(label, 20);
        DrawText(label, labelPos.x - labelWidth / 2, labelPos.y - 10, 20, tickColor);
    }

    // Draw minor tick marks (between major marks)
    for (int i = 0; i < (numSteps - 1) * 5; i++) {
        if (i % 5 == 0) continue;  // Skip major marks

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

    // Draw center text
    DrawText("RPM x1000", center.x - 50, center.y - 90, 15, LIGHTGRAY);
}

void DrawNeedle(Vector2 center, float rpm, float maxRPM) {
    float startAngle = 135.0f;
    float endAngle = 405.0f;
    float currentAngle = startAngle + (endAngle - startAngle) * (rpm / maxRPM);
    float angleRad = currentAngle * DEG2RAD;

    // Needle tip
    Vector2 needleTip = {
        center.x + cosf(angleRad) * (GAUGE_RADIUS - 40),
        center.y + sinf(angleRad) * (GAUGE_RADIUS - 40)
    };

    // Needle base points (triangular)
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

    // Draw needle shadow
    DrawTriangle(
        (Vector2){needleTip.x + 2, needleTip.y + 2},
        (Vector2){basePoint1.x + 2, basePoint1.y + 2},
        (Vector2){basePoint2.x + 2, basePoint2.y + 2},
        (Color){0, 0, 0, 100}
    );

    // Draw needle
    Color needleColor = RED;
    if (rpm < REDLINE_RPM) needleColor = ORANGE;

    DrawTriangle(needleTip, basePoint1, basePoint2, needleColor);
    DrawTriangle(needleTip, basePoint2, basePoint1, needleColor);

    // Center cap
    DrawCircleV(center, 12, BLACK);
    DrawCircleV(center, 10, DARKGRAY);
    DrawCircleV(center, 6, needleColor);
}

void DrawDigitalReadout(Vector2 center, float rpm) {
    char rpmText[16];
    sprintf(rpmText, "%04d", (int)rpm);

    // Background
    Rectangle box = {center.x - 60, center.y + 20, 120, 50};
    DrawRectangleRounded(box, 0.2f, 8, BLACK);
    DrawRectangleRoundedLines(box, 0.2f, 8, DARKGRAY);

    // RPM value
    int textWidth = MeasureText(rpmText, 35);
    DrawText(rpmText, center.x - textWidth / 2, center.y + 30, 35, LIME);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Car Tachometer");
    SetTargetFPS(60);

    Tachometer tach = {0};
    tach.currentRPM = 0.0f;
    tach.targetRPM = 1000.0f;

    Vector2 gaugeCenter = {CENTER_X, CENTER_Y};

    while (!WindowShouldClose()) {
        // Update
        if (IsKeyDown(KEY_UP)) {
            tach.targetRPM += 50.0f;
            if (tach.targetRPM > MAX_RPM) tach.targetRPM = MAX_RPM;
        }
        if (IsKeyDown(KEY_DOWN)) {
            tach.targetRPM -= 50.0f;
            if (tach.targetRPM < MIN_RPM) tach.targetRPM = MIN_RPM;
        }

        // Smooth RPM transition
        float diff = tach.targetRPM - tach.currentRPM;
        tach.currentRPM += diff * 0.1f;

        // Draw
        BeginDrawing();
        ClearBackground((Color){15, 15, 25, 255});

        // Draw tachometer
        DrawTachometerGauge(gaugeCenter, GAUGE_RADIUS);
        DrawNeedle(gaugeCenter, tach.currentRPM, MAX_RPM);
        DrawDigitalReadout(gaugeCenter, tach.currentRPM);

        // Draw instructions
        DrawText("UP/DOWN ARROWS: Control RPM", 20, 20, 20, WHITE);

        // Redline warning
        if (tach.currentRPM >= REDLINE_RPM) {
            DrawText("REDLINE!", CENTER_X - 70, CENTER_Y + 100, 30, RED);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
