
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>


char buffer[1024];

int Error(const char* msg) {
    printf("%s (%u)\n", msg, GetLastError());
    return -1;
}
int main(int argc, char* argv[]) {
	memset(buffer, 0, sizeof(buffer));
	std::ifstream ifs("driverlist.txt");
	std::string line;
	std::string string_template = "\\\\?\\GLOBALROOT\\Device\\";
	while (std::getline(ifs, line)){
		std::string path = string_template + line;
		HANDLE hDevice = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hDevice == INVALID_HANDLE_VALUE) {
			//return Error("Failed to open driver");
			continue;
		}
		else {
			std::cout << "[+] Access " << path << std::endl;
			CloseHandle(hDevice);
		}
	}
}
