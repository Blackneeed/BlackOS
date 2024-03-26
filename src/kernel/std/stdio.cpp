#pragma once

#include "stdlib.cpp"
#include "stdport.cpp"
#include "stdcolor.cpp"
#include "stdcharInfo.hpp"
#include "../modules/kb/key.hpp"

extern uint32_t keyPressCount;
extern Key lastKeyInfo;
bool capsLockPressed = false, leftShiftPressed = false, rightShiftPressed = false, altPressed = false, controlPressed = false;
extern uint16_t lastPrint;

// Debugging

void E9_WriteChar(uint8_t character, const char* fg = ANSI_WHITE_FG, const char* bg = ANSI_BLACK_BG) {
    for (int i = 0; fg[i] != '\0'; i++) {
        outb(0xE9, fg[i]);
    }
    for (int i = 0; bg[i] != '\0'; i++) {
        outb(0xE9, bg[i]);
    }
    outb(0xE9, character);
    for (int i = 0; ANSI_RESET[i] != '\0'; i++) {
        outb(0xE9, ANSI_RESET[i]);
    }
}

void E9_WriteString(const char* string, const char* fg = ANSI_WHITE_FG, const char* bg = ANSI_BLACK_BG) {
    for (size_t i = 0; string[i] != '\0'; i++) {
        E9_WriteChar(string[i], fg, bg);
    }
}

// Settings

uint16_t cursorPos;
uint8_t termColor = BG_BLACK | FG_WHITE;
#define VGA_ADDRESS (uint8_t*)0xb8000
#define VGA_COLS 80
#define VGA_ROWS 25
#define CRT_CONTROLLER_PORT 0x3D4

// Cursor position stuff

uint16_t posFromCoords(uint8_t x, uint8_t y) {
    return y * VGA_COLS + x;
}

void setCursorPos(uint16_t newPos = cursorPos) { // Defaults to cursor pos becuase it is often used to refresh the cursor
    outb(CRT_CONTROLLER_PORT, 0x0F);
    outb(CRT_CONTROLLER_PORT + 1, (uint8_t)(newPos & 0xFF));
    outb(CRT_CONTROLLER_PORT, 0x0E);
    outb(CRT_CONTROLLER_PORT + 1, (uint8_t)((newPos >> 8) & 0xFF));

    cursorPos = newPos;
}

// Screen scrolling

void scrollScreenUp(uint8_t color = termColor) {
    for (int y = 1; y < VGA_ROWS; y++) {
        for (int x = 0; x < VGA_COLS; x++) {
            uint16_t srcPos = posFromCoords(x, y);
            uint16_t destPos = posFromCoords(x, y - 1);
            *(VGA_ADDRESS + destPos * 2) = *(VGA_ADDRESS + srcPos * 2);
            *(VGA_ADDRESS + destPos * 2 + 1) = *(VGA_ADDRESS + srcPos * 2 + 1);
        }
    }

    uint16_t lastRowPos = posFromCoords(0, VGA_ROWS - 1);
    for (int x = 0; x < VGA_COLS; x++) {
        *(VGA_ADDRESS + lastRowPos * 2) = ' ';
        *(VGA_ADDRESS + lastRowPos * 2 + 1) = color;
        ++lastRowPos;
    }
}

void scrollScreenDown(uint8_t color = termColor) {
    for (int y = VGA_ROWS - 2; y >= 0; y--) {
        for (int x = 0; x < VGA_COLS; x++) {
            uint16_t srcPos = posFromCoords(x, y);
            uint16_t destPos = posFromCoords(x, y + 1);
            *(VGA_ADDRESS + destPos * 2) = *(VGA_ADDRESS + srcPos * 2);
            *(VGA_ADDRESS + destPos * 2 + 1) = *(VGA_ADDRESS + srcPos * 2 + 1);
        }
    }

    uint16_t firstRowPos = posFromCoords(0, 0);
    for (int x = 0; x < VGA_COLS; x++) {
        *(VGA_ADDRESS + firstRowPos * 2) = ' ';
        *(VGA_ADDRESS + firstRowPos * 2 + 1) = color;
        ++firstRowPos;
    }
}

// Printing

void printChar(char character, uint8_t color = termColor)
{
    switch (character) {
        case '\n':
            cursorPos += VGA_COLS;
            if (cursorPos >= VGA_COLS * VGA_ROWS) {
                scrollScreenUp();
                cursorPos -= VGA_COLS;
            }
        case '\r':
            cursorPos -= cursorPos % VGA_COLS;
            break;
        case '\t':
            for (int i = 0; i < TAB_WIDTH; i++) {
                if (cursorPos >= VGA_COLS * VGA_ROWS) {
                    scrollScreenUp(color);
                    cursorPos -= VGA_COLS;
                }
                *(VGA_ADDRESS + cursorPos * 2) = ' ';
                *(VGA_ADDRESS + cursorPos * 2 + 1) = color;
                cursorPos++;
            }
            break;
        default:
            if (cursorPos >= VGA_COLS * VGA_ROWS) {
                scrollScreenUp();
                cursorPos -= VGA_COLS;
            }
            *(VGA_ADDRESS + cursorPos * 2) = character;
            *(VGA_ADDRESS + cursorPos * 2 + 1) = color;
            cursorPos++;
            break;
    }
    setCursorPos();
}

void printString(const char* string, uint8_t color = termColor)
{
    for (size_t i = 0; string[i] != '\0'; i++) {
        printChar(string[i], color);
    }
}

void printLn(const char* string, uint8_t color = termColor) {
    printString(string, color);
    printString("\r\n", color);
}

void printLn(uint8_t color = termColor) {
    printString("\r\n", color);
}

Key getKey() {
    int lastKeyPressCount = keyPressCount;
    while (keyPressCount == lastKeyPressCount) {
        // Hang
    }
    // Now a key has been pressed
    return lastKeyInfo;
}

void deleteChar(uint8_t color = termColor) {
    if (cursorPos > 0) {
        cursorPos--;
        printChar(' ', color);
        cursorPos--;
        setCursorPos();
    }
}

int readLine(const char* message, char buffer[], size_t bufferSize, uint8_t color = termColor) {
    printString(message, color);
    lastPrint = cursorPos;
    int length = 0;
    while (true) {
        Key key = getKey();
        if (key.isCharacter && key.keyCode == character && length < bufferSize - 1) {
            CharInfo chrInfo = processCharacter(key.charScanCode);
            if (chrInfo.isEmpty) {
                continue;
            }
            printChar(chrInfo.chr, color);
            buffer[length++] = chrInfo.chr;
        } else if (key.keyCode == tab) {
            printChar('\t', color);
        } else if (key.keyCode == backspace && cursorPos > lastPrint) {
            deleteChar(color);
            buffer[length--] = '\0';
        } else if (key.keyCode == enter) {
            printLn();
            break;
        } else if (key.keyCode == lshiftpress || key.keyCode == lshiftrelease) {
            leftShiftPressed = !leftShiftPressed;
        } else if (key.keyCode == rshiftpress || key.keyCode == rshiftrelease) {
            rightShiftPressed = !rightShiftPressed;
        } else if (key.keyCode == caps) {
            capsLockPressed = !capsLockPressed;
        } else if (key.keyCode == controlpress || key.keyCode == controlrelease) {
            controlPressed = !controlPressed;
        } else if (key.keyCode == altpress || key.keyCode == altrelease) {
            altPressed = !altPressed;
        }
    }
    lastPrint = cursorPos;
    return length;
}

// Screen clearing

void clearScreen(uint64_t color = termColor)
{
    for (uint16_t pos = VGA_COLS * VGA_ROWS; pos > 0; pos--) {
        setCursorPos(pos);
        deleteChar(color);
    }

    setCursorPos(0);
}

void setColor(uint64_t color = termColor) {
    for (uint16_t pos = 0; pos < VGA_COLS * VGA_ROWS; pos++) {
        *(VGA_ADDRESS + pos * 2 + 1) = color;
    }
}