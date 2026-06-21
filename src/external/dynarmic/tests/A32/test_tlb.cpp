/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>
#include <mcl/stdint.hpp>

#include "./testenv.h"
#include "dynarmic/interface/A32/a32.h"
#include "dynarmic/interface/tlb.h"

template <size_t TLB_BITS>
static Dynarmic::A32::UserConfig GetUserConfig(ArmTestEnv* testenv, Dynarmic::TLB<TLB_BITS> &tlb) {
    Dynarmic::A32::UserConfig user_config;
    user_config.callbacks = testenv;
    user_config.tlb_entries = tlb.entries.data();
    user_config.tlb_index_mask_bits = static_cast<int>(TLB_BITS);
    return user_config;
}

TEST_CASE("tlb: All entries hit", "[tlb][A32]") {
    ArmTestEnv env;

    std::uint32_t page1[2] = { 100, 200 };
    std::uint32_t page2[3] = { 400, 600, 800 }; 

    Dynarmic::TLB<9> tlb(12);

    // Add entries that continuous so that they don't hit
    tlb.Add(0x12345000, reinterpret_cast<std::uint8_t*>(page1), Dynarmic::MemoryPermission::ReadWrite);
    tlb.Add(0x12346000, reinterpret_cast<std::uint8_t*>(page2), Dynarmic::MemoryPermission::ReadWrite);

    Dynarmic::A32::Jit jit{GetUserConfig(&env, tlb)};

    env.code_mem = {
        0xe5933000,  // ldr r3, [r3]
        0xe5845000,  // str r5, [r4]
        0xeafffffe,  // b +#0
    };
    
    jit.Regs()[3] = 0x12345004;
    jit.Regs()[4] = 0x12346008;
    jit.Regs()[5] = 0x11111111;
    jit.SetCpsr(0x000001d0); // User-mode

    env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 200);
    REQUIRE(page2[2] == 0x11111111);
}

TEST_CASE("tlb: Miss TLB", "[tlb][A32]") {
    ArmTestEnv env;

    std::uint32_t page1[2] = { 100, 200 };

    Dynarmic::TLB<9> tlb(12);
    tlb.Add(0x12346000, reinterpret_cast<std::uint8_t*>(page1), Dynarmic::MemoryPermission::ReadWrite);

    Dynarmic::A32::Jit jit{GetUserConfig(&env, tlb)};
    env.MemoryWrite32(0x1234500C, 0xABCDEF);

    // This time the load will miss, which trigger MemoryRead*
    env.code_mem = {
        0xe5933008,  // ldr r3, [r3, #8]
        0xe5845000,  // str r5, [r4]
        0xeafffffe,  // b +#0
    };
    
    jit.Regs()[3] = 0x12345004;
    jit.Regs()[4] = 0x12346004;
    jit.Regs()[5] = 0x11111111;
    jit.SetCpsr(0x000001d0); // User-mode

    env.ticks_left = 3;
    jit.Run();

    REQUIRE(jit.Regs()[3] == 0xABCDEF);
    REQUIRE(page1[1] == 0x11111111);
}

TEST_CASE("tlb: Wrong permission", "[tlb][A32]") {
    ArmTestEnv env;

    std::uint32_t page1[2] = { 100, 200 };
    std::uint32_t page2[2] = { 300, 400 };

    Dynarmic::TLB<9> tlb(12);
    tlb.Add(0x12345000, reinterpret_cast<std::uint8_t*>(page1), Dynarmic::MemoryPermission::Write);
    tlb.Add(0x12346000, reinterpret_cast<std::uint8_t*>(page2), Dynarmic::MemoryPermission::ReadWrite);

    Dynarmic::A32::Jit jit{GetUserConfig(&env, tlb)};
    env.MemoryWrite32(0x1234500C, 0xABCDEF);

    // This time the load will fail (permission only allow write), which trigger MemoryRead*
    env.code_mem = {
        0xe5933008,  // ldr r3, [r3, #8]
        0xe5944004,  // ldr r4, [r4, #4]
        0xeafffffe,  // b +#0
    };
    
    jit.Regs()[3] = 0x12345004;
    jit.Regs()[4] = 0x12346000;
    jit.SetCpsr(0x000001d0); // User-mode

    env.ticks_left = 3;
    jit.Run();

    // The first instruction will miss TLB, and fallback to MemoryRead*
    REQUIRE(jit.Regs()[3] == 0xABCDEF);
    REQUIRE(jit.Regs()[4] == 400);
}