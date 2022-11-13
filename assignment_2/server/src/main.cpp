#include <stdexcept>
#include <string>

#include "Server.hpp"

struct Args {
    size_t hash_table_size = 0;
    unsigned workers = std::thread::hardware_concurrency();
};

void print_usage() {
	std::fprintf(stderr, "usage: ./server <hash-table-size> <workers> [default=%u]\n", std::thread::hardware_concurrency());
}

Args parse_arguments(int argc, char *const *argv) {

    Args args;

    try {
        args.hash_table_size = std::stoul(argv[1], nullptr, 10);
    }
    catch (const std::invalid_argument &e) {
        fprintf(stderr, "[error] :: Cannot parse HashTable's size correctly.\n");
        std::exit(EXIT_FAILURE);
    }

    if (argc == 3) {
        try {
            args.workers = std::stoul(argv[2], nullptr, 10);
        }
        catch (const std::invalid_argument &e) {
            fprintf(stderr, "[error] :: Cannot parse workers number correctly, using default.\n");
        }
    }

    return args;
}

int main(int argc, char **argv) {

	if (argc < 2 || argc > 3) {
		print_usage();
		return EXIT_FAILURE;
	}

    auto args = parse_arguments(argc, argv);

    Server<MyString, MyString> server(args.workers, args.hash_table_size);
    server.start();

    return EXIT_SUCCESS;
}