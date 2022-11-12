#include <iostream>
#include <string>

#include "Client.hpp"

enum class Command {
    None    = 0,
    Insert  = 1,
    Remove  = 2,
    Read    = 3,
    Quit    = 4
};

std::string read_string(const char* msg) {
    std::string str{};
    std::cout << msg;
    std::cin >> str;
    return str;
}

int main(int argc, char **argv) {

    Client<MyString, MyString> client{};
    client.start();

    std::string input{};
    Command command = Command::None;

    while (command != Command::Quit) {

        std::cout << "> Insert command (Insert = 1, Remove = 2, Read = 3, Quit = 4): \n";
        std::cin >> input;

        try {
            command = static_cast<Command>(std::stoi(input));
        }
        catch (const std::invalid_argument &e) {
            command = Command::None;
        }

        if (command == Command::None || command > Command::Quit) {
            std::cerr << "> Command not recognized, please try again.\n";
        }

        switch (command) {
            case Command::Quit: {
                std::cout << "> Quitting...\n";
                break;
            }
            case Command::None: {
                break;
            }
            case Command::Insert: {
                auto key = read_string("Insert a key to insert: ");
                auto value = read_string("Insert a value to insert: ");
                client.send_insert_request(MyString::from_string(key), MyString::from_string(value));
                break;
            }
            case Command::Remove: {
                auto key = read_string("Insert a key to remove: ");
                client.send_remove_request(MyString::from_string(key));
                break;
            }
            case Command::Read: {

                auto key = read_string("Insert a key to read: ");

                if (auto val = client.send_read_request(MyString::from_string(key))) {
                    std::cout << "> Value read from server: '" << val.value().data << "'\n";
                }
                else {
                    std::cout << "> Key not present!\n";
                }
                break;
            }

        }

    }

    return 0;
}