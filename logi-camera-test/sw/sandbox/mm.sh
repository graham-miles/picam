#!/bin/sh
make clean
make 
sudo ./logi_loader logi_camera_test.bit
sudo ./test_logi_cam
