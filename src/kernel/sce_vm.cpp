#include "sce_vm.hpp"
SceVM::SceVM ( ) {
 sp = -1 ;
 ip = 0 ;
 code = nullptr ;
 code_size = 0 ;

}
void SceVM::push ( int32_t val ) {
 if ( sp < STACK_SIZE - 1 ) {
 stack[++sp] = val ;

 }
}
int32_t SceVM::pop ( ) {
 if ( sp >= 0 ) {
 return stack[sp--] ;

 }
return 0 ;

// Stack underflow }
bool SceVM::execute ( const
uint8_t* file_buffer , uint32_t file_size , limine_framebuffer* fb ) {
 if ( file_size < sizeof ( SCEHeader ) ) return false ;

 const SCEHeader* header = ( const SCEHeader* ) file_buffer ;

 if ( header->magic[0] != 'S' || header->magic[1] != 'C' || header->magic[2] != 'E' || header->magic[3] != '1' ) {
 return false ;

 // Invalid magic }
 code = file_buffer + sizeof ( SCEHeader ) ;

 code_size = header->code_size ;

 ip = 0 ;

 sp = -1 ;

 // Reset stack
 while ( ip < code_size ) {
 SceOp op = ( SceOp ) code[ip++] ;

 switch ( op ) {
 case SceOp::HALT:
 return true ;

 // Execution finished successfully case SceOp::PUSH: {
 int32_t val = * ( int32_t* ) &code[ip] ;

 ip += 4 ;

 push ( val ) ;

 break ;

 }
 case SceOp::POP: pop ( ) ;

 break ;

 case SceOp::ADD: {
 int32_t b = pop ( ) ;

 int32_t a = pop ( ) ;

 push ( a + b ) ;

 break ;

 }
 case SceOp::SUB: {
 int32_t b = pop ( ) ;

 int32_t a = pop ( ) ;

 push ( a - b ) ;

 break ;

 }
case SceOp::CALL_UI: {
 SceUiCall call_id = ( SceUiCall ) code[ip++] ;

 if ( call_id == SceUiCall::DRAW_WINDOW ) {
 // oRdEr OF pOPs is rEVERSE of PUSHes
 uint32_t color = ( uint32_t ) pop ( ) ;

 int32_t h = pop ( ) ;

 int32_t w = pop ( ) ;

 int32_t y = pop ( ) ;

 int32_t x = pop ( ) ;
 // in A REAL scEnariO , You'D paSS a tITle sTrInG PoinTER. // For noW , we haRdCOde IT tO "sce APP" draw_window ( fb , x , y , w , h , "SCE App" , color ) ;

 }
break ;

}
default: // unkNoWn OPcODe
return false ;

}
}
return true ;

}
