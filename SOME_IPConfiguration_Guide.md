
# SOME/IP Configuration Guide

## JSON Configuration File

### What is it?

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    JSON CONFIGURATION PURPOSE                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   YOUR APPLICATION                                                          │
│        │                                                                    │
│        │  "I want to offer service 0x1234"                                  │
│        ▼                                                                    │
│   ┌─────────────┐                                                           │
│   │   vsomeip   │ ──── "But WHERE? Which IP? Which Port?"                   │
│   └──────┬──────┘                                                           │
│          │                                                                  │
│          ▼                                                                  │
│   ┌─────────────┐                                                           │
│   │  JSON FILE  │ ──── "Use IP 127.0.0.1, Port 30509, UDP"                  │
│   └─────────────┘                                                           │
│                                                                             │
│   JSON tells vsomeip HOW and WHERE to communicate                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### JSON Sections Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       JSON FILE STRUCTURE                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  {                                                                          │
│      "unicast"           → This device's IP address                         │
│                                                                             │
│      "logging"           → Debug output settings                            │
│                                                                             │
│      "applications"      → App name and ID (must match code!)               │
│                                                                             │
│      "services"          → [SERVER ONLY] Port to listen on                  │
│                                                                             │
│      "routing"           → Which app is routing manager                     │
│                                                                             │
│      "service-discovery" → Multicast settings for SD                        │
│  }                                                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Server vs Client JSON

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SERVER JSON vs CLIENT JSON                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│        SERVER JSON                         CLIENT JSON                      │
│        ───────────                         ───────────                      │
│                                                                             │
│   ✓ unicast                            ✓ unicast                            │
│   ✓ logging                            ✓ logging                            │
│   ✓ applications                       ✓ applications                       │
│   ✓ services      ◄── HAS THIS         ✗ services     ◄── NO THIS           │
│   ✓ routing       ◄── IS routing mgr   ✓ routing      ◄── POINTS TO server  │
│   ✓ service-discovery                  ✓ service-discovery                  │
│                                                                             │
│                                                                             │
│   KEY DIFFERENCE:                                                           │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │  Server has "services" section (defines WHERE to listen)            │   │
│   │  Client does NOT have "services" section                            │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Important: Name Must Match!

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    APPLICATION NAME MATCHING                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   CODE:                                                                     │
│   ──────                                                                    │
│   create_application("my_server")                                           │
│                           │                                                 │
│                           │  MUST MATCH!                                    │
│                           │                                                 │
│   JSON:                   ▼                                                 │
│   ──────                                                                    │
│   "applications": [{                                                        │
│       "name": "my_server",  ◄─────────────────────────────────────          │
│       "id": "0x1001"                                                        │
│   }]                                                                        │
│                                                                             │
│   If names don't match → vsomeip won't find config!                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### How to Load JSON

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    LOADING JSON CONFIGURATION                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   METHOD: Environment Variable                                              │
│   ────────────────────────────                                              │
│                                                                             │
│   VSOMEIP_CONFIGURATION=/path/to/config.json ./my_application               │
│                                                                             │
│                                                                             │
│   EXAMPLE:                                                                  │
│   ─────────                                                                 │
│                                                                             │
│   Terminal 1 (Server):                                                      │
│   VSOMEIP_CONFIGURATION=server.json ./server                                │
│                                                                             │
│   Terminal 2 (Client):                                                      │
│   VSOMEIP_CONFIGURATION=client.json ./client                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Routing Manager

### What is Routing Manager?

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ROUTING MANAGER CONCEPT                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The Routing Manager handles ALL communication on a device                 │
│                                                                             │
│                                                                             │
│        App 1              App 2              App 3                          │
│   (routing mgr)                                                             │
│          │                  │                  │                            │
│          │                  │                  │                            │
│          └──────────────────┼──────────────────┘                            │
│                             │                                               │
│                             ▼                                               │
│                   ┌───────────────────┐                                     │
│                   │  ROUTING MANAGER  │                                     │
│                   │    (App 1)        │                                     │
│                   └─────────┬─────────┘                                     │
│                             │                                               │
│                             ▼                                               │
│                         NETWORK                                             │
│                                                                             │
│                                                                             │
│   RULES:                                                                    │
│   ───────                                                                   │
│   • Only ONE routing manager per device                                     │
│   • Usually the SERVER is routing manager                                   │
│   • Routing manager must START FIRST                                        │
│   • Other apps connect TO routing manager                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Routing in JSON

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ROUTING CONFIGURATION                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   SERVER JSON (IS the routing manager):                                     │
│   ─────────────────────────────────────                                     │
│   {                                                                         │
│       "applications": [{                                                    │
│           "name": "my_server",                                              │
│           "id": "0x1001"                                                    │
│       }],                                                                   │
│       "routing": "my_server"    ◄─── Points to ITSELF                       │
│   }                                                                         │
│                                                                             │
│                                                                             │
│   CLIENT JSON (connects TO routing manager):                                │
│   ──────────────────────────────────────────                                │
│   {                                                                         │
│       "applications": [{                                                    │
│           "name": "my_client",                                              │
│           "id": "0x2001"                                                    │
│       }],                                                                   │
│       "routing": "my_server"    ◄─── Points to SERVER                       │
│   }                                                                         │
│                                                                             │
│                                                                             │
│   BOTH point to the SAME routing manager name!                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Startup Order

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    STARTUP ORDER                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   CORRECT ORDER:                                                            │
│   ───────────────                                                           │
│                                                                             │
│   Time ──────────────────────────────────────────────────────────►          │
│                                                                             │
│        │                    │                                               │
│        ▼                    ▼                                               │
│   ┌─────────┐          ┌─────────┐                                          │
│   │ SERVER  │          │ CLIENT  │                                          │
│   │ (start) │          │ (start) │                                          │
│   └─────────┘          └─────────┘                                          │
│        │                    │                                               │
│        │                    └───► Connects to routing manager ✓             │
│        │                                                                    │
│        └───► Becomes routing manager ✓                                      │
│                                                                             │
│                                                                             │
│   WRONG ORDER (Client starts first):                                        │
│   ──────────────────────────────────                                        │
│                                                                             │
│   Time ──────────────────────────────────────────────────────────►          │
│                                                                             │
│        │                              │                                     │
│        ▼                              ▼                                     │
│   ┌─────────┐                    ┌─────────┐                                │
│   │ CLIENT  │                    │ SERVER  │                                │
│   │ (start) │                    │ (start) │                                │
│   └─────────┘                    └─────────┘                                │
│        │                              │                                     │
│        └───► "Cannot connect!"        │                                     │
│              (retries...)             │                                     │
│              (retries...)             │                                     │
│              ─────────────────────────┘                                     │
│              Finally connects ✓                                             │
│                                                                             │
│   NOTE: Client will keep retrying until server starts                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Common Routing Error

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMMON ERROR                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ERROR MESSAGE:                                                            │
│   ───────────────                                                           │
│   "Could not open /tmp/vsomeip.lck: Permission denied"                      │
│   "other routing manager present"                                           │
│                                                                             │
│                                                                             │
│   CAUSE:                                                                    │
│   ───────                                                                   │
│   Old lock file exists from previous run                                    │
│                                                                             │
│                                                                             │
│   FIX:                                                                      │
│   ─────                                                                     │
│   sudo rm -f /tmp/vsomeip*                                                  │
│                                                                             │
│   Then restart server first, then client.                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick JSON Templates

### Minimal Server JSON

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MINIMAL SERVER JSON                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   {                                                                         │
│       "unicast": "127.0.0.1",                                               │
│       "applications": [{ "name": "server", "id": "0x1001" }],               │
│       "services": [{ "service": "0x1234", "instance": "0x0001",             │
│                      "unreliable": "30509" }],                              │
│       "routing": "server",                                                  │
│       "service-discovery": {                                                │
│           "enable": "true",                                                 │
│           "multicast": "224.224.224.245",                                   │
│           "port": "30490",                                                  │
│           "protocol": "udp"                                                 │
│       }                                                                     │
│   }                                                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Minimal Client JSON

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MINIMAL CLIENT JSON                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   {                                                                         │
│       "unicast": "127.0.0.1",                                               │
│       "applications": [{ "name": "client", "id": "0x2001" }],               │
│       "routing": "server",       ◄─── Points to server name                 │
│       "service-discovery": {                                                │
│           "enable": "true",                                                 │
│           "multicast": "224.224.224.245",                                   │
│           "port": "30490",                                                  │
│           "protocol": "udp"                                                 │
│       }                                                                     │
│   }                                                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    JSON & ROUTING SUMMARY                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   JSON FILE:                                                                │
│   ───────────                                                               │
│   • Tells vsomeip IP, ports, and settings                                   │
│   • App name in JSON MUST match code                                        │
│   • Server has "services" section, client doesn't                           │
│   • Load with: VSOMEIP_CONFIGURATION=file.json ./app                        │
│                                                                             │
│   ROUTING MANAGER:                                                          │
│   ─────────────────                                                         │
│   • One per device (usually the server)                                     │
│   • All apps point to same routing manager                                  │
│   • Start server FIRST, then clients                                        │
│   • If errors: sudo rm -f /tmp/vsomeip*                                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```