/**
 * @file keyboard.hpp
 *
 * キーボード制御プログラム．
 */

#pragma once

#include "message.hpp"
#include <deque>

void InitializeKeyboard(std::deque<Message> &msg_queue);
