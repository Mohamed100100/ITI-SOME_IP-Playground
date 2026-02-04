#include <vsomeip/vsomeip.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include "capslock_ids.hpp"

using namespace control;

class Server {
public:
    Server() : app_(vsomeip::runtime::get()->create_application("control_server")), 
               running_(true) {}

    void run() {
        // Initialize the application
        // This loads the JSON configuration file
        app_->init();
        
        /*
         * CALLBACK: State Handler
         * ========================
         * WHEN CALLED: 
         *   - When application successfully registers with vsomeip routing manager
         *   - State changes to ST_REGISTERED or ST_DEREGISTERED
         * 
         * PURPOSE:
         *   - Know when it's safe to offer services
         *   - Only offer service AFTER ST_REGISTERED
         */
        app_->register_state_handler([this](vsomeip::state_type_e state) {
            if (state == vsomeip::state_type_e::ST_REGISTERED) {
                // NOW safe to offer our service to the network
                // This sends OFFER message via Service Discovery
                app_->offer_service(SERVICE_ID, INSTANCE_ID);
                std::cout << "[Server] Service offered. Waiting for requests...\n";
            }
        });

        /*
         * CALLBACK: Message Handler
         * ==========================
         * WHEN CALLED:
         *   - When client sends a REQUEST to METHOD_SET
         *   - After client calls app_->send(request)
         * 
         * PURPOSE:
         *   - Process incoming requests from clients
         *   - Perform the actual work (write to capslock file)
         *   - Send response back to client
         */
        app_->register_message_handler(SERVICE_ID, INSTANCE_ID, METHOD_SET,
            [this](const std::shared_ptr<vsomeip::message>& request) {
                on_request(request);
            });

        // Start vsomeip in separate thread (blocking call)
        std::thread t([this]() { app_->start(); });

        std::cout << "Press Enter to stop...\n";
        std::cin.get();
        
        running_ = false;
        app_->stop();
        t.join();
    }

private:
    /*
     * Process incoming request from client
     * Extract command, write to capslock file, send response
     */
    void on_request(const std::shared_ptr<vsomeip::message>& request) {
        // Extract payload from request
        auto payload = request->get_payload();
        uint8_t cmd = payload->get_data()[0];

        std::cout << "[Server] Received command: " << (int)cmd << std::endl;

        // Write to capslock LED file
        std::ofstream file(CAPSLOCK_FILE_PATH);
        if (file.is_open()) {
            if (cmd == 1) {
                file << "1";  // Turn ON
                std::cout << "[Server] Caps Lock ON\n";
            } else if (cmd == 2) {
                file << "0";  // Turn OFF
                std::cout << "[Server] Caps Lock OFF\n";
            }
            file.close();
        } else {
            std::cout << "[Server] ERROR: Cannot write to file (run with sudo)\n";
        }

        // Create and send response back to client
        auto response = vsomeip::runtime::get()->create_response(request);
        auto resp_payload = vsomeip::runtime::get()->create_payload();
        std::vector<vsomeip::byte_t> data = {1}; // 1 = success
        resp_payload->set_data(data);
        response->set_payload(resp_payload);
        app_->send(response);
    }

    std::shared_ptr<vsomeip::application> app_;
    std::atomic<bool> running_;
};

int main() {
    Server server;
    server.run();
    return 0;
}