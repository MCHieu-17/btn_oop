#include "ui/ConsoleUI.h"
#include "exceptions/ParkingException.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
std::filesystem::path executableDirectory(const char* argument) {
#ifdef _WIN32
    (void)argument;
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, &buffer[0], static_cast<DWORD>(buffer.size()));
    if (length == 0 || length == buffer.size()) throw StorageException("Không xác định được thư mục chương trình.");
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
#else
    return std::filesystem::absolute(argument).parent_path();
#endif
}
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        bool demo = false;
        auto root = executableDirectory(argv[0]);
        if (root.filename() == "build") root = root.parent_path();
        std::filesystem::path customDirectory;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--demo") demo = true;
            else if (argument == "--data-dir" && i + 1 < argc) customDirectory = std::filesystem::u8path(argv[++i]);
            else if (argument == "--help") {
                std::cout << "parking.exe [--demo] [--data-dir DIRECTORY]\n"
                             "--demo: nhập giờ thủ công, mặc định lưu trong demo_data.\n"
                             "--data-dir: chọn thư mục dữ liệu riêng.\n";
                return 0;
            } else throw ValidationException("Tham số không hợp lệ. Dùng --help để xem hướng dẫn.");
        }
        const auto directory = customDirectory.empty() ? root / (demo ? "demo_data" : "data") : customDirectory;
        ParkingRepository repository(directory / "parking_data.txt");
        std::cout << "Tệp dữ liệu: " << repository.getPath().u8string() << '\n';
        int initialCapacity = 1;
        if (!repository.hasData()) {
            std::cout << "Chưa có dữ liệu. Khởi tạo bãi mới.\n";
            while (true) {
                try {
                    initialCapacity = static_cast<int>(ConsoleUI::readNumber("Sức chứa ban đầu: ", 1, 1000000));
                    break;
                } catch (const ValidationException& error) {
                    if (std::cin.eof()) return 0;
                    std::cout << "Lỗi: " << error.what() << '\n';
                }
            }
        }
        ParkingService service(std::move(repository), initialCapacity);
        ConsoleUI ui(service, demo);
        ui.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Không thể tiếp tục: " << error.what() << '\n';
        return 1;
    }
}
