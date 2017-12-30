# Cell SPU test code.
# Written by Andrew Church <achurch@achurch.org>
# No copyright is claimed on this file.
#
# Update history:
#    - 2015-02-07: Fixed some comment typos.
#    - 2015-01-18: Added tests for double-precision operations with both
#         denormal and NaN operands.
#    - 2015-01-18: Added more extensive tests for single-precision fused
#         multiply-add instructions.
#    - 2014-08-28: Initial release.
#
# This file implements a test routine which exercises all instructions
# implemented by the Cell Broadband Engine's SPU and verifies that they
# return correct results.  The routine (excluding the halt and stop
# instructions and invalid channel operations) has been validated against
# a real Cell processor.
#
# Call with four arguments:
#    R3: {0,0,0,0} (to bootstrap a constant value into the register file)
#    R4: pointer to an 8k (8192-byte) pre-cleared, cache-aligned scratch
#           memory block (must not be address zero)
#    R5: pointer to a memory block into which failing instructions will be
#           written (should be at least 64k to avoid overruns)
#    R6: {0,1,2,4} (for register file bootstrapping)
# This follows the SPU ABI and can be called from C code with a prototype
# like:
#     extern vector int test(vector int r3, void *scratch, void *failures,
#                            vector int r6);
# (Note that this file does not itself define a symbol for the beginning
# of the test routine.  You can edit this file to insert such a symbol, or
# include this file in another file immediately after a symbol definition.)
#
# Returns a word vector in R3 whose first word (word 0) is the number of
# failing instructions, or a negative value if the test failed to bootstrap
# itself.  The second word (word 1) is defined only when the first word is
# -256 and contains the detected load address of the code; see the
# description of the TEST_ABSOLUTE symbol below for details.  The remaining
# words are undefined.
#
# On return, if R3[0] > 0 then the buffer in R5 contains R3 failure records.
# Each failure record is 16 words (64 bytes) long and has the following
# format:
#    Quadword 0, word 0: Failing instruction word
#    Quadword 0, word 1: Address of failing instruction word
#    Quadword 1: Instruction output value (where applicable)
#    Quadword 2: Expected output value (where applicable)
#    Quadword 3: FPSCR (where applicable) or other relevant data
# Instructions have been coded so that the operands uniquely identify the
# failing instruction; the address is provided as an additional aid in
# locating the instruction.
#
# The code assumes that the following instructions work correctly:
#    - brz RT,target (forward displacement)
#    - brnz RT,target (forward displacement)
#
# The following instructions and features are not tested:
#    - The bisled instruction
#    - The iret instruction
#    - Interrupt enable/disable flags in indirect branch instructions
#    - Precise behavior of the floating-point reciprocal instructions
#         (see the relevant section of the tests for details)
#
# For reference, register usage is as follows:
#    - R0: not modified (caller's return address)
#    - R1: not referenced (caller's stack pointer)
#    - R2: not referenced
#    - R3: result from instruction under test; also used as temporary
#    - R4: not modified (scratch memory pointer from caller)
#    - R5: not modified (failure buffer pointer from caller)
#    - R6: pointer to next failure record to write
#    - R7: not referenced
#    - R8-R23: temporaries used in tests; also arguments to subroutines
#    - R24-R63: constants common to multiple tests
#    - R64-R78: temporaries used by utility subroutines
#    - R79: link register for calling utility subroutines
#

# LS_SIZE:  Set this to the size of the local storage in bytes.
.ifndef LS_SIZE
LS_SIZE = 0x40000
.endif

# TEST_ABSOLUTE:  Set this to 1 to test the absolute load/store and branch
# instructions.  For these tests to work, the code must be loaded at
# absolute address 0x1000.  (If this option is enabled and the code is
# loaded at the wrong address, the bootstrap will fail with return value
# -256 in word 0 of R3 and the detected load address in word 1.)
.ifndef TEST_ABSOLUTE
TEST_ABSOLUTE = 0
.endif

# TEST_FP_EXTENDED:  Set this to 1 to test for proper handling of
# extended-range single-precision floating-point values.  This symbol is
# provided so that implementations which deliberately do not implement
# extended-range values (to improve performance, for example) can skip
# tests which would be expected to fail with such an implementation.
.ifndef TEST_FP_EXTENDED
TEST_FP_EXTENDED = 1
.endif

# TEST_HALT:  Set this to 1 to test the conditional halt instructions.
# These tests require that the SPU halts within a reasonably short time
# (specifically, 1000 iterations of an empty loop) after a halt condition
# is triggered; the controlling code must then write a value of 1
# (0x00000001) to word 0 of the scratch area and restart the SPU.
.ifndef TEST_HALT
TEST_HALT = 0
.endif

# TEST_STOP:  Set this to 1 to test the stop and stopd instructions.  For
# these tests to work, the controlling code must respond to a stop event
# by copying the 14-bit stop code to word 0 of the scratch area and
# restarting the SPU.
.ifndef TEST_STOP
TEST_STOP = 0
.endif

# TEST_CHANNEL_INVALID:  Set this to 1 to test invalid channel operations.
# For these tests to work, the controlling code must respond to an invalid
# channel operation event by writing a value of 1 (0x0000001) to word 0 of
# the scratch area and restarting the SPU.
.ifndef TEST_CHANNEL_INVALID
TEST_CHANNEL_INVALID = 0
.endif

.global test

test:

# Convenience macros.

.macro il32 rt,imm
   .if (\imm >= -0x8000) && (\imm <= 0x7FFF)
      il \rt,\imm
   .else
      .if \imm & 0xFFFF0000
         ilhu \rt,(\imm)>>16
      .endif
      .if \imm & 0xFFFF
         iohl \rt,(\imm)&0xFFFF
      .endif
   .endif
.endm

.macro neg rt,ra
   sfi \rt,\ra,0
.endm

.macro negh rt,ra
   sfhi \rt,\ra,0
.endm

.macro not rt,ra
   nor \rt,\ra,\ra
.endm

.macro shri rt,ra,imm
   rotmi \rt,\ra,0-(\imm)
.endm

# Call a subroutine.  To avoid having to save and restore R0, we use R79
# as the link register.
.macro call target
   brsl $79,\target
.endm

# Return from a subroutine called with the "call" macro.
.macro ret
   bi $79
.endm

# FIXME temp
.macro invalid
   .int 0x08600000
.endm


   .text

   ########################################################################
   # Jump over a couple of tests which need to be located at a known address.
   ########################################################################

0: brz $3,0f        # 0x1000

   ########################################################################
   # Subroutine called from the bootstrap code to verify the load address.
   # Returns the load address of the test code (label 0b above) in R3;
   # destroys R78.
   ########################################################################

get_load_address:
   brsl $78,1f      # 0x1004
1: ai $3,$78,0b-1b  # 0x1008
   ret              # 0x100C

   ########################################################################
   # 7. Compare, Branch, and Halt Instructions - absolute branches
   ########################################################################

.if TEST_ABSOLUTE
   bra 0x101C       # 0x1010
   # These two instructions should be skipped.
   call record      # 0x1014
   ai $6,$6,32      # 0x1018
   # Execution continues here.
   ai $8,$8,8       # 0x101C
   bi $8            # 0x1020
.endif

   ########################################################################
   # 3. Load/Store Instructions - data and buffer for absolute loads/stores
   ########################################################################

.if TEST_ABSOLUTE
   .int 0           # 0x1024
   .int 0           # 0x1028
   .int 0           # 0x102C
absolute_1030:
   .int 0x00112233  # 0x1030
   .int 0x44556677  # 0x1034
   .int 0x8899AABB  # 0x1038
   .int 0xCCDDEEFF  # 0x103C
absolute_1040:
   .int 0           # 0x1040
   .int 0           # 0x1044
   .int 0           # 0x1048
   .int 0           # 0x104C
.endif

   ########################################################################
   # Bootstrap a few instructions so we have a little more flexibility.
   ########################################################################

   # lr RT,RA (ori RT,RA,0)
0: lr $8,$3         # Zero value.
   brz $8,0f
   il $3,-1
   bi $0
0: lr $8,$4         # Nonzero value.
   brnz $8,0f
   il $3,-2
   bi $0

   # il RT,0
0: il $8,0
   brz $8,0f
   il $3,-3
   bi $0

   # il RT,imm (nonzero value)
0: il $8,1
   brnz $8,0f
   il $3,-4
   bi $0

   # br target (forward displacement)
0: br 0f
   il $3,-5
   bi $0

   # ceq RT,RA,RB
0: il $8,0
   ceq $8,$3,$3
   brnz $8,0f
   il $3,-6
   bi $0
0: ceq $8,$3,$4
   brz $8,0f
   il $3,-7
   bi $0

   # ceqi RT,RA,imm
0: il $9,0
   il $8,1
   ceqi $9,$8,1
   brnz $9,0f
   il $3,-8
   bi $0
0: ceqi $9,$8,2
   brz $9,0f
   il $3,-9
   bi $0

   # ceq RT,RA,RB
0: il $9,0
   il $8,1
   il $10,1
   ceq $9,$8,$10
   brnz $9,0f
   il $3,-10
   bi $0
0: il $10,2
   ceq $9,$8,$10
   brz $9,0f
   il $3,-11
   bi $0

   # ai RT,RA,imm (RT != RA, imm > 0)
0: ai $9,$8,2
   il $10,0
   ceqi $10,$9,3
   brnz $10,0f
   il $3,-12
   bi $0

   # not RT,RA (nor RT,RA,RA)
0: il $8,1
   not $9,$8
   ceqi $10,$9,-2
   brnz $10,0f
   il $3,-13
   bi $0

   # orx RT,RA
0: orx $8,$6
   ceqi $9,$8,7
   brnz $9,0f
   il $3,-14
   bi $0

   # brsl (forward displacement), bi
0: brsl $10,3f
1: il $3,-15
   bi $0
2: br 0f
   il $3,-16
   bi $0
3: ai $10,$10,8
   bi $10
   il $3,-17
   bi $0

   # lqd RT,0(RA) (aligned)
0: lqd $8,0($4)
   ceqi $9,$8,0
   brnz $9,0f
   il $3,-18
   bi $0

   # stq RT,0(RA) (aligned)
0: il $8,1
   stqd $8,0($4)
   il $8,0
   lqd $8,0($4)
   ceqi $9,$8,1
   brnz $9,0f
   il $3,-19
   bi $0

   # lqd RT,imm(RA) (imm > 0, aligned)
   # stqd RT,imm(RA) (imm > 0, aligned)
0: il $8,2
   stqd $8,16($4)
   il $8,0
   lqd $8,16($4)
   ceqi $9,$8,2
   brnz $9,0f
   il $3,-20
   bi $0

   # lqd RT,imm(RA) (imm < 0, aligned)
   # stqd RT,imm(RA) (imm < 0, aligned)
0: ai $9,$4,32
   il $8,3
   stqd $8,-16($9)
   il $8,0
   lqd $8,-16($9)
   ceqi $9,$8,3
   brnz $9,0f
   il $3,-21
   bi $0
0: il $8,0
   lqd $8,16($4)
   ceqi $9,$8,3
   brnz $8,0f
   il $3,-22
   bi $0

   ########################################################################
   # Set up some registers used in the test proper:
   #     R6 = pointer to next slot in failed-instruction buffer
   ########################################################################

0: lr $6,$5

   ########################################################################
   # If we'll be testing the absolute address instructions, ensure that the
   # test code was loaded at the correct address.
   ########################################################################

.if TEST_ABSOLUTE
0: call get_load_address
   il $10,0x1000
   ceq $10,$3,$10
   brnz $10,0f
   il $8,-256
   ilhu $9,0x0001
   iohl $9,0x0203
   orbi $10,$9,0x10
   fsmbi $11,0x0F00
   selb $9,$9,$10,$11
   shufb $3,$8,$3,$9
   bi $0
.endif

   ########################################################################
   # Beginning of the test proper.  The test is divided into sections,
   # each with a header comment (like this one) which indicates the group
   # of instructions being tested.  Section numbers in each test (such as
   # "3" in "3. Load/Store Instructions") refer to sections in the document
   # "Synergistic Processor Unit Instruction Set Architecture, Version 1.2".
   ########################################################################

   ########################################################################
   # 3. Load/Store Instructions - lq*, stq*
   ########################################################################

0: il $32,0
   il $33,1
   il $34,2
   stqd $32,0($4)
   stqd $33,16($4)
   stqd $34,32($4)

   # lqd
   # Aligned loads were tested in the bootstrap code; check here that an
   # unaligned address is rounded down to a quadword boundary.  Note that
   # the displacement is forced to be a multiple of 16, so it has to be
   # the register that is unaligned.
0: ai $8,$4,12
   lqd $3,16($8)
   call record
   il $8,1
   call check_value

   # lqx
0: il $3,16
   lqx $3,$4,$3
   call record
   call check_value
0: il $9,28
   il $3,0
   lqx $3,$4,$9
   call record
   call check_value

   # lqr
0: lqr $3,1f
   call record
   il $8,0x1234
   call check_value
0: il $3,0
   lqr $3,2f
   call record
   il $8,0x1234
   call check_value
   br 0f
   .balign 16
1: .int 0x1234,0x1234,0x1234
2: .int 0x1234

.if TEST_ABSOLUTE
   # lqa
0: lqa $3,0x1030
   call record
   lqr $8,absolute_1030
   call check_value
0: il $3,0
   lqa $3,0x103C
   call record
   call check_value
.endif

   # stqd
0: il $8,3
   ai $9,$4,40
   stqd $8,0($9)
   call record
   lqd $3,32($4)
   call check_value

   # stqx
0: il $8,4
   il $9,32
   stqx $8,$4,$9
   call record
   lqd $3,32($4)
   call check_value
0: il $8,5
   il $9,40
   stqx $8,$9,$4
   call record
   lqd $3,32($4)
   call check_value

   # stqr
0: brsl $10,1f
1: ai $10,$10,2f-1b
   il $8,6
   stqr $8,2f
   call record
   lqd $3,0($10)
   call check_value
0: il $8,7
   stqr $8,3f
   call record
   lqd $3,0($10)
   il $9,0          # Clear it for subsequent runs.
   stqd $9,0($10)
   call check_value
   br 0f
   .balign 16
2: .int 0,0,0
3: .int 0

.if TEST_ABSOLUTE
   # stqa
0: il $8,8
   stqa $8,0x1040
   call record
   lqr $3,absolute_1040
   call check_value
0: il $8,9
   stqa $8,0x104C
   call record
   lqr $3,absolute_1040
   call check_value
.endif

   ########################################################################
   # 5. Integer and Logical Instructions - shufb
   ########################################################################

   # shufb
0: lqr $9,5f
   lqr $10,5f+16
   lqr $11,5f+32
   lqr $8,5f+48
   shufb $3,$9,$10,$11
   call record
   call check_value
0: lr $3,$9
   shufb $3,$3,$10,$11
   call record
   call check_value
0: lr $3,$10
   shufb $3,$9,$3,$11
   call record
   call check_value
   br 0f
   .balign 16
5: .int 0x20212223,0x24252627,0x28292A2B,0x2C2D2E2F
   .int 0x30313233,0x34353637,0x38393A3B,0x3C3D3E3F
   .int 0x0E3C4C98,0xCFE81B12,0x0B1F0211,0x1501050B
   .int 0x2E3C2C00,0xFF803B32,0x2B3F2231,0x3521252B

   # From here on it's safe to call check_value_literal.

   ########################################################################
   # 3. Load/Store Instructions - control word generation
   ########################################################################

   # cbd
0: ai $10,$4,5
   il $17,0
   cbd $3,17($10)
   call record
   call check_value_literal
.int 0x10111213,0x14150317,0x18191A1B,0x1C1D1E1F

   # cbx
0: il $17,4
   cbx $3,$17,$10
   call record
   call check_value_literal
.int 0x10111213,0x14151617,0x18031A1B,0x1C1D1E1F

   # chd
0: il $20,0
   chd $3,20($10)
   call record
   call check_value_literal
.int 0x10111213,0x14151617,0x02031A1B,0x1C1D1E1F

   # chx
0: il $20,18
   chx $3,$20,$10
   call record
   call check_value_literal
.int 0x10111213,0x14150203,0x18191A1B,0x1C1D1E1F

   # cwd
0: il $20,0
   cwd $3,20($10)
   call record
   call check_value_literal
.int 0x10111213,0x14151617,0x00010203,0x1C1D1E1F

   # cwx
0: il $20,18
   cwx $3,$20,$10
   call record
   call check_value_literal
.int 0x10111213,0x00010203,0x18191A1B,0x1C1D1E1F

   # cdd
0: il $20,0
   cdd $3,20($10)
   call record
   call check_value_literal
.int 0x10111213,0x14151617,0x00010203,0x04050607

   # cdx
0: il $20,18
   cdx $3,$20,$10
   call record
   call check_value_literal
.int 0x00010203,0x04050607,0x18191A1B,0x1C1D1E1F

   ########################################################################
   # 4. Constant-Formation Instructions
   ########################################################################

   # ilh
0: ilh $3,0x1234
   call record
   call check_value_literal
.int 0x12341234,0x12341234,0x12341234,0x12341234

   # ilhu
0: ilhu $3,0x1234
   call record
   call check_value_literal
.int 0x12340000,0x12340000,0x12340000,0x12340000

   # il
0: il $3,0x1234
   call record
   call check_value_literal
.int 0x00001234,0x00001234,0x00001234,0x00001234
0: il $3,-0x1234
   call record
   call check_value_literal
.int 0xFFFFEDCC,0xFFFFEDCC,0xFFFFEDCC,0xFFFFEDCC

   # ila
0: ila $3,0x1235
   call record
   call check_value_literal
.int 0x00001235,0x00001235,0x00001235,0x00001235
0: ila $3,0xEDCB
   call record
   call check_value_literal
.int 0x0000EDCB,0x0000EDCB,0x0000EDCB,0x0000EDCB
0: ila $3,0x3EDCB
   call record
   call check_value_literal
.int 0x0003EDCB,0x0003EDCB,0x0003EDCB,0x0003EDCB

   # iohl
0: ilhu $3,0x1234
   iohl $3,0xABCD
   call record
   call check_value_literal
.int 0x1234ABCD,0x1234ABCD,0x1234ABCD,0x1234ABCD

   # From here on it's safe to use the il32 macro.

   # fsmbi
0: fsmbi $3,0x9876
   call record
   call check_value_literal
.int 0xFF0000FF,0xFF000000,0x00FFFFFF,0x00FFFF00

   ########################################################################
   # 5. Integer and Logical Instructions - addition and subtraction
   ########################################################################

0: lqr $32,1f
   lqr $33,1f+16
   lqr $34,1f+32
   lqr $35,1f+48
   lqr $36,1f+64
   lqr $37,1f+80
   br 0f
   .balign 16
1: .int 0x01234567,0x89ABCDEF,0xFEDCBA98,0x76543210
   .int 0x00112233,0x44556677,0x8899AABB,0xCCDDEEFF
   .int 0x00000004,0x00000003,0x00000003,0x00000002
   .int 0x01234567,0x00000000,0x9ABCDEF0,0xFFFFFFFF
   .int 0xCCA15507,0x36980BC8,0xAF07B6AE,0x6B6D6861
   .int 0xFF00FF00,0xF0F0000F,0x0F0F0F0F,0x00000FFF

   # ah
0: ah $3,$32,$33
   call record
   call check_value_literal
   .int 0x0134679A,0xCE003466,0x87756553,0x4331210F

   # ahi
0: ahi $3,$32,0x121
   call record
   call check_value_literal
   .int 0x02444688,0x8ACCCF10,0xFFFDBBB9,0x77753331
0: ahi $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0x00444488,0x88CCCD10,0xFDFDB9B9,0x75753131

   # a
0: a $3,$32,$33
   call record
   call check_value_literal
   .int 0x0134679A,0xCE013466,0x87766553,0x4332210F

   # ai
0: ai $3,$32,0x121
   call record
   call check_value_literal
   .int 0x01234688,0x89ABCF10,0xFEDCBBB9,0x76543331
0: ai $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0x01234488,0x89ABCD10,0xFEDCB9B9,0x76543131

   # sfh
0: sfh $3,$32,$33
   call record
   call check_value_literal
   .int 0xFEEEDCCC,0xBAAA9888,0x89BDF023,0x5689BCEF

   # sfhi
0: sfhi $3,$32,0x121
   call record
   call check_value_literal
   .int 0xFFFEBBBA,0x77763332,0x02454689,0x8ACDCF11
0: sfhi $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0xFDFEB9BA,0x75763132,0x00454489,0x88CDCD11

   # sf
0: sf $3,$32,$33
   call record
   call check_value_literal
   .int 0xFEEDDCCC,0xBAA99888,0x89BCF023,0x5689BCEF

   # sfi
0: sfi $3,$32,0x121
   call record
   call check_value_literal
   .int 0xFEDCBBBA,0x76543332,0x01234689,0x89ABCF11
0: sfi $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0xFEDCB9BA,0x76543132,0x01234489,0x89ABCD11

   # cg
0: cg $3,$32,$33
   call record
   call check_value_literal
   .int 0,0,1,1

   # addx
0: addx $3,$32,$33
   call record
   call check_value_literal
   .int 0x0134679A,0xCE013466,0x87766554,0x43322110

   # cgx
0: cg $3,$32,$33
   il $8,-4
   cgx $3,$8,$34
   call record
   call check_value_literal
   .int 1,0,1,0

   # bg
0: bg $3,$32,$33
   call record
   call check_value_literal
   .int 0,0,0,1

   # sfx
0: sfx $3,$32,$33
   call record
   call check_value_literal
   .int 0xFEEDDCCB,0xBAA99887,0x89BCF022,0x5689BCEF

   # bgx
0: bg $3,$32,$33
   il $8,3
   bgx $3,$8,$34
   call record
   call check_value_literal
   .int 1,0,0,0

   ########################################################################
   # 5. Integer and Logical Instructions - multiplication
   ########################################################################

   # mpy
0: mpy $3,$32,$33
   call record
   call check_value_literal
   .int 0x09458185,0xEBF5F419,0x171E3D08,0xFCACBDF0

   # mpyu
0: mpyu $3,$32,$33
   call record
   call check_value_literal
   .int 0x09458185,0x526CF419,0x7C713D08,0x2EBCBDF0

   # mpyi
0: mpyi $3,$32,0x121
   call record
   call check_value_literal
   .int 0x004E5947,0xFFC77ACF,0xFFB1A598,0x00388410
0: mpyi $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0xFFC38B47,0x002B9CCF,0x003C7598,0xFFD46410

   # mpyui
0: mpyui $3,$32,0x121
   call record
   call check_value_literal
   .int 0x004E5947,0x00E87ACF,0x00D2A598,0x00388410
0: mpyui $3,$32,0x321-0x400
   call record
   call check_value_literal
   .int 0x452A8B47,0xCD3B9CCF,0xB9F57598,0x31E46410

   # mpya
0: mpya $3,$32,$33,$32
   call record
   call check_value_literal
   .int 0x0A68C6EC,0x75A1C208,0x15FAF7A0,0x7300F000

   # mpyh
0: mpyh $3,$32,$33
   call record
   call check_value_literal
   .int 0xDFF90000,0x207D0000,0x42B40000,0xF5AC0000
0: mpyh $3,$33,$32
   call record
   call check_value_literal
   .int 0x9BD70000,0xDC5B0000,0x44D80000,0xF7D00000

   # mpys
0: mpys $3,$32,$33
   call record
   call check_value_literal
   .int 0x00000945,0xFFFFEBF5,0x0000171E,0xFFFFFCAC

   # mpyhh
0: mpyhh $3,$32,$33
   call record
   call check_value_literal
   .int 0x00001353,0xE06A21C7,0x0088317C,0xE85D1684

   # mpyhha
0: lr $3,$32
   mpyhha $3,$32,$33
   call record
   call check_value_literal
   .int 0x012358BA,0x6A15EFB6,0xFF64EC14,0x5EB14894

   # mpyhhu
0: mpyhhu $3,$32,$33
   call record
   call check_value_literal
   .int 0x00001353,0x24BF21C7,0x87FD317C,0x5EB11684

   # mpyhhau
0: lr $3,$32
   mpyhhau $3,$32,$33
   call record
   call check_value_literal
   .int 0x012358BA,0xAE6AEFB6,0x86D9EC14,0xD5054894

   ########################################################################
   # 5. Integer and Logical Instructions - miscellaneous bit manipulation
   ########################################################################

   # clz
0: clz $3,$35
   call record
   call check_value_literal
   .int 7,32,0,0

   # cntb
0: cntb $3,$35
   call record
   call check_value_literal
   .int 0x01030305,0x00000000,0x04050604,0x08080808

   # fsmb
   fsmb $3,$35
   call record
   call check_value_literal
   .int 0x00FF0000,0x00FF00FF,0x00FFFF00,0x00FFFFFF

   # fsmh
   fsmh $3,$35
   call record
   call check_value_literal
   .int 0x0000FFFF,0xFFFF0000,0x0000FFFF,0xFFFFFFFF

   # fsm
   fsm $3,$35
   call record
   call check_value_literal
   .int 0x00000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF

   # gbb
   il $3,-1
   gbb $3,$36
   call record
   call check_value_literal
   .int 0x000072CD,0x00000000,0x00000000,0x00000000

   # gbh
   il $3,-1
   gbh $3,$36
   call record
   call check_value_literal
   .int 0x000000CB,0x00000000,0x00000000,0x00000000

   # gb
   il $3,-1
   gb $3,$36
   call record
   call check_value_literal
   .int 0x00000009,0x00000000,0x00000000,0x00000000

   ########################################################################
   # 5. Integer and Logical Instructions - miscellaneous arithmetic
   ########################################################################

   # avgb
0: avgb $3,$32,$33
   call record
   call check_value_literal
   .int 0x011A344D,0x67809AB3,0xC3BBB2AA,0xA1999088

   # absdb
0: absdb $3,$32,$33
   call record
   call check_value_literal
   .int 0x01122334,0x45566778,0x76431023,0x5689BCEF

   # sumb
0: sumb $3,$32,$33
   call record
   call check_value_literal
   .int 0x006600D0,0x017602F0,0x0286032C,0x0396010C
0: lr $3,$32
   sumb $3,$3,$33
   call record
   call check_value_literal
   .int 0x006600D0,0x017602F0,0x0286032C,0x0396010C
0: lr $3,$33
   sumb $3,$32,$3
   call record
   call check_value_literal
   .int 0x006600D0,0x017602F0,0x0286032C,0x0396010C

   ########################################################################
   # 5. Integer and Logical Instructions - sign extension
   ########################################################################

   # xsbh
   # The SPU ISA documentation claims that xsbh is "the only instruction
   # that treats bytes as signed", but that's incorrect: cgtb and cgtbi
   # also treat bytes as signed.
0: xsbh $3,$32
   call record
   call check_value_literal
   .int 0x00230067,0xFFABFFEF,0xFFDCFF98,0x00540010
0: lr $3,$32
   xsbh $3,$3
   call record
   call check_value_literal
   .int 0x00230067,0xFFABFFEF,0xFFDCFF98,0x00540010

   # xshw
0: xshw $3,$32
   call record
   call check_value_literal
   .int 0x00004567,0xFFFFCDEF,0xFFFFBA98,0x00003210
0: lr $3,$32
   xshw $3,$3
   call record
   call check_value_literal
   .int 0x00004567,0xFFFFCDEF,0xFFFFBA98,0x00003210

   # xswd
0: xswd $3,$32
   call record
   call check_value_literal
   .int 0xFFFFFFFF,0x89ABCDEF,0x00000000,0x76543210
0: lr $3,$32
   xswd $3,$3
   call record
   call check_value_literal
   .int 0xFFFFFFFF,0x89ABCDEF,0x00000000,0x76543210

   ########################################################################
   # 5. Integer and Logical Instructions - bitwise logical operations
   ########################################################################

   # and
0: and $3,$32,$36
   call record
   call check_value_literal
   .int 0x00214507,0x008809C8,0xAE04B288,0x62442000

   # andc
0: andc $3,$32,$36
   call record
   call check_value_literal
   .int 0x01020060,0x8923C427,0x50D80810,0x14101210

   # andbi
   andbi $3,$33,0x5A
   call record
   call check_value_literal
   .int 0x00100212,0x40504252,0x08180A1A,0x48584A5A

   # andhi
   andhi $3,$36,0x147
   call record
   call check_value_literal
   .int 0x00010107,0x00000140,0x01070006,0x01450041
   andhi $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0xCC015407,0x36000A40,0xAE07B606,0x6A456841

   # andi
   andi $3,$36,0x147
   call record
   call check_value_literal
   .int 0x00000107,0x00000140,0x00000006,0x00000041
   andi $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0xCCA15407,0x36980A40,0xAF07B606,0x6B6D6841

   # or
0: or $3,$32,$36
   call record
   call check_value_literal
   .int 0xCDA35567,0xBFBBCFEF,0xFFDFBEBE,0x7F7D7A71

   # orc
0: orc $3,$32,$36
   call record
   call check_value_literal
   .int 0x337FEFFF,0xC9EFFDFF,0xFEFCFBD9,0xF6D6B79E

   # orbi
   orbi $3,$33,0x5A
   call record
   call check_value_literal
   .int 0x5A5B7A7B,0x5E5F7E7F,0xDADBFAFB,0xDEDFFEFF

   # orhi
   orhi $3,$36,0x147
   call record
   call check_value_literal
   .int 0xCDE75547,0x37DF0BCF,0xAF47B7EF,0x6B6F6967
   orhi $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0xFEE7FF47,0xFEDFFFCF,0xFF47FEEF,0xFF6FFE67

   # ori
   ori $3,$36,0x147
   call record
   call check_value_literal
   .int 0xCCA15547,0x36980BCF,0xAF07B7EF,0x6B6D6967
   ori $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0xFFFFFF47,0xFFFFFFCF,0xFFFFFEEF,0xFFFFFE67

   # orx
0: orx $3,$36
   call record
   call check_value_literal
   .int 0xFFFFFFEF,0,0,0

   # xor
0: xor $3,$32,$36
   call record
   call check_value_literal
   .int 0xCD821060,0xBF33C627,0x51DB0C36,0x1D395A71

   # xorbi
   xorbi $3,$33,0x5A
   call record
   call check_value_literal
   .int 0x5A4B7869,0x1E0F3C2D,0xD2C3F0E1,0x9687B4A5

   # xorhi
   xorhi $3,$36,0x147
   call record
   call check_value_literal
   .int 0xCDE65440,0x37DF0A8F,0xAE40B7E9,0x6A2A6926
   xorhi $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0x32E6AB40,0xC8DFF58F,0x514048E9,0x952A9626

   # xori
   xori $3,$36,0x147
   call record
   call check_value_literal
   .int 0xCCA15440,0x36980A8F,0xAF07B7E9,0x6B6D6926
   xori $3,$36,0x247-0x400
   call record
   call check_value_literal
   .int 0x335EAB40,0xC967F58F,0x50F848E9,0x94929626

   # nand
0: nand $3,$32,$36
   call record
   call check_value_literal
   .int 0xFFDEBAF8,0xFF77F637,0x51FB4D77,0x9DBBDFFF

   # nor
0: nor $3,$32,$36
   call record
   call check_value_literal
   .int 0x325CAA98,0x40443010,0x00204141,0x8082858E

   # eqv
0: eqv $3,$32,$36
   call record
   call check_value_literal
   .int 0x327DEF9F,0x40CC39D8,0xAE24F3C9,0xE2C6A58E

   # selb
0: selb $3,$32,$36,$37
   call record
   call check_value_literal
   .int 0xCC235567,0x399BCDE8,0xFFD7B69E,0x76543861

   ########################################################################
   # 6. Shift and Rotate Instructions - shift left
   ########################################################################

0: not $40,$32
   lqr $41,1f
   lqr $42,1f+16
   lqr $43,1f+32
   br 0f
   .balign 16
1: .int 0x00000001,0x00030008,0x000F0010,0x001F0020
   .int 0x00000000,0x00000005,0x0000001F,0x00000020
   .int 0x0000003F,0x00000040,0x00010000,0x00000000

   # shlh
0: shlh $3,$40,$41
   call record
   call check_value_literal
   .int 0xFEDC7530,0xB2A01000,0x80000000,0x0000CDEF

   # shlhi
0: shlhi $3,$32,0
   call record
   lr $8,$32
   call check_value
0: shlhi $3,$32,5
   call record
   call check_value_literal
   .int 0x2460ACE0,0x3560BDE0,0xDB805300,0xCA804200
0: shlhi $3,$32,16
   call record
   il $8,0
   call check_value
0: shlhi $3,$32,31
   call record
   call check_value

   # shl
0: shl $3,$40,$42
   call record
   call check_value_literal
   .int 0xFEDCBA98,0xCA864200,0x80000000,0x00000000
0: shl $3,$40,$43
   call record
   call check_value_literal
   .int 0x00000000,0x76543210,0x01234567,0x89ABCDEF

   # shli
0: shli $3,$32,0
   call record
   lr $8,$32
   call check_value
0: shli $3,$32,5
   call record
   call check_value_literal
   .int 0x2468ACE0,0x3579BDE0,0xDB975300,0xCA864200
0: shli $3,$32,32
   call record
   il $8,0
   call check_value
0: shli $3,$32,63
   call record
   call check_value

   # shlqbi
0: il $9,0
   shlqbi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,5
   shlqbi $3,$40,$10
   call record
   call check_value_literal
   .int 0xDB97530E,0xCA864200,0x2468ACF1,0x3579BDE0
0: lr $3,$40
   shlqbi $3,$3,$10
   call record
   call check_value
0: il $11,9
   shlqbi $3,$40,$11
   call record
   call check_value_literal
   .int 0xFDB97530,0xECA86420,0x02468ACF,0x13579BDE
0: lr $3,$40
   shlqbi $3,$3,$11
   call record
   call check_value

   # shlqbii
0: shlqbii $3,$40,0
   call record
   lr $8,$40
   call check_value
0: shlqbii $3,$40,5
   call record
   call check_value_literal
   .int 0xDB97530E,0xCA864200,0x2468ACF1,0x3579BDE0
0: lr $3,$40
   shlqbii $3,$3,5
   call record
   call check_value

   # shlqby
0: il $9,0
   shlqby $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,5
   shlqby $3,$40,$10
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEF00,0x00000000
0: lr $3,$40
   shlqby $3,$3,$10
   call record
   call check_value
0: il $11,16
   shlqby $3,$40,$11
   call record
   il $8,0
   call check_value
0: lr $3,$40
   shlqby $3,$3,$11
   call record
   call check_value
0: il $12,32
   shlqby $3,$40,$12
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   shlqby $3,$3,$12
   call record
   call check_value

   # shlqbyi
0: shlqbyi $3,$40,0
   call record
   lr $8,$40
   call check_value
0: shlqbyi $3,$40,5
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEF00,0x00000000
0: lr $3,$40
   shlqbyi $3,$3,5
   call record
   call check_value
0: shlqbyi $3,$40,16
   call record
   il $8,0
   call check_value
0: lr $3,$40
   shlqbyi $3,$3,16
   call record
   call check_value

   # shlqbybi
0: il $9,0
   shlqbybi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,42        # The low 3 bits of the count should be ignored.
   shlqbybi $3,$40,$10
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEF00,0x00000000
0: lr $3,$40
   shlqbybi $3,$3,$10
   call record
   call check_value
0: il $11,128
   shlqbybi $3,$40,$11
   call record
   il $8,0
   call check_value
0: lr $3,$40
   shlqbybi $3,$3,$11
   call record
   call check_value
0: il $12,256
   shlqbybi $3,$40,$12
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   shlqbybi $3,$3,$12
   call record
   call check_value

   ########################################################################
   # 6. Shift and Rotate Instructions - rotate left
   ########################################################################

   # roth
0: roth $3,$40,$41
   call record
   call check_value_literal
   .int 0xFEDC7531,0xB2A31032,0x80914567,0xC4D5CDEF

   # rothi
0: rothi $3,$32,0
   call record
   lr $8,$32
   call check_value
0: rothi $3,$32,5
   call record
   call check_value_literal
   .int 0x2460ACE8,0x3571BDF9,0xDB9F5317,0xCA8E4206
0: rothi $3,$32,16
   call record
   lr $8,$32
   call check_value

   # rot
0: rot $3,$40,$42
   call record
   call check_value_literal
   .int 0xFEDCBA98,0xCA86420E,0x8091A2B3,0x89ABCDEF
0: rot $3,$40,$43
   call record
   call check_value_literal
   .int 0x7F6E5D4C,0x76543210,0x01234567,0x89ABCDEF

   # roti
0: roti $3,$32,0
   call record
   lr $8,$32
   call check_value
0: roti $3,$32,5
   call record
   call check_value_literal
   .int 0x2468ACE0,0x3579BDF1,0xDB97531F,0xCA86420E
0: roti $3,$32,32
   call record
   lr $8,$32
   call check_value

   # rotqbi
0: il $9,0
   rotqbi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,5
   rotqbi $3,$40,$10
   call record
   call check_value_literal
   .int 0xDB97530E,0xCA864200,0x2468ACF1,0x3579BDFF
0: lr $3,$40
   rotqbi $3,$3,$10
   call record
   call check_value
0: il $11,9
   rotqbi $3,$40,$11
   call record
   call check_value_literal
   .int 0xFDB97530,0xECA86420,0x02468ACF,0x13579BDF
0: lr $3,$40
   rotqbi $3,$3,$11
   call record
   call check_value

   # rotqbii
0: rotqbii $3,$40,0
   call record
   lr $8,$40
   call check_value
0: rotqbii $3,$40,5
   call record
   call check_value_literal
   .int 0xDB97530E,0xCA864200,0x2468ACF1,0x3579BDFF
0: lr $3,$40
   rotqbii $3,$3,5
   call record
   call check_value

   # rotqby
0: il $9,0
   rotqby $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,5
   rotqby $3,$40,$10
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEFFE,0xDCBA9876
0: lr $3,$40
   rotqby $3,$3,$10
   call record
   call check_value
0: il $11,16
   rotqby $3,$40,$11
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   rotqby $3,$3,$11
   call record
   call check_value

   # rotqbyi
0: rotqbyi $3,$40,0
   call record
   lr $8,$40
   call check_value
0: rotqbyi $3,$40,5
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEFFE,0xDCBA9876
0: lr $3,$40
   rotqbyi $3,$3,5
   call record
   call check_value
0: rotqbyi $3,$40,16
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   rotqbyi $3,$3,16
   call record
   call check_value

   # rotqbybi
0: il $9,0
   rotqbybi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,42
   rotqbybi $3,$40,$10
   call record
   call check_value_literal
   .int 0x54321001,0x23456789,0xABCDEFFE,0xDCBA9876
0: lr $3,$40
   rotqbybi $3,$3,$10
   call record
   call check_value
0: il $12,128
   rotqbybi $3,$40,$12
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   rotqbybi $3,$3,$12
   call record
   call check_value

   ########################################################################
   # 6. Shift and Rotate Instructions - shift right (logical)
   ########################################################################

   # rothm
0: negh $9,$41
   rothm $3,$32,$9
   call record
   call check_value_literal
   .int 0x012322B3,0x113500CD,0x00010000,0x00003210

   # rothmi
0: rothmi $3,$32,-0
   call record
   lr $8,$32
   call check_value
0: rothmi $3,$32,-5
   call record
   call check_value_literal
   .int 0x0009022B,0x044D066F,0x07F605D4,0x03B20190
0: rothmi $3,$32,-16
   call record
   il $8,0
   call check_value
0: rothmi $3,$32,-31
   call record
   call check_value

   # rotm
0: neg $9,$42
   rotm $3,$32,$9
   call record
   call check_value_literal
   .int 0x01234567,0x044D5E6F,0x00000001,0x00000000
0: neg $10,$43
   rotm $3,$32,$10
   call record
   call check_value_literal
   .int 0x00000000,0x89ABCDEF,0xFEDCBA98,0x76543210

   # rotmi
0: rotmi $3,$32,-0
   call record
   lr $8,$32
   call check_value
0: rotmi $3,$32,-5
   call record
   call check_value_literal
   .int 0x00091A2B,0x044D5E6F,0x07F6E5D4,0x03B2A190
0: rotmi $3,$32,-32
   call record
   il $8,0
   call check_value
0: rotmi $3,$32,-63
   call record
   call check_value

   # rotqmbi
0: il $9,-0
   rotqmbi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,-5
   rotqmbi $3,$40,$10
   call record
   call check_value_literal
   .int 0x07F6E5D4,0xC3B2A190,0x80091A2B,0x3C4D5E6F
0: lr $3,$40
   rotqmbi $3,$3,$10
   call record
   call check_value
0: il $11,-9
   rotqmbi $3,$40,$11
   call record
   call check_value_literal
   .int 0x7F6E5D4C,0x3B2A1908,0x0091A2B3,0xC4D5E6F7
0: lr $3,$40
   rotqmbi $3,$3,$11
   call record
   call check_value

   # rotqmbii
0: rotqmbii $3,$40,-0
   call record
   lr $8,$40
   call check_value
0: rotqmbii $3,$40,-5
   call record
   call check_value_literal
   .int 0x07F6E5D4,0xC3B2A190,0x80091A2B,0x3C4D5E6F
0: lr $3,$40
   rotqmbii $3,$3,-5
   call record
   call check_value

   # rotqmby
0: il $9,-0
   rotqmby $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,-5
   rotqmby $3,$40,$10
   call record
   call check_value_literal
   .int 0x00000000,0x00FEDCBA,0x98765432,0x10012345
0: lr $3,$40
   rotqmby $3,$3,$10
   call record
   call check_value
0: il $11,-16
   rotqmby $3,$40,$11
   call record
   il $8,0
   call check_value
0: lr $3,$40
   rotqmby $3,$3,$11
   call record
   call check_value
0: il $12,-32
   rotqmby $3,$40,$12
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   rotqmby $3,$3,$12
   call record
   call check_value

   # rotqmbyi
0: rotqmbyi $3,$40,-0
   call record
   lr $8,$40
   call check_value
0: rotqmbyi $3,$40,-5
   call record
   call check_value_literal
   .int 0x00000000,0x00FEDCBA,0x98765432,0x10012345
0: lr $3,$40
   rotqmbyi $3,$3,-5
   call record
   call check_value
0: rotqmbyi $3,$40,-16
   call record
   il $8,0
   call check_value
0: lr $3,$40
   rotqmbyi $3,$3,-16
   call record
   call check_value

   # rotqmbybi
0: il $9,-0
   rotqmbybi $3,$40,$9
   call record
   lr $8,$40
   call check_value
0: il $10,-42
   rotqmbybi $3,$40,$10
   call record
   call check_value_literal
   .int 0x00000000,0x0000FEDC,0xBA987654,0x32100123
0: lr $3,$40
   rotqmbybi $3,$3,$10
   call record
   call check_value
0: il $11,-128
   rotqmbybi $3,$40,$11
   call record
   il $8,0
   call check_value
0: lr $3,$40
   rotqmbybi $3,$3,$11
   call record
   call check_value
0: il $12,-256
   rotqmbybi $3,$40,$12
   call record
   lr $8,$40
   call check_value
0: lr $3,$40
   rotqmbybi $3,$3,$12
   call record
   call check_value

   ########################################################################
   # 6. Shift and Rotate Instructions - shift right (arithmetic)
   ########################################################################

   # rotmah
0: negh $9,$41
   rotmah $3,$32,$9
   call record
   call check_value_literal
   .int 0x012322B3,0xF135FFCD,0xFFFFFFFF,0x00003210

   # rotmahi
0: rotmahi $3,$32,-0
   call record
   lr $8,$32
   call check_value
0: rotmahi $3,$32,-5
   call record
   call check_value_literal
   .int 0x0009022B,0xFC4DFE6F,0xFFF6FDD4,0x03B20190
0: rotmahi $3,$32,-16
   call record
   fsmbi $8,0x0FF0
   call check_value
0: rotmahi $3,$32,-31
   call record
   call check_value

   # rotma
0: neg $9,$42
   rotma $3,$32,$9
   call record
   call check_value_literal
   .int 0x01234567,0xFC4D5E6F,0xFFFFFFFF,0x00000000
0: neg $10,$43
   rotma $3,$32,$10
   call record
   call check_value_literal
   .int 0x00000000,0x89ABCDEF,0xFEDCBA98,0x76543210

   # rotmai
0: rotmai $3,$32,-0
   call record
   lr $8,$32
   call check_value
0: rotmai $3,$32,-5
   call record
   call check_value_literal
   .int 0x00091A2B,0xFC4D5E6F,0xFFF6E5D4,0x03B2A190
0: rotmai $3,$32,-32
   call record
   fsmbi $8,0x0FF0
   call check_value
0: rotmai $3,$32,-63
   call record
   call check_value

   ########################################################################
   # 7. Compare, Branch, and Halt Instructions - conditional halts
   ########################################################################

.if TEST_HALT
   # heq
0: il $10,0
   stqd $10,0($4)
   il $8,1
   heq $8,$10
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: heq $10,$10
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # heqi
0: stqd $10,0($4)
   heqi $10,1
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: heqi $10,0
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # hgt
0: stqd $10,0($4)
   il $33,2
   il $34,-2
   hgt $34,$33
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: hgt $33,$34
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # hgti
0: stqd $10,0($4)
   hgti $34,2
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: hgti $33,-2
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # hlgt
0: stqd $10,0($4)
   hlgt $33,$34
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: hlgt $34,$33
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # hlgti
0: stqd $10,0($4)
   hlgti $33,-2
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,0
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: hlgti $34,2
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
.endif

   ########################################################################
   # 7. Compare, Branch, and Halt Instructions - compare instructions
   ########################################################################

0: lqr $40,1f
   lqr $41,1f+16
   lqr $42,1f+32
   br 0f
   .balign 16
1: .int 0x01234567,0x89ABCDEF,0xEFCD89AB,0x01234567
   .int 0x00000123,0x89ABCDEF,0x45670123,0x00000123
   .int 0x01234567,0x89ABCDEF,0x45670123,0x01234567

   # ceqb
0: ilh $10,0xCDDC
   ceqb $3,$32,$10
   call record
   fsmbi $8,0x0240
   call check_value

   # ceqbi
0: ceqbi $3,$40,0x89
   call record
   fsmbi $8,0x0820
   call check_value

   # ceqh
0: il32 $11,0x89ABBA98
   ceqh $3,$32,$11
   call record
   fsmbi $8,0x0C30
   call check_value

   # ceqhi
0: ceqhi $3,$40,0x123
   call record
   fsmbi $8,0xC00C
   call check_value

   # ceq
0: il32 $12,0x01234567
   ceq $3,$40,$12
   call record
   fsmbi $8,0xF00F
   call check_value

   # ceqi
0: ceqi $3,$41,0x123
   call record
   fsmbi $8,0xF00F
   call check_value

   # cgtb
0: cgtb $3,$32,$10
   call record
   fsmbi $8,0xF18F
   call check_value

   # cgtbi
0: cgtbi $3,$40,0xAB
   call record
   fsmbi $8,0xF3CF
   call check_value

   # cgth
0: il32 $13,0xFEDCCDEF
   cgth $3,$32,$13
   call record
   fsmbi $8,0xF00F
   call check_value

   # cgthi
0: cgthi $3,$40,0x123
   call record
   fsmbi $8,0x3003
   call check_value

   # cgt
0: cgt $3,$42,$12
   call record
   fsmbi $8,0x00F0
   call check_value

   # cgti
0: cgti $3,$41,0x123
   call record
   fsmbi $8,0x00F0
   call check_value

   # clgtb
0: clgtb $3,$32,$10
   call record
   fsmbi $8,0x0180
   call check_value

   # clgtbi
0: clgtbi $3,$40,0xAB
   call record
   fsmbi $8,0x03C0
   call check_value

   # clgth
0: clgth $3,$32,$11
   call record
   fsmbi $8,0x03C0
   call check_value

   # clgthi
0: clgthi $3,$41,0x123
   call record
   fsmbi $8,0x0FC0
   call check_value

   # clgt
0: clgt $3,$42,$12
   call record
   fsmbi $8,0x0FF0
   call check_value

   # clgti
0: clgti $3,$41,0x123
   call record
   fsmbi $8,0x0FF0
   call check_value

   ########################################################################
   # 7. Compare, Branch, and Halt Instructions - unconditional branches
   ########################################################################

   # br (forward displacement) was tested in the bootstrap code.

   # br (backward displacement)
0: br 2f
1: br 0f
2: br 1b
   call record
   ai $6,$6,64
   br 0f

   # brsl (forward displacement) was tested in the bootstrap code.

   # brsl (backward displacement)
0: br 2f
1: ai $3,$8,12
   lqd $3,0($3)
   br 3f
   .balignl 16,0x40200000
2: brsl $8,1b
   br 5f
   .int 0x12345678,0x12345678,0x12345678,0x12345678
3: ai $10,$8,12
   lqd $3,0($10)
   il32 $9,0x12345678
   ceq $11,$3,$9
   brz $11,4f
   andi $12,$10,0xF
   brz $12,0f
4: stqd $3,16($6)
   stqd $9,32($6)
   stqd $8,48($6)
5: brsl $3,6f
6: ai $3,$3,6b-2b
   call record_r3
   ai $6,$6,64

   # bi was tested in the bootstrap code.

   # bi (address wraparound)
0: il32 $32,LS_SIZE
   brsl $8,1f
1: a $9,$8,$32
   ai $9,$9,3f-1b
2: bi $9
   br 5f
3: call record
   brsl $3,4f
4: cgt $8,$32,$3    # Make sure the PC after branching is still within LS_SIZE.
   brnz $8,0f
   stqd $3,16($6)
5: brsl $3,6f
6: ai $3,$3,6b-2b
   call record_r3
   ai $6,$6,64

   # bisl
0: brsl $8,2f
1: .balignl 16,0x40200000
2: ai $9,$8,3f-1b
8: bisl $8,$9
   br 6f
3: br 4f
   .int 0x23456789,0x23456789,0x23456789,0x23456789
4: ai $10,$8,8
   lqd $3,0($10)
   il32 $9,0x23456789
   ceq $11,$3,$9
   brz $11,5f
   andi $12,$10,0xF
   brz $12,0f
5: stqd $3,16($6)
   stqd $9,32($6)
   stqd $8,48($6)
6: brsl $3,7f
7: ai $3,$3,7b-8b
   call record_r3
   ai $6,$6,64

   # bisl (address wraparound)
0: brsl $8,1f
1: a $9,$8,$32
   ai $9,$9,3f-1b
2: bisl $8,$9
   br 5f
3: brsl $3,4f
4: cgt $8,$32,$3
   brnz $8,0f
   stqd $3,16($6)
5: brsl $3,6f
6: ai $3,$3,6b-2b
   call record_r3
   ai $6,$6,64

.if TEST_ABSOLUTE
   # bra, brasl
0: brasl $8,0x1010
   call record      # Skipped.
   ai $6,$6,64      # Skipped.
.endif

   ########################################################################
   # 7. Compare, Branch, and Halt Instructions - conditional branches
   ########################################################################

   # brz, brnz (forward displacement) are assumed to work correctly.

   # brz (backward displacement)
0: br 2f
1: br 0f
2: il $3,0
   brz $3,1b
   call record
   ai $6,$6,64

   # biz
0: brsl $8,1f
1: ai $8,$8,0f-1b
   biz $3,$8
   call record
   ai $6,$6,64

   # brnz (backward displacement)
0: br 2f
1: br 0f
2: ilhu $3,1
   brnz $3,1b
   call record
   ai $6,$6,64

   # binz
0: brsl $8,1f
1: ai $8,$8,0f-1b
   binz $3,$8
   call record
   ai $6,$6,64

   # brhz
0: brhz $3,0f
   call record
   ai $6,$6,64
0: br 2f
1: br 0f
2: brhz $3,1b
   call record
   ai $6,$6,64

   # bihz
0: brsl $8,1f
1: ai $8,$8,0f-1b
   bihz $3,$8
   call record
   ai $6,$6,64

   # brhnz
0: il $3,1
   brhnz $3,0f
   call record
   ai $6,$6,64
0: br 2f
1: br 0f
2: brhnz $3,1b
   call record
   ai $6,$6,64

   # bihnz
0: brsl $8,1f
1: ai $8,$8,0f-1b
   bihnz $3,$8
   call record
   ai $6,$6,64

   ########################################################################
   # 8. Hint-for-Branch Instructions
   ########################################################################

   # None of these instructions have any visible effect on program state,
   # so we just execute them to verify that they can be decoded correctly.
0: hbr 1f,$0
   hbra 1f,0x1000
   hbrr 1f,0f
   hbrp
1: br 0f

   ########################################################################
   # 9. Floating-Point Instructions - FPSCR control
   ########################################################################

   # fscrrd, fscrwr
0: il $8,0
   fscrwr $8
   il $3,-1
   fscrrd $3
   call record
   call check_value
0: il $8,-1
   fscrwr $8
   call record
   fscrrd $3
   call check_value_literal
   .int 0x00000F07,0x00003F07,0x00003F07,0x00000F07
   il $8,0
   fscrwr $8

   ########################################################################
   # 9. Floating-Point Instructions - single-precision arithmetic
   ########################################################################

0: il32 $32,0x00000000  # +0.0f
   il32 $33,0x3F800000  # +1.0f
   il32 $34,0x3F000000  # +0.5f
   il32 $35,0x3FC00000  # +1.5f
   il32 $36,0x3FD55555  # +1.666...f
   il32 $37,0x40000000  # +2.0f
   il32 $38,0x40200000  # +2.5f
   il32 $39,0x00400000  # denormal
   il32 $40,0x00800000  # smallest positive normal
   il32 $41,0x00C00000  # 1.5 * smallest positive normal
   il32 $42,0x7F600000
   il32 $43,0x7F7FFFFF
   il32 $44,0x7F800000
   il32 $45,0x7F800001
   il32 $46,0x7FE00000
   il32 $47,0x7FFFFFFF
   # These are all arithmetic inverses of the above.
   il32 $48,0x80000000
   il32 $49,0xBF800000
   il32 $50,0xBF000000
   il32 $51,0xBFC00000
   il32 $52,0xBFD55555
   il32 $53,0xC0000000
   il32 $54,0xC0200000
   il32 $55,0x80400000
   il32 $56,0x80800000
   il32 $57,0x80C00000
   il32 $58,0xFF600000
   il32 $59,0xFF7FFFFF
   il32 $60,0xFF800000
   il32 $61,0xFF800001
   il32 $62,0xFFE00000
   il32 $63,0xFFFFFFFF

   # fa
0: fa $3,$33,$33    # Basic tests.
   call record
   lr $8,$37
   call check_float_arith
0: fa $3,$32,$32
   call record
   lr $8,$32
   call check_float_arith
0: fa $3,$48,$48    # Zero results should always be positive.
   call record
   lr $8,$32
   call check_float_arith
0: fa $3,$41,$39    # Denormals should be treated as zero (DIFF exception).
   call record
   lr $8,$41
   call check_float_arith_diff
0: fa $3,$39,$41
   call record
   lr $8,$41
   call check_float_arith_diff
0: fa $3,$39,$39
   call record
   lr $8,$32
   call check_float_arith_diff
0: fa $3,$39,$32
   call record
   lr $8,$32
   call check_float_arith_diff
0: fa $3,$32,$39
   call record
   lr $8,$32
   call check_float_arith_diff
0: fa $3,$41,$56    # Denormal results should be flushed to zero (UNF | DIFF).
   call record
   lr $8,$32
   call check_float_arith_unf
0: il32 $9,0x7DFFFFF8
   fa $3,$42,$9    # Maximum-magnitude normal value; should not set DIFF.
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED
0: fa $3,$42,$42    # Check extended-range arithmetic.
   call record
   lr $8,$46
   call check_float_arith_diff
0: fa $3,$44,$44
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fa $3,$44,$60
   call record
   lr $8,$32
   call check_float_arith_diff
0: fa $3,$44,$40    # Check very large +/- very small values.
   call record
   lr $8,$44
   call check_float_arith_diff
0: fa $3,$40,$44
   call record
   lr $8,$44
   call check_float_arith_diff
0: fa $3,$45,$56
   call record
   lr $8,$44
   call check_float_arith_diff
0: fa $3,$56,$45
   call record
   lr $8,$44
   call check_float_arith_diff
0: fa $3,$44,$56
   call record
   lr $8,$43
   call check_float_arith_diff
0: fa $3,$56,$44
   call record
   lr $8,$43
   call check_float_arith_diff
0: fa $3,$44,$43  # Exactly equal to Smax after rounding; should not overflow.
   call record
   lr $8,$47
   call check_float_arith_diff
0: fa $3,$43,$44
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   # fs
   # These are identical to the tests for fa except that the second operand
   # is negated in each case.
0: fs $3,$33,$49
   call record
   lr $8,$37
   call check_float_arith
0: fa $3,$32,$48
   call record
   lr $8,$32
   call check_float_arith
0: fs $3,$48,$32
   call record
   lr $8,$32
   call check_float_arith
0: fs $3,$41,$55
   call record
   lr $8,$41
   call check_float_arith_diff
0: fs $3,$39,$57
   call record
   lr $8,$41
   call check_float_arith_diff
0: fs $3,$39,$55
   call record
   lr $8,$32
   call check_float_arith_diff
0: fs $3,$39,$48
   call record
   lr $8,$32
   call check_float_arith_diff
0: fs $3,$32,$55
   call record
   lr $8,$32
   call check_float_arith_diff
0: fs $3,$41,$40
   call record
   lr $8,$32
   call check_float_arith_unf
0: il32 $9,0xFDFFFFF8
   fs $3,$42,$9
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED
0: fs $3,$42,$58
   call record
   lr $8,$46
   call check_float_arith_diff
0: fs $3,$44,$60
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fs $3,$44,$44
   call record
   lr $8,$32
   call check_float_arith_diff
0: fs $3,$44,$56
   call record
   lr $8,$44
   call check_float_arith_diff
0: fs $3,$40,$60
   call record
   lr $8,$44
   call check_float_arith_diff
0: fs $3,$45,$40
   call record
   lr $8,$44
   call check_float_arith_diff
0: fs $3,$56,$61
   call record
   lr $8,$44
   call check_float_arith_diff
0: fs $3,$44,$40
   call record
   lr $8,$43
   call check_float_arith_diff
0: fs $3,$56,$60
   call record
   lr $8,$43
   call check_float_arith_diff
0: fs $3,$44,$59
   call record
   lr $8,$47
   call check_float_arith_diff
0: fs $3,$43,$60
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   # fm
0: fm $3,$33,$33    # Basic tests.
   call record
   lr $8,$33
   call check_float_arith
0: fm $3,$34,$37
   call record
   lr $8,$33
   call check_float_arith
0: fm $3,$32,$32
   call record
   lr $8,$32
   call check_float_arith
0: fm $3,$33,$48    # Zero results should always be positive.
   call record
   lr $8,$32
   call check_float_arith
0: fm $3,$35,$36    # Check rounding direction.
   call record
   ai $8,$38,-1
   call check_float_arith
0: fm $3,$37,$39    # Denormals should be treated as zero.
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$39,$37
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$41,$34    # Denormal results should be flushed to zero.
   call record
   lr $8,$32
   call check_float_arith_unf
0: il32 $9,0x7EFFFFFF
   fm $3,$37,$9    # Maximum-magnitude normal value; should not set DIFF.
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED
0: fm $3,$37,$42    # Check extended-range arithmetic.
   call record
   lr $8,$46
   call check_float_arith_diff
0: fm $3,$42,$37
   call record
   lr $8,$46
   call check_float_arith_diff
0: fm $3,$34,$46
   call record
   lr $8,$42
   call check_float_arith_diff
0: fm $3,$46,$34
   call record
   lr $8,$42
   call check_float_arith_diff
0: fm $3,$44,$40
   call record
   il32 $8,0x40800000  # +4.0f
   call check_float_arith_diff
0: fm $3,$37,$44
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fm $3,$44,$37
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fm $3,$37,$60
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fm $3,$60,$37
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fm $3,$35,$46
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fm $3,$46,$35
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fm $3,$42,$42
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fm $3,$42,$58
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fm $3,$58,$42
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fm $3,$32,$44
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$44,$32
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$48,$44
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$44,$48
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$32,$47
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$47,$32
   call record
   lr $8,$32
   call check_float_arith_diff
0: fm $3,$37,$43    # Exactly equal to Smax; should not overflow.
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   # fma
0: fma $3,$33,$33,$33   # Basic tests.
   call record
   lr $8,$37
   call check_float_arith
0: fma $3,$34,$37,$34
   call record
   lr $8,$35
   call check_float_arith
0: fma $3,$32,$32,$33
   call record
   lr $8,$33
   call check_float_arith
0: fma $3,$33,$33,$32
   call record
   lr $8,$33
   call check_float_arith
0: fma $3,$33,$32,$32
   call record
   lr $8,$32
   call check_float_arith
0: fma $3,$32,$33,$32
   call record
   lr $8,$32
   call check_float_arith
0: fma $3,$33,$48,$48   # Zero results should always be positive.
   call record
   lr $8,$32
   call check_float_arith
0: fma $3,$35,$36,$32   # Check rounding behavior.
   call record
   ai $8,$38,-1
   call check_float_arith
0: ilhu $8,0x3380       # 2.0f - (1.666...f * 1.5f) < ulp(2.0f)
   fma $3,$35,$36,$8    # The product should not be rounded.
   call record
   lr $8,$38
   call check_float_arith
0: fma $3,$37,$39,$40   # Denormals should be treated as zero.
   call record
   lr $8,$40
   call check_float_arith_diff
0: fma $3,$39,$37,$40
   call record
   lr $8,$40
   call check_float_arith_diff
0: fma $3,$33,$40,$39
   call record
   lr $8,$40
   call check_float_arith_diff
0: fma $3,$33,$40,$55
   call record
   lr $8,$40
   call check_float_arith_diff
0: fma $3,$34,$40,$32   # Denormal results should be flushed to zero.
   call record
   lr $8,$32
   call check_float_arith_unf
0: fma $3,$40,$56,$40
   call record
   lr $8,$32
   call check_float_arith_unf
0: fma $3,$34,$40,$40   # A denormal product by itself should not be flushed.
   call record
   lr $8,$41
   call check_float_arith
0: il32 $9,0x7EFFFFFF
   fma $3,$37,$9,$32    # Maximum-magnitude normal value; should not set DIFF.
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED    # Check extended-range arithmetic.
0: fma $3,$37,$42,$32   # normal * normal + normal = extended
   call record
   lr $8,$46
   call check_float_arith_diff
0: fma $3,$42,$37,$32
   call record
   lr $8,$46
   call check_float_arith_diff
0: fma $3,$37,$42,$39
   call record
   lr $8,$46
   call check_float_arith_diff
0: fma $3,$33,$42,$42
   call record
   lr $8,$46
   call check_float_arith_diff
0: fma $3,$34,$46,$32   # normal * extended + normal = normal
   call record
   lr $8,$42
   call check_float_arith_diff
0: fma $3,$46,$34,$32
   call record
   lr $8,$42
   call check_float_arith_diff
0: fma $3,$33,$46,$58
   call record
   lr $8,$42
   call check_float_arith_diff
0: fma $3,$44,$40,$53   # Maximum exponent * minimum exponent.
   call record
   lr $8,$37
   call check_float_arith_diff
0: il32 $8,0x35000000   # Addend c equal to ulp(a*b).
   fma $3,$44,$40,$8
   call record
   il32 $8,0x40800001
   call check_float_arith_diff
0: fma $3,$44,$40,$56   # Addend c with opposite sign and tiny magnitude.
   call record
   il32 $8,0x407FFFFF
   call check_float_arith_diff
0: fma $3,$44,$40,$55   # normal * extended + denormal = normal
   call record
   il32 $8,0x40800000
   call check_float_arith_diff
0: fma $3,$37,$44,$32   # normal * extended + normal = overflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$44,$37,$32
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$33,$46,$42
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$33,$44,$44   # normal * extended + extended = overflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$42,$42,$42   # normal * normal + normal = superoverflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$46,$46,$46   # extended * extended + extended = superoverflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$37,$60,$32   # Check overflow sign.
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fma $3,$42,$58,$58
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fma $3,$32,$44,$33   # zero * extended + normal = normal
   call record
   lr $8,$33
   call check_float_arith_diff
0: fma $3,$44,$32,$33
   call record
   lr $8,$33
   call check_float_arith_diff
0: fma $3,$32,$44,$39   # zero * extended + denormal = zero
   call record
   lr $8,$32
   call check_float_arith_diff
0: fma $3,$33,$33,$44   # small_normal * small_normal + extended = extended
   call record
   lr $8,$44
   call check_float_arith_diff
0: fma $3,$33,$49,$44   # small_normal * -(small_normal) + extended = extended - 1ulp
   call record
   lr $8,$43
   call check_float_arith_diff
0: fma $3,$32,$49,$44   # zero * -(small_normal) + extended = extended
   call record
   lr $8,$44
   call check_float_arith_diff
0: fma $3,$33,$42,$44   # normal * normal + extended = extended
   call record
   ilhu $8,0x7FF0
   call check_float_arith_diff
0: fma $3,$37,$42,$44   # normal * normal + extended = overflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$38,$42,$44   # normal * normal + extended = superoverflow
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fma $3,$37,$43,$33   # Exactly Smax after rounding; should not overflow.
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   # fnms
   # These are identical to the tests for fma except that the second operand
   # is negated in each case.
0: fnms $3,$33,$49,$33
   call record
   lr $8,$37
   call check_float_arith
0: fnms $3,$34,$53,$34
   call record
   lr $8,$35
   call check_float_arith
0: fnms $3,$32,$48,$33
   call record
   lr $8,$33
   call check_float_arith
0: fnms $3,$33,$49,$32
   call record
   lr $8,$33
   call check_float_arith
0: fnms $3,$33,$48,$32
   call record
   lr $8,$32
   call check_float_arith
0: fnms $3,$32,$49,$32
   call record
   lr $8,$32
   call check_float_arith
0: fnms $3,$33,$32,$48
   call record
   lr $8,$32
   call check_float_arith
0: fnms $3,$35,$52,$32
   call record
   ai $8,$38,-1
   call check_float_arith
0: ilhu $8,0x3380
   fnms $3,$35,$52,$8
   call record
   lr $8,$38
   call check_float_arith
0: fnms $3,$37,$55,$40
   call record
   lr $8,$40
   call check_float_arith_diff
0: fnms $3,$39,$53,$40
   call record
   lr $8,$40
   call check_float_arith_diff
0: fnms $3,$33,$56,$39
   call record
   lr $8,$40
   call check_float_arith_diff
0: fnms $3,$33,$56,$55
   call record
   lr $8,$40
   call check_float_arith_diff
0: fnms $3,$34,$56,$32
   call record
   lr $8,$32
   call check_float_arith_unf
0: fnms $3,$40,$40,$40
   call record
   lr $8,$32
   call check_float_arith_unf
0: fnms $3,$34,$56,$40
   call record
   lr $8,$41
   call check_float_arith
0: il32 $9,0xFEFFFFFF
   fnms $3,$37,$9,$32
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED
0: fnms $3,$37,$58,$32
   call record
   lr $8,$46
   call check_float_arith_diff
0: fnms $3,$42,$53,$32
   call record
   lr $8,$46
   call check_float_arith_diff
0: fnms $3,$37,$58,$39
   call record
   lr $8,$46
   call check_float_arith_diff
0: fnms $3,$33,$58,$42
   call record
   lr $8,$46
   call check_float_arith_diff
0: fnms $3,$34,$62,$32
   call record
   lr $8,$42
   call check_float_arith_diff
0: fnms $3,$46,$50,$32
   call record
   lr $8,$42
   call check_float_arith_diff
0: fnms $3,$33,$62,$58
   call record
   lr $8,$42
   call check_float_arith_diff
0: fnms $3,$44,$56,$53
   call record
   lr $8,$37
   call check_float_arith_diff
0: il32 $8,0x35000000
   fnms $3,$44,$56,$8
   call record
   il32 $8,0x40800001
   call check_float_arith_diff
0: fnms $3,$44,$56,$56
   call record
   il32 $8,0x407FFFFF
   call check_float_arith_diff
0: fnms $3,$44,$56,$55
   call record
   il32 $8,0x40800000
   call check_float_arith_diff
0: fnms $3,$37,$60,$32
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$44,$53,$32
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$33,$62,$42
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$33,$60,$44
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$42,$58,$42
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$46,$62,$46
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$37,$44,$32
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fnms $3,$42,$42,$58
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fnms $3,$32,$60,$33
   call record
   lr $8,$33
   call check_float_arith_diff
0: fnms $3,$44,$48,$33
   call record
   lr $8,$33
   call check_float_arith_diff
0: fnms $3,$32,$60,$39
   call record
   lr $8,$32
   call check_float_arith_diff
0: fnms $3,$33,$49,$44
   call record
   lr $8,$44
   call check_float_arith_diff
0: fnms $3,$33,$33,$44
   call record
   lr $8,$43
   call check_float_arith_diff
0: fnms $3,$32,$33,$44
   call record
   lr $8,$44
   call check_float_arith_diff
0: fnms $3,$33,$58,$44
   call record
   ilhu $8,0x7FF0
   call check_float_arith_diff
0: fnms $3,$37,$58,$44
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$38,$58,$44
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fnms $3,$37,$59,$33
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   # fms
   # These are identical to the tests for fms except that the third operand
   # is negated in each case.
0: fms $3,$33,$33,$49
   call record
   lr $8,$37
   call check_float_arith
0: fms $3,$34,$37,$50
   call record
   lr $8,$35
   call check_float_arith
0: fms $3,$32,$32,$49
   call record
   lr $8,$33
   call check_float_arith
0: fms $3,$33,$33,$48
   call record
   lr $8,$33
   call check_float_arith
0: fms $3,$33,$32,$48
   call record
   lr $8,$32
   call check_float_arith
0: fms $3,$32,$33,$48
   call record
   lr $8,$32
   call check_float_arith
0: fms $3,$33,$48,$32
   call record
   lr $8,$32
   call check_float_arith
0: fms $3,$35,$36,$48
   call record
   ai $8,$38,-1
   call check_float_arith
0: ilhu $8,0xB380
   fms $3,$35,$36,$8
   call record
   lr $8,$38
   call check_float_arith
0: fms $3,$37,$39,$56
   call record
   lr $8,$40
   call check_float_arith_diff
0: fms $3,$39,$37,$56
   call record
   lr $8,$40
   call check_float_arith_diff
0: fms $3,$33,$40,$55
   call record
   lr $8,$40
   call check_float_arith_diff
0: fms $3,$33,$40,$39
   call record
   lr $8,$40
   call check_float_arith_diff
0: fms $3,$34,$40,$48
   call record
   lr $8,$32
   call check_float_arith_unf
0: fms $3,$40,$56,$56
   call record
   lr $8,$32
   call check_float_arith_unf
0: fms $3,$34,$40,$56
   call record
   lr $8,$41
   call check_float_arith
0: il32 $9,0x7EFFFFFF
   fms $3,$37,$9,$48
   call record
   lr $8,$43
   call check_float_arith
.if TEST_FP_EXTENDED
0: fms $3,$37,$42,$48
   call record
   lr $8,$46
   call check_float_arith_diff
0: fms $3,$42,$37,$48
   call record
   lr $8,$46
   call check_float_arith_diff
0: fms $3,$37,$42,$55
   call record
   lr $8,$46
   call check_float_arith_diff
0: fms $3,$33,$42,$58
   call record
   lr $8,$46
   call check_float_arith_diff
0: fms $3,$34,$46,$48
   call record
   lr $8,$42
   call check_float_arith_diff
0: fms $3,$46,$34,$48
   call record
   lr $8,$42
   call check_float_arith_diff
0: fms $3,$33,$46,$42
   call record
   lr $8,$42
   call check_float_arith_diff
0: fms $3,$44,$40,$37
   call record
   lr $8,$37
   call check_float_arith_diff
0: il32 $8,0xB5000000
   fms $3,$44,$40,$8
   call record
   il32 $8,0x40800001
   call check_float_arith_diff
0: fms $3,$44,$40,$40
   call record
   il32 $8,0x407FFFFF
   call check_float_arith_diff
0: fms $3,$44,$40,$39
   call record
   il32 $8,0x40800000
   call check_float_arith_diff
0: fms $3,$37,$44,$48
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$44,$37,$48
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$33,$46,$58
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$33,$44,$60
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$42,$42,$58
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$46,$46,$62
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$37,$60,$48
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fms $3,$42,$58,$42
   call record
   lr $8,$63
   call check_float_arith_ovf
0: fms $3,$32,$44,$49
   call record
   lr $8,$33
   call check_float_arith_diff
0: fms $3,$44,$32,$49
   call record
   lr $8,$33
   call check_float_arith_diff
0: fms $3,$32,$44,$55
   call record
   lr $8,$32
   call check_float_arith_diff
0: fms $3,$33,$33,$60
   call record
   lr $8,$44
   call check_float_arith_diff
0: fms $3,$33,$49,$60
   call record
   lr $8,$43
   call check_float_arith_diff
0: fms $3,$32,$49,$60
   call record
   lr $8,$44
   call check_float_arith_diff
0: fms $3,$33,$42,$60
   call record
   ilhu $8,0x7FF0
   call check_float_arith_diff
0: fms $3,$37,$42,$60
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$38,$42,$60
   call record
   lr $8,$47
   call check_float_arith_ovf
0: fms $3,$37,$43,$49
   call record
   lr $8,$47
   call check_float_arith_diff
.endif  # TEST_FP_EXTENDED

   ########################################################################
   # 9. Floating-Point Instructions - frest, frsqest, fi
   ########################################################################

   # In order to allow for implementations that compute an exact inverse
   # and implement fi as an effective no-op, we use the sequence of
   # instructions documented to give an IEEE-equivalent result and only
   # test that result against the expected value.

.macro fir rt,ra
   fi $8,\ra,\rt
   fnms $9,\ra,$8,$33
   fma \rt,$9,$8,$8
.endm

.macro firsq rt,ra
   fi $8,\ra,\rt
   fm $9,\ra,$8
   fm $10,$8,$34
   fnms $9,$9,$8,$33
   fma \rt,$9,$10,$8
.endm

   # frest (and fi)
0: frest $3,$33
   call record
   fir $3,$33
   lr $8,$33
   call check_float_recip
0: frest $3,$37
   call record
   fir $3,$37
   lr $8,$34
   call check_float_recip
0: frest $3,$53
   call record
   fir $3,$53
   lr $8,$50
   call check_float_recip
0: frest $3,$42
   call record
   fscrrd $10       # Hide the DIFF exception from check_float_recip.
   fir $3,$42
   fscrwr $10
   lr $8,$32
   call check_float_recip
.if TEST_FP_EXTENDED
0: frest $3,$47
   call record
   fscrrd $10
   fir $3,$47
   fscrwr $10
   lr $8,$32
   call check_float_recip
.endif
0: frest $3,$32
   call record
   fir $3,$32
   lr $8,$47
   call check_float_recip_dbz
   # The SPU ISA documentation suggests that the recommended instruction
   # sequence for interpolation produces a positive output for both
   # positive and negative zeros:
   #    Zeros: 1/0 is defined to give the maximum SPU single-precision
   #           extended-range floating point (sfp) number:
   #              y2 = x'7FFF FFFF' (1.999... * 2^128)
   # However, a real SPU returns a negative output for negative input.
   # It's unclear whether this is an error in the documentation or a bug
   # in the hardware, but either way we check for the actual behavior.
0: frest $3,$48
   call record
   fir $3,$48
   lr $8,$63
   call check_float_recip_dbz
0: fsmbi $8,0x0FFF  # Check that DBZ0 is in the right place.
   selb $10,$32,$33,$8
   frest $3,$10
   call record
   fir $3,$10
   fscrrd $3
   il $8,0x800
   fsmbi $9,0x000F
   selb $8,$32,$8,$9
   il $10,5
   fsmbi $9,0x0FFF
   selb $8,$10,$8,$9
   call check_value
   fscrwr $32

   # frsqest (and fi)
0: il32 $16,0x40800000  # 4.0f
   il32 $17,0x3E800000  # 0.25f
   il32 $18,0xBE800000  # -0.25f
   frsqest $3,$33
   call record
   firsq $3,$33
   lr $8,$33
   call check_float_recip
0: frsqest $3,$16
   call record
   firsq $3,$16
   lr $8,$34
   call check_float_recip
0: frsqest $3,$17
   call record
   firsq $3,$17
   lr $8,$37
   call check_float_recip
0: frsqest $3,$18   # The sign of the operand should be ignored.
   call record
   firsq $3,$17
   lr $8,$37
   call check_float_recip
.if TEST_FP_EXTENDED
0: frsqest $3,$44   # 2^128
   call record
   fscrrd $11
   firsq $3,$44
   fscrwr $11
   ilhu $8,0x1F80   # 2^-64
   call check_float_recip
.endif
0: frsqest $3,$32
   call record
   firsq $3,$32
   lr $8,$47
   call check_float_recip_dbz
0: frsqest $3,$48
   call record
   firsq $3,$32
   lr $8,$47
   call check_float_recip_dbz
0: fsmbi $8,0x0FFF
   selb $10,$32,$33,$8
   frsqest $3,$10
   call record
   firsq $3,$10
   fscrrd $3
   il $8,0x800
   fsmbi $9,0x000F
   selb $8,$32,$8,$9
   il $10,5
   fsmbi $9,0x0FFF
   selb $8,$10,$8,$9
   call check_value
   fscrwr $32

   ########################################################################
   # 9. Floating-Point Instructions - single-precision comparison
   ########################################################################

   # fceq
0: fceq $3,$33,$33
   call record
   il $8,-1
   call check_value
0: fceq $3,$33,$32
   call record
   il $8,0
   call check_value
0: fsmbi $9,0xF00F
   selb $3,$32,$33,$9
   fceq $3,$3,$33
   call record
   il $8,-1
   selb $8,$32,$8,$9
0: fceq $3,$33,$49
   call record
   il $8,0
   call check_value
0: fceq $3,$32,$48  # +0.0 and -0.0 should compare equal.
   call record
   il $8,-1
   call check_value
0: fceq $3,$32,$39  # Denormals should compare equal to zero.
   call record
   il $8,-1
   call check_value
0: fceq $3,$55,$32
   call record
   il $8,-1
   call check_value
.if TEST_FP_EXTENDED
0: fceq $3,$32,$47
   call record
   il $8,0
   call check_value
0: fceq $3,$47,$44
   call record
   il $8,0
   call check_value
0: fceq $3,$47,$47
   call record
   il $8,-1
   call check_value
.endif

   # fcmeq
0: fcmeq $3,$33,$33
   call record
   il $8,-1
   call check_value
0: fcmeq $3,$33,$32
   call record
   il $8,0
   call check_value
0: fsmbi $9,0xF00F
   selb $3,$32,$33,$9
   fcmeq $3,$3,$33
   call record
   il $8,-1
   selb $8,$32,$8,$9
0: fcmeq $3,$33,$49
   call record
   il $8,-1
   call check_value
0: fcmeq $3,$32,$48
   call record
   il $8,-1
   call check_value
0: fcmeq $3,$32,$39
   call record
   il $8,-1
   call check_value
0: fcmeq $3,$55,$32
   call record
   il $8,-1
   call check_value
.if TEST_FP_EXTENDED
0: fcmeq $3,$32,$47
   call record
   il $8,0
   call check_value
0: fcmeq $3,$47,$44
   call record
   il $8,0
   call check_value
0: fcmeq $3,$47,$47
   call record
   il $8,-1
   call check_value
.endif

   # fcgt
0: fcgt $3,$33,$33
   call record
   il $8,0
   call check_value
0: fcgt $3,$33,$32
   call record
   il $8,-1
   call check_value
0: fcgt $3,$32,$33
   call record
   il $8,0
   call check_value
0: fsmbi $9,0xF00F
   selb $3,$32,$33,$9
   fcgt $3,$3,$32
   call record
   il $8,-1
   selb $8,$32,$8,$9
0: fcgt $3,$32,$49
   call record
   il $8,-1
   call check_value
0: fcgt $3,$49,$32
   call record
   il $8,0
   call check_value
0: fcgt $3,$33,$53
   call record
   il $8,-1
   call check_value
0: fcgt $3,$49,$53
   call record
   il $8,-1
   call check_value
0: fcgt $3,$53,$49
   call record
   il $8,0
   call check_value
0: fcgt $3,$53,$33
   call record
   il $8,0
   call check_value
0: fcgt $3,$32,$48
   call record
   il $8,0
   call check_value
0: fcgt $3,$39,$32
   call record
   il $8,0
   call check_value
0: fcgt $3,$32,$55
   call record
   il $8,0
   call check_value
.if TEST_FP_EXTENDED
0: fcgt $3,$44,$32
   call record
   il $8,-1
   call check_value
0: fcgt $3,$60,$32
   call record
   il $8,0
   call check_value
0: fcgt $3,$32,$60
   call record
   il $8,-1
   call check_value
0: fcgt $3,$47,$44
   call record
   il $8,-1
   call check_value
0: fcgt $3,$63,$44
   call record
   il $8,0
   call check_value
0: fcgt $3,$44,$63
   call record
   il $8,-1
   call check_value
0: fcgt $3,$47,$47
   call record
   il $8,0
   call check_value
.endif

   # fcmgt
0: fcmgt $3,$33,$33
   call record
   il $8,0
   call check_value
0: fcmgt $3,$33,$32
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$32,$33
   call record
   il $8,0
   call check_value
0: fsmbi $9,0xF00F
   selb $3,$32,$33,$9
   fcmgt $3,$3,$32
   call record
   il $8,-1
   selb $8,$32,$8,$9
0: fcmgt $3,$32,$49
   call record
   il $8,0
   call check_value
0: fcmgt $3,$49,$32
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$33,$53
   call record
   il $8,0
   call check_value
0: fcmgt $3,$49,$53
   call record
   il $8,0
   call check_value
0: fcmgt $3,$53,$49
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$53,$33
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$32,$48
   call record
   il $8,0
   call check_value
0: fcmgt $3,$39,$32
   call record
   il $8,0
   call check_value
0: fcmgt $3,$32,$55
   call record
   il $8,0
   call check_value
.if TEST_FP_EXTENDED
0: fcmgt $3,$44,$32
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$60,$32
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$32,$60
   call record
   il $8,0
   call check_value
0: fcmgt $3,$47,$44
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$63,$44
   call record
   il $8,-1
   call check_value
0: fcmgt $3,$44,$63
   call record
   il $8,0
   call check_value
0: fcmgt $3,$47,$47
   call record
   il $8,0
   call check_value
.endif

   ########################################################################
   # 9. Floating-Point Instructions - double-precision arithmetic
   ########################################################################

0: lqr $24,1f
   lqr $25,1f+16
   lqr $26,1f+32
   lqr $27,1f+48
   lqr $28,1f+64
   lqr $29,1f+80
   lqr $30,1f+96
   lqr $31,1f+112
   br 0f
   .balign 16
1: .int 0x3FF00000,0x00000000,0x40000000,0x00000000
   .int 0xC0000000,0x00000000,0xFFF00000,0x00000000
   .int 0x7FF00000,0x00000000,0x7FF00000,0x00000000
   .int 0x7FFA0000,0x00000000,0x7FF40000,0x00000000
   .int 0x80000000,0x00000000,0x3FE00000,0x00000000
   .int 0x00080000,0x00000000,0x00100000,0x00000000
   .int 0x7FE00000,0x00000000,0x00180000,0x00000000
   .int 0x80080000,0x00000000,0x80100000,0x00000000

   # dfa
0: dfa $3,$24,$25
   call record
   call check_double_literal
   .int 0xBFF00000,0x00000000,0xFFF00000,0x00000000
0: dfa $3,$25,$26
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: dfa $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: dfa $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: dfa $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x3FE00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000
0: dfa $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x00280000,0x00000000
   .int 0x00000000,0x00002800,0x00000000,0x00000000
0: dfa $3,$30,$31
   call record
   call check_double_fpscr_literal
   .int 0x7FE00000,0x00000000,0x00080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000

   # dfs
0: dfs $3,$24,$25
   call record
   call check_double_literal
   .int 0x40080000,0x00000000,0x7FF00000,0x00000000
0: dfs $3,$25,$25
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: dfs $3,$27,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: dfs $3,$29,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: dfs $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x3FE00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000
0: dfs $3,$30,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FE00000,0x00000000,0x00080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: dfs $3,$31,$29
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x80200000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000

   # dfm
0: dfm $3,$24,$25
   call record
   call check_double_literal
   .int 0xC0000000,0x00000000,0xFFF00000,0x00000000
0: dfm $3,$25,$26
   call record
   call check_double_literal
   .int 0xFFF00000,0x00000000,0xFFF00000,0x00000000
0: dfm $3,$26,$28
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000400,0x00000000,0x00000000
0: dfm $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: dfm $3,$26,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000500,0x00000000,0x00000000
0: dfm $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: dfm $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x00080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: dfm $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00002800,0x00001800,0x00000000
0: dfm $3,$31,$30
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x80000000,0x00000000
   .int 0x00000000,0x00000100,0x00001800,0x00000000

   # dfma
0: lr $3,$24
   dfma $3,$24,$24
   call record
   call check_double_literal
   .int 0x40000000,0x00000000,0x40180000,0x00000000
0: il $3,0
   dfma $3,$24,$25
   call record
   call check_double_literal
   .int 0xC0000000,0x00000000,0xFFF00000,0x00000000
0: il $3,0
   dfma $3,$25,$26
   call record
   call check_double_literal
   .int 0xFFF00000,0x00000000,0xFFF00000,0x00000000
0: il $3,0
   dfma $3,$26,$28
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000400,0x00000000,0x00000000
0: il $3,0
   dfma $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfma $3,$27,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfma $3,$26,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000500,0x00000000,0x00000000
0: il $3,0
   dfma $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: lr $3,$27
   dfma $3,$29,$26
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000700,0x00000600,0x00000000
0: il $3,0
   dfma $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x00080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: il $3,0
   dfma $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00002800,0x00001800,0x00000000
0: il $3,0
   dfma $3,$31,$30
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x80000000,0x00000000
   .int 0x00000000,0x00000100,0x00001800,0x00000000
0: lr $3,$26
   dfma $3,$25,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: lr $3,$27
   dfma $3,$25,$25
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: lr $3,$29
   dfma $3,$28,$28
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x3FD00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000
   # Check that the product is not rounded.
0: il $8,0x500
   shlqbyi $8,$8,12
   fscrwr $8
   lqr $3,1f+32
   lqr $8,1f
   lqr $9,1f+16
   dfma $3,$8,$9
   call record
   lqr $8,1f+48
   lqr $9,1f+64
   call check_double_fpscr
   fscrwr $32
   br 0f
   .balign 16
1: .int 0x3FF55555,0x55555555,0x3FF55555,0x55555555
   .int 0x3FF80000,0x00000000,0x3FF80000,0x00000000
   .int 0x00000000,0x00000000,0x3CA00000,0x00000000
   .int 0x3FFFFFFF,0xFFFFFFFF,0x40000000,0x00000000
   .int 0x00000500,0x00000800,0x00000000,0x00000000

   # dfnms
0: lr $3,$24
   dfnms $3,$24,$24
   call record
   call check_double_literal
   .int 0x80000000,0x00000000,0xC0000000,0x00000000
0: il $3,0
   dfnms $3,$24,$25
   call record
   call check_double_literal
   .int 0x40000000,0x00000000,0x7FF00000,0x00000000
0: il $3,0
   dfnms $3,$25,$26
   call record
   call check_double_literal
   .int 0x7FF00000,0x00000000,0x7FF00000,0x00000000
0: il $3,0
   dfnms $3,$26,$28
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0xFFF00000,0x00000000
   .int 0x00000000,0x00000400,0x00000000,0x00000000
0: il $3,0
   dfnms $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfnms $3,$27,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfnms $3,$26,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0xFFF00000,0x00000000
   .int 0x00000000,0x00000500,0x00000000,0x00000000
0: il $3,0
   dfnms $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: lr $3,$27
   dfnms $3,$29,$26
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000700,0x00000600,0x00000000
0: il $3,0
   dfnms $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x80080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: il $3,0
   dfnms $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0xFFF00000,0x00000000,0x80000000,0x00000000
   .int 0x00000000,0x00002800,0x00001800,0x00000000
0: il $3,0
   dfnms $3,$31,$30
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00000100,0x00001800,0x00000000
0: lr $3,$25
   dfnms $3,$25,$24
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: lr $3,$27
   dfnms $3,$25,$25
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: lr $3,$29
   dfnms $3,$28,$28
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0xBFD00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000

   # dfms
0: lr $3,$24
   dfms $3,$24,$24
   call record
   call check_double_literal
   .int 0x00000000,0x00000000,0x40000000,0x00000000
0: il $3,0
   dfms $3,$24,$25
   call record
   call check_double_literal
   .int 0xC0000000,0x00000000,0xFFF00000,0x00000000
0: il $3,0
   dfms $3,$25,$26
   call record
   call check_double_literal
   .int 0xFFF00000,0x00000000,0xFFF00000,0x00000000
0: il $3,0
   dfms $3,$26,$28
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000400,0x00000000,0x00000000
0: il $3,0
   dfms $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfms $3,$27,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfms $3,$26,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF00000,0x00000000
   .int 0x00000000,0x00000500,0x00000000,0x00000000
0: il $3,0
   dfms $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: lr $3,$27
   dfms $3,$29,$26
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000700,0x00000600,0x00000000
0: il $3,0
   dfms $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x00080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: il $3,0
   dfms $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0x7FF00000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00002800,0x00001800,0x00000000
0: il $3,0
   dfms $3,$31,$30
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x80000000,0x00000000
   .int 0x00000000,0x00000100,0x00001800,0x00000000
0: lr $3,$25
   dfms $3,$25,$24
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: lr $3,$27
   dfms $3,$25,$25
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: lr $3,$29
   dfms $3,$28,$28
   call record
   call check_double_fpscr_literal
   .int 0x00000000,0x00000000,0x3FD00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000

   # dfnma
0: lr $3,$24
   dfnma $3,$24,$24
   call record
   call check_double_literal
   .int 0xC0000000,0x00000000,0xC0180000,0x00000000
0: il $3,0
   dfnma $3,$24,$25
   call record
   call check_double_literal
   .int 0x40000000,0x00000000,0x7FF00000,0x00000000
0: il $3,0
   dfnma $3,$25,$26
   call record
   call check_double_literal
   .int 0x7FF00000,0x00000000,0x7FF00000,0x00000000
0: il $3,0
   dfnma $3,$26,$28
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0xFFF00000,0x00000000
   .int 0x00000000,0x00000400,0x00000000,0x00000000
0: il $3,0
   dfnma $3,$24,$27
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfnma $3,$27,$24
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: il $3,0
   dfnma $3,$26,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0xFFF00000,0x00000000
   .int 0x00000000,0x00000500,0x00000000,0x00000000
0: il $3,0
   dfnma $3,$27,$29
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000300,0x00000600,0x00000000
0: lr $3,$27
   dfnma $3,$29,$26
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000700,0x00000600,0x00000000
0: il $3,0
   dfnma $3,$28,$29
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x80080000,0x00000000
   .int 0x00000000,0x00000100,0x00000000,0x00000000
0: il $3,0
   dfnma $3,$30,$30
   call record
   call check_double_fpscr_literal
   .int 0xFFF00000,0x00000000,0x80000000,0x00000000
   .int 0x00000000,0x00002800,0x00001800,0x00000000
0: il $3,0
   dfnma $3,$31,$30
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00000100,0x00001800,0x00000000
0: lr $3,$26
   dfnma $3,$25,$24
   call record
   call check_double_fpscr_literal
   .int 0xFFF00000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000000,0x00000400,0x00000000
0: lr $3,$27
   dfnma $3,$25,$25
   call record
   call check_double_fpscr_literal
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000200,0x00000600,0x00000000
0: lr $3,$29
   dfnma $3,$28,$28
   call record
   call check_double_fpscr_literal
   .int 0x80000000,0x00000000,0xBFD00000,0x00000000
   .int 0x00000000,0x00000100,0x00000800,0x00000000

   ########################################################################
   # 9. Floating-Point Instructions - type conversion
   ########################################################################

   # csflt
0: csflt $3,$32,0
   call record
   lr $8,$32
   call check_float_arith
0: il $8,1
   csflt $3,$8,0
   call record
   lr $8,$33
   call check_float_arith
0: il $9,1
   csflt $3,$9,1
   call record
   lr $8,$34
   call check_float_arith
0: il $10,-1
   csflt $3,$10,0
   call record
   lr $8,$49
   call check_float_arith
0: il32 $11,0x7FFFFFFF
   csflt $3,$11,0
   call record
   il32 $8,0x4EFFFFFF
   call check_float_arith
0: il32 $12,0x80000000
   csflt $3,$12,0
   call record
   il32 $8,0xCF000000
   call check_float_arith
0: il $13,1
   csflt $3,$13,126
   call record
   lr $8,$40
   call check_float_arith
0: il $14,3
   csflt $3,$14,127
   call record
   lr $8,$41
   call check_float_arith
   # The SPU ISA documentation claims that the integer conversion
   # instructions can never set exception bits, but in this specific case
   # (denormal result), a real SPU reports UNF+DIFF as it would for a
   # denormal result from any other instruction.
0: il $15,1
   csflt $3,$15,127
   call record
   lr $8,$32
   call check_float_arith_unf
0: il $16,-1        # Check for a negative value as well.
   csflt $3,$16,127
   call record
   lr $8,$32
   call check_float_arith_unf

   # cflts
0: cflts $3,$32,0
   call record
   lr $8,$32
   call check_float_arith
0: cflts $3,$33,0
   call record
   il $8,1
   call check_float_arith
0: cflts $3,$34,1
   call record
   il $8,1
   call check_float_arith
0: cflts $3,$49,0
   call record
   il $8,-1
   call check_float_arith
0: il32 $11,0x4EFFFFFF
   cflts $3,$11,0
   call record
   il32 $8,0x7FFFFF80
   call check_float_arith
0: il32 $12,0xCF000000
   cflts $3,$12,0
   call record
   il32 $8,0x80000000
   call check_float_arith
0: cflts $3,$43,0
   call record
   il32 $8,0x7FFFFFFF
   call check_float_arith
0: cflts $3,$59,0
   call record
   il32 $8,0x80000000
   call check_float_arith
.if TEST_FP_EXTENDED
0: cflts $3,$47,0
   call record
   il32 $8,0x7FFFFFFF
   call check_float_arith
0: cflts $3,$63,0
   call record
   il32 $8,0x80000000
   call check_float_arith
.endif
0: cflts $3,$40,126
   call record
   il $8,1
   call check_float_arith
0: cflts $3,$41,127
   call record
   il $8,3
   call check_float_arith

   # cuflt
0: cuflt $3,$32,0
   call record
   lr $8,$32
   call check_float_arith
0: il $8,1
   cuflt $3,$8,0
   call record
   lr $8,$33
   call check_float_arith
0: il $9,1
   cuflt $3,$9,1
   call record
   lr $8,$34
   call check_float_arith
0: il $10,-1
   cuflt $3,$10,0
   call record
   il32 $8,0x4F7FFFFF
   call check_float_arith
0: il32 $12,0x80000000
   cuflt $3,$12,0
   call record
   il32 $8,0x4F000000
   call check_float_arith
0: il $13,1
   cuflt $3,$13,126
   call record
   lr $8,$40
   call check_float_arith
0: il $14,3
   cuflt $3,$14,127
   call record
   lr $8,$41
   call check_float_arith
0: il $15,1
   cuflt $3,$15,127
   call record
   lr $8,$32
   call check_float_arith_unf

   # cfltu
0: cfltu $3,$32,0
   call record
   lr $8,$32
   call check_float_arith
0: cfltu $3,$33,0
   call record
   il $8,1
   call check_float_arith
0: cfltu $3,$34,1
   call record
   il $8,1
   call check_float_arith
0: cfltu $3,$49,0
   call record
   il $8,0
   call check_float_arith
0: il32 $11,0x4F7FFFFF
   cfltu $3,$11,0
   call record
   il32 $8,0xFFFFFF00
   call check_float_arith
0: cfltu $3,$43,0
   call record
   il32 $8,0xFFFFFFFF
   call check_float_arith
0: cfltu $3,$59,0
   call record
   il32 $8,0
   call check_float_arith
.if TEST_FP_EXTENDED
0: cfltu $3,$47,0
   call record
   il32 $8,0xFFFFFFFF
   call check_float_arith
0: cfltu $3,$63,0
   call record
   il32 $8,0
   call check_float_arith
.endif
0: cfltu $3,$40,126
   call record
   il $8,1
   call check_float_arith
0: cfltu $3,$41,127
   call record
   il $8,3
   call check_float_arith

   # frds
0: lqr $8,9f
   frds $3,$8
   call record
   lqr $8,9f+16
   il $9,0
   call check_float_fpscr
   br 0f
0: lqr $9,9f+32
   frds $3,$9
   call record
   lqr $8,9f+48
   il $9,0
   call check_float_fpscr
   br 0f
0: lqr $10,9f+64
   frds $3,$10
   call record
   lqr $8,9f+80
   il $9,0
   call check_float_fpscr
   br 0f
0: lqr $11,9f+128
   frds $3,$11
   call record
   lqr $8,9f+144
   lqr $9,9f+160
   call check_float_fpscr
   br 0f
0: lqr $12,9f+176
   frds $3,$12
   call record
   lqr $8,9f+192
   lqr $9,9f+208
   call check_float_fpscr
   br 0f
0: lqr $13,9f+224
   frds $3,$13
   call record
   lqr $8,9f+240
   lqr $9,9f+288
   call check_float_fpscr
   br 0f
0: lqr $14,9f+304
   frds $3,$14
   call record
   lqr $8,9f+320
   lqr $9,9f+336
   call check_float_fpscr
   br 0f
0: lqr $15,9f+304
   il $8,0x500
   shlqbyi $8,$8,12
   fscrwr $8
   frds $3,$15
   call record
   lqr $8,9f+352
   lqr $9,9f+368
   call check_float_fpscr
   br 0f
0: lqr $16,9f+304
   il $8,0xB00
   shlqbyi $8,$8,12
   fscrwr $8
   frds $3,$16
   call record
   lqr $8,9f+384
   lqr $9,9f+400
   call check_float_fpscr
   br 0f

   .balign 16
9: .int 0x00000000,0x00000000,0x3FF00000,0x00000000
   .int 0x00000000,0x00000000,0x3F800000,0x00000000

   .int 0x80000000,0x00000000,0xBFF00000,0x00000000
   .int 0x80000000,0x00000000,0xBF800000,0x00000000

   .int 0x38100000,0x00000000,0x38000000,0x00000000
   .int 0x00800000,0x00000000,0x00400000,0x00000000
   .int 0x38100000,0x00000000,0x00000000,0x00000000
   .int 0x00000000,0x00000000,0x00000100,0x00000000

   .int 0x00100000,0x00000000,0x7FE00000,0x00000000
   .int 0x00000000,0x00000000,0x7F800000,0x00000000
   .int 0x00000000,0x00001800,0x00002800,0x00000000

   .int 0x7FF00000,0x00000000,0x7FF80000,0x00000000
   .int 0x7F800000,0x00000000,0x7FC00000,0x00000000
   .int 0x00000000,0x00000000,0x00000200,0x00000000

   .int 0x7FF40000,0x00000000,0x7FFFFFFF,0xFFFFFFFF
   .int 0x7FC00000,0x00000000,0x7FC00000,0x00000000
   .int 0x7FA00000,0x00000000,0x7FFFFFFF,0x00000000
   .int 0x7FF80000,0x00000000,0x7FF80000,0x00000000
   .int 0x00000000,0x00000600,0x00000200,0x00000000

   .int 0x3FF00000,0x08000000,0x3FF00000,0x18000000
   .int 0x3F800000,0x00000000,0x3F800001,0x00000000
   .int 0x00000000,0x00000800,0x00000800,0x00000000
   .int 0x3F800000,0x00000000,0x3F800000,0x00000000
   .int 0x00000500,0x00000800,0x00000800,0x00000000
   .int 0x3F800001,0x00000000,0x3F800000,0x00000000
   .int 0x00000B00,0x00000800,0x00000800,0x00000000

   # fesd
0: fsmbi $31,0x00FF
   selb $8,$32,$33,$31
0: lqr $8,9b+16
   fesd $3,$8
   call record
   lqr $8,9b
   il $9,0
   call check_float_fpscr
0: lqr $9,9b+48
   fesd $3,$9
   call record
   lqr $8,9b+32
   il $9,0
   call check_float_fpscr
0: lqr $10,9b+80
   fesd $3,$10
   call record
   lqr $8,9b+96
   lqr $9,9b+112
   call check_float_fpscr
0: lqr $12,9b+192
   fesd $3,$12
   call record
   lqr $8,9b+176
   lqr $9,9b+208
   call check_float_fpscr
0: lqr $13,9b+256
   fesd $3,$13
   call record
   lqr $8,9b+272
   lqr $9,9b+288
   call check_float_fpscr

   ########################################################################
   # 9. Floating-Point Instructions - double-precision comparison
   # (added as optional instructions in version 1.2 of the ISA)
   ########################################################################

   # dfceq, dfcmeq, dfcgt, dfcmgt, dftsv -- these instructions are
   # documented but not actually implemented on the Cell, so we don't
   # test them.

   ########################################################################
   # 10. Control Instructions
   ########################################################################

.if TEST_STOP
   # stop
0: il $3,0
   stqd $3,0($4)
   stop 0x1234
   call record
   lqd $3,0($4)
   il $8,0x1234
   ceq $8,$3,$8
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64

   # stopd
0: il $3,0
   stqd $3,0($4)
   stopd $3,$3,$3
   call record
   lqd $3,0($4)
   il $8,0x3FFF
   ceq $8,$3,$8
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
.endif

   # lnop, nop, sync, dsync
   # None of these instructions have any visible effect on program state,
   # so we just execute them to verify that they can be decoded correctly.
0: lnop
   nop $31
   sync
   syncc
   dsync

   # mfspr, mtspr
   # There are no SPRs implemented in the Cell SPU, so every register
   # should ignore writes and read as zero.  We explicitly check all
   # registers.
0: il $8,-1
   mtspr $sp0,$8
   mfspr $8,$sp0
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp1,$8
   mfspr $8,$sp1
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp2,$8
   mfspr $8,$sp2
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp3,$8
   mfspr $8,$sp3
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp4,$8
   mfspr $8,$sp4
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp5,$8
   mfspr $8,$sp5
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp6,$8
   mfspr $8,$sp6
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp7,$8
   mfspr $8,$sp7
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp8,$8
   mfspr $8,$sp8
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp9,$8
   mfspr $8,$sp9
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp10,$8
   mfspr $8,$sp10
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp11,$8
   mfspr $8,$sp11
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp12,$8
   mfspr $8,$sp12
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp13,$8
   mfspr $8,$sp13
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp14,$8
   mfspr $8,$sp14
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp15,$8
   mfspr $8,$sp15
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp16,$8
   mfspr $8,$sp16
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp17,$8
   mfspr $8,$sp17
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp18,$8
   mfspr $8,$sp18
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp19,$8
   mfspr $8,$sp19
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp20,$8
   mfspr $8,$sp20
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp21,$8
   mfspr $8,$sp21
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp22,$8
   mfspr $8,$sp22
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp23,$8
   mfspr $8,$sp23
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp24,$8
   mfspr $8,$sp24
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp25,$8
   mfspr $8,$sp25
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp26,$8
   mfspr $8,$sp26
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp27,$8
   mfspr $8,$sp27
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp28,$8
   mfspr $8,$sp28
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp29,$8
   mfspr $8,$sp29
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp30,$8
   mfspr $8,$sp30
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp31,$8
   mfspr $8,$sp31
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp32,$8
   mfspr $8,$sp32
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp33,$8
   mfspr $8,$sp33
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp34,$8
   mfspr $8,$sp34
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp35,$8
   mfspr $8,$sp35
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp36,$8
   mfspr $8,$sp36
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp37,$8
   mfspr $8,$sp37
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp38,$8
   mfspr $8,$sp38
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp39,$8
   mfspr $8,$sp39
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp40,$8
   mfspr $8,$sp40
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp41,$8
   mfspr $8,$sp41
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp42,$8
   mfspr $8,$sp42
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp43,$8
   mfspr $8,$sp43
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp44,$8
   mfspr $8,$sp44
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp45,$8
   mfspr $8,$sp45
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp46,$8
   mfspr $8,$sp46
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp47,$8
   mfspr $8,$sp47
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp48,$8
   mfspr $8,$sp48
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp49,$8
   mfspr $8,$sp49
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp50,$8
   mfspr $8,$sp50
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp51,$8
   mfspr $8,$sp51
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp52,$8
   mfspr $8,$sp52
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp53,$8
   mfspr $8,$sp53
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp54,$8
   mfspr $8,$sp54
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp55,$8
   mfspr $8,$sp55
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp56,$8
   mfspr $8,$sp56
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp57,$8
   mfspr $8,$sp57
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp58,$8
   mfspr $8,$sp58
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp59,$8
   mfspr $8,$sp59
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp60,$8
   mfspr $8,$sp60
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp61,$8
   mfspr $8,$sp61
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp62,$8
   mfspr $8,$sp62
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp63,$8
   mfspr $8,$sp63
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp64,$8
   mfspr $8,$sp64
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp65,$8
   mfspr $8,$sp65
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp66,$8
   mfspr $8,$sp66
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp67,$8
   mfspr $8,$sp67
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp68,$8
   mfspr $8,$sp68
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp69,$8
   mfspr $8,$sp69
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp70,$8
   mfspr $8,$sp70
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp71,$8
   mfspr $8,$sp71
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp72,$8
   mfspr $8,$sp72
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp73,$8
   mfspr $8,$sp73
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp74,$8
   mfspr $8,$sp74
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp75,$8
   mfspr $8,$sp75
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp76,$8
   mfspr $8,$sp76
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp77,$8
   mfspr $8,$sp77
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp78,$8
   mfspr $8,$sp78
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp79,$8
   mfspr $8,$sp79
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp80,$8
   mfspr $8,$sp80
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp81,$8
   mfspr $8,$sp81
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp82,$8
   mfspr $8,$sp82
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp83,$8
   mfspr $8,$sp83
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp84,$8
   mfspr $8,$sp84
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp85,$8
   mfspr $8,$sp85
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp86,$8
   mfspr $8,$sp86
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp87,$8
   mfspr $8,$sp87
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp88,$8
   mfspr $8,$sp88
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp89,$8
   mfspr $8,$sp89
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp90,$8
   mfspr $8,$sp90
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp91,$8
   mfspr $8,$sp91
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp92,$8
   mfspr $8,$sp92
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp93,$8
   mfspr $8,$sp93
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp94,$8
   mfspr $8,$sp94
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp95,$8
   mfspr $8,$sp95
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp96,$8
   mfspr $8,$sp96
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp97,$8
   mfspr $8,$sp97
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp98,$8
   mfspr $8,$sp98
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp99,$8
   mfspr $8,$sp99
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp100,$8
   mfspr $8,$sp100
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp101,$8
   mfspr $8,$sp101
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp102,$8
   mfspr $8,$sp102
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp103,$8
   mfspr $8,$sp103
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp104,$8
   mfspr $8,$sp104
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp105,$8
   mfspr $8,$sp105
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp106,$8
   mfspr $8,$sp106
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp107,$8
   mfspr $8,$sp107
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp108,$8
   mfspr $8,$sp108
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp109,$8
   mfspr $8,$sp109
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp110,$8
   mfspr $8,$sp110
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp111,$8
   mfspr $8,$sp111
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp112,$8
   mfspr $8,$sp112
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp113,$8
   mfspr $8,$sp113
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp114,$8
   mfspr $8,$sp114
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp115,$8
   mfspr $8,$sp115
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp116,$8
   mfspr $8,$sp116
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp117,$8
   mfspr $8,$sp117
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp118,$8
   mfspr $8,$sp118
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp119,$8
   mfspr $8,$sp119
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp120,$8
   mfspr $8,$sp120
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp121,$8
   mfspr $8,$sp121
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp122,$8
   mfspr $8,$sp122
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp123,$8
   mfspr $8,$sp123
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp124,$8
   mfspr $8,$sp124
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp125,$8
   mfspr $8,$sp125
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp126,$8
   mfspr $8,$sp126
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64
0: il $8,-1
   mtspr $sp127,$8
   mfspr $8,$sp127
   call record
   orx $8,$8
   brz $8,0f
   ai $6,$6,64

   ########################################################################
   # 11. Channel Instructions
   ########################################################################
.if TEST_CHANNEL_INVALID
   # rdch, wrch
0: il32 $8,0x01234567
   wrch $ch14,$8
   rdch $3,$ch15
   call record
   il $9,0
   fsmbi $10,0xF000
   selb $8,$9,$8,$10
   il32 $9,LS_SIZE
   ai $9,$9,-4
   and $8,$8,$9
   call check_value

   # rchcnt
0: rchcnt $3,$ch14
   call record
   il $8,1
   il $9,0
   fsmbi $10,0xF000
   selb $8,$9,$8,$10
   call check_value
0: il $3,0
   rchcnt $3,$ch15
   call record
   il $8,1
   il $9,0
   fsmbi $10,0xF000
   selb $8,$9,$8,$10
   call check_value
0: rchcnt $3,$ch99  # Invalid channel.
   call record
   il $8,0
   call check_value

#.if TEST_CHANNEL_INVALID
0: il $3,0
   stqd $3,0($4)
   rdch $3,$ch14
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: il $3,0
   stqd $3,0($4)
   rdch $3,$ch79
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: il $3,0
   stqd $3,0($4)
   wrch $ch15,$3
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
0: il $3,0
   stqd $3,0($4)
   wrch $ch78,$3
   call record
   call halt_wait
   lqd $3,0($4)
   ceqi $8,$3,1
   brnz $8,0f
   stqd $3,16($6)
   ai $6,$6,64
.endif

   ########################################################################
   # End of the test code.
   ########################################################################

0: sf $3,$5,$6
   shri $3,$3,6
   bi $0

   ########################################################################
   # Subroutines to store an instruction word in the failure buffer.
   # One of these is called for every tested instruction; failures are
   # recorded by advancing the store pointer.
   # On entry:
   #     R6 = pointer at which to store instruction
   ########################################################################

record:
   ai $78,$79,-8
   lqd $77,0($78)
   andi $76,$78,0xF
   shlqby $77,$77,$76
   cwd $76,4($6)
   shufb $77,$78,$77,$76
   stqd $77,0($6)
   ret

   # Alternate version used when the subroutine call is separated by one
   # instruction from the instruction under test.  Used in testing branch
   # instructions.
record2:
   ai $78,$79,-12
   lqd $77,0($78)
   andi $76,$78,0xF
   shlqby $77,$77,$76
   cwd $76,4($6)
   shufb $77,$78,$77,$76
   stqd $77,0($6)
   ret

   # Alternate version which takes the address of the instruction under
   # test in R3.
record_r3:
   lqd $77,0($3)
   andi $76,$78,0xF
   shlqby $77,$77,$76
   cwd $76,4($6)
   shufb $77,$78,$77,$76
   stqd $77,0($6)
   ret

   ########################################################################
   # Subroutine to validate the result of an instruction.  On failure, the
   # instruction is automatically recorded in the failure table (assuming a
   # previous "bl record" at the appropriate place).
   # On entry:
   #     R3 = result of instruction
   #     R8 = expected result
   ########################################################################

check_value:
   ceq $78,$3,$8
   not $77,$78
   orx $76,$77
   brz $76,0f
   stqd $3,16($6)
   stqd $8,32($6)
   ai $6,$6,64
0: ret

   ########################################################################
   # Subroutine to validate the result of an instruction against an
   # expected value encoded in the instruction stream.  The expected value
   # should be encoded immediately after the subroutine call; the value
   # does not need to be aligned.
   # On entry:
   #     R3 = result of instruction
   # On return:
   #     R8 = expected result
   ########################################################################

check_value_literal:
   brsl $78,load_literal
   br check_value

load_literal:
   lqd $74,0($79)
   lqd $75,16($79)
   ai $79,$79,16
   andi $77,$79,0xC
   # We avoid shli here to reduce the number of correctly functioning
   # instructions we depend on.
   ceqi $76,$77,0
   brnz $76,0f
   ceqi $76,$77,4
   brnz $76,1f
   ceqi $76,$77,8
   brnz $76,2f
   lqr $77,8f
   br 3f
2: lqr $77,7f
   br 3f
1: lqr $77,6f
   br 3f
0: lqr $77,5f
3: brsl $76,4f
4: shufb $8,$74,$75,$77
   bi $78
   .balign 16
5: .int 0x00010203,0x04050607,0x08090A0B,0x0C0D0E0F
6: .int 0x04050607,0x08090A0B,0x0C0D0E0F,0x10111213
7: .int 0x08090A0B,0x0C0D0E0F,0x10111213,0x14151617
8: .int 0x0C0D0E0F,0x10111213,0x14151617,0x18191A1B

   ########################################################################
   # Basic subroutines to validate the result of a floating-point
   # instruction, including FPSCR flags.
   # On entry:
   #     R3 = result of instruction
   #     R8 = expected result
   #     R9 = expected value of FPSCR
   # On return:
   #     FPSCR = 0
   ########################################################################

check_float_fpscr:
   fscrrd $75
   ceq $78,$3,$8
   not $77,$78
   orx $76,$77
   brnz $76,1f
   xor $76,$75,$9
   orx $76,$76
   brz $76,0f
1: stqd $3,16($6)
   stqd $8,32($6)
   stqd $75,48($6)
   ai $6,$6,64
0: il $78,0
   fscrwr $78
   ret

   ########################################################################
   # Subroutines to validate the result of a single-precision floating-point
   # arithmetic instruction.
   # On entry:
   #     R3 = result of instruction
   #     R8 = expected result
   # On return:
   #     FPSCR = 0
   ########################################################################

check_float_arith:  # No exceptions
   il $9,0
   br check_float_fpscr

check_float_arith_diff:
   il $9,1
   br check_float_fpscr

check_float_arith_unf:
   il $9,3          # UNF always implies DIFF.
   br check_float_fpscr

check_float_arith_ovf:
   il $9,5          # OVF also always implies DIFF.
   br check_float_fpscr

   ########################################################################
   # Subroutines to validate the result of a single-precision floating-point
   # reciprocal estimation instruction.  The test passes if the result is
   # within 1 ulp of the expected value (unless the expected value is zero
   # and there is no divide-by-zero condition, in which case the match must
   # be exact).
   # On entry:
   #     R3 = result of instruction
   #     R8 = expected result
   # On return:
   #     FPSCR = 0
   ########################################################################

check_float_recip:
   fscrrd $75
   ceq $78,$3,$8
   not $77,$78
   orx $76,$77
   brz $76,1f
   orx $78,$8
   brz $78,2f
   ai $77,$8,-1
   ceq $78,$3,$77
   not $77,$78
   orx $76,$77
   brz $76,1f
   ai $77,$8,1
   ceq $78,$3,$77
   not $77,$78
   orx $76,$77
   brnz $76,2f
1: orx $76,$75
   brz $76,0f
2: stqd $3,16($6)
   stqd $8,32($6)
   stqd $75,48($6)
   ai $6,$6,64
0: il $78,0
   fscrwr $78
   ret

check_float_recip_dbz:  # Divide By Zero
   fscrrd $75
   lqr $74,2f
   ceq $78,$3,$8
   not $77,$78
   orx $76,$77
   brnz $76,1f
   xor $76,$75,$74
   orx $76,$76
   brz $76,0f
1: stqd $3,16($6)
   stqd $8,32($6)
   stqd $75,48($6)
   ai $6,$6,64
0: il $78,0
   fscrwr $78
   ret
   .balign 16
2: .int 5,5,5,0xF05

   ########################################################################
   # Subroutines to validate the result of a double-precision floating-point
   # instruction.
   # On entry:
   #     R3 = result of instruction
   #     R8 = expected result (except *_literal functions)
   #     R9 = expected FPSCR value (check_double_fpscr only)
   # On return:
   #     R8 = expected result (for *_literal functions)
   #     FPSCR = 0
   ########################################################################

check_double_fpscr:
   br check_float_fpscr

check_double:  # No exceptions
   il $9,0
   br check_double_fpscr

check_double_literal:
   brsl $78,load_literal
   br check_double

check_double_fpscr_literal:
   brsl $78,load_literal
   lr $71,$8
   brsl $78,load_literal
   lr $9,$8
   lr $8,$71
   br check_float_fpscr

   ########################################################################
   # Subroutine to wait for the SPU to halt after triggering a halt
   # condition or executing an invalid channel operation.
   ########################################################################

halt_wait:
   il $78,1000
0: ai $78,$78,-1
   brnz $78,0b
   ret
