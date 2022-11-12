#include <stdexcept>
#include <string>
#include "Server.hpp"
#include "Common.hpp"

void print_usage() {
	std::fprintf(stderr, "usage: ./server <hash-table-size>\n");
}

std::size_t parse_argument(char *const *argv) {
    std::size_t hash_table_size = 0;
    try {
        hash_table_size = std::stoul(argv[1], nullptr, 10);
    }
    catch (const std::invalid_argument &e) {
        fprintf(stderr, "[error] :: Cannot parse HashTable's size correctly.\n");
    }
    return hash_table_size;
}

int main(int argc, char **argv) {

	if (argc != 2) {
		print_usage();
		return EXIT_FAILURE;
	}

    std::size_t hash_table_size = parse_argument(argv);
    if (hash_table_size == 0) {
        return EXIT_FAILURE;
    }

    Server<MyString, MyString> server(4, hash_table_size);
    server.start();

    return EXIT_SUCCESS;
}