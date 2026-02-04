#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <set>
#include "capslock_ids.hpp"

using namespace monitor;

class Server {
public:
    Server() : app_(vsomeip::runtime::get()->create_application("monitor_server")),
               running_(true), last_state_(false) {}

    void run() {
        // Initialize the application
        app_->init();

        /*
         * CALLBACK: State Handler
         * ========================
         * WHEN CALLED:
         *   - When application registers with vsomeip routing manager
         * 
         * PURPOSE:
         *   - Offer service and event after registration
         */
        app_->register_state_handler([this](vsomeip::state_type_e state) {
            if (state == vsomeip::state_type_e::ST_REGISTERED) {
                // Offer the service
                app_->offer_service(SERVICE_ID, INSTANCE_ID);

                // Offer the event (declare we will publish this event)
                std::set<vsomeip::eventgroup_t> groups;
                groups.insert(EVENTGROUP_ID);
                app_->offer_event(SERVICE_ID, INSTANCE_ID, EVENT_ID, groups,
                    vsomeip::event_type_e::ET_FIELD);

                std::cout << "[Server] Service offered. Monitoring Caps Lock...\n";
            }
        });

        /*
         * CALLBACK: Subscription Handler
         * ================================
         * WHEN CALLED:
         *   - When client sends SUBSCRIBE request
         *   - When client sends UNSUBSCRIBE (or disconnects)
         * 
         * PURPOSE:
         *   - Accept or reject subscriptions (return true/false)
         *   - Track who is subscribed
         * 
         * RETURN VALUE:
         *   - true  = Accept subscription (send SUBSCRIBE_ACK)
         *   - false = Reject subscription (send SUBSCRIBE_NACK)
         */
        app_->register_subscription_handler(SERVICE_ID, INSTANCE_ID, EVENTGROUP_ID,
            [](vsomeip::client_t client, vsomeip::uid_t, vsomeip::gid_t, bool subscribed) {
                std::cout << "[Server] Client 0x" << std::hex << client 
                          << (subscribed ? " subscribed" : " unsubscribed") << "\n";
                return true;  // Accept all subscriptions
            });

        // Start vsomeip in separate thread
        std::thread app_thread([this]() { app_->start(); });
        
        // Start monitoring thread (watches capslock file)
        std::thread monitor_thread([this]() { monitor_loop(); });

        std::cout << "Press Enter to stop...\n";
        std::cin.get();

        running_ = false;
        app_->stop();
        app_thread.join();
        monitor_thread.join();
    }

private:
    /*
     * Read current capslock state from file
     * Returns: true if ON, false if OFF
     */
    bool read_capslock() {
        std::ifstream file(CAPSLOCK_FILE_PATH);
        if (!file.is_open()) return false;
        int val = 0;
        file >> val;
        return (val > 0);
    }

    /*
     * Send notification to ALL subscribed clients
     * This is called when capslock state changes
     */
    void send_notification(bool state) {
        auto payload = vsomeip::runtime::get()->create_payload();
        std::vector<vsomeip::byte_t> data = {state ? (uint8_t)1 : (uint8_t)0};
        payload->set_data(data);
        
        // notify() sends to ALL subscribers automatically
        app_->notify(SERVICE_ID, INSTANCE_ID, EVENT_ID, payload);
    }

    /*
     * Monitor loop - runs in separate thread
     * Checks capslock state every 100ms
     * Sends notification when state changes
     */
    void monitor_loop() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            bool current = read_capslock();
            
            // Check if state changed
            if (current != last_state_) {
                last_state_ = current;
                std::cout << "[Server] Caps Lock changed: " << (current ? "ON" : "OFF") << "\n";
                
                // Notify all subscribed clients
                send_notification(current);
            }
        }
    }

    std::shared_ptr<vsomeip::application> app_;
    std::atomic<bool> running_;
    std::atomic<bool> last_state_;
};

int main() {
    Server server;
    server.run();
    return 0;
}