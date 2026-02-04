
# Caps Lock SOME/IP Examples

Two examples demonstrating SOME/IP communication using Caps Lock LED.

---

## Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TWO EXAMPLES                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   EXAMPLE 1: REQUEST/RESPONSE                EXAMPLE 2: EVENT/NOTIFY        │
│   ───────────────────────────                ───────────────────────        │
│                                                                             │
│   Client CONTROLS Caps Lock                  Server MONITORS Caps Lock      │
│                                                                             │
│      CLIENT           SERVER                    SERVER          CLIENT      │
│         │                │                         │               │        │
│         │──"Turn ON"────►│                         │               │        │
│         │                │ [writes LED]            │ [watches LED] │        │
│         │◄───"OK"────────│                         │               │        │
│         │                │                         │               │        │
│         │──"Turn OFF"───►│                         │──"LED is ON"──┼──►     │
│         │                │ [writes LED]            │               │        │
│         │◄───"OK"────────│                         │──"LED is OFF"─┼──►     │
│                                                                             │
│   User sends commands                        User presses Caps Lock key     │
│   via menu (1=ON, 2=OFF)                     and client receives updates    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Example 1: Request/Response (Control)

### How it Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXAMPLE 1: REQUEST/RESPONSE FLOW                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   CLIENT                                                    SERVER          │
│      │                                                         │            │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 1: Server offers service                     │  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │                        OFFER                            │            │
│      │  ◄──────────────────────────────────────────────────────┤            │
│      │                                                         │            │
│      │                  ╔════════════════════╗                 │            │
│      │                  ║availability_handler║                 │            │
│      │                  ║  AVAILABLE = TRUE  ║                 │            │
│      │                  ╚════════════════════╝                 │            │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 2: User selects "1" (Turn ON)                │  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │                      REQUEST                            │            │
│      │                     cmd = 1                             │            │
│      │  ──────────────────────────────────────────────────────►│            │
│      │                                                         │            │
│      │                                    ╔════════════════════╗            │
│      │                                    ║  message_handler   ║            │
│      │                                    ║  (process request) ║            │
│      │                                    ╚════════════════════╝            │
│      │                                              │                       │
│      │                                              ▼                       │
│      │                                    ┌─────────────────┐               │
│      │                                    │ Write "1" to    │               │
│      │                                    │ capslock file   │               │
│      │                                    │ (LED turns ON)  │               │
│      │                                    └─────────────────┘               │
│      │                                              │                       │
│      │                      RESPONSE                ▼                       │
│      │                     success                                          │
│      │  ◄──────────────────────────────────────────────────────│            │
│      │                                                         │            │
│      │  ╔════════════════════╗                                 │            │
│      │  ║  message_handler   ║                                 │            │
│      │  ║ (receive response) ║                                 │            │
│      │  ╚════════════════════╝                                 │            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Server Code (Simple)

```cpp
// Register handler for requests
app_->register_message_handler(SERVICE_ID, INSTANCE_ID, METHOD_SET,
    [this](const std::shared_ptr<vsomeip::message>& request) {
        
        // Get command from client (1=ON, 2=OFF)
        uint8_t cmd = request->get_payload()->get_data()[0];
        
        // Write to capslock LED file
        std::ofstream file(CAPSLOCK_FILE_PATH);
        if (cmd == 1) {
            file << "1";  // Turn ON
        } else if (cmd == 2) {
            file << "0";  // Turn OFF
        }
        file.close();
        
        // Send response back
        auto response = vsomeip::runtime::get()->create_response(request);
        app_->send(response);
    });
```

### Client Code (Simple)

```cpp
// When user presses 1 or 2, send command to server
void send_command(uint8_t cmd) {
    auto request = vsomeip::runtime::get()->create_request();
    request->set_service(SERVICE_ID);
    request->set_instance(INSTANCE_ID);
    request->set_method(METHOD_SET);
    
    // Set payload (1=ON, 2=OFF)
    auto payload = vsomeip::runtime::get()->create_payload();
    std::vector<vsomeip::byte_t> data = {cmd};
    payload->set_data(data);
    request->set_payload(payload);
    
    app_->send(request);
}
```

### Run Example 1

```bash
# Clean up first
sudo rm -f /tmp/vsomeip*

# Terminal 1 (Server) - needs sudo to write LED
cd build
sudo VSOMEIP_CONFIGURATION=../example_01_control/server.json ./control_server

# Terminal 2 (Client)
cd build
VSOMEIP_CONFIGURATION=../example_01_control/client.json ./control_client
```

**Usage:**
- Press `1` → Caps Lock ON
- Press `2` → Caps Lock OFF
- Press `0` → Exit

---

## Example 2: Event/Notify (Monitor)

### How it Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    EXAMPLE 2: EVENT/NOTIFY FLOW                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   SERVER                                                    CLIENT          │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 1: Client subscribes to events               │  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │                      SUBSCRIBE                          │            │
│      │  ◄──────────────────────────────────────────────────────┤            │
│      │                                                         │            │
│      │  ╔════════════════════════╗                             │            │
│      │  ║ subscription_handler   ║                             │            │
│      │  ║ return TRUE (accept)   ║                             │            │
│      │  ╚════════════════════════╝                             │            │
│      │                                                         │            │
│      │                    SUBSCRIBE_ACK                        │            │
│      │  ──────────────────────────────────────────────────────►│            │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 2: Server monitors capslock file             │  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │  ┌────────────────────┐                                 │            │
│      │  │ Monitor Loop:      │                                 │            │
│      │  │ Read file every    │                                 │            │
│      │  │ 100ms              │                                 │            │
│      │  └────────────────────┘                                 │            │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 3: User presses Caps Lock key (state changes)│  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │  [Detects change!]                                      │            │
│      │         │                                               │            │
│      │         ▼                                               │            │
│      │  notify()                                               │            │
│      │         │            NOTIFICATION                       │            │
│      │         │            state = ON                         │            │
│      │         └──────────────────────────────────────────────►│            │
│      │                                                         │            │
│      │                                    ╔════════════════════╗            │
│      │                                    ║  message_handler   ║            │
│      │                                    ║ "CAPS LOCK IS ON"  ║            │
│      │                                    ╚════════════════════╝            │
│      │                                                         │            │
│      │  ┌───────────────────────────────────────────────────┐  │            │
│      │  │ STEP 4: User presses Caps Lock key again          │  │            │
│      │  └───────────────────────────────────────────────────┘  │            │
│      │                                                         │            │
│      │  notify()             NOTIFICATION                      │            │
│      │         └─────────────state = OFF──────────────────────►│            │
│      │                                                         │            │
│      │                                    ╔════════════════════╗            │
│      │                                    ║  message_handler   ║            │
│      │                                    ║ "CAPS LOCK IS OFF" ║            │
│      │                                    ╚════════════════════╝            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Server Code (Simple)

```cpp
// Monitor loop - runs in separate thread
void monitor_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Read current state from file
        std::ifstream file(CAPSLOCK_FILE_PATH);
        int val = 0;
        file >> val;
        bool current = (val > 0);
        
        // Check if state changed
        if (current != last_state_) {
            last_state_ = current;
            
            // Send notification to ALL subscribers
            auto payload = vsomeip::runtime::get()->create_payload();
            std::vector<vsomeip::byte_t> data = {current ? (uint8_t)1 : (uint8_t)0};
            payload->set_data(data);
            
            app_->notify(SERVICE_ID, INSTANCE_ID, EVENT_ID, payload);
        }
    }
}
```

### Client Code (Simple)

```cpp
// Subscribe when service becomes available
void subscribe() {
    std::set<vsomeip::eventgroup_t> groups;
    groups.insert(EVENTGROUP_ID);
    
    app_->request_event(SERVICE_ID, INSTANCE_ID, EVENT_ID, groups,
                        vsomeip::event_type_e::ET_FIELD);
    app_->subscribe(SERVICE_ID, INSTANCE_ID, EVENTGROUP_ID);
}

// Receive events
app_->register_message_handler(SERVICE_ID, INSTANCE_ID, EVENT_ID,
    [](const std::shared_ptr<vsomeip::message>& event) {
        bool state = (event->get_payload()->get_data()[0] == 1);
        std::cout << "CAPS LOCK IS NOW: " << (state ? "ON" : "OFF") << std::endl;
    });
```

### Run Example 2

```bash
# Clean up first
sudo rm -f /tmp/vsomeip*

# Terminal 1 (Server)
cd build
VSOMEIP_CONFIGURATION=../example_02_monitor/server.json ./monitor_server

# Terminal 2 (Client)
cd build
VSOMEIP_CONFIGURATION=../example_02_monitor/client.json ./monitor_client
```

**Usage:**
- Press **Caps Lock key** on your keyboard
- Watch client receive notifications!

---

## Callbacks Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CALLBACKS USED IN EXAMPLES                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   EXAMPLE 1 (Request/Response):                                             │
│   ─────────────────────────────                                             │
│                                                                             │
│   SERVER:                              CLIENT:                              │
│   • state_handler                      • state_handler                      │
│     → offer_service()                    → request_service()                │
│                                                                             │
│   • message_handler                    • availability_handler               │
│     → receive REQUEST                    → service found                    │
│     → process command                                                       │
│     → send RESPONSE                    • message_handler                    │
│                                          → receive RESPONSE                 │
│                                                                             │
│                                                                             │
│   EXAMPLE 2 (Event/Notify):                                                 │
│   ─────────────────────────                                                 │
│                                                                             │
│   SERVER:                              CLIENT:                              │
│   • state_handler                      • state_handler                      │
│     → offer_service()                    → request_service()                │
│     → offer_event()                                                         │
│                                        • availability_handler               │
│   • subscription_handler                 → subscribe()                      │
│     → accept/reject client                                                  │
│                                        • message_handler                    │
│   (notify() is ACTION,                   → receive EVENT                    │
│    not callback)                         → print status                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
capslock_someip/
├── CMakeLists.txt
├── common/
│   └── capslock_ids.hpp          # Shared IDs
├── example_01_control/           # Request/Response
│   ├── server.cpp
│   ├── client.cpp
│   ├── server.json
│   └── client.json
├── example_02_monitor/           # Event/Notify
│   ├── server.cpp
│   ├── client.cpp
│   ├── server.json
│   └── client.json
└── build.sh
```

---

## Build

```bash
chmod +x build.sh
./build.sh
```

---

## Troubleshooting

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         COMMON ISSUES                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  PROBLEM                              SOLUTION                              │
│  ─────────────────────────────────    ─────────────────────────────────     │
│                                                                             │
│  "Permission denied"                  sudo rm -f /tmp/vsomeip*              │
│  "other routing manager"              Then restart server first             │
│                                                                             │
│  "Cannot write to file"               Run server with sudo                  │
│  (Example 1 only)                                                           │
│                                                                             │
│  "Service UNAVAILABLE"                Start server BEFORE client            │
│                                                                             │
│  Capslock path not found              Check your path:                      │
│                                       ls /sys/class/leds/ | grep caps       │
│                                       Update capslock_ids.hpp               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick Comparison

```
┌─────────────────────────────────────────────────────────────────────────────┐
│              REQUEST/RESPONSE  vs  EVENT/NOTIFY                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   REQUEST/RESPONSE                    EVENT/NOTIFY                          │
│   ────────────────                    ────────────                          │
│                                                                             │
│   Client initiates                    Server initiates                      │
│   Client sends REQUEST                Server sends NOTIFY                   │
│   Server sends RESPONSE               (no response needed)                  │
│                                                                             │
│   One client gets answer              ALL subscribers get data              │
│                                                                             │
│   Use for:                            Use for:                              │
│   • Commands                          • Status updates                      │
│   • Queries                           • Sensor data                         │
│   • "Do something"                    • "Something happened"                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```
