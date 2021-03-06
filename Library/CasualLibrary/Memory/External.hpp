/**
 * Serves as communicator between your application and the games memory.
 *
 * @author CasualComunity: Alien, C3x0r, CasualGamer
 * @version 0.1.2 06.11.2020
 * @todo: optimize scanner speed, optional parameter with desiredAccess, memory checks, add readString(...) to internal
*/

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

#include "../Helper.hpp"
#include "../Address.hpp"
#include "Memory.hpp"
#include "../Macros.hpp"

namespace Memory {
    class External final
    {
    public:
        External(void) noexcept = default;
        External(const char* proc, bool debug = false) noexcept;
        ~External(void) noexcept;

        /**
        @brief reads a value from memory, supports strings in C++17 or greater.
        @param addToBeRead address from which the value will be read.
        */
        template <typename T>
        [[nodiscard]] T read(const Address& addToBeRead, const bool memoryCheck = false) noexcept {
            const uintptr_t address = addToBeRead.get();

            if (memoryCheck) {
                MEMORY_BASIC_INFORMATION mbi;
                VirtualQueryEx(handle, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi));

                if (mbi.Protect & (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS)) {
                    //if constexpr (!std::is_same<T, std::string>::value){
                    //    return -1; 
                    //}
                    //if constexpr (std::is_same<T, std::string>::value) {
                    //    return "";
                    //}
                    return T{};
                }
            }

#ifdef CPP17GRT
            if constexpr (is_any_type<T, const char*, std::string, char*>()) {
                constexpr const std::size_t size = 200;
                std::vector<char> chars(size);
                if (!ReadProcessMemory(handle, reinterpret_cast<LPBYTE*>(address), chars.data(), size, NULL) && debug)
                    std::cout << getLastErrorAsString() << std::endl;

                const std::string name(chars.begin(), chars.end());

                return name.substr(0, name.find('\0'));;
            }
#endif

            T varBuff;
            ReadProcessMemory(handle, reinterpret_cast<LPBYTE*>(address), &varBuff, sizeof(varBuff), nullptr);
            return varBuff;
        }

        /**
        @brief writes a value to memory
        @param addToWrite address to which the value will be written to.
        @param valToWrite the value that gets written to desired address.
        */
        template <typename T>
        T write(const uintptr_t addToWrite, const T valToWrite, const bool memoryCheck = false) noexcept {
            if (memoryCheck) {
                MEMORY_BASIC_INFORMATION mbi;
                VirtualQueryEx(handle, reinterpret_cast<LPCVOID>(addToWrite), &mbi, sizeof(mbi));

                if (mbi.Protect & (PAGE_GUARD | PAGE_NOCACHE | PAGE_NOACCESS) || mbi.Protect & PAGE_READONLY)
                    return -1;
            }

            WriteProcessMemory(handle, reinterpret_cast<LPBYTE*>(addToWrite), &valToWrite, sizeof(valToWrite), nullptr);
            return valToWrite;
        }

        [[deprecated("use read function with std::string/const char* template argument")]] [[nodiscard]] std::string readString(const uintptr_t addToBeRead, std::size_t size = 0) noexcept;

        /**@brief returns process id of the target process*/
        [[nodiscard]] DWORD getProcessID(void) noexcept;

        /**@brief returns the module base address of \p modName (example: "client.dll")*/
        [[nodiscard]] Address getModule(const char* modName) noexcept;

        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        [[nodiscard]] Address getAddress(const uintptr_t basePrt, const std::vector<uintptr_t>& offsets) noexcept;

        /**@brief returns the dynamic pointer from base pointer \p basePrt and \p offsets*/
        [[nodiscard]] Address getAddress(const Address& basePrt, const std::vector<uintptr_t>& offsets) noexcept;

        [[nodiscard]] bool memoryCompare(const BYTE* bData, const std::vector<int>& signature) noexcept;

        /**
        @brief a basic signature scanner
        @param start Address where to start scanning.
        @param sig Signature to search, for example: "? 39 05 F0 A2 F6 FF" where "?" (-1) is a wildcard.
        @param size Size of the area to search within.
        */
        [[nodiscard]] Address findSignature(const uintptr_t start, const char* sig, const size_t size) noexcept;
        [[nodiscard]] Address findSignature(const Address& start, const char* sig, const size_t size) noexcept;
    private:
        /** @brief handle of the target process */
        HANDLE handle = nullptr;

        /** @brief ID of the target process */
        DWORD processID = 0;

        /** @brief debug flag. Set true for debug messages*/
        bool debug = false;

        /**@brief initialize member variables.
        @param proc Name of the target process ("target.exe")
        @param access Desired access right to the process
        @returns whether or not initialization was successful*/
        bool init(const char* proc, const DWORD access = PROCESS_ALL_ACCESS) noexcept;

        // Sadly, only C++17 feature because of the folding
#ifdef CPP17GRT
        template<typename T, typename... Ts>
        [[nodiscard]] constexpr bool is_any_type(void) noexcept {
            return (std::is_same_v<T, Ts> || ...);
        }
#endif
    };
}