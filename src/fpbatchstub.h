#pragma once

// The hook on the game's camera transform set (fpbatchrules.h kSetter) for the first-person batches probe: included by fpbatch.cpp and by
// tools\fpbatchtest.cpp only, which runs it on a stand-in of the setter. The includer defines FpNoteCall in its own anonymous namespace.
//
// The setter is thiscall: ECX the camera block, one pointer on the stack, ret 4; it reads EAX and EDX as nothing (its first use of EAX is
// "mov eax, ecx"). The hook looks only at its return address: unless it's one of the two calls of the batch camera (g_fpRelReturn,
// g_fpAbsReturn) it goes straight on into the game's own (MinHook's trampoline), every register and the stack as they came. For those two
// it calls FpNoteCall with the block, the matrix, EBX (the batch, which the batch camera keeps there), EBP (the batch camera's frame) and
// the stack as the setter was entered, keeping EAX, ECX and EDX (FpNoteCall keeps EBX, ESI, EDI and EBP as every function does), then goes
// on into the game's own with the stack as it came. Nothing of the game's is written.

#include <cstdint>

namespace {

void* g_fpSetterOriginal = nullptr;                  // the game's own setter past the hook (MinHook's trampoline)
uintptr_t g_fpRelReturn = 0, g_fpAbsReturn = 0;      // the two calls looked at (0 none)

void __stdcall FpNoteCall(const uint8_t* camera, const float* matrix, uintptr_t ebx, uintptr_t ebp, uintptr_t entry);

#if defined(_M_IX86)
__declspec(naked) void FpSetterHook() {
    __asm {
        push eax
        mov eax, dword ptr [esp + 4]  // the return address
        cmp eax, dword ptr [g_fpRelReturn]
        je noted
        cmp eax, dword ptr [g_fpAbsReturn]
        je noted
        pop eax
        jmp dword ptr [g_fpSetterOriginal]
    noted:
        push ecx
        push edx
        lea eax, [esp + 12]           // the stack as the setter was entered: its return address
        push eax                      // entry
        push ebp                      // the batch camera's frame
        push ebx                      // the batch
        push dword ptr [eax + 4]      // the matrix
        push ecx                      // the camera block
        call FpNoteCall               // (__stdcall: it takes its five off)
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [g_fpSetterOriginal]
    }
}
#endif

}  // namespace
