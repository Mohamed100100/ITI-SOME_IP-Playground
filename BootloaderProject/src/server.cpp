


#include <iostream>
#include <memory>
#include <fstream>
#include "v1/firmware/BootloaderStubDefault.hpp"

#include "CommonAPI/CommonAPI.hpp"
#include <thread>


#define FILE_NOT_PROVIDED 1
#define FAILED_TO_OPEN_FILE 2
#define END_OF_FILE     3

class MyServerImpl : public v1::firmware::BootloaderStubDefault {
    private : 
        std::ifstream file;
        bool APPStatus = true;
        std::string file_path = "none";
    public : 

        
        MyServerImpl(){
            std::cout<<" MyServer implemented successfully\n";
        }

        ~MyServerImpl(){
            std::cout<<" MyServer destructed successfully\n";
        }
        
        void request_download(const std::shared_ptr<CommonAPI::ClientId> _client, request_downloadReply_t _reply) override {
            std::cout<<"Received request_download call from client\n";
            _reply (APPStatus);
        }

        void get_app(const std::shared_ptr<CommonAPI::ClientId> _client, uint32_t _size, get_appReply_t _reply) override{
            if(file_path == "none"){
                _reply( {FILE_NOT_PROVIDED} );
            }else{
                if(!file.is_open()){
                    file.open(file_path, std::ios::binary);
                    if (!file) {
                        std::cerr << "Failed to open file\n";
                        _reply( {FAILED_TO_OPEN_FILE} );
                    }
                }else{
                    char buffer[_size];         // fixed size buffer
                    file.read(buffer, _size);
                    std::streamsize bytesRead = file.gcount();
                    if (bytesRead > 0) {
                        std::vector<uint8_t> data(buffer, buffer + bytesRead);
                        _reply(data);
                    } else {
                        // End of file or read error
                        file.close(); // Close the file if end is reached
                        _reply( {END_OF_FILE} ); // Send empty vector to indicate end of file
                    }
                }

            }

        }

        void setFilePath(const std::string& path){
            file_path = path;
            APPStatus = true;
        }
};

#include <iostream>
#include <string>
#include <sys/stat.h>

class FileWatcher {
public:
    FileWatcher(const std::string& path)
        : filePath(path), lastModTime(0) 
    {
        updateTimestamp(); // initialize
    }

    // Returns true if file has been updated since last check
    bool hasUpdated() {
        struct stat statbuf;
        if (stat(filePath.c_str(), &statbuf) != 0) {
            std::cerr << "Error: Cannot access file " << filePath << "\n";
            return false;
        }

        time_t currentTime = statbuf.st_mtime;
        if (currentTime > lastModTime) {
            lastModTime = currentTime;
            return true;  // file was updated
        }
        return false;  // no change
    }

private:
    std::string filePath;
    time_t lastModTime;

    void updateTimestamp() {
        struct stat statbuf;
        if (stat(filePath.c_str(), &statbuf) == 0) {
            lastModTime = statbuf.st_mtime;
        }
    }
};


int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::shared_ptr<MyServerImpl> serverImpl = std::make_shared<MyServerImpl>();

    FileWatcher fileWatcher("firmware.txt");
    serverImpl->setFilePath("firmware.txt");
    
    bool success = runtime->registerService("local", "my.company.service.Calculator", serverImpl);
    if (!success) {
        std::cerr << "Failed to register service" << std::endl;
        return 1;
    }
    
    
    while(true){
        if(fileWatcher.hasUpdated()){
            serverImpl->fireNew_firmware_availableEvent("v1.0.0");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}