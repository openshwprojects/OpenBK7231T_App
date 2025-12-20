#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		std::cout << "arg1: ota1_path, arg2: ota2_path\n";
		exit(1);
	}
	else if(argc < 3)
	{
		std::cout << "Not enough args\n";
		exit(1);
	}

	std::string out_path = "OpenRTL8710B_Test_ota.img";
	if(argc >= 4)
	{
		out_path = argv[3];
	}
	fs::path ota1_path{ argv[1] };
	fs::path ota2_path{ argv[2] };
	uint32_t ota1_size = fs::file_size(ota1_path);
	uint32_t ota2_size = fs::file_size(ota2_path);
	std::cout << "OpenBeken AmebaZ OTA image generator.\n";
	std::cout << "The size of " << ota1_path.filename() << " is " << ota1_size << " bytes.\n";
	std::cout << "The size of " << ota2_path.filename() << " is " << ota2_size << " bytes.\n";
	std::cout << "Output image path: " << out_path << "\n";
	std::cout << "Generating OpenBeken OTA image...\n";
	std::ofstream obkota(out_path, std::ios_base::binary);
	std::ifstream ota1(ota1_path, std::ios_base::binary);
	std::ifstream ota2(ota2_path, std::ios_base::binary);
	obkota.write(reinterpret_cast<char*>(&ota1_size), sizeof(ota1_size));
	obkota.write(reinterpret_cast<char*>(&ota2_size), sizeof(ota2_size));
	obkota << ota1.rdbuf() << ota2.rdbuf();
	obkota.close();
	ota1.close();
	ota2.close();
	std::cout << "Done, OTA image size: " << fs::file_size(out_path) << "\n";
}
