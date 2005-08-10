//----------------------------------------------------------------------
//  $Id: head.S,v 1.1 2005/08/01 08:27:05 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: head.S,v $
//
//----------------------------------------------------------------------


 
#include <os.h>

.equ HAHA_JETZT, 4567
 
.section __xen_guest
	.ascii	"GUEST_OS=linux,GUEST_VER=2.6,XEN_VER=2.0.6,VIRT_BASE=0xC0000000"
	.ascii	",LOADER=generic"
	.ascii	",PT_MODE_WRITABLE"
	.byte	0
 
.text

.globl _start, shared_info
                        
_start:
        cld
        lss stack_start,%esp
        push %esi 
        call start_kernel

stack_start:
	.long stack+8192, __KERNEL_DS

        /* Unpleasant -- the PTE that maps this page is actually overwritten */
        /* to map the real shared-info page! :-)                             */
        .org 0x1000
shared_info:
        .org 0x2000	