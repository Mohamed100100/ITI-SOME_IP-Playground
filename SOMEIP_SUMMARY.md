
# SOME/IP (vsomeip) Complete Guide

## Table of Contents
1. [Overview](#overview)
2. [Service Discovery](#service-discovery)
3. [Request/Response Pattern](#requestresponse-pattern)
4. [Event/Notify Pattern](#eventnotify-pattern)
5. [All Callbacks Summary](#all-callbacks-summary)
6. [Actions Summary](#actions-summary)
7. [Complete Flow Diagrams](#complete-flow-diagrams)

---

## Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          SOME/IP ARCHITECTURE                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                              NETWORK                                        │
│                                 │                                           │
│            ┌────────────────────┼────────────────────┐                      │
│            │                    │                    │                      │
│            ▼                    ▼                    ▼                      │
│      ┌──────────┐        ┌──────────┐        ┌──────────┐                   │
│      │  SERVER  │        │ CLIENT 1 │        │ CLIENT 2 │                   │
│      │ (ECU 1)  │        │ (ECU 2)  │        │ (ECU 3)  │                   │
│      └──────────┘        └──────────┘        └──────────┘                   │
│            │                    │                    │                      │
│            │                    │                    │                      │
│      ┌─────┴─────┐        ┌─────┴─────┐        ┌─────┴─────┐                │
│      │  OFFERS   │        │  FINDS    │        │  FINDS    │                │
│      │ SERVICES  │        │ SERVICES  │        │ SERVICES  │                │
│      └───────────┘        └───────────┘        └───────────┘                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Communication Patterns

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      THREE MAIN PATTERNS                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. REQUEST/RESPONSE          2. FIRE & FORGET         3. EVENT/NOTIFY      │
│     (RPC style)                  (One-way)               (Pub/Sub)          │
│                                                                             │
│     CLIENT     SERVER         CLIENT     SERVER        SERVER    CLIENTS    │
│        │          │              │          │             │      │  │  │    │
│        │─REQUEST─►│              │─REQUEST─►│             │      │  │  │    │
│        │          │              │          │             │──────┼──┼──┼──► │
│        │◄RESPONSE─│              │    (no response)       │NOTIFY│  │  │    │
│        │          │              │          │             │      │  │  │    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Service Discovery

### What is Service Discovery?

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    WHY SERVICE DISCOVERY?                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  PROBLEM: Client doesn't know where server is                               │
│                                                                             │
│     CLIENT                                    SERVER                        │
│        │                                         │                          │
│        │   "I need service 0x1234"               │                          │
│        │   "But where is it?"                    │  IP: ???                 │
│        │   "What IP? What port?"                 │  Port: ???               │
│        │                                         │                          │
│                                                                             │
│  SOLUTION: Service Discovery (SD)                                           │
│                                                                             │
│     CLIENT                                    SERVER                        │
│        │                                         │                          │
│        │         MULTICAST NETWORK               │                          │
│        │    ◄───────────────────────────────     │                          │
│        │    "Service 0x1234 available at         │                          │
│        │     IP 192.168.1.10, Port 30509"        │                          │
│        │                                         │                          │
│        │   Now client knows where to connect!    │                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### OFFER and FIND

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         OFFER vs FIND                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                                                                             │
│   SERVER                          MULTICAST                      CLIENT     │
│      │                            NETWORK                           │       │
│      │                               │                              │       │
│      │                               │                              │       │
│      │   ┌─────────────────┐         │                              │       │
│      │   │ offer_service() │         │                              │       │
│      │   └────────┬────────┘         │                              │       │
│      │            │                  │                              │       │
│      │            │   ┌──────────────┴───────────────┐              │       │
│      │            └──►│     SD: OFFER MESSAGE        │──────────────┼──►    │
│      │                │  "Service 0x1234 available   │              │       │
│      │                │   at 192.168.1.10:30509"     │              │       │
│      │                └──────────────────────────────┘              │       │
│      │                                                              │       │
│      │                                                              │       │
│      │                                               ┌──────────────┴──┐    │
│      │                                               │request_service()│    │
│      │                                               └──────────────┬──┘    │
│      │                                                              │       │
│      │                ┌──────────────────────────────┐              │       │
│      │   ◄────────────│     SD: FIND MESSAGE         │◄─────────────┘       │
│      │                │  "Looking for service 0x1234"│                      │
│      │                └──────────────────────────────┘                      │
│      │                                                                      │
│      │                                                                      │
│      │                ┌──────────────────────────────┐                      │
│      │   ────────────►│     SD: OFFER MESSAGE        │─────────────────►    │
│      │                │  (Response to FIND)          │                      │
│      │                └──────────────────────────────┘                      │
│      │                                                                      │
│      │                                                    ╔════════════════╗│
│      │                                                    ║ availability_  ║│
│      │                                                    ║ handler()      ║│
│      │                                                    ║ AVAILABLE=TRUE ║│
│      │                                                    ╚════════════════╝│
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### OFFER Timing

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         OFFER TIMING                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Time ──────────────────────────────────────────────────────────────────►   │
│                                                                             │
│        │         │         │         │                   │                  │
│     OFFER     OFFER     OFFER     OFFER               OFFER                 │
│        │         │         │         │                   │                  │
│   ─────┼─────────┼─────────┼─────────┼───────────────────┼─────────────►    │
│        │         │         │         │                   │                  │
│        │◄───────►│◄───────►│◄───────►│◄─────────────────►│                  │
│        │  200ms  │  200ms  │  200ms  │    2000ms         │                  │
│        │         │         │         │  (cyclic delay)   │                  │
│        │         │         │         │                   │                  │
│        └─────────┴─────────┴─────────┘                   │                  │
│           INITIAL PHASE                              MAIN PHASE             │
│         (repetitions = 3)                        (periodic offers)          │
│                                                                             │
│                                                                             │
│  JSON Configuration:                                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │  "initial_delay_min": "10"      → Random delay before first offer   │    │
│  │  "initial_delay_max": "100"                                         │    │
│  │  "repetitions_base_delay": "200" → Delay between initial offers     │    │
│  │  "repetitions_max": "3"          → Number of initial offers         │    │
│  │  "cyclic_offer_delay": "2000"    → Delay between periodic offers    │    │
│  │  "ttl": "3"                      → Time-to-live (in cycles)         │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Request/Response Pattern

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST/RESPONSE COMPLETE FLOW                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   SERVER                                                      CLIENT        │
│      │                                                           │          │
│      │                                                           │          │
│      │  ═══════════════ PHASE 1: STARTUP ═══════════════════     │          │
│      │                                                           │          │
│      │  app_->init()                                 app_->init()│          │
│      │  app_->start()                               app_->start()│          │
│      │      │                                                │   │          │
│      │      ▼                                                ▼   │          │
│      │  ╔═══════════════════════╗            ╔═══════════════════════╗      │
│      │  ║    state_handler()    ║            ║    state_handler()    ║      │
│      │  ║    ST_REGISTERED      ║            ║    ST_REGISTERED      ║      │
│      │  ╚═══════════════════════╝            ╚═══════════════════════╝      │
│      │      │                                                │              │
│      │      │                                                │              │
│      │  ═══════════════ PHASE 2: DISCOVERY ═════════════════                │
│      │      │                                                │              │
│      │      ▼                                                ▼              │
│      │  offer_service()                          request_service()          │
│      │      │                                                │              │
│      │      │                                                │              │
│      │      │            ┌─────────────┐                     │              │
│      │      └────────────│   OFFER     │─────────────────────┼────►         │
│      │                   │  (via SD)   │                     │              │
│      │                   └─────────────┘                     │              │
│      │                                                       ▼              │
│      │                                       ╔═══════════════════════╗      │
│      │                                       ║ availability_handler()║      │
│      │                                       ║   is_available=TRUE   ║      │
│      │                                       ╚═══════════════════════╝      │
│      │                                                       │              │
│      │                                                       │              │
│      │  ═══════════════ PHASE 3: COMMUNICATION ═════════════                │
│      │                                                       │              │
│      │                                                       ▼              │
│      │                                               send(request)          │
│      │                   ┌─────────────┐                     │              │
│      │      ◄────────────│   REQUEST   │◄────────────────────┘              │
│      │                   │  Method=X   │                                    │
│      │                   │  Payload=Y  │                                    │
│      │                   └─────────────┘                                    │
│      │      │                                                               │
│      │      ▼                                                               │
│      │  ╔═══════════════════════╗                                           │
│      │  ║   message_handler()   ║                                           │
│      │  ║  (process request)    ║                                           │
│      │  ╚═══════════════════════╝                                           │
│      │      │                                                               │
│      │      │ Process data...                                               │
│      │      │                                                               │
│      │      ▼                                                               │
│      │  send(response)                                                      │
│      │      │                   ┌─────────────┐                             │
│      │      └───────────────────│  RESPONSE   │────────────────────►        │
│      │                          │  Result=Z   │                     │       │
│      │                          └─────────────┘                     │       │
│      │                                                              ▼       │
│      │                                          ╔═══════════════════════╗   │
│      │                                          ║   message_handler()   ║   │
│      │                                          ║  (process response)   ║   │
│      │                                          ╚═══════════════════════╝   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Event/Notify Pattern

### Subscription Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       SUBSCRIPTION FLOW                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   SERVER                                                      CLIENT        │
│      │                                                           │          │
│      │  ═══════════════ PHASE 1: SETUP ═════════════════════     │          │
│      │                                                           │          │
│      │  ╔═══════════════════════╗            ╔═══════════════════════╗      │
│      │  ║    state_handler()    ║            ║    state_handler()    ║      │
│      │  ║    ST_REGISTERED      ║            ║    ST_REGISTERED      ║      │
│      │  ╚═══════════════════════╝            ╚═══════════════════════╝      │
│      │      │                                                │              │
│      │      ▼                                                ▼              │
│      │  offer_service()                          request_service()          │
│      │  offer_event()                                        │              │
│      │      │                                                │              │
│      │      │            ┌─────────────┐                     │              │
│      │      └────────────│    OFFER    │─────────────────────┼────►         │
│      │                   └─────────────┘                     │              │
│      │                                                       ▼              │
│      │                                       ╔═══════════════════════╗      │
│      │                                       ║ availability_handler()║      │
│      │                                       ║   is_available=TRUE   ║      │
│      │                                       ╚═══════════════════════╝      │
│      │                                                       │              │
│      │  ═══════════════ PHASE 2: SUBSCRIBE ═════════════════                │
│      │                                                       │              │
│      │                                                       ▼              │
│      │                                               request_event()        │
│      │                                               subscribe()            │
│      │                   ┌─────────────┐                     │              │
│      │      ◄────────────│  SUBSCRIBE  │◄────────────────────┘              │
│      │                   │ EventGroup  │                                    │
│      │                   └─────────────┘                                    │
│      │      │                                                               │
│      │      ▼                                                               │
│      │  ╔════════════════════════════╗                                      │
│      │  ║  subscription_handler()    ║                                      │
│      │  ║  return TRUE = accept      ║                                      │
│      │  ║  return FALSE = reject     ║                                      │
│      │  ╚════════════════════════════╝                                      │
│      │      │                                                               │
│      │      │ (if accepted)                                                 │
│      │      ▼                                                               │
│      │      │                   ┌───────────────┐                           │
│      │      └───────────────────│ SUBSCRIBE_ACK │──────────────────►        │
│      │                          └───────────────┘                   │       │
│      │                                                              ▼       │
│      │                                     ╔════════════════════════════╗   │
│      │                                     ║subscription_status_handler()║  │
│      │                                     ║      status = ACCEPTED      ║  │
│      │                                     ╚════════════════════════════╝   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Notification Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       NOTIFICATION FLOW                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   SERVER                                         CLIENT 1    CLIENT 2       │
│      │                                              │           │           │
│      │  (After subscription is established)         │           │           │
│      │                                              │           │           │
│      │  ═══════════════ PHASE 3: NOTIFY ═══════════════════════             │
│      │                                              │           │           │
│      │  [Something happens - data changes]          │           │           │
│      │      │                                       │           │           │
│      │      ▼                                       │           │           │
│      │  notify()                                    │           │           │
│      │      │                                       │           │           │
│      │      │         ┌──────────────┐              │           │           │
│      │      ├─────────│ NOTIFICATION │──────────────┼────►      │           │
│      │      │         │  Event Data  │              │           │           │
│      │      │         └──────────────┘              ▼           │           │
│      │      │                           ╔══════════════════╗    │           │
│      │      │                           ║ message_handler()║    │           │
│      │      │                           ║ (receive event)  ║    │           │
│      │      │                           ╚══════════════════╝    │           │
│      │      │                                       │           │           │
│      │      │         ┌──────────────┐              │           │           │
│      │      └─────────│ NOTIFICATION │──────────────┼───────────┼────►      │
│      │                │  Event Data  │              │           │           │
│      │                └──────────────┘              │           ▼           │
│      │                                              │ ╔══════════════════╗  │
│      │                                              │ ║ message_handler()║  │
│      │                                              │ ║ (receive event)  ║  │
│      │                                              │ ╚══════════════════╝  │
│      │                                              │           │           │
│      │                                              │           │           │
│      │  [Data changes again]                        │           │           │
│      │      │                                       │           │           │
│      │      ▼                                       │           │           │
│      │  notify()  ──────────────────────────────────┼────►      │           │
│      │            ──────────────────────────────────┼───────────┼────►      │
│      │                                              │           │           │
│      │                                                                      │
│      │  Note: notify() sends to ALL subscribers automatically               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Event Types

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         EVENT TYPES                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. ET_EVENT (Pure Event)                                                   │
│  ─────────────────────────                                                  │
│                                                                             │
│     • No initial value                                                      │
│     • Subscriber receives ONLY future notifications                         │
│                                                                             │
│     SERVER:  notify    notify    notify                                     │
│                │         │         │                                        │
│     ───────────┼─────────┼─────────┼──────────────►                         │
│                │         │         │                                        │
│                ▼         │         ▼                                        │
│     CLIENT: (missed)  subscribe  receives                                   │
│                                                                             │
│                                                                             │
│  2. ET_FIELD (Field Event)                                                  │
│  ─────────────────────────                                                  │
│                                                                             │
│     • HAS initial/current value                                             │
│     • New subscriber receives current value immediately                     │
│                                                                             │
│     SERVER:  notify    notify    notify                                     │
│              (val=25)  (val=26)  (val=27)                                   │
│                │         │         │                                        │
│     ───────────┼─────────┼─────────┼──────────────►                         │
│                │         │         │                                        │
│                ▼         │         ▼                                        │
│     CLIENT: (missed)  subscribe  receives(27)                               │
│                          │                                                  │
│                          ▼                                                  │
│                   immediately gets                                          │
│                   current value (26)                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Eventgroups

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         EVENTGROUPS                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Eventgroups bundle related events together                                 │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────┐      │
│  │                     SERVICE: Vehicle (0x1234)                     │      │
│  │                                                                   │     │
│  │   ┌─────────────────────────────────────────────────────────┐     │      │
│  │   │  EVENTGROUP: Temperature (0x0001)                       │     │      │
│  │   │                                                         │     │      │
│  │   │    • EVENT: Engine_Temp (0x8001)                        │     │      │
│  │   │    • EVENT: Cabin_Temp (0x8002)                         │     │      │
│  │   │    • EVENT: Outside_Temp (0x8003)                       │     │      │
│  │   │                                                         │     │      │
│  │   └─────────────────────────────────────────────────────────┘     │      │
│  │                                                                   │      │
│  │   ┌─────────────────────────────────────────────────────────┐     │      │
│  │   │  EVENTGROUP: Speed (0x0002)                             │     │      │
│  │   │                                                         │     │      │
│  │   │    • EVENT: Vehicle_Speed (0x8010)                      │     │      │
│  │   │    • EVENT: Wheel_Speed (0x8011)                        │     │      │
│  │   │                                                         │     │      │
│  │   └─────────────────────────────────────────────────────────┘     │      │
│  │                                                                   │      │
│  └───────────────────────────────────────────────────────────────────┘      │
│                                                                             │
│  Subscribe to EVENTGROUP, not individual events:                            │
│  • subscribe(SERVICE, INSTANCE, EVENTGROUP_TEMP) → gets ALL temp events     │
│  • subscribe(SERVICE, INSTANCE, EVENTGROUP_SPEED) → gets ALL speed events   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## All Callbacks Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ALL CALLBACKS                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      state_handler()                                │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  USED BY:    Server AND Client                                      │    │
│  │                                                                     │    │
│  │  WHEN CALLED:                                                       │    │
│  │    • ST_REGISTERED   → App connected to routing manager             │    │
│  │    • ST_DEREGISTERED → App disconnected from routing manager        │    │
│  │                                                                     │    │
│  │  WHAT TO DO:                                                        │    │
│  │    • Server: Call offer_service() after ST_REGISTERED               │    │
│  │    • Client: Call request_service() after ST_REGISTERED             │    │
│  │                                                                     │    │
│  │  WHY IMPORTANT:                                                     │    │
│  │    • MUST wait for ST_REGISTERED before offering/requesting         │    │
│  │    • If you don't wait, operations will fail silently               │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                   availability_handler()                            │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  USED BY:    Client ONLY                                            │    │
│  │                                                                     │    │
│  │  WHEN CALLED:                                                       │    │
│  │    • is_available=TRUE  → Service found (server sent OFFER)         │    │
│  │    • is_available=FALSE → Service lost (server stopped/crashed)     │    │
│  │                                                                     │    │
│  │  WHAT TO DO:                                                        │    │
│  │    • When TRUE: Safe to send requests or subscribe to events        │    │
│  │    • When FALSE: Stop sending, wait for service to return           │    │
│  │                                                                     │    │
│  │  WHY IMPORTANT:                                                     │    │
│  │    • Tells you when server is ready                                 │    │
│  │    • Sending to unavailable service will fail                       │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                     message_handler()                               │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  USED BY:    Server AND Client                                      │    │
│  │                                                                     │    │
│  │  WHEN CALLED:                                                       │    │
│  │    • Server: When client sends REQUEST                              │    │
│  │    • Client: When server sends RESPONSE                             │    │
│  │    • Client: When server sends EVENT NOTIFICATION                   │    │
│  │                                                                     │    │
│  │  WHAT TO DO:                                                        │    │
│  │    • Server: Process request, send response                         │    │
│  │    • Client: Process response or event data                         │    │
│  │                                                                     │    │
│  │  NOTE:                                                              │    │
│  │    • Same handler for responses AND events (differentiate by ID)    │    │
│  │    • Register separate handlers for different methods/events        │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                   subscription_handler()                            │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  USED BY:    Server ONLY                                            │    │
│  │                                                                     │    │
│  │  WHEN CALLED:                                                       │    │
│  │    • subscribed=TRUE  → Client wants to subscribe                   │    │
│  │    • subscribed=FALSE → Client wants to unsubscribe                 │    │
│  │                                                                     │    │
│  │  WHAT TO DO:                                                        │    │
│  │    • return TRUE  → Accept subscription (send SUBSCRIBE_ACK)        │    │
│  │    • return FALSE → Reject subscription (send SUBSCRIBE_NACK)       │    │
│  │                                                                     │    │
│  │  USE CASES FOR REJECTION:                                           │    │
│  │    • Too many subscribers                                           │    │
│  │    • Client not authorized                                          │    │
│  │    • Resource limitations                                           │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │               subscription_status_handler()                         │    │
│  ├─────────────────────────────────────────────────────────────────────┤    │
│  │  USED BY:    Client ONLY                                            │    │
│  │                                                                     │    │
│  │  WHEN CALLED:                                                       │    │
│  │    • status=0 (success) → Server accepted subscription (ACK)        │    │
│  │    • status≠0 (error)   → Server rejected subscription (NACK)       │    │
│  │                                                                     │    │
│  │  WHAT TO DO:                                                        │    │
│  │    • If accepted: Events will start arriving                        │    │
│  │    • If rejected: Handle error, maybe retry later                   │    │
│  │                                                                     │    │
│  │  NOTE:                                                              │    │
│  │    • Optional callback - subscription can work without it           │    │
│  │    • Useful for confirming subscription success                     │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Actions Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ALL ACTIONS (You Call These)                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                           SERVER ACTIONS                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                                                                     │    │
│  │  SETUP:                                                             │    │
│  │  ───────                                                            │    │
│  │  • init()           → Load configuration, prepare application       │    │
│  │  • start()          → Start the application (blocking)              │    │
│  │  • stop()           → Stop the application                          │    │
│  │                                                                     │    │
│  │  SERVICE DISCOVERY:                                                 │    │
│  │  ───────────────────                                                │    │
│  │  • offer_service()      → Announce service availability             │    │
│  │  • stop_offer_service() → Stop announcing service                   │    │
│  │                                                                     │    │
│  │  EVENTS:                                                            │    │
│  │  ───────                                                            │    │
│  │  • offer_event()        → Declare an event will be published        │    │
│  │  • stop_offer_event()   → Stop offering event                       │    │
│  │  • notify()             → Send event to ALL subscribers             │    │
│  │  • notify_one()         → Send event to ONE specific client         │    │
│  │                                                                     │    │
│  │  REQUEST/RESPONSE:                                                  │    │
│  │  ─────────────────                                                  │    │
│  │  • send(response)       → Send response back to client              │    │
│  │                                                                     │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│                           CLIENT ACTIONS                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                                                                     │    │
│  │  SETUP:                                                             │    │
│  │  ───────                                                            │    │
│  │  • init()           → Load configuration, prepare application       │    │
│  │  • start()          → Start the application (blocking)              │    │
│  │  • stop()           → Stop the application                          │    │
│  │                                                                     │    │
│  │  SERVICE DISCOVERY:                                                 │    │
│  │  ───────────────────                                                │    │
│  │  • request_service()    → Express interest in a service (FIND)      │    │
│  │  • release_service()    → No longer interested in service           │    │
│  │                                                                     │    │
│  │  EVENTS:                                                            │    │
│  │  ───────                                                            │    │
│  │  • request_event()      → Prepare to receive an event (local)       │    │
│  │  • release_event()      → No longer interested in event             │    │
│  │  • subscribe()          → Subscribe to eventgroup (network)         │    │
│  │  • unsubscribe()        → Unsubscribe from eventgroup               │    │
│  │                                                                     │    │
│  │  REQUEST/RESPONSE:                                                  │    │
│  │  ─────────────────                                                  │    │
│  │  • send(request)        → Send request to server                    │    │
│  │                                                                     │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Complete Flow Diagrams

### Server Lifecycle

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       SERVER LIFECYCLE                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          ┌─────────┐                                        │
│                          │  START  │                                        │
│                          └────┬────┘                                        │
│                               │                                             │
│                               ▼                                             │
│                       ┌───────────────┐                                     │
│                       │    init()     │                                     │
│                       └───────┬───────┘                                     │
│                               │                                             │
│                               ▼                                             │
│                ┌──────────────────────────────┐                             │
│                │  Register all handlers:       │                            │
│                │  • state_handler              │                            │
│                │  • message_handler            │                            │
│                │  • subscription_handler       │                            │
│                └──────────────┬───────────────┘                             │
│                               │                                             │
│                               ▼                                             │
│                       ┌───────────────┐                                     │
│                       │    start()    │◄──────────────────────┐             │
│                       └───────┬───────┘                       │             │
│                               │                               │             │
│                               ▼                               │             │
│                 ╔═════════════════════════════╗               │             │
│                 ║      state_handler()        ║               │             │
│                 ║      ST_REGISTERED          ║               │             │
│                 ╚═════════════╤═══════════════╝               │             │
│                               │                               │             │
│                               ▼                               │             │
│                    ┌─────────────────────┐                    │             │
│                    │   offer_service()   │                    │             │
│                    │   offer_event()     │                    │             │
│                    └──────────┬──────────┘                    │             │
│                               │                               │             │
│                               ▼                               │             │
│                    ┌─────────────────────┐                    │             │
│                    │  WAITING FOR:       │                    │             │
│                    │  • Requests         │────────┐           │             │
│                    │  • Subscriptions    │        │           │             │
│                    └─────────────────────┘        │           │             │
│                               │                   │           │             │
│           ┌───────────────────┼───────────────────┤           │             │
│           │                   │                   │           │             │
│           ▼                   ▼                   ▼           │             │
│   ╔═══════════════╗  ╔═══════════════════╗  ┌─────────┐       │             │
│   ║message_handler║  ║subscription_handler║ │ notify()│       │             │
│   ║  (request)    ║  ║  (subscribe)       ║ │(on data │       │             │
│   ╚═══════╤═══════╝  ╚═══════════════════╝  │ change) │       │             │
│           │                                 └────┬────┘       │             │
│           ▼                                      │            │             │
│   ┌───────────────┐                              │            │             │
│   │send(response) │                              │            │             │
│   └───────────────┘                              │            │             │
│           │                                      │            │             │
│           └───────────────────┬──────────────────┘            │             │
│                               │                               │             │
│                               └───────────────────────────────┘             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Client Lifecycle

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       CLIENT LIFECYCLE                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          ┌─────────┐                                        │
│                          │  START  │                                        │
│                          └────┬────┘                                        │
│                               │                                             │
│                               ▼                                             │
│                       ┌───────────────┐                                     │
│                       │    init()     │                                     │
│                       └───────┬───────┘                                     │
│                               │                                             │
│                               ▼                                             │
│                ┌──────────────────────────────┐                             │
│                │  Register all handlers:       │                            │
│                │  • state_handler              │                            │
│                │  • availability_handler       │                            │
│                │  • message_handler            │                            │
│                └──────────────┬───────────────┘                             │
│                               │                                             │
│                               ▼                                             │
│                       ┌───────────────┐                                     │
│                       │    start()    │                                     │
│                       └───────┬───────┘                                     │
│                               │                                             │
│                               ▼                                             │
│                 ╔═════════════════════════════╗                             │
│                 ║      state_handler()        ║                             │
│                 ║      ST_REGISTERED          ║                             │
│                 ╚═════════════╤═══════════════╝                             │
│                               │                                             │
│                               ▼                                             │
│                    ┌─────────────────────┐                                  │
│                    │  request_service()  │                                  │
│                    └──────────┬──────────┘                                  │
│                               │                                             │
│                               ▼                                             │
│                    ┌─────────────────────┐                                  │
│                    │ WAITING FOR SERVICE │                                  │
│                    └──────────┬──────────┘                                  │
│                               │                                             │
│                               ▼                                             │
│                 ╔═════════════════════════════╗                             │
│                 ║   availability_handler()    ║                             │
│                 ║     is_available = TRUE     ║                             │
│                 ╚═════════════╤═══════════════╝                             │
│                               │                                             │
│               ┌───────────────┴───────────────┐                             │
│               │                               │                             │
│               ▼                               ▼                             │
│   ┌───────────────────────┐       ┌───────────────────────┐                 │
│   │  REQUEST/RESPONSE     │       │  EVENT/NOTIFY         │                 │
│   │                       │       │                       │                 │
│   │  send(request)        │       │  request_event()      │                 │
│   │       │               │       │  subscribe()          │                 │
│   │       ▼               │       │       │               │                 │
│   │  ╔════════════════╗   │       │       ▼               │                 │
│   │  ║message_handler ║   │       │  ╔════════════════╗   │                 │
│   │  ║  (response)    ║   │       │  ║message_handler ║   │                 │
│   │  ╚════════════════╝   │       │  ║  (event)       ║   │                 |             
│   |                       |       |  ╚════════════════╝   |                 |
│   └───────────────────────┘       └───────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Callback Trigger Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  WHEN IS EACH CALLBACK TRIGGERED?                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   TRIGGER EVENT                        CALLBACK CALLED                      │
│   ─────────────                        ───────────────                      │
│                                                                             │
│   App connects to routing      ──────► state_handler(ST_REGISTERED)         │
│   manager                                                                   │
│                                                                             │
│   App disconnects from         ──────► state_handler(ST_DEREGISTERED)       │
│   routing manager                                                           │
│                                                                             │
│   Server sends OFFER           ──────► availability_handler(TRUE)           │
│   (service discovered)                  [CLIENT]                            │
│                                                                             │
│   Server stops/crashes         ──────► availability_handler(FALSE)          │
│   (service lost)                        [CLIENT]                            │
│                                                                             │
│   Client sends REQUEST         ──────► message_handler(request)             │
│                                         [SERVER]                            │
│                                                                             │
│   Server sends RESPONSE        ──────► message_handler(response)            │
│                                         [CLIENT]                            │
│                                                                             │
│   Client sends SUBSCRIBE       ──────► subscription_handler(TRUE)           │
│                                         [SERVER]                            │
│                                                                             │
│   Client sends UNSUBSCRIBE     ──────► subscription_handler(FALSE)          │
│                                         [SERVER]                            │
│                                                                             │
│   Server accepts subscription  ──────► subscription_status_handler(0)       │
│   (SUBSCRIBE_ACK)                       [CLIENT]                            │
│                                                                             │
│   Server rejects subscription  ──────► subscription_status_handler(error)   │
│   (SUBSCRIBE_NACK)                      [CLIENT]                            │
│                                                                             │
│   Server calls notify()        ──────► message_handler(event)               │
│                                         [ALL SUBSCRIBED CLIENTS]            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Quick Reference Card

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SOME/IP QUICK REFERENCE                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SERVER                              CLIENT                                 │
│  ══════                              ══════                                 │
│                                                                             │
│  1. init()                           1. init()                              │
│  2. register_state_handler()         2. register_state_handler()            │
│  3. register_message_handler()       3. register_availability_handler()     │
│  4. register_subscription_handler()  4. register_message_handler()          │
│  5. start()                          5. start()                             │
│                                                                             │
│  In state_handler (ST_REGISTERED):   In state_handler (ST_REGISTERED):      │
│  • offer_service()                   • request_service()                    │
│  • offer_event()                                                            │
│                                      In availability_handler (TRUE):        │
│                                      • send(request)  OR                    │
│                                      • request_event() + subscribe()        │
│                                                                             │
│  In message_handler:                 In message_handler:                    │
│  • Process request                   • Process response                     │
│  • send(response)                    • Process event                        │
│                                                                             │
│  In subscription_handler:                                                   │
│  • return true/false                                                        │
│                                                                             │
│  To send events:                                                            │
│  • notify()                                                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```
