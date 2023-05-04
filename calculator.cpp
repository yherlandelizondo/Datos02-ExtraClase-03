#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <thread>

using tcp = boost::asio::ip::tcp;

int main(){

    auto const address = boost::asio::ip::make_address("127.0.0.1");
    auto const port = static_cast<unsigned short>(std::atoi("8083"));

    boost::asio::io_context ioc{1}; // with this the system execute the connection

    boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};// "tcp accepter"

    while(1){
        tcp::socket socket{ioc};
        acceptor.accept(socket);
        std::cout<<"socket accepted"<<std::endl;

        std::thread{[q {std::move(socket)}](){

            boost::beast::websocket::stream<tcp::socket> ws {std::move(const_cast<tcp::socket&>(q))};

            ws.accept();//accepting the connection

            while(1){
                try{
                    boost::beast::flat_buffer buffer;//like a container
                    
                    ws.read(buffer);//reading the client data

                    std::string out = boost::beast::buffers_to_string(buffer.data()); //guardando lo que tiene el buffer

                    //storing the json data
                    nlohmann::json data = nlohmann::json::parse(out);
                    float num1 = std::stof(data["input1"].get<std::string>());
                    float num2 = std::stof(data["input2"].get<std::string>());
                    std::string op = data["operator"];

                    float result = 0;
                    bool error = false;

                    //making the operations
                    if(op == "add"){
                        result = num1 + num2;
                    }else if(op == "sub"){
                        result = num1 - num2;
                    }else if(op == "mult"){
                        result = num1 * num2;
                    }else if(op == "div"){
                        if(num2 == 0){
                            error = true;              
                        }else{
                            result = num1 / num2;
                        }
                    }

                    //sending the result to the client, and cheking if an error occurred
                    if(error){
                        error = false;
                        std::string errorMessage = "Division by 0";
                        ws.write(boost::asio::buffer(errorMessage));
                    }else{
                        std::ostringstream ss;
                        ss << std::fixed << std::setprecision(2) << result;
                        ws.write(boost::asio::buffer(ss.str()));
                    }
                    
                }
                //used to close the connection
                catch(boost::beast::system_error const& se)
                {
                    if(se.code() != boost::beast::websocket::error::closed){
                        std::cout<< se.code().message() << std::endl;
                        break;
                    }
                }
            }
        }}.detach();
    }
    return 0;
}