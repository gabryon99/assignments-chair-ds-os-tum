#include <iostream>
#include <string>

#include "Client.hpp"

int main(int argc, char **argv) {

    Client<MyString, MyString> client{};
    client.start();

    MyString key = MyString::from_string("gabriele");
    MyString value = MyString::from_string("pappalardo");

    //client.send_insert_request(key, value);

    if (auto rcv = client.send_read_request(key)) {
        std::fprintf(stdout, "[client] :: value: %s\n", rcv.value().data);
    }
    else {
        std::fprintf(stdout, "[client] :: no key\n");
    }


    return 0;
}