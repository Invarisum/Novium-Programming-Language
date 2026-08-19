#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Test program started\n";
    std::cout.flush();
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>\n";
        return 1;
    }
    
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Could not open: " << argv[1] << "\n";
        return 1;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::cout << "Read " << content.size() << " bytes\n";
    std::cout.flush();
    return 0;
}