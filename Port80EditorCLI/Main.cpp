#define NOMINMAX

#include <string>
#include <print>
#include <cctype>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cassert>

#include <Windows.h>
#include <winioctl.h>

#include "../Public.h"

static bool ParseHexByte(const char* s, uint8_t& out)
{
	if (s == nullptr)
	{
		return false;
	}

	constexpr const char* HEX_DIGITS = "0123456789ABCDEF";

	std::string input = s;
	
	if (input.length() != 2 && input.length() != 4)
	{
		return false;
	}

	std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {return std::toupper(c); });

	if (input.length() == 4)
	{
		if (input[0] != '0')
		{
			return false;
		}
		
		if (input[1] != 'X')
		{
			return false;
		}

		input = input.substr(2, 2);
	}

	auto toValue = [&](char c) {for (int i = 0; i < 16; i++) { if (HEX_DIGITS[i] == c) { return i; } } return -241; }; //0xFGのような場合でもマイナスになるように0xF0(240)から1大きい数のマイナス

	int t = toValue(input[0]) * 16 + toValue(input[1]);

	if (t < 0)
	{
		return false;
	}
	else
	{
		out = t;
		return true;
	}
}

int main(int argc, char** argv)
{

	try
	{
		if (argc != 2)
		{
			assert(argv[0] != nullptr);

			std::println("Usage: {} <1-Byte Hex>", argv[0]);
			return 0;
		}

		uint8_t value = 0;
		if (!ParseHexByte(argv[1], value))
		{
			std::println("Failed to parse value.");
			return 1;
		}

		HANDLE h = CreateFileW(L"\\\\.\\Port80", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (h == INVALID_HANDLE_VALUE)
		{
			DWORD e = GetLastError();
			std::println("CreateFileW failed. LastError = {}", e);
			return 2;
		}

		DWORD bytes = 0;

		BOOL ok = DeviceIoControl(h, IOCTL_PORT80_WRITE_U8, &value, sizeof(value), nullptr, 0, &bytes, nullptr);

		if (!ok)
		{

			DWORD e = GetLastError();
			std::println("DeviceIoControl failed. LastError = {}", e);
			return 3;
		}

		CloseHandle(h);

		return 0;
	}
	catch (const std::exception& e)
	{
		std::println("Unhandled exception: {}", e.what());
		return 4;
	}
	catch (...)
	{
		std::println("Unhandled exception. Exiting.");
		return -1;
	}
}