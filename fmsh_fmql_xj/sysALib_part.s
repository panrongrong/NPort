#ifdef ARM_AARCH64
        .long 0xd5384241        /*   MRS     X1, CurrentEL                    */
        .long 0xf100303f        /*   CMP     X1, 0xC                          */
        .long 0x540000a0        /*   B.EQ    el3_mode                         */
        .long 0xf100203f        /*   CMP     X1, 0x8                          */
        .long 0x540001c0        /*   B.EQ    el2_mode                         */
        .long 0xf100103f        /*   CMP     X1, 0x4                          */
        .long 0x540005a0        /*   B.EQ    el1_mode                         */
 el3_mode:
        .long 0xd280b621        /*   MOV     X1, #(SCR_EL3_DEFAULT)           */
        .long 0xd51e1101        /*   MSR     SCR_EL3, X1                      */
        .long 0xd51e115f        /*   MSR     CPTR_EL3, XZR                    */
        .long 0xd2807921        /*   MOV     X1, #(SPSR_EL3_DEFAULT)          */
        .long 0xd51e4001        /*   MSR     SPSR_EL3, X1                     */
        .long 0xd53ec001        /*   MRS     X1, VBAR_EL3                     */
        .long 0xd51cc001        /*   MSR     VBAR_EL2, X1                     */
        .long 0x10000080        /*   ADR     X0, el2_mode                     */
        .long 0xaa0003fe        /*   MOV     LR, X0                           */
        .long 0xd51e403e        /*   MSR     ELR_EL3, LR                      */
        .long 0xd69f03e0        /*   ERET                                     */

 el2_mode:
        .long 0xd5380001        /*   MRS     X1, MIDR_EL1                     */
        .long 0xd51c0001        /*   MSR     VPIDR_EL2, X1                    */
        .long 0xd53800a1        /*   MRS     X1, MPIDR_EL1                    */
        .long 0xd51c00a1        /*   MSR     VMPIDR_EL2, X1                   */
        .long 0xd53ce101        /*   MRS     X1, CNTHCTL_EL2                  */
        .long 0xb2400421        /*   ORR     X1, X1,  #(CNTHCTL_EL2_EL1PCTEN |*/ 
                                /*   CNTHCTL_EL2_EL1PCEN)                     */
        .long 0xd51ce101        /*   MSR     CNTHCTL_EL2, X1                  */
        .long 0xd51ce07f        /*   MSR     CNTVOFF_EL2, XZR                 */
        .long 0xd2801a61        /*   MOV     X1, #0xD3                        */
        .long 0xd51c4001        /*   MSR     SPSR_EL2, X1                     */
        .long 0xd2867fe1        /*   MOV     X1, #(CPTR_EL2_RES1)             */
        .long 0xd51c1141        /*   MSR     CPTR_EL2, X1                     */
        .long 0xd2810601        /*   MOV     X1, #0x830                       */
        .long 0xf2a618a1        /*   MOVK    X1, #0x30C5, LSL #16             */
        .long 0xd51c1001        /*   MSR     SCTLR_EL2, X1                    */
        .long 0xd5381000        /*   MRS     X0, SCTLR_EL1                    */
        .long 0xd2820081        /*   MOV     X1, #SCTLR_C | SCTLR_I           */
        .long 0x8a010000        /*   AND     X0, X0, X1                       */
        .long 0xd2810001        /*   MOV     X1, #0x800                       */
        .long 0xf2a61a01        /*   MOVK    X1, #0x30D0, LSL #16             */
        .long 0xaa000021        /*   ORR     X1, X1, X0                       */
        .long 0xd5181001        /*   MSR     SCTLR_EL1, X1                    */
        .long 0xd51c117f        /*   MSR     HSTR_EL2, XZR                    */
        .long 0xd2a00601        /*   MOV     X1,                              */
                                /*   #(CPACR_EL1_FPEN_NO_INST_TRAPPED)        */
        .long 0xd5181041        /*   MSR     CPACR_EL1, X1                    */
        .long 0xd2800001        /*   MOV     X1, #0x0                         */
        .long 0xd51c1101        /*   MSR     HCR_EL2, X1                      */
        .long 0xd53cc001        /*   MRS     X1, VBAR_EL2                     */
        .long 0xd518c001        /*   MSR     VBAR_EL1, X1                     */
        .long 0x10000080        /*   ADR     X0, el1_mode                     */
        .long 0xaa0003fe        /*   MOV     LR, X0                           */
        .long 0xd51c403e        /*   MSR     ELR_EL2, LR                      */
        .long 0xd69f03e0        /*   ERET                                  */
 el1_mode: