
#include <os.h>
#include <xen_traps.h>
#include <hypervisor.h>
//#include <mm.h>
//#include <lib.h>

/*
 * These are assembler stubs in entry.S.
 * They are the actual entry points for virtual exceptions.
 */
void divide_error(void);
void debug(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void simd_coprocessor_error(void);
void alignment_check(void);
void spurious_interrupt_bug(void);
void machine_check(void);


extern void do_exit(void);

void dump_regs(struct pt_regs *regs)
{
    xenprintf("FIXME: proper register dump (with the stack dump)\n");
}	


static void do_trap(int trapnr, char *str, struct pt_regs * regs, unsigned long error_code)
{
    xenprintf("FATAL:  Unhandled Trap %d (%s), error code=0x%lx\n", trapnr, str, error_code);
    xenprintf("Regs address %p\n", regs);
    dump_regs(regs);
    do_exit();
}

#define DO_ERROR(trapnr, str, name) \
void do_##name(struct pt_regs * regs, unsigned long error_code) \
{ \
	do_trap(trapnr, str, regs, error_code); \
}

#define DO_ERROR_INFO(trapnr, str, name, sicode, siaddr) \
void do_##name(struct pt_regs * regs, unsigned long error_code) \
{ \
	do_trap(trapnr, str, regs, error_code); \
}

DO_ERROR_INFO( 0, "divide error", divide_error, FPE_INTDIV, regs->eip)
DO_ERROR( 3, "int3", int3)
DO_ERROR( 4, "overflow", overflow)
DO_ERROR( 5, "bounds", bounds)
DO_ERROR_INFO( 6, "invalid operand", invalid_op, ILL_ILLOPN, regs->eip)
DO_ERROR( 7, "device not available", device_not_available)
DO_ERROR( 9, "coprocessor segment overrun", coprocessor_segment_overrun)
DO_ERROR(10, "invalid TSS", invalid_TSS)
DO_ERROR(11, "segment not present", segment_not_present)
DO_ERROR(12, "stack segment", stack_segment)
DO_ERROR_INFO(17, "alignment check", alignment_check, BUS_ADRALN, 0)
DO_ERROR(18, "machine check", machine_check)

void do_page_fault(struct pt_regs *regs, unsigned long error_code,
								                     unsigned long addr)
{
    xenprintf("Page fault at linear address %p\n", addr);
    dump_regs(regs);
#ifdef __x86_64__
    /* FIXME: _PAGE_PSE */
    {
        unsigned long *tab = (unsigned long *)start_info.pt_base;
        unsigned long page;
    
        xenprintf("Pagetable walk from %p:\n", tab);
        
        page = tab[l4_table_offset(addr)];
        tab = to_virt(mfn_to_pfn(pte_to_mfn(page)) << PAGE_SHIFT);
        xenprintf(" L4 = %p (%p)\n", page, tab);

        page = tab[l3_table_offset(addr)];
        tab = to_virt(mfn_to_pfn(pte_to_mfn(page)) << PAGE_SHIFT);
        xenprintf("  L3 = %p (%p)\n", page, tab);
        
        page = tab[l2_table_offset(addr)];
        tab =  to_virt(mfn_to_pfn(pte_to_mfn(page)) << PAGE_SHIFT);
        xenprintf("   L2 = %p (%p)\n", page, tab);
        
        page = tab[l1_table_offset(addr)];
        xenprintf("    L1 = %p\n", page);
    }
#endif
    //do_exit();
}

void do_general_protection(struct pt_regs *regs, long error_code)
{
    xenprintf("GPF %p, error_code=%lx\n", regs, error_code);
    dump_regs(regs);
    do_exit();
}


void do_debug(struct pt_regs * regs)
{
    xenprintf("Debug exception\n");
#define TF_MASK 0x100
    regs->eflags &= ~TF_MASK;
    dump_regs(regs);
    do_exit();
}

void do_coprocessor_error(struct pt_regs * regs)
{
    xenprintf("Copro error\n");
    dump_regs(regs);
    do_exit();
}

void simd_math_error(void *eip)
{
    xenprintf("SIMD error\n");
}

void do_simd_coprocessor_error(struct pt_regs * regs)
{
    xenprintf("SIMD copro error\n");
}

void do_spurious_interrupt_bug(struct pt_regs * regs)
{
}

/*
 * Submit a virtual IDT to teh hypervisor. This consists of tuples
 * (interrupt vector, privilege ring, CS:EIP of handler).
 * The 'privilege ring' field specifies the least-privileged ring that
 * can trap to that vector using a software-interrupt instruction (INT).
 */
static trap_info_t trap_table[] = {
    {  0, 0, __KERNEL_CS, (unsigned long)divide_error                },
    {  1, 0, __KERNEL_CS, (unsigned long)debug                       },
    {  3, 3, __KERNEL_CS, (unsigned long)int3                        },
    {  4, 3, __KERNEL_CS, (unsigned long)overflow                    },
    {  5, 3, __KERNEL_CS, (unsigned long)bounds                      },
    {  6, 0, __KERNEL_CS, (unsigned long)invalid_op                  },
    {  7, 0, __KERNEL_CS, (unsigned long)device_not_available        },
    {  9, 0, __KERNEL_CS, (unsigned long)coprocessor_segment_overrun },
    { 10, 0, __KERNEL_CS, (unsigned long)invalid_TSS                 },
    { 11, 0, __KERNEL_CS, (unsigned long)segment_not_present         },
    { 12, 0, __KERNEL_CS, (unsigned long)stack_segment               },
    { 13, 0, __KERNEL_CS, (unsigned long)general_protection          },
    { 14, 0, __KERNEL_CS, (unsigned long)page_fault                  },
    { 15, 0, __KERNEL_CS, (unsigned long)spurious_interrupt_bug      },
    { 16, 0, __KERNEL_CS, (unsigned long)coprocessor_error           },
    { 17, 0, __KERNEL_CS, (unsigned long)alignment_check             },
    { 18, 0, __KERNEL_CS, (unsigned long)machine_check               },
    { 19, 0, __KERNEL_CS, (unsigned long)simd_coprocessor_error      },
    {  0, 0,           0, 0                           }
};
    


void trap_init(void)
{
    HYPERVISOR_set_trap_table(trap_table);    
}

