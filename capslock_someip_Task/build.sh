#!/bin/bash

mkdir -p build
cd build
cmake ..
make -j$(nproc)

echo ""
echo "Build complete!"
echo ""
echo "Example 01 (Control):"
echo "  Terminal 1: sudo VSOMEIP_CONFIGURATION=../example_01_control/server.json ./control_server"
echo "  Terminal 2: VSOMEIP_CONFIGURATION=../example_01_control/client.json ./control_client"
echo ""
echo "Example 02 (Monitor):"
echo "  Terminal 1: VSOMEIP_CONFIGURATION=../example_02_monitor/server.json ./monitor_server"
echo "  Terminal 2: VSOMEIP_CONFIGURATION=../example_02_monitor/client.json ./monitor_client"