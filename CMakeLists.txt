cmake_minimum_required(VERSION 2.8)
project(sendcom)
add_executable(sendcom sendcom.c)
target_link_libraries(sendcom libusb-1.0.so)
