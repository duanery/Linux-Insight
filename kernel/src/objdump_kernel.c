/*
 * 内核反汇编函数。
 * 来自于kvm指令模拟模块中指令解码的部分。
 * 受限于kvm指令模拟的指令数量，因此不是所有的指令都能被解析。
 * 缺陷：
 *   只解析64位指令，只在centos 7.6上测试过。
**/

#include <linux/module.h>
#include <asm/kvm_host.h>
#include <asm/kvm_emulate.h>

#define __unused __attribute__((unused))

static char symbol[128] = "do_page_fault";
module_param_string(symbol, symbol, 128, 0644);

static int len = 16; 
module_param(len, int, 0644);

static int (*_x86_decode_insn)(struct x86_emulate_ctxt *ctxt, void *insn, int insn_len);
static struct x86_emulate_ctxt *ctxt;

static ulong __read_gpr(struct x86_emulate_ctxt *ctxt, unsigned reg)
{
    return 0;
}

static int __fetch(struct x86_emulate_ctxt *ctxt,
             unsigned long addr, void *val, unsigned int bytes,
             struct x86_exception *fault)
{
    memcpy(val, (void *)addr, bytes);
    return X86EMUL_CONTINUE;
}

static const struct x86_emulate_ops emulate_ops = {
    .read_gpr = __read_gpr,
    .fetch = __fetch,
};

static int decode_insn(void *insn)
{
    ctxt->eip = (unsigned long)insn;
    ctxt->mode = X86EMUL_MODE_PROT64;
    ctxt->interruptibility = 0;
    ctxt->have_exception = false;
    ctxt->exception.vector = -1;
    ctxt->perm_ok = false;
    memset(&ctxt->rip_relative, 0,
       (void *)&ctxt->modrm - (void *)&ctxt->rip_relative);
	ctxt->io_read.pos = 0;
	ctxt->io_read.end = 0;
	ctxt->mem_read.end = 0;
    return _x86_decode_insn(ctxt, NULL, 0);
}


// from emulate.c

#define OpCL               9ull  /* CL register (for shifts) */
#define OpOne             11ull  /* Implied 1 */
#define OpES              20ull  /* ES */
#define OpCS              21ull  /* CS */
#define OpSS              22ull  /* SS */
#define OpDS              23ull  /* DS */
#define OpFS              24ull  /* FS */
#define OpGS              25ull  /* GS */

#define OpBits             5  /* Width of operand field */
#define OpMask             ((1ull << OpBits) - 1)

#define ByteOp      (1<<0)	/* 8-bit operands. */
/* Source 2 operand type */
#define Src2Shift   (31)
#define Src2Mask    (OpMask << Src2Shift)
#define NearBranch  ((u64)1 << 52)  /* Near branches */

static __unused int insn_len(void *insn)
{
    int len = 0;
    if(decode_insn(insn) == EMULATION_OK) {
        len = ctxt->_eip - ctxt->eip;
    }
    return len;
}
static int has_0x66prefix(void)
{
    int i = 0;
    u8 b;
    /* Legacy prefixes. */
	for (;;) {
		switch (b = ctxt->fetch.data[i++]) {
		case 0x66:	/* operand-size override */
            return 1;
		case 0x67:	/* address-size override */
		case 0x26:	/* ES override */
		case 0x2e:	/* CS override */
		case 0x36:	/* SS override */
		case 0x3e:	/* DS override */
		case 0x64:	/* FS override */
		case 0x65:	/* GS override */
			break;
		case 0x40 ... 0x4f: /* REX */
			if (ctxt->mode != X86EMUL_MODE_PROT64)
				goto done_prefixes;
			continue;
		case 0xf0:	/* LOCK */
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			break;
		default:
			goto done_prefixes;
		}
	}
done_prefixes:
    return 0;
}
static __unused int has_0x67prefix(void)
{
    if(ctxt->ad_bytes == 4) return 1;
    return 0;
}
static int has_seg_override(void)
{
    int i = 0;
    u8 b;
    /* Legacy prefixes. */
	for (;;) {
		switch (b = ctxt->fetch.data[i++]) {
		case 0x66:	/* operand-size override */
		case 0x67:	/* address-size override */
            break;
		case 0x26:	/* ES override */
		case 0x2e:	/* CS override */
		case 0x36:	/* SS override */
		case 0x3e:	/* DS override */
		case 0x64:	/* FS override */
		case 0x65:	/* GS override */
			return 1;
		case 0x40 ... 0x4f: /* REX */
			if (ctxt->mode != X86EMUL_MODE_PROT64)
				goto done_prefixes;
			continue;
		case 0xf0:	/* LOCK */
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			break;
		default:
			goto done_prefixes;
		}
	}
done_prefixes:
    return 0;
}
static u8 insn_sib(void)
{
    int i = 0;
    u8 b;
    /* Legacy prefixes. */
	for (;;) {
		switch (b = ctxt->fetch.data[i++]) {
		case 0x66:	/* operand-size override */
		case 0x67:	/* address-size override */
		case 0x26:	/* ES override */
		case 0x2e:	/* CS override */
		case 0x36:	/* SS override */
		case 0x3e:	/* DS override */
		case 0x64:	/* FS override */
		case 0x65:	/* GS override */
			break;
		case 0x40 ... 0x4f: /* REX */
			if (ctxt->mode != X86EMUL_MODE_PROT64)
				goto done_prefixes;
			continue;
		case 0xf0:	/* LOCK */
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			break;
		default:
			goto done_prefixes;
		}
	}
done_prefixes:
    return ctxt->fetch.data[i+ctxt->opcode_len];
}
static void printk_reg(int mem, int reg)
{
    const char *prefix="";
    static const char *none[16] = {NULL,};
    static const char *gp64[16] = {"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                                    "r8","r9","r10","r11","r12","r13","r14","r15"};
    static const char *gp32[16] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi",
                                   "r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d"};
    static const char *gp16[16] = {"ax","cx","dx","bx","sp","bp","si","di",
                                   "r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w"};
    static const char *gp8[16] = {"al","cl","dl","bl","ah","ch","dh","bh",
                                   "r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b"};
    const char **sse = none;
    const char **mmx = none;
    const char **this;
    
    if(mem == OP_XMM) {
        prefix = "xmm";
        this = sse;
    }else if(mem == OP_MM) {
        prefix = "mm";
        this = mmx;
    }else{
        if(mem == OP_MEM) this = gp64;
        else if(mem == OP_REG) {
            int byte_regs = (ctxt->rex_prefix == 0) && (ctxt->d & ByteOp);
            if(byte_regs) {
                if(reg & 1) reg += 4*sizeof(ctxt->memop.addr.reg);
                reg /= sizeof(ctxt->memop.addr.reg);
                this = gp8;
            }else {
                switch(ctxt->op_bytes) {
                    case 1: this = gp8; break;
                    case 2: this = gp16;  break;
                    case 4: this = gp32; break;
                    default:this = gp64; break;
                }
                reg /= sizeof(ctxt->memop.addr.reg);
            }
        }else
            return;
    }
    if(this[reg])
        printk("%%%s%s", prefix, this[reg]);
    else
        printk("%%%s%d", prefix, reg);
}
static void printk_sreg(int sreg)
{
    switch(sreg) {
        case VCPU_SREG_ES: printk("%%es"); break;
        case VCPU_SREG_CS: printk("%%cs"); break;
        case VCPU_SREG_SS: printk("%%ss"); break;
        case VCPU_SREG_DS: printk("%%ds"); break;
        case VCPU_SREG_FS: printk("%%fs"); break;
        case VCPU_SREG_GS: printk("%%gs"); break;
    }
}
static void printk_eee(const char *prefix, const char *suffix)
{
    int eee = ctxt->modrm_reg;
    printk("%s%d%s", prefix, eee, suffix);
}
static void printk_operand(struct operand *op)
{
    switch(op->type) {
        case OP_REG:
            printk_reg(op->type, (void *)op->addr.reg - (void *)ctxt->_regs);
            break;
        case OP_XMM:
        case OP_MM:
            printk_reg(op->type, op->addr.mm);
            break;
        case OP_MEM:
            if(has_seg_override()) {
                printk_sreg(ctxt->seg_override);
                printk(":");
            }
            //disp
            switch(ctxt->modrm_mod) {
                case 1: printk("0x%x", (unsigned char)op->addr.mem.ea); break;
                case 2: printk("0x%x", (unsigned int)op->addr.mem.ea); break;
            }
            if ((ctxt->modrm_rm & 7) == 4) {
                u8 sib = insn_sib();
                int index_reg, base_reg, scale;
                index_reg = (ctxt->rex_prefix << 2) & 8; /* REX.X */
                base_reg = (ctxt->rex_prefix << 3) & 8; /* REX.B */
                index_reg |= (sib >> 3) & 7;
                base_reg |= sib & 7;
                scale = sib >> 6;
                if ((base_reg & 7) == 5 && ctxt->modrm_mod == 0)
                    if(index_reg == 4) {
                        printk("0x%x", (unsigned int)op->addr.mem.ea);
                        break;
                    }else
                        printk("0x%x(",(unsigned int)op->addr.mem.ea);
                else {
                    printk("("); printk_reg(OP_MEM, base_reg);
                }
                if(index_reg != 4) {
                    printk(","); printk_reg(OP_MEM, index_reg); printk(",%d", 1<<scale);
                }
                printk(")");
            } else if ((ctxt->modrm_rm & 7) == 5 && ctxt->modrm_mod == 0) {
                printk("0x%x(%%rip)", (unsigned int)(op->addr.mem.ea - ctxt->eip));
            }else{
                printk("(");
                printk_reg(OP_MEM, ctxt->modrm_rm);
                printk(")");
            }
            break;
        case OP_IMM:
            if(ctxt->d & NearBranch) {
                u64 dstip = (u64)op->val+ctxt->_eip;
                printk("0x%llx", dstip);
                print_symbol("        # %s", dstip);
            } else {
                int src2 = (ctxt->d & Src2Mask) >> Src2Shift;
                switch(src2) {
                case OpOne:
                    op->type = OP_NONE;
                    break;
                case OpCL:
                    printk("%%cl");
                    break;
                case OpES ... OpGS:
                    printk_sreg(op->val);
                    break;
                default:
                    switch(op->bytes) {
                        case 1: printk("$0x%x", (u8)op->val); break;
                        case 2: printk("$0x%x", (u16)op->val); break;
                        case 4: printk("$0x%x", (u32)op->val); break;
                        case 8: printk("$0x%llx", (u64)op->val); break;
                    }
                }
            }
            break;
        default:
            return ;
    }
}

static void printk_insn_src2op(void)
{
    struct operand *src2 = &ctxt->src2;
    printk_operand(src2);
}
static void printk_insn_srcop(void)
{
    struct operand *src = &ctxt->src;
    struct operand *src2 = &ctxt->src2;
    if(src2->type != OP_NONE && src->type != OP_NONE) printk(",");
    printk_operand(src);
}
static void printk_insn_dstop(void)
{
    struct operand *src = &ctxt->src;
    struct operand *src2 = &ctxt->src2;
    struct operand *dst = &ctxt->dst;
    if((src->type != OP_NONE || src2->type != OP_NONE)
        && dst->type != OP_NONE) printk(",");
    printk_operand(dst);
}
static void printk_byteop(const char *op)
{
    int w = 7-(int)strlen(op);
    if(ctxt->d & ByteOp)
        printk("%sb%*s",op, w-1, "");
    else
        printk("%s%*s",op, w, "");
}

static void printk_group1(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("add    "); break;
        case 1: printk("or     "); break;
        case 2: printk("adc    "); break;
        case 3: printk("sbb    "); break;
        case 4: printk("and    "); break;
        case 5: printk("sub    "); break;
        case 6: printk("xor    "); break;
        case 7: printk("cmp    "); break;
    }
}
static void printk_cc(const char *prefix, int j)
{
    const char *cc[16] = {"o", "no", "b", "nb", "e", "ne", "na", "a",
                          "s", "ns", "p", "np", "l", "nl", "ng", "g",};
    int l = strlen(prefix) + strlen(cc[j]);
    l = 7-l;
    printk("%s%s%*s", prefix, cc[j], l<1?1:l, "");
}
static int printk_group1a(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("pop    "); return 0;
        default: return -1;
    }
}

static void printk_group2(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("rol    "); break;
        case 1: printk("ror    "); break;
        case 2: printk("rcl    "); break;
        case 3: printk("rcr    "); break;
        case 4: printk("shl    "); break;
        case 5: printk("shr    "); break;
        case 6: printk("shl    "); break;
        case 7: printk("sar    "); break;
    }
}
static void printk_group3(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk_byteop("test"); break;
        case 1: printk_byteop("test"); break;
        case 2: printk("not    "); break;
        case 3: printk("neg    "); break;
        case 4: printk("mul    "); break;
        case 5: printk("imul   "); break;
        case 6: printk("div    "); break;
        case 7: printk("idiv   "); break;
    }
}
static void printk_group4(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("inc    "); break;
        case 1: printk("dec    "); break;
    }
}
static void printk_group5(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("inc    "); break;
        case 1: printk("dec    "); break;
        case 2: printk("call   "); break;
        case 3: printk("call   "); break;
        case 4: printk("jmp    "); break;
        case 5: printk("jmp    "); break;
        case 6: printk("push   "); break;
    }
}
static void printk_group6(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch (goffset) {
        case 0: printk("sldt   "); break;
        case 1: printk("str    "); break;
        case 2: printk("lldt   "); break;
        case 3: printk("ltr    "); break;
        case 4: printk("verr   "); break;
        case 5: printk("verw   "); break;
    }
}
static int printk_group7(void)
{
    int ret = 0;
    int goffset = (ctxt->modrm >> 3) & 7;
    if ((ctxt->modrm >> 6) != 3) { //mem
        switch(goffset) {
            case 0: printk("sgdt   "); break;
            case 1: printk("sidt   "); break;
            case 2: printk("lgdt   "); break;
            case 3: printk("lidt   "); break;
            case 4: printk("smsw   "); break;
            case 5: ret = -1; break;
            case 6: printk("lmsw   "); break;
            case 7: printk("invlpg "); break;
        }
    }else{
        const char *case0[8]={NULL,"vmcall  ","vmlaunch ","vmresume ","vmxoff ",NULL,NULL,NULL};
        const char *case1[8]={"monitor ","mwait  ","clac   ","stac  ",NULL,NULL,NULL,"encls  "};
        const char *case2[8]={"xgetbv ","xsetbv ",NULL,NULL,"vmfunc ","xend   ","xtest  ","enclu  "};
        const char *case7[8]={"swapgs ","rdtscp ",NULL,NULL,NULL,NULL,NULL,NULL};
        int i = (ctxt->modrm & 7);
        switch(goffset) {
            case 0: if(case0[i]) printk(case0[i]); else ret = -1; break;
            case 1: if(case1[i]) printk(case1[i]); else ret = -1; break;
            case 2: if(case2[i]) printk(case2[i]); else ret = -1; break;
            case 7: if(case7[i]) printk(case7[i]); else ret = -1; break;
            default:
                ret = -1;
        }
    }
    return ret;
}
static int printk_group8(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch(goffset) {
        case 4: printk("bt     "); return 0;
        case 5: printk("bts    "); return 0;
        case 6: printk("btr    "); return 0;
        case 7: printk("btc    "); return 0;
        default: return -1;
    }
}
static int printk_group9(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    if ((ctxt->modrm >> 6) != 3) { //mem
        if(has_0x66prefix()) {
            switch(goffset) {
                case 6: printk("vmclear "); return 0;
            }
        }else if(ctxt->rep_prefix == 0xf3) {
            switch(goffset) {
                case 6: printk("vmxon  "); return 0;
            }
        }else{
            switch(goffset) {
                case 1: printk("cmpxch%db ", ctxt->memop.bytes); return 0;
                case 6: printk("cmptrld "); return 0;
                case 7: printk("cmptrst "); return 0;
            }
        }
    }else{
        
    }
    return -1;
}
static int printk_group11(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    switch(goffset) {
        case 0: printk("mov    "); return 0;
        default: return -1;
    }
}
static int printk_group15(void)
{
    int goffset = (ctxt->modrm >> 3) & 7;
    if ((ctxt->modrm >> 6) != 3) //mem
        switch(goffset) {
            case 0: printk("fxsave "); return 0;
            case 1: printk("fxstor "); return 0;
            case 7: if(!has_0x66prefix() && ctxt->rep_prefix==0) printk("clflush ");
            default: return -1;
        }
    else return -1;
}
static void printk_insn(void)
{
    if (ctxt->opcode_len == 1) {
        switch (ctxt->b) {
            case 0x00 ... 0x05: printk("add    "); break;
            case 0x08 ... 0x0d: printk("or     "); break;
            case 0x10 ... 0x15: printk("adc    "); break;
            case 0x18 ... 0x1d: printk("sbb    "); break;
            case 0x07         :
            case 0x17         :
            case 0x1f         : printk("pop    "); break;
            case 0x20 ... 0x25: printk("and    "); break;
            case 0x28 ... 0x2d: printk("sub    "); break;
            case 0x2f:          printk("das    "); break;
            case 0x30 ... 0x35: printk("xor    "); break;
            case 0x38 ... 0x3d: printk("cmp    "); break;
            case 0x40 ... 0x47: printk("inc    "); break;
            case 0x48 ... 0x4f: printk("dec    "); break;
            case 0x06         :
            case 0x0e         :
            case 0x16         :
            case 0x1e         :
            case 0x50 ... 0x57: printk("push   "); break;
            case 0x58 ... 0x5f: printk("pop    "); break;
            case 0x60         : printk("pusha  "); break;
            case 0x61         : printk("popa   "); break;
            case 0x63: if (ctxt->mode == X86EMUL_MODE_PROT64) printk("movsxd "); else printk("arpl   "); break;
            case 0x68         : printk("push   "); break;
            case 0x69         : printk("imul   "); break;
            case 0x6a         : printk("push   "); break;
            case 0x6b         : printk("imul   "); break;
            case 0x6c ... 0x6d: printk("ins    "); break;
            case 0x6e ... 0x6f: printk("outs   "); break;
            case 0x70 ... 0x7f: printk_cc("j", ctxt->b&0x0f); break;
            case 0x80 ... 0x83: printk_group1();   break;
            case 0x84 ... 0x85: printk("test   "); break;
            case 0x86 ... 0x87: printk("xchg   "); break;
            case 0x88 ... 0x8b: printk("mov    "); break;
            case 0x8c:          printk("mov    "); printk_sreg(ctxt->modrm_reg); break;
            case 0x8d:          printk("lea    "); break;
            case 0x8e:          printk("mov    "); 
                                printk_insn_srcop();printk(",");
                                printk_sreg(ctxt->modrm_reg);
                                return;
            case 0x8f: if(printk_group1a() == 0) break; else return;
            case 0x90: if(ctxt->rep_prefix == 0xf3) {printk("pause"); return;}
                       else if(ctxt->rex_prefix == 0) {printk("nop"); return;}
            case 0x91 ... 0x97: printk("xchg   "); break;
            case 0x98         : printk("cwde   "); return;
            case 0x99         : printk("cdq    "); return;
            case 0x9a         : printk("lcall  "); break;
            case 0x9c         : printk("pushf  "); return;
            case 0x9d         : printk("popf   "); return;
            case 0x9e         : printk("sahf   "); return;
            case 0x9f         : printk("lahf   "); return;
            case 0xa0 ... 0xa5: printk("mov    "); break;
            case 0xa6 ... 0xa7: printk("cmp    "); break;
            case 0xa8 ... 0xa9: printk("test   "); break;
            case 0xaa ... 0xad: printk("mov    "); break;
            case 0xae ... 0xaf: printk("cmp    "); break;
            case 0xb0 ... 0xbf: printk("mov    "); break;
            case 0xc0 ... 0xc1: printk_group2();   break;
            case 0xc2 ... 0xc3: printk("ret    "); break;
            case 0xc4         : printk("les    ");
                                ctxt->src2.type = OP_NONE; break;
            case 0xc5         : printk("lds    ");
                                ctxt->src2.type = OP_NONE; break;
            case 0xc6 ... 0xc7: if(printk_group11() == 0) break; else return;
            case 0xc8         : printk("enter  "); break;
            case 0xc9         : printk("leave  "); break;
            case 0xcd         : printk("int    "); break;
            case 0xcf         : printk("iret   "); break;
            case 0xd0 ... 0xd3: printk_group2();   break;
            case 0xd4         : printk("aam    "); break;
            case 0xd5         : printk("aad    "); break;
            case 0xd6         : printk("salc   "); break;
            case 0xd7         : printk("mov    "); break;
            case 0xd9         : return; //TODO escape_d9
            case 0xdb         : return; //TODO escape_db
            case 0xdd         : return; //TODO escape_dd
            case 0xe0         : printk("loopne "); break;
            case 0xe1         : printk("loope  "); break;
            case 0xe2         : printk("loop   "); break;
            case 0xe4 ... 0xe5: printk("in     "); break;
            case 0xe6 ... 0xe7: printk("out    "); break;
            case 0xe8         : printk("call   "); break;
            case 0xe9         : printk("jmp    "); break;
            case 0xea         : printk("ljmp   "); break;
            case 0xeb         : printk("jmp    "); break;
            case 0xec ... 0xed: printk("in     "); break;
            case 0xee ... 0xef: printk("out    "); break;
            case 0xf1         : printk("int1   "); break;
            case 0xf4         : printk("hlt    "); break;
            case 0xf5         : printk("cmc");     break;
            case 0xf6 ... 0xf7: printk_group3();   break;
            case 0xf8         : printk("clc");     break;
            case 0xf9         : printk("stc");     break;
            case 0xfa         : printk("cli");     break;
            case 0xfb         : printk("sti");     break;
            case 0xfc         : printk("cld");     break;
            case 0xfd         : printk("std");     break;
            case 0xfe         : printk_group4();   break;
            case 0xff         : printk_group5();   break;
            default:
                return;
        }
    }else if(ctxt->opcode_len == 2) {
        switch(ctxt->b) {
            case 0x00         : printk_group6();   break;
            case 0x01         : if(printk_group7()==0) break; else return;
            case 0x05         : printk("syscall"); break;
            case 0x06         : printk("clts");    break;
            case 0x07         : printk("sysret");  break;
            case 0x08         : printk("invd");    break;
            case 0x09         : printk("wbinvd");  break;
            case 0x0d         : printk("prefetchw"); break;
            case 0x18         : printk("prefetch");ctxt->modrm_reg--;printk_eee("","");printk(" "); break;
            case 0x1f         : printk("nopl   "); break;
            case 0x20         : printk("mov    ");printk_eee("%cr",","); break;
            case 0x21         : printk("mov    ");printk_eee("%dr",","); break;
            case 0x22         : printk("mov    ");printk_insn_srcop();printk_eee(",%cr",""); return;
            case 0x23         : printk("mov    ");printk_insn_srcop();printk_eee(",%dr",""); return; 
            case 0x28 ... 0x29: if(has_0x66prefix()) printk("movapd "); else printk("movaps "); break;
            case 0x2b         : if(has_0x66prefix()) printk("movntpd "); else printk("movntps "); break;
            case 0x30         : printk("wrmsr");   break;
            case 0x31         : printk("rdtsc");   break;
            case 0x32         : printk("rdmsr");   break;
            case 0x33         : printk("rdpmc");   break;
            case 0x34         : printk("sysenter");break;
            case 0x35         : printk("sysexit"); break;
            case 0x37         : printk("getsec");  break;
            case 0x40 ... 0x4f: printk_cc("cmov",ctxt->b&0x0f); break;
            case 0x80 ... 0x8f: printk_cc("j",ctxt->b&0x0f); break;
            case 0x90 ... 0x9f: printk_cc("set",ctxt->b&0x0f); break;
            case 0xa0         : printk("push   "); break;
            case 0xa1         : printk("pop    "); break;
            case 0xa2         : printk("cpuid  "); break;
            case 0xa3         : printk("bt     "); break;
            case 0xa4 ... 0xa5: printk("shld   "); break;
            case 0xa8         : printk("push   "); break;
            case 0xa9         : printk("pop    "); break;
            case 0xaa         : printk("rsm    "); break;
            case 0xab         : printk("bts    "); break;
            case 0xac ... 0xad: printk("shrd   "); break;
            case 0xae         : if(printk_group15()==0) break; else return;
            case 0xaf         : printk("imul   "); break;
            case 0xb0 ... 0xb1: printk("cmpxchg "); break;
            case 0xb2         : printk("lss    "); ctxt->src2.type = OP_NONE; break;
            case 0xb3         : printk("btr    "); break;
            case 0xb4         : printk("lfs    "); ctxt->src2.type = OP_NONE; break;
            case 0xb5         : printk("lgs    "); ctxt->src2.type = OP_NONE; break;
            case 0xb6 ... 0xb7: printk("movzx  "); break;
            case 0xba         : if(printk_group8()==0) break; else return;
            case 0xbb         : printk("btc    "); break;
            case 0xbc         : printk("bsf    "); break;
            case 0xbd         : printk("bsr    "); break;
            case 0xbe ... 0xbf: printk("movsx  "); break;
            case 0xc0 ... 0xc1: printk("xadd   "); break;
            case 0xc7         : if(printk_group9()==0) break; else return;
            case 0xc8 ... 0xcf: printk("bswap  "); break;
            default:
                return;
        }
    }else 
        return;
    printk_insn_src2op();
    printk_insn_srcop();
    printk_insn_dstop();
}

static int insn_dump(void *insn)
{
    int len, m;
    if(decode_insn(insn) != EMULATION_OK)
        return -1;
    len = ctxt->_eip - ctxt->eip;
    printk("%p: ", insn);
    for(m=0; m<len; m++) printk("%02x ", *((const unsigned char *)insn++));
    for(   ; m<10 ; m++) printk("   ");
    printk_insn();
    printk("\n");
    return len;
}

static void __objdump(void *insn, int count)
{
    int len, i;
    if(!insn) return;
    for(i = 0; i < count; i++) {
        len = insn_dump(insn);
        if(len < 0) break;
        insn += len;
    }
}
EXPORT_SYMBOL(__objdump);

static void __objdump_region(void *insn, void *end)
{
    int len;
    if(!insn) return;
    while(insn < (void*)end) {
        len = insn_dump(insn);
        if(len < 0) break;
        insn += len;
    }
}
EXPORT_SYMBOL(__objdump_region);

static void objdump(void)
{
    void *ip = (void *)kallsyms_lookup_name(symbol);
    __objdump(ip, len);
}

//指令测试函数
static __unused void objdump_test(void)
{
    unsigned char buff[] = {0x44,0x89,0xef,0x90,0x74,0xd2,0xc2,0x12,0x34,0xcd,0x90,
    0xe0,0x05,
    0x06, 0x0e, 0x16, 0x1e,
    0x07, 0x17, 0x1f,
    0xe8,0x01,0x00,0x00,0x00,
    0xf6,0x83,0x88,0,0,0,3,
    0xf6,0,3,
    0x0f,0x1f,0x40,0,
    0x05,1,0,0,0,
    0x8e,0x00,
    0x45,0x31,0xf6,
    0x0f,0xc7,0x08,
    0x69,8,1,0,0,0,
    0x40|0x1,0x90,
    0x48,0x98,
    0x99, 0x48,0x99,
    0x9c,0x9d,0x9e,0x9f,
    0xf1,
    0x0f,0x22,0xd0,
    0x0f,0x18,0x08,
    /*0x0f,0x28,0x00,   0x66,0x0f,0x28,0,
    0x0f,0x29,0x00,   0x66,0x0f,0x29,0,
    0x0f,0x2b,0x00,   0x66,0x0f,0x2b,0,*/
    0x0f,0x30,  0x0f,0x31,  0x0f,0x32,  0x0f,0x33, 0x0f,0x34, 0x0f,0x35, //0x0f,0x37,
    0x0f,0x45,0,
    0x0f,0x85,0,1,2,3,
    0x0f,0x95,0,
    0x0f,0xa0,  0x0f,0xa1,  0x0f,0xa2,
    0x0f,0xa3,0,
    0x0f,0xa4,0,2,
    0x0f,0xad,0,
    0x0f,0xaf,0,
    0x0f,0xb2,0,
    0x0f,0xba,0x38,1,
    0x0f,0xbc,0,
    0x0f,0xbe,0,
    0x0f,0xc0,0x10, 0x0f,0xc0,0x30,
    0x0f,0xc9,
    };
    __objdump_region((void *)buff, (void*)buff + sizeof(buff));
}

static int objdump_init(void)
{
    _x86_decode_insn = (void *)kallsyms_lookup_name("x86_decode_insn");
    if(!_x86_decode_insn)
        return 0;
    ctxt = kmalloc(sizeof(*ctxt), GFP_KERNEL|__GFP_ZERO);
    if(!ctxt) return 0;
    ctxt->ops = &emulate_ops;
    objdump();
    return 0;
}

static void objdump_exit(void)
{
    if(ctxt)
        kfree(ctxt);
}

module_init(objdump_init);
module_exit(objdump_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duanery");
MODULE_DESCRIPTION("objdump: disassembler kernel function");
