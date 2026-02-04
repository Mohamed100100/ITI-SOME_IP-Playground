#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <set>
#include "capslock_ids.hpp"

using namespace monitor;

class Client {
public:
    Client() : app_(vsomeip::runtime::get()->create_application("monitor_client")),
               running_(true) {}

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
         *   - Request service after registration
         */
        app_->register_state_handler([this](vsomeip::state_type_e state) {
            if (state == vsomeip::state_type_e::ST_REGISTERED) {
                // Request the service (triggers FIND)
                app_->request_service(SERVICE_ID, INSTANCE_ID);
            }
        });

        /*
         * CALLBACK: Availability Handler
         * ================================
         * WHEN CALLED:
         *   - When service becomes available (server offers it)
         *   - When service becomes unavailable
         * 
         * PURPOSE:
         *   - Subscribe to events when service is available
         */
        app_->register_availability_handler(SERVICE_ID, INSTANCE_ID,
            [this](vsomeip::service_t, vsomeip::instance_t, bool is_available) {
                std::cout << "[Client] Service " << (is_available ? "AVAILABLE" : "UNAVAILABLE") << "\n";
                if (is_available) {
                    subscribe();  // Subscribe when service becomes available
                }
            });

        /*
         * CALLBACK: Message Handler (for events)
         * ========================================
         * WHEN CALLED:
         *   - When server sends NOTIFICATION via notify()
         *   - Every time capslock state changes on server
         * 
         * PURPOSE:
         *   - Receive and display capslock state changes
         */
        app_->register_message_handler(SERVICE_ID, INSTANCE_ID, EVENT_ID,
            [](const std::shared_ptr<vsomeip::message>& event) {
                auto payload = event->get_payload();
                bool state = (payload->get_data()[0] == 1);
                std::cout << "\n*** CAPS LOCK IS NOW: " << (state ? "ON" : "OFF") << " ***\n";
            });

        // Start vsomeip in separate thread
        std::thread t([this]() { app_->start(); });

        std::cout << "Listening for Caps Lock changes...\n";
        std::cout << "Press Enter to stop...\n";
        std::cin.get();

        running_ = false;
        app_->stop();
        t.join();
    }

private:
    /*
     * Subscribe to capslock events
     * Called when service becomes available
     */
    void subscribe() {
        std::set<vsomeip::eventgroup_t> groups;
        groups.insert(EVENTGROUP_ID);

        // Step 1: Request the event (local preparation)
        app_->request_event(SERVICE_ID, INSTANCE_ID, EVENT_ID, groups,
            vsomeip::event_type_e::ET_FIELD);
        
        // Step 2: Subscribe (sends SUBSCRIBE to server)
        app_->subscribe(SERVICE_ID, INSTANCE_ID, EVENTGROUP_ID);

        std::cout << "[Client] Subscribed to Caps Lock events\n";
    }

    std::shared_ptr<vsomeip::application> app_;
    std::atomic<bool> running_;
};

int main() {
    Client client;
    client.run();
    return 0;
}