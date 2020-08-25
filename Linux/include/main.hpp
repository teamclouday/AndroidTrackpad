#pragma once

// include UI component
#include "myUI.hpp"
// include communication component
#include "myTransfer.hpp"
// include trackpad component
#include "myTrackpadHelper.hpp"

#include <thread>
#include <chrono>
#include <mutex>

void task_UI();
void task_Transfer();
void task_Trackpad();