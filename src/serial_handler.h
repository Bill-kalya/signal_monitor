#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <chrono>
#include <thread>

class SerialHandler {
private:
    int fd;

public:
    SerialHandler() : fd(-1) {}
    ~SerialHandler() {
        if (fd != -1) {
            close(fd);
        }
    }

    bool open_serial(const char* device, int baud = B115200) {
        fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) return false;
        
        struct termios tty;
        memset(&tty, 0, sizeof tty);
        if (tcgetattr(fd, &tty) != 0) {
            close(fd);
            fd = -1;
            return false;
        }

        cfsetospeed(&tty, baud);
        cfsetispeed(&tty, baud);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 5;

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            close(fd);
            fd = -1;
            return false;
        }
        
        tcflush(fd, TCIOFLUSH);
        return true;
    }

    bool write_cmd(const std::string& cmd) {
        std::string full_cmd = cmd + "\r";
        ssize_t n = write(fd, full_cmd.c_str(), full_cmd.size());
        return n == static_cast<ssize_t>(full_cmd.size());
    }

    std::string read_response(int timeout_ms = 1000) {
        std::string response;
        char buffer[256];
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = 0;
                response += buffer;
                
                if (response.find("\r\nOK\r\n") != std::string::npos ||
                    response.find("\r\nERROR\r\n") != std::string::npos) {
                    break;
                }
            }
            
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            if (elapsed.count() > timeout_ms) break;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        return response;
    }
};