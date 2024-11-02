/* Generated automatically by the program 'build/genpreds'
   from the machine description file '../../riscv-gcc/gcc/config/riscv/riscv.md'.  */

#ifndef GCC_TM_PREDS_H
#define GCC_TM_PREDS_H

#ifdef HAVE_MACHINE_MODES
extern int general_operand (rtx, machine_mode);
extern int address_operand (rtx, machine_mode);
extern int register_operand (rtx, machine_mode);
extern int pmode_register_operand (rtx, machine_mode);
extern int scratch_operand (rtx, machine_mode);
extern int immediate_operand (rtx, machine_mode);
extern int const_int_operand (rtx, machine_mode);
extern int const_scalar_int_operand (rtx, machine_mode);
extern int const_double_operand (rtx, machine_mode);
extern int nonimmediate_operand (rtx, machine_mode);
extern int nonmemory_operand (rtx, machine_mode);
extern int push_operand (rtx, machine_mode);
extern int pop_operand (rtx, machine_mode);
extern int memory_operand (rtx, machine_mode);
extern int indirect_operand (rtx, machine_mode);
extern int ordered_comparison_operator (rtx, machine_mode);
extern int comparison_operator (rtx, machine_mode);
extern int const_arith_operand (rtx, machine_mode);
extern int arith_operand (rtx, machine_mode);
extern int lui_operand (rtx, machine_mode);
extern int sfb_alu_operand (rtx, machine_mode);
extern int const_csr_operand (rtx, machine_mode);
extern int csr_operand (rtx, machine_mode);
extern int csr_address (rtx, machine_mode);
extern int sle_operand (rtx, machine_mode);
extern int sleu_operand (rtx, machine_mode);
extern int const_0_operand (rtx, machine_mode);
extern int const_1_operand (rtx, machine_mode);
extern int reg_or_0_operand (rtx, machine_mode);
extern int reg_or_mem_operand (rtx, machine_mode);
extern int uimm5_operand (rtx, machine_mode);
extern int reg_or_uimm5_operand (rtx, machine_mode);
extern int reg_or_simm5_operand (rtx, machine_mode);
extern int branch_on_bit_operand (rtx, machine_mode);
extern int splittable_const_int_operand (rtx, machine_mode);
extern int p2m1_shift_operand (rtx, machine_mode);
extern int high_mask_shift_operand (rtx, machine_mode);
extern int move_operand (rtx, machine_mode);
extern int symbolic_operand (rtx, machine_mode);
extern int absolute_symbolic_operand (rtx, machine_mode);
extern int plt_symbolic_operand (rtx, machine_mode);
extern int call_insn_operand (rtx, machine_mode);
extern int modular_operator (rtx, machine_mode);
extern int equality_operator (rtx, machine_mode);
extern int ltge_operator (rtx, machine_mode);
extern int vector_comparison_operator (rtx, machine_mode);
extern int order_operator (rtx, machine_mode);
extern int signed_order_operator (rtx, machine_mode);
extern int fp_native_comparison (rtx, machine_mode);
extern int fp_scc_comparison (rtx, machine_mode);
extern int fp_branch_comparison (rtx, machine_mode);
extern int gpr_save_operation (rtx, machine_mode);
extern int single_bit_mask_operand (rtx, machine_mode);
extern int not_single_bit_mask_operand (rtx, machine_mode);
extern int const31_operand (rtx, machine_mode);
extern int const63_operand (rtx, machine_mode);
extern int const_poly_int_operand (rtx, machine_mode);
extern int const_vector_int_operand (rtx, machine_mode);
extern int const_vector_int_0_operand (rtx, machine_mode);
extern int const_vector_int_1_operand (rtx, machine_mode);
extern int vector_arith_operand (rtx, machine_mode);
extern int neg_const_vector_int_operand (rtx, machine_mode);
extern int neg_vector_arith_operand (rtx, machine_mode);
extern int vector_move_operand (rtx, machine_mode);
extern int const_vector_shift_operand (rtx, machine_mode);
extern int vector_shift_operand (rtx, machine_mode);
extern int ltge_const_vector_int_operand (rtx, machine_mode);
extern int ltge_vector_arith_operand (rtx, machine_mode);
extern int shift_b_operand (rtx, machine_mode);
extern int shift_h_operand (rtx, machine_mode);
extern int shift_w_operand (rtx, machine_mode);
extern int shift_d_operand (rtx, machine_mode);
#endif /* HAVE_MACHINE_MODES */

#define CONSTRAINT_NUM_DEFINED_P 1
enum constraint_num
{
  CONSTRAINT__UNKNOWN = 0,
  CONSTRAINT_r,
  CONSTRAINT_f,
  CONSTRAINT_j,
  CONSTRAINT_l,
  CONSTRAINT_vr,
  CONSTRAINT_vd,
  CONSTRAINT_vm,
  CONSTRAINT_vt,
  CONSTRAINT_I,
  CONSTRAINT_J,
  CONSTRAINT_K,
  CONSTRAINT_L,
  CONSTRAINT_m,
  CONSTRAINT_o,
  CONSTRAINT_A,
  CONSTRAINT_p,
  CONSTRAINT_Wsa,
  CONSTRAINT_Wsb,
  CONSTRAINT_Wsh,
  CONSTRAINT_Wsw,
  CONSTRAINT_Wsd,
  CONSTRAINT_Ws5,
  CONSTRAINT_G,
  CONSTRAINT_S,
  CONSTRAINT_U,
  CONSTRAINT_vc,
  CONSTRAINT_vi,
  CONSTRAINT_vj,
  CONSTRAINT_vk,
  CONSTRAINT_v0,
  CONSTRAINT_v1,
  CONSTRAINT_vp,
  CONSTRAINT_C,
  CONSTRAINT_V,
  CONSTRAINT__l,
  CONSTRAINT__g,
  CONSTRAINT_i,
  CONSTRAINT_s,
  CONSTRAINT_n,
  CONSTRAINT_E,
  CONSTRAINT_F,
  CONSTRAINT_X,
  CONSTRAINT_T,
  CONSTRAINT__LIMIT
};

extern enum constraint_num lookup_constraint_1 (const char *);
extern const unsigned char lookup_constraint_array[];

/* Return the constraint at the beginning of P, or CONSTRAINT__UNKNOWN if it
   isn't recognized.  */

static inline enum constraint_num
lookup_constraint (const char *p)
{
  unsigned int index = lookup_constraint_array[(unsigned char) *p];
  return (index == UCHAR_MAX
          ? lookup_constraint_1 (p)
          : (enum constraint_num) index);
}

extern bool (*constraint_satisfied_p_array[]) (rtx);

/* Return true if X satisfies constraint C.  */

static inline bool
constraint_satisfied_p (rtx x, enum constraint_num c)
{
  int i = (int) c - (int) CONSTRAINT_I;
  return i >= 0 && constraint_satisfied_p_array[i] (x);
}

static inline bool
insn_extra_register_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_r && c <= CONSTRAINT_vt;
}

static inline bool
insn_extra_memory_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_m && c <= CONSTRAINT_A;
}

static inline bool
insn_extra_special_memory_constraint (enum constraint_num)
{
  return false;
}

static inline bool
insn_extra_address_constraint (enum constraint_num c)
{
  return c >= CONSTRAINT_p && c <= CONSTRAINT_p;
}

static inline void
insn_extra_constraint_allows_reg_mem (enum constraint_num c,
				      bool *allows_reg, bool *allows_mem)
{
  if (c >= CONSTRAINT_Wsa && c <= CONSTRAINT_C)
    return;
  if (c >= CONSTRAINT_V && c <= CONSTRAINT__g)
    {
      *allows_mem = true;
      return;
    }
  (void) c;
  *allows_reg = true;
  *allows_mem = true;
}

static inline size_t
insn_constraint_len (char fc, const char *str ATTRIBUTE_UNUSED)
{
  switch (fc)
    {
    case 'W': return 3;
    case 'v': return 2;
    default: break;
    }
  return 1;
}

#define CONSTRAINT_LEN(c_,s_) insn_constraint_len (c_,s_)

extern enum reg_class reg_class_for_constraint_1 (enum constraint_num);

static inline enum reg_class
reg_class_for_constraint (enum constraint_num c)
{
  if (insn_extra_register_constraint (c))
    return reg_class_for_constraint_1 (c);
  return NO_REGS;
}

extern bool insn_const_int_ok_for_constraint (HOST_WIDE_INT, enum constraint_num);
#define CONST_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    insn_const_int_ok_for_constraint (v_, lookup_constraint (s_))

enum constraint_type
{
  CT_REGISTER,
  CT_CONST_INT,
  CT_MEMORY,
  CT_SPECIAL_MEMORY,
  CT_ADDRESS,
  CT_FIXED_FORM
};

static inline enum constraint_type
get_constraint_type (enum constraint_num c)
{
  if (c >= CONSTRAINT_p)
    {
      if (c >= CONSTRAINT_Wsa)
        return CT_FIXED_FORM;
      return CT_ADDRESS;
    }
  if (c >= CONSTRAINT_m)
    return CT_MEMORY;
  if (c >= CONSTRAINT_I)
    return CT_CONST_INT;
  return CT_REGISTER;
}
#endif /* tm-preds.h */
