void doop(Mips * emu, uint32_t op) {
    switch(op & 0xfc000000) {
        case 0x8000000:
            op_j(emu,op);
            return;
        case 0xc000000:
            op_jal(emu,op);
            return;
        case 0x10000000:
            op_beq(emu,op);
            return;
        case 0x14000000:
            op_bne(emu,op);
            return;
        case 0x18000000:
            op_blez(emu,op);
            return;
        case 0x1c000000:
            op_bgtz(emu,op);
            return;
        case 0x20000000:
            op_addi(emu,op);
            return;
        case 0x24000000:
            op_addiu(emu,op);
            return;
        case 0x28000000:
            op_slti(emu,op);
            return;
        case 0x2c000000:
            op_sltiu(emu,op);
            return;
        case 0x30000000:
            op_andi(emu,op);
            return;
        case 0x34000000:
            op_ori(emu,op);
            return;
        case 0x38000000:
            op_xori(emu,op);
            return;
        case 0x3c000000:
            op_lui(emu,op);
            return;
        case 0x50000000:
            op_beql(emu,op);
            return;
        case 0x54000000:
            op_bnel(emu,op);
            return;
        case 0x58000000:
            op_blezl(emu,op);
            return;
        case 0x80000000:
            op_lb(emu,op);
            return;
        case 0x84000000:
            op_lh(emu,op);
            return;
        case 0x88000000:
            op_lwl(emu,op);
            return;
        case 0x8c000000:
            op_lw(emu,op);
            return;
        case 0x90000000:
            op_lbu(emu,op);
            return;
        case 0x94000000:
            op_lhu(emu,op);
            return;
        case 0x98000000:
            op_lwr(emu,op);
            return;
        case 0xa0000000:
            op_sb(emu,op);
            return;
        case 0xa4000000:
            op_sh(emu,op);
            return;
        case 0xa8000000:
            op_swl(emu,op);
            return;
        case 0xac000000:
            op_sw(emu,op);
            return;
        case 0xb8000000:
            op_swr(emu,op);
            return;
        case 0xbc000000:
            op_cache(emu,op);
            return;
        case 0xc0000000:
            op_ll(emu,op);
            return;
        case 0xcc000000:
            op_pref(emu,op);
            return;
        case 0xe0000000:
            op_sc(emu,op);
            return;
    }
    switch(op & 0xfc00003f) {
        case 0x0:
            op_sll(emu,op);
            return;
        case 0x2:
            op_srl(emu,op);
            return;
        case 0x3:
            op_sra(emu,op);
            return;
        case 0x4:
            op_sllv(emu,op);
            return;
        case 0x6:
            op_srlv(emu,op);
            return;
        case 0x7:
            op_srav(emu,op);
            return;
        case 0x8:
            op_jr(emu,op);
            return;
        case 0x9:
            op_jalr(emu,op);
            return;
        case 0xc:
            op_syscall(emu,op);
            return;
        case 0xf:
            op_sync(emu,op);
            return;
        case 0x10:
            op_mfhi(emu,op);
            return;
        case 0x11:
            op_mthi(emu,op);
            return;
        case 0x12:
            op_mflo(emu,op);
            return;
        case 0x13:
            op_mtlo(emu,op);
            return;
        case 0x18:
            op_mult(emu,op);
            return;
        case 0x19:
            op_multu(emu,op);
            return;
        case 0x1a:
            op_div(emu,op);
            return;
        case 0x1b:
            op_divu(emu,op);
            return;
        case 0x20:
            op_add(emu,op);
            return;
        case 0x21:
            op_addu(emu,op);
            return;
        case 0x22:
            op_sub(emu,op);
            return;
        case 0x23:
            op_subu(emu,op);
            return;
        case 0x24:
            op_and(emu,op);
            return;
        case 0x25:
            op_or(emu,op);
            return;
        case 0x26:
            op_xor(emu,op);
            return;
        case 0x27:
            op_nor(emu,op);
            return;
        case 0x2a:
            op_slt(emu,op);
            return;
        case 0x2b:
            op_sltu(emu,op);
            return;
	case 0x2d:
	    op_addu(emu,op); //daddu
	    return;
        case 0x36:
            op_tne(emu,op);
            return;
    }
    switch(op & 0xfc1f0000) {
        case 0x4000000:
            op_bltz(emu,op);
            return;
        case 0x4010000:
            op_bgez(emu,op);
            return;
        case 0x4020000:
            op_bltzl(emu,op);
            return;
        case 0x4030000:
            op_bgezl(emu,op);
            return;
        case 0x4100000:
            op_bltzal(emu,op);
            return;
        case 0x4110000:
            op_bgezal(emu,op);
            return;
        case 0x5c000000:
            op_bgtzl(emu,op);
            return;
    }
    switch(op & 0xffffffff) {
        case 0x42000002:
            op_tlbwi(emu,op);
            return;
        case 0x42000006:
            op_tlbwr(emu,op);
            return;
        case 0x42000008:
            op_tlbp(emu,op);
            return;
        case 0x42000018:
            op_eret(emu,op);
            return;
    }
    switch(op & 0xfc0007ff) {
        case 0xa:
            op_movz(emu,op);
            return;
        case 0xb:
            op_movn(emu,op);
            return;
        case 0x70000002:
            op_mul(emu,op);
            return;
    }
    switch(op & 0xffe00000) {
        case 0x40000000:
            op_mfc0(emu,op);
            return;
        case 0x40800000:
            op_mtc0(emu,op);
            return;
    }
    switch(op & 0xfe00003f) {
        case 0x42000020:
            op_wait(emu,op);
            return;
    }
    // printf("unhandled opcode at %x -> %x\n",emu->pc,op);
    setExceptionCode(emu,EXC_RI);
    emu->exceptionOccured = 1;
    return;
}
