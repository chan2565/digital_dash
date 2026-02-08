#include "raylib/src/raylib.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    SetTargetFPS(20);
    InitWindow(800, 480, "Tachometer");

    const int rev_limit = 7200;
    const int num_bars = 26;
    const int green_cutoff = 17;
    const int yellow_cutoff = 23;
    const float rev_step = rev_limit / num_bars;
    char rpm[5];
    unsigned int raw_rpm = 0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_LEFT)) {
            if (raw_rpm >= 100) {
                raw_rpm -= 100;
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (raw_rpm < rev_limit) {
                raw_rpm += 100;
            }
        }

        sprintf(rpm, "%u", raw_rpm);
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText(rpm, 20, 220, 300, DARKGRAY);
            DrawText("R\nP\nM", 730, 244, 75, DARKGRAY);
            int bar_x_val = 15;
            for (float i = rev_step; i <= raw_rpm; i += rev_step) {
                if (i < (rev_step * green_cutoff)) {
                    DrawRectangle(bar_x_val, 20, 20, 210, GREEN);
                } else if (i >= (rev_step * green_cutoff) && i < (rev_step * yellow_cutoff)) {
                    DrawRectangle(bar_x_val, 20, 20, 210, YELLOW);
                } else if (i >= (rev_step * yellow_cutoff)) {
                    DrawRectangle(bar_x_val, 20, 20, 210, RED);
                } else {
                    DrawRectangle(bar_x_val, 20, 20, 210, BLUE);
                }
                bar_x_val += 30;
            }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}