#pragma once
namespace GuestSockets {
constexpr int FirstDescriptor = 0x10000000;
int Close(int descriptor);
int Fcntl(int descriptor, int command, int flags);
}
