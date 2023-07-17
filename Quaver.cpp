#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <vector>
#include <regex>

HWND handle; // variable to store the quaver window handle once we find it

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) { // callback for enumwindows, match window name to quaver
    char buffer[255];

    if (IsWindowVisible(hwnd)) {
        GetWindowTextA(hwnd, static_cast<LPSTR>(buffer), 254); // get window name

        if (std::regex_match(buffer, std::regex("Quaver v[0-9]\.[0-9]\.[0-9]"))) { // match window name to Quaver window name
            handle = hwnd;
        }
    }

    return TRUE;
}

int main()
{
    EnumWindows(EnumWindowsProc, 0); // call enumwindows with our search callback to find the quaver window

    if (!handle) {
        std::cout << "Make sure Quaver is opened before launching the autoplayer\n";
        std::exit(0);
    }

    auto sevenKey = 0;

    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\nMAKE SURE YOUR NOTE SPEED IS SET TO 31\nMAKE SURE YOU ARE USING THE DEFAULT SKIN WITH CIRCLES\n4 KEY BINDINGS ARE [ A S K L ]\n7 KEY BINDINGS ARE [ A S D SPACE J K L ]\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
    std::cout << "Type '1' for seven key, '0' for 4 key\n";
    std::cin >> sevenKey;
    std::cout << "Seven key has been set to " << sevenKey << "\n\nAutoplayer is ready, enjoy!";

    RECT rect;
    auto dc = GetDC(handle); // gets the device context

    const int pressColors[] = { 212, 242, 213, 243, 73 }; // colors for start of normal or long note (they have colors off by 1 point for some reason)
    const int releaseColors[] = { 141, 152, 47 }; // colors for long part of long notes
    const auto yFactorHit = 0.972; // screen height multiplier to get y that is inside the note reciever things

    const double xFactors4K[] = { 0.398, 0.465, 0.531, 0.598 }; // screen width multiplier for 4 keys to get the x for the 4 note recievers
    const double xFactors7K[] = { 0.371, 0.414, 0.457, 0.5, 0.542, 0.586, 0.629 }; // screen width multiplier for 7 keys to get the x for the 7 note recievers

    const unsigned short keys4K[] = {'A', 'S', 'K', 'L'}; // 4 key binds (change this if yours are different)
    const unsigned short keys7K[] = { 'A', 'S', 'D', VK_SPACE, 'J', 'K', 'L' }; // 7 key binds (change this if yours are different)

    auto yFactorRelease = sevenKey ? 0.922 : 0.88; // screen height multiplier to get the y that is just above the note recievers
    const double* xFactors = sevenKey ? xFactors7K : xFactors4K; // use 4 key or 7 key multipliers depending on which is selected
    const auto keyAmount = sevenKey ? 7 : 4; // numerical value of keys
    const unsigned short* keys = sevenKey ? keys7K : keys4K; // use 4 key or 7 key keybinds depending on which is selected

    bool pressedLanes[7] = { 0 }; // array that stores which lanes are currently pressed

    std::vector<INPUT> inputs; // input buffer

    while (true) {
        if (IsWindow(handle) && GetForegroundWindow() != handle) { // make sure quaver is tabbed in
            continue;
        }

        GetClientRect(handle, &rect); // gets coodinates for the window

        auto windowWidth = rect.right - rect.left, windowHeight = rect.bottom - rect.top; // calculate window size

        for (auto index = 0; index < keyAmount; index++) { // loop from 0 to key amount - 1 (for indexing the array)
            auto xCoord = windowWidth * xFactors[index]; // get the x coodinate for the note reciever based on the loop index (multipying total window width by the x multiplier)

            if (!pressedLanes[index]) { // if the lane is not pressed
                auto pressColor = GetPixel(dc, xCoord, windowHeight * yFactorHit); // get pixel color at note x coord, note y coord
                auto pressGValue = GetGValue(pressColor); // get the g value of the color specifically, since that is what we use to identify

                if (std::find(std::begin(pressColors), std::end(pressColors), pressGValue) != std::end(pressColors)) { // if the g value is found inside of the pressColors array
                    pressedLanes[index] = 1; // set the lane to pressed in the array

                    inputs.push_back(INPUT{ // add key press for the lane to our buffer
                        .type = INPUT_KEYBOARD,
                        .ki = {
                            .wScan = static_cast<USHORT>(MapVirtualKeyA(keys[index], 0)),
                            .dwFlags = KEYEVENTF_SCANCODE
                        }
                    });
                }
            }
            else {
                auto releaseColor = GetPixel(dc, xCoord, windowHeight * yFactorRelease);  // get pixel color at note x coord, just above note y coord
                auto releaseGValue = GetGValue(releaseColor); // get the g value of the color specifically, since that is what we use to identify

                if (std::find(std::begin(releaseColors), std::end(releaseColors), releaseGValue) == std::end(releaseColors)) { // if the g value is not found inside of the pressColors array
                    pressedLanes[index] = 0; // set the lane to not pressed in the array

                    inputs.push_back(INPUT{ // add key release for the lane to our buffer
                        .type = INPUT_KEYBOARD,
                        .ki = {
                            .wScan = static_cast<USHORT>(MapVirtualKeyA(keys[index], 0)),
                            .dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP
                        }
                    });
                }
            }
        }

        SendInput(inputs.size(), inputs.data(), sizeof(INPUT)); // send all inputs in the buffer
        inputs.clear(); // clear input buffer

        Sleep(1); // wait 1 ms to make sure the notes we pressed disappear to prevent double clicks
    }

    ReleaseDC(handle, dc); // release the device context

    return 0;
}
