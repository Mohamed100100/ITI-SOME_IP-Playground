

#include <CommonAPI/CommonAPI.hpp>
#include <v1/firmware/BootloaderProxy.hpp>


class MyClientImpl{
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();

public:

};


int main() {
    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    std::shared_ptr<MyClientImpl> clientImpl = std::make_shared<MyClientImpl>();

    auto proxy = runtime->buildProxy<v1::firmware::BootloaderProxy>("local", "my.company.service.Calculator");

    if (!proxy) {
        std::cerr << "Failed to build proxy!" << std::endl;
        return 1;
    }

    proxy->getNew_firmware_availableEvent().subscribe([](const std::string &version){
        std::cout<<"New firmware available with version: "<<version<<"\n";
    });

    while(true){
        std::cout<<"Choose from the following options:\n";
        std::cout<<"1- Get App\n";
        std::cout<<"2- Request Download\n";
        int choice;
        std::cin>>choice;
        if(choice == 1){
            std::vector<uint8_t> app_data;
            CommonAPI::CallStatus callStatus;
            proxy->get_app(10, callStatus, app_data);
            if(callStatus == CommonAPI::CallStatus::SUCCESS){
                std::cout<<"App Data received of size: "<<app_data.data()<<"\n";
            }else{
                std::cout<<"Failed to get App Data, Error Code: "<<static_cast<int>(callStatus)<<"\n";
            }
        }else if(choice == 2){
            bool ready;
            CommonAPI::CallStatus callStatus;
            proxy->request_download(callStatus, ready);
            if(callStatus == CommonAPI::CallStatus::SUCCESS){
                std::cout<<"Download Request status: "<<(ready ? "Ready" : "Not Ready")<<"\n";
            }else{
                std::cout<<"Failed to request download, Error Code: "<<static_cast<int>(callStatus)<<"\n";
            }
        }else{
            std::cout<<"Invalid Choice, Try again.\n";  
        }
    }
}
