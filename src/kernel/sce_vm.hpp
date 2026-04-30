#pragma once
#include <stdint.h>
#include "ui/ui.hpp"
struct __attribute__ ( ( packed ) ) SCEHeader {
 char magic[4] ;
 // Must be "SCE1"
 uint32_t code_size ;

 // Size of the bytecode payload
}
;

enum class SceOp : uint8_t
{
 HALT = 0x00 , PUSH = 0x01 , // PUSH <4-byte
 int> POP = 0x02 , ADD = 0x03 , SUB = 0x04 , MUL = 0x05 , DIV = 0x06 , CALL_UI = 0x10 // CALL_UI <1-byte func_id>
}
;

enum class SceUiCall : uint8_t
{
 DRAW_WINDOW = 0x01 , 
}
;

class SceVM
{
 private: static const int STACK_SIZE = 256 ;

 int32_t stack[STACK_SIZE] ;

 int sp ;

 // Stack pointer const
 uint8_t* code ;

 uint32_t ip ;

 // Instruction pointer
 uint32_t code_size ;

 void push ( int32_t val ) ;

 int32_t pop ( ) ;

 public: SceVM ( ) ;

 bool execute ( const uint8_t* file_buffer , uint32_t file_size , limine_framebuffer* fb ) ;

}
;