#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include "capslock_ids.hpp"

using namespace control;

class Client {
public:
    Client() : app_(vsomeip::runtime::get()->create_application("control_client")),
               running_(true), available_(false) {}

    void run() {
        // Initialize the application
        // This loads the JSON configuration file
        app_->init();

        /*
         * CALLBACK: State Handler
         * ========================
         * WHEN CALLED:
         *   - When application registers with vsomeip routing manager
         *   - State changes to ST_REGISTERED or ST_DEREGISTERED
         * 
         * PURPOSE:
         *   - Know when it's safe to request services
         *   - Only request service AFTER ST_REGISTERED
         */
        app_->register_state_handler([this](vsomeip::state_type_e state) {
            if (state == vsomeip::state_type_e::ST_REGISTERED) {
                // Request the service - this triggers FIND in Service Discovery
                app_->request_service(SERVICE_ID, INSTANCE_ID);
            }
        });

        /*
         * CALLBACK: Availability Handler
         * ================================
         * WHEN CALLED:
         *   - When requested service becomes AVAILABLE (server offers it)
         *   - When service becomes UNAVAILABLE (server stops/crashes)
         *   - Triggered by receiving OFFER message from server
         * 
         * PURPOSE:
         *   - Know when it's safe to send requests
         *   - Handle service disconnection
         */
        app_->register_availability_handler(SERVICE_ID, INSTANCE_ID,
            [this](vsomeip::service_t, vsomeip::instance_t, bool is_available) {
                available_ = is_available;
                std::cout << "[Client] Service " << (is_available ? "AVAILABLE" : "UNAVAILABLE") << "\n";
            });

        /*
         * CALLBACK: Message Handler (for responses)
         * ==========================================
         * WHEN CALLED:
         *   - When server sends RESPONSE to our request
         *   - After server calls app_->send(response)
         * 
         * PURPOSE:
         *   - Receive and process response from server
         */
        app_->register_message_handler(SERVICE_ID, INSTANCE_ID, METHOD_SET,
            [](const std::shared_ptr<vsomeip::message>&) {
                std::cout << "[Client] Response received\n";
            });

        // Start vsomeip in separate thread
        std::thread t([this]() { app_->start(); });

        // User interface loop
        while (running_) {
            std::cout << "\n--- Caps Lock Control ---\n";
            std::cout << "1: Turn ON\n";
            std::cout << "2: Turn OFF\n";
            std::cout << "0: Exit\n";
            std::cout << "Choice: ";

            int choice;
            std::cin >> choice;

            if (choice == 0) {
                running_ = false;
            } else if ((choice == 1 || choice == 2) && available_) {
                send_command(choice);
            } else if (!available_) {
                std::cout << "[Client] Service not available!\n";
            }
        }

        app_->stop();
        t.join();
    }

private:
    /*
     * Create and send request to server
     */
    void send_command(uint8_t cmd) {
        // Create request message
        auto request = vsomeip::runtime::get()->create_request();
        request->set_service(SERVICE_ID);
        request->set_instance(INSTANCE_ID);
        request->set_method(METHOD_SET);

        // Set payload with command (1=ON, 2=OFF)
        auto payload = vsomeip::runtime::get()->create_payload();
        std::vector<vsomeip::byte_t> data = {cmd};
        payload->set_data(data);
        request->set_payload(payload);

        // Send request to server
        app_->send(request);
        std::cout << "[Client] Sent command: " << (int)cmd << "\n";
    }

    std::shared_ptr<vsomeip::application> app_;
    std::atomic<bool> running_;
    std::atomic<bool> available_;
};

int main() {
    Client client;
    client.run();
    return 0;
}