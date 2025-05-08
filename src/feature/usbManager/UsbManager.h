#pragma once

#include <string>

void usb_comm_init();
void usb_send_json(const std::string &json);
std::string usb_read_line();
void start_usb_comm_task();