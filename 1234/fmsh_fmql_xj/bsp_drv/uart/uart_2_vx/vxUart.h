/* vxSpi.h - vxSpi driver header */

/*
 * Copyright (c) 2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a, 20Dec19, jc  written.
*/

#ifndef __INC_VX_UART_H__
#define __INC_VX_UART_H__



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define  VX_UART_0_CFG_BASE   (0xE0040000)
#define  VX_UART_1_CFG_BASE   (0xE0023000)


#define  UART_CTRL_0   (0)
#define  UART_CTRL_1   (1)


/*
uart ctrl register definitions
*/
#define UART_REG_rbr_thr_dll  (0x00)  /* receive buffer register    (0x00)   */
                                      /* receive buffer register      */
                                      /* divisor latch low       */
                                      
#define UART_REG_ier_dlh      (0x04)  /* interrupt enable register  (0x04)                                  */
                                      /* divisor latch high        */
                                      
#define UART_REG_iir_fcr      (0x08)  /* interrupt identity register(0x08)                                    */
                                      /* FIFO control register      */
                                      
#define UART_REG_lcr          (0x0c)  /* line control register      (0x0c)*/
#define UART_REG_mcr          (0x10)  /* modem control register     (0x10)*/
#define UART_REG_lsr          (0x14)  /* line status register       (0x14)*/
#define UART_REG_msr          (0x18)  /* modem status register      (0x18)*/
#define UART_REG_scr          (0x1c)  /* scratch register           (0x1c)*/
                                        
/*#define UART_REG_rsv1[4]            // reserved                   (0x20-0x2c)        */
                                      /* This is deliberately not marked as volatile as the address needs    */
                                      /* to be used with memcpy, which won't accept a volatile pointer.*/
                                        
#define UART_REG_srbr_sthr_BASE  (0x30)     /* UART_REG_[16] shadow receive buffer and  (0x30-0x6c)    */
                                            /* transmit holding registers  */
                                        
/*#define UART_REG_rsv2[3]                                          (0x70-0x78)*/
                                        
#define UART_REG_usr          (0x7c)  /* uart status register       (0x7c)*/
#define UART_REG_tfl          (0x80)  /* transmit FIFO level        (0x80)*/
#define UART_REG_rfl          (0x84)  /* receive FIFO level         (0x84)*/
#define UART_REG_srr          (0x88)  /* software reset register    (0x88)*/
#define UART_REG_srts         (0x8c)  /* shadow request to send     (0x8c)*/
#define UART_REG_sbcr         (0x90)  /* shadow break control       (0x90)*/

/*#define UART_REG_rsv3               //                            (0x94)*/

#define UART_REG_sfe          (0x98)  /* shadow FIFO enable         (0x98)*/
#define UART_REG_srt          (0x9c)  /* shadow receiver trigger    (0x9c)*/
#define UART_REG_stet         (0xa0)  /* shadow transmitter trigger (0xa0)*/
#define UART_REG_htx          (0xa4)  /* halt Tx                    (0xa4)*/
#define UART_REG_dmasa        (0xa8)  /* dma software acknowledge   (0xa8)*/
 
/*#define UART_REG_reserved2[5]       // reserved                   (0xac-0xbc) */
 
#define UART_REG_dlf          (0xc0)  /*                            (0xc0)  */

/*#define UART_REG_reserved3[12]                                    (0xc4-0xf0)*/

#define UART_REG_comp_param_1 (0xf4)  /* component parameters       (0xf4)*/
#define UART_REG_comp_version (0xf8)  /* component version          (0xf8)*/
#define UART_REG_comp_type    (0xfc)  /* component type             (0xfc)*/

 /**
 * DESCRIPTION
 *  This is a generic data type used for 1-bit wide bitfields which have
 *  a "set/clear" property.  This is used when modifying registers
 *  within a peripheral's memory map.
 */   
#define    UART_err    (-1)
#define    UART_clear  (0)
#define    UART_set    (1)


/**
 * DESCRIPTION
 *  This is the data type used for specifying UART events.  An event is
 *  the occurrence of an interrupt which must be handled appropriately.
 *  One of these events is passed at a time to the user listener
 *  function to be processed.  The exception to this are the
 *  Uart_event_thre and Uart_event_timeout interrupts which are handled
 *  internally by the interrupt handler.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - iir/interrupt_id
 * SEE ALSO
 *  FUartPs_setListener(), FUartPs_getActiveIrq()
 * SOURCE
 */
#define  Uart_event_modem   (0x0)     /* CTR, DSR, RI or DCD status changed.*/
#define  Uart_event_none    (0x1)      /* No event/interrupt pending.*/
#define  Uart_event_thre    (0x2)      /* Transmit holding register empty or TX*/
                                       /* FIFO at or below trigger threshold.*/
#define  Uart_event_data    (0x4)      /* Receive buffer register data*/
                                       /* available (non-FIFO mode) or RX FIFO*/
                                       /* trigger level reached.*/
#define  Uart_event_line    (0x6)      /* Overrun/parity/framing error or break*/
                                       /* interrupt occurred.*/
#define  Uart_event_busy    (0x7)      /* Attempt to write to the LCR[7] while*/
                                       /* FMSH_apb_uart was busy (DLAB).*/
#define  Uart_event_timeout (0xc)      /* No characters in or out of the*/
                                       /* receiver FIFO during the last four*/
                                       /* character times and there is at least*/
                                       /* one character in it during this time.*/

/**
 * DESCRIPTION
 *  These are the bit definitions used for managing UART interrupts.
 *  The functionality of ETBEI and ERBFI alters when programmable THRE
 *  interrupt mode is active (FUartPs_enablePtime()).  See the
 *  FMSH_apb_uart databook for a detailed description.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - ier/erbfi
 *   - ier/etbei
 *   - ier/elsi
 *   - ier/edssi
 * SEE ALSO
 *  FUartPs_enableIrq(), FUartPs_disableIrq(), FUartPs_isIrqEnabled()
 * SOURCE
 */
#define    Uart_irq_erbfi    (0x01)      /* receive data available*/
#define    Uart_irq_etbei    (0x02)      /* transmitter holding register empty*/
#define    Uart_irq_elsi     (0x04)      /* receiver line status*/
#define    Uart_irq_edssi    (0x08)      /* modem status*/
#define    Uart_irq_all      (0x0f)      /* all interrupts*/

/**
 * DESCRIPTION
 *  These are the definitions used for reporting the line status
 *  including errors, if any.  Note that they represent bits of an 8-bit
 *  register and more than one can be active at any time.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lsr/dr
 *   - lsr/oe
 *   - lsr/pe
 *   - lsr/fe
 *   - lsr/bi
 *   - lsr/thre
 *   - lsr/temt
 *   - lsr/rfe
 * SEE ALSO
 *  FUartPs_getLineStatus()
 * SOURCE
 */
#define    Uart_line_dr     (0x01)        /* data ready*/
#define    Uart_line_oe     (0x02)        /* overrun error*/
#define    Uart_line_pe     (0x04)        /* parity error*/
#define    Uart_line_fe     (0x08)        /* framing error*/
#define    Uart_line_bi     (0x10)        /* break interrupt*/
#define    Uart_line_thre   (0x20)        /* transmit holding register empty*/
#define    Uart_line_temt   (0x40)        /* transmitter empty*/
#define    Uart_line_rfe    (0x80)        /* receiver FIFO error*/



/**
 * DESCRIPTION
 *  These are the bit definitions used for reporting the modem status.
 *  Note that they represents bits of an 8-bit register and more than
 *  one can be active at any time.  The delta signals reflect a change
 *  in the relevant pin since the last read of the modem status
 *  register.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - msr/dcts
 *   - msr/ddsr
 *   - msr/teri
 *   - msr/ddcd
 *   - msr/cts
 *   - msr/dsr
 *   - msr/ri
 *   - msr/dcd
 * SEE ALSO
 *  FUartPs_getModemStatus()
 * SOURCE
 */
#define    Uart_modem_dcts  (0x01)     /* delta clear to send*/
#define    Uart_modem_ddsr  (0x02)     /* delta data set ready*/
#define    Uart_modem_teri  (0x04)     /* ring indicator active->inactive*/
#define    Uart_modem_ddcd  (0x08)     /* delta data carrier detect*/
#define    Uart_modem_cts   (0x10)     /* clear to send*/
#define    Uart_modem_dsr   (0x20)     /* data set ready*/
#define    Uart_modem_ri    (0x40)     /* ring indicator*/
#define    Uart_modem_dcd   (0x80)     /* data carrier detect*/

/**
 * DESCRIPTION
 *  These are the bit definitions used for reporting the modem status.
 *  Note that they represents bits of an 4-bit register so one or more
 *  of them may be set/active at any time.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - mcr/dtr
 *   - mcr/rts
 *   - mcr/out1
 *   - mcr/out2
 * SEE ALSO
 *  FUartPs_setModemLine(), FUartPs_clearModemLine(),
 *  FUartPs_getModemLine()
 * SOURCE
 */
#define   Uart_modem_dtr        (0x1)      /* data terminal ready*/
#define   Uart_modem_rts        (0x2)      /* request to send differs since lastread*/
#define   Uart_modem_out1       (0x4)      /* user-designated output one*/
#define   Uart_modem_out2       (0x8)      /* user-designated output two*/
#define   Uart_modem_line_all   (0xf)      /* all modem control lines*/

/**
 * DESCRIPTION
 *  This is the data type used for the UART receiver FIFO full trigger.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - fcr/rcvr_trigger
 * SEE ALSO
 *  FUartPs_setRxTrigger(), FUartPs_getRxTrigger()
 * SOURCE
 */
#define   Uart_one_char_in_fifo   (0x0)
#define   Uart_fifo_quarter_full  (0x1)
#define   Uart_fifo_half_full     (0x2)
#define   Uart_fifo_two_less_full (0x3)

/**
 * DESCRIPTION
 *  This is the data type used for the UART DMA mode.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - fcr/dma_mode
 * SEE ALSO
 *  FUartPs_setDmaMode(), FUartPs_getDmaMode()
 * SOURCE
 */
#define    Uart_dma_single (0x0)
#define    Uart_dma_multi  (0x1)

/**
 * DESCRIPTION
 *  This is the data type used for specifying the number of stop bits.
 *  Note that if that number of bits per character is 5, the actual
 *  number of stop bits transmitted by FMSH_apb_uart will be either 1
 *  (Uart_one_stop_bit) or 1.5 (Uart_two_stop_bits).
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/stop
 * SEE ALSO
 *  FUartPs_getStopBits()
 * SOURCE
 */
#define    Uart_one_stop_bit  (0x0)    /* 1 stop bit*/
#define    Uart_two_stop_bits (0x1)    /* 2 stop bits (or 1.5 stop bits if the*/
                                       /* number of bits per character is 5).*/

/**
 * DESCRIPTION
 *  This is the data type used for specifying the number of bits per
 *  character (Character Length Select).
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/cls
 * SEE ALSO
 *  FUartPs_getDataBits()
 * SOURCE
 */
#define    Uart_five_bits  (0x0)       /* 5 bits per character*/
#define    Uart_six_bits   (0x1)        /* 6 bits per character*/
#define    Uart_seven_bits (0x2)      /* 7 bits per character*/
#define    Uart_eight_bits (0x3)       /* 8 bits per character*/

/**
 * DESCRIPTION
 *  This is the data type used for specifying UART parity.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  FUartPs_getParity()
 * SOURCE
 */
#define    Uart_no_parity   (0x0)       /* no parity*/
#define    Uart_odd_parity  (0x1)      /* odd parity*/
#define    Uart_even_parity (0x3)      /* even parity*/

#define    Uart_Stick_disable  (0x0)       /* no parity*/
#define    Uart_Stick_enable   (0x1)     /* odd parity*/

/**
 * DESCRIPTION
 *  This is the data type used for manipulation of the UART line control
 *  settings.
 * NOTES
 *  This data type relates to the following register bit field(s):
 *   - lcr/cls
 *   - lcr/stop
 *   - lcr/pen
 *   - lcr/eps
 * SEE ALSO
 *  FUartPs_getLineControl(), FUartPs_setLineControl()
 * SOURCE
 */
#define    Uart_line_5n1     (0x00)   /* 5 data bits, no parity, 1 stop bit*/
#define    Uart_line_5n1_5   (0x04)   /* 5 data bits, no parity, 1.5 stop bits*/
#define    Uart_line_5e1     (0x18)   /* 5 data bits, even parity, 1 stop bit*/
#define    Uart_line_5e1_5   (0x1c)   /* 5 data bits, even parity, 1.5 stop bits*/
#define    Uart_line_5o1     (0x08)   /* 5 data bits, odd parity, 1 stop bit*/
#define    Uart_line_5o1_5   (0x0c)   /* 5 data bits, odd parity, 1.5 stop bits*/
#define    Uart_line_6n1     (0x01)   /* 6 data bits, no parity, 1 stop bit*/
#define    Uart_line_6n2     (0x05)   /* 6 data bits, no parity, 2 stop bits*/
#define    Uart_line_6e1     (0x19)   /* 6 data bits, even parity, 1 stop bit*/
#define    Uart_line_6e2     (0x1d)   /* 6 data bits, even parity, 2 stop bits*/
#define    Uart_line_6o1     (0x09)   /* 6 data bits, odd parity, 1 stop bit*/
#define    Uart_line_6o2     (0x0d)   /* 6 data bits, odd parity, 2 stop bits*/
#define    Uart_line_7n1     (0x02)   /* 7 data bits, no parity, 1 stop bit*/
#define    Uart_line_7n2     (0x06)   /* 7 data bits, no parity, 2 stop bits*/
#define    Uart_line_7e1     (0x1a)   /* 7 data bits, even parity, 1 stop bit*/
#define    Uart_line_7e2     (0x1e)   /* 7 data bits, even parity, 2 stop bits*/
#define    Uart_line_7o1     (0x0a)   /* 7 data bits, odd parity, 1 stop bit*/
#define    Uart_line_7o2     (0x0e)   /* 7 data bits, odd parity, 2 stop bits*/
#define    Uart_line_8n1     (0x03)   /* 8 data bits, no parity, 1 stop bit*/
#define    Uart_line_8n2     (0x07)   /* 8 data bits, no parity, 2 stop bits*/
#define    Uart_line_8e1     (0x1b)   /* 8 data bits, even parity, 1 stop bit*/
#define    Uart_line_8e2     (0x1f)   /* 8 data bits, even parity, 2 stop bits*/
#define    Uart_line_8o1     (0x0b)   /* 8 data bits, odd parity, 1 stop bit*/
#define    Uart_line_8o2     (0x0f)   /* 8 data bits, odd parity, 2 stop bits*/


#define STATE_UART_IDLE        (0)
#define STATE_UART_TX          (1)
#define STATE_UART_RX          (2)
#define STATE_UART_RX_REQ      (3)
#define STATE_UART_TX_RX       (4)
#define STATE_UART_TX_RX_REQ   (5)

#define MODE_UART_BUF_FULL        (0)
#define MODE_UART_RX_TIMEOUT      (1)

#define TX_TRIG_UART_EMPTY_FIFO            (0)
#define TX_TRIG_UART_TWO_CHAR_IN_FIFO      (1)
#define TX_TRIG_UART_QUARTER_FULL_FIFO     (2)
#define TX_TRIG_UART_HALF_FULL_FIFO        (3)



/**
 * DESCRIPTION
 *  Used in conjunction with bitops.h to access register bitfields.
 *  They are defined as bit offset/mask tuples for each uart register
 *  bitfield.
 * NOTES
 *  bfo is the offset of the bitfield with respect to LSB;
 *  bfw is the width of the bitfield
 */
/* interrupt enable register*/
#define bfoUART_IER_ERBFI               0   /* received data available*/
#define bfwUART_IER_ERBFI               1
#define bfoUART_IER_ETBEI               1   /* transmitter holding*/
#define bfwUART_IER_ETBEI               1   /*  register*/
#define bfoUART_IER_ELSI                2   /* receiver line status*/
#define bfwUART_IER_ELSI                1
#define bfoUART_IER_EDSSI               3   /* modem status*/
#define bfwUART_IER_EDSSI               1
#define bfoUART_IER_PTIME               7   /* programmable THRE*/
#define bfwUART_IER_PTIME               1   /*  interrupt mode*/
/* interrupt identity register*/
#define bfoUART_IIR_INTERRUPT_ID        0   /* interrupt identity*/
#define bfwUART_IIR_INTERRUPT_ID        4
#define bfoUART_IIR_FIFO_STATUS         6   /* FIFO status*/
#define bfwUART_IIR_FIFO_STATUS         2
/* FIFO control register*/
#define bfoUART_FCR_FIFO_ENABLE         0   /* FIFO enable*/
#define bfwUART_FCR_FIFO_ENABLE         1
#define bfoUART_FCR_RCVR_FIFO_RESET     1   /* receiver FIFO reset*/
#define bfwUART_FCR_RCVR_FIFO_RESET     1
#define bfoUART_FCR_XMIT_FIFO_RESET     2   /* transmitter FIFO reset*/
#define bfwUART_FCR_XMIT_FIFO_RESET     1
#define bfoUART_FCR_DMA_MODE            3   /* dma mode*/
#define bfwUART_FCR_DMA_MODE            1
#define bfoUART_FCR_TX_EMPTY_TRIGGER    4   /* transmitter empty trigger*/
#define bfwUART_FCR_TX_EMPTY_TRIGGER    2
#define bfoUART_FCR_RCVR_TRIGGER        6   /* receiver trigger*/
#define bfwUART_FCR_RCVR_TRIGGER        2
/* line control register*/
#define bfoUART_LCR_CLS                 0   /* number of bits per*/
#define bfwUART_LCR_CLS                 2   /*  character*/
#define bfoUART_LCR_STOP                2   /* stop bits*/
#define bfwUART_LCR_STOP                1
#define bfoUART_LCR_PEN                 3   /* parity enable*/
#define bfwUART_LCR_PEN                 1
#define bfoUART_LCR_EPS                 4   /* parity select*/
#define bfwUART_LCR_EPS                 1
#define bfoUART_LCR_PARITY              3   /* composite enable and*/
#define bfwUART_LCR_PARITY              2   /*  select field*/
#define bfoUART_LCR_LINE                0   /* composite line*/
#define bfwUART_LCR_LINE                5   /*   control field*/
#define bfoUART_LCR_STICK_PARITY        5   /* unsupported*/
#define bfwUART_LCR_STICK_PARITY        1   /* unsupported*/
#define bfoUART_LCR_BREAK               6   /* line break*/
#define bfwUART_LCR_BREAK               1
#define bfoUART_LCR_DLAB                7   /* divisor latch address bit*/
#define bfwUART_LCR_DLAB                1
/* modem control register*/
#define bfoUART_MCR_DTR                 0   /* dtr*/
#define bfwUART_MCR_DTR                 1
#define bfoUART_MCR_RTS                 1   /* rts*/
#define bfwUART_MCR_RTS                 1
#define bfoUART_MCR_OUT1                2   /* out1*/
#define bfwUART_MCR_OUT1                1
#define bfoUART_MCR_OUT2                3   /* out2*/
#define bfwUART_MCR_OUT2                1
#define bfoUART_MCR_LOOPBACK            4   /* loopback*/
#define bfwUART_MCR_LOOPBACK            1
#define bfoUART_MCR_AFCE                5   /* automatic flow control*/
#define bfwUART_MCR_AFCE                1
#define bfoUART_MCR_SIRE                6   /* serial infra-red*/
#define bfwUART_MCR_SIRE                1
/* line status register*/
#define bfoUART_LSR_DR                  0   /* data ready*/
#define bfwUART_LSR_DR                  1
#define bfoUART_LSR_OE                  1   /* overrun error*/
#define bfwUART_LSR_OE                  1
#define bfoUART_LSR_PE                  2   /* parity error*/
#define bfwUART_LSR_PE                  1
#define bfoUART_LSR_FE                  3   /* framing error*/
#define bfwUART_LSR_FE                  1
#define bfoUART_LSR_BI                  4   /* break interrupt*/
#define bfwUART_LSR_BI                  1
#define bfoUART_LSR_THRE                5   /* transmitter holding*/
#define bfwUART_LSR_THRE                1   /*  register empty*/
#define bfoUART_LSR_TEMT                6   /* transmitter empty*/
#define bfwUART_LSR_TEMT                1
#define bfoUART_LSR_RX_FIFO_ERROR       7   /* receiver FIFO error*/
#define bfwUART_LSR_RX_FIFO_ERROR       1
/* modem status register*/
#define bfoUART_MSR_DCTS                0   /* dcts*/
#define bfwUART_MSR_DCTS                1
#define bfoUART_MSR_DDSR                1   /* ddsr*/
#define bfwUART_MSR_DDSR                1
#define bfoUART_MSR_TERI                2   /* teri*/
#define bfwUART_MSR_TERI                1
#define bfoUART_MSR_DDCD                3   /* ddcd*/
#define bfwUART_MSR_DDCD                1
#define bfoUART_MSR_CTS                 4   /* cts*/
#define bfwUART_MSR_CTS                 1
#define bfoUART_MSR_DSR                 5   /* dsr*/
#define bfwUART_MSR_DSR                 1
#define bfoUART_MSR_RI                  6   /* ri*/
#define bfwUART_MSR_RI                  1
#define bfoUART_MSR_DCD                 7   /* dcd*/
#define bfwUART_MSR_DCD                 1
/* uart status register*/
#define bfoUART_USR_BUSY                0   /* serial transfer is in*/
#define bfwUART_USR_BUSY                1   /*  progress*/
#define bfoUART_USR_TFNF                1   /* Tx FIFO not full*/
#define bfwUART_USR_TFNF                1
#define bfoUART_USR_TFE                 2   /* Tx FIFO empty*/
#define bfwUART_USR_TFE                 1
#define bfoUART_USR_RFNE                3   /* Rx FIFO not empty*/
#define bfwUART_USR_RFNE                1
#define bfoUART_USR_RFF                 4   /* Rx FIFO full*/
#define bfwUART_USR_RFF                 1
/* transmit FIFO level register*/
#define bfoUART_TFL_LEVEL               0   /* Tx FIFO data level*/
#define bfwUART_TFL_LEVEL               8
/* receive FIFO level register*/
#define bfoUART_RFL_LEVEL               0   /* Rx FIFO data level*/
#define bfwUART_RFL_LEVEL               8
/* software reset register*/
#define bfoUART_SRR_UR                  0   /* UART reset*/
#define bfwUART_SRR_UR                  1
#define bfoUART_SRR_RFR                 1   /* Rx FIFO reset*/
#define bfwUART_SRR_RFR                 1
#define bfoUART_SRR_XFR                 2   /* Tx FIFO reset*/
#define bfwUART_SRR_XFR                 1
/* shadow request to send register*/
#define bfoUART_SRTS_RTS                0   /* shadow request to send*/
#define bfwUART_SRTS_RTS                1
/* shadow break control register*/
#define bfoUART_SBCR_BCR                0   /* shadow break control*/
#define bfwUART_SBCR_BCR                1
/* shadow FIFO enable register*/
#define bfoUART_SFE_FE                  0   /* shadow FIFO enable*/
#define bfwUART_SFE_FE                  1
/* shadow receiver trigger register*/
#define bfoUART_SRT_TRIGGER             0   /* shadow Rx FIFO full*/
#define bfwUART_SRT_TRIGGER             2   /*  trigger*/
/* shadow transmitter empty trigger register*/
#define bfoUART_STET_TRIGGER            0   /* shadow Tx FIFO empty*/
#define bfwUART_STET_TRIGGER            2   /*  trigger*/
/* halt transmitter register*/
#define bfoUART_HTX_HALT                0   /* halt transmitter*/
#define bfwUART_HTX_HALT                1
/* DMA software acknowledge register*/
#define bfoUART_DMASA_ACK               0   /* DMA software acknowledge*/
#define bfwUART_DMASA_ACK               1
/* uart component parameters*/
#define bfoUART_PARAM_DATA_WIDTH        0
#define bfwUART_PARAM_DATA_WIDTH        2
#define bfoUART_PARAM_AFCE_MODE         4
#define bfwUART_PARAM_AFCE_MODE         1
#define bfoUART_PARAM_THRE_MODE         5
#define bfwUART_PARAM_THRE_MODE         1
#define bfoUART_PARAM_SIR_MODE          6
#define bfwUART_PARAM_SIR_MODE          1
#define bfoUART_PARAM_SIR_LP_MODE       7
#define bfwUART_PARAM_SIR_LP_MODE       1
#define bfoUART_PARAM_NEW_FEAT          8
#define bfwUART_PARAM_NEW_FEAT          1
#define bfoUART_PARAM_FIFO_ACCESS       9
#define bfwUART_PARAM_FIFO_ACCESS       1
#define bfoUART_PARAM_FIFO_STAT         10
#define bfwUART_PARAM_FIFO_STAT         1
#define bfoUART_PARAM_SHADOW            11
#define bfwUART_PARAM_SHADOW            1
#define bfoUART_PARAM_ADD_ENCODED_PARAMS    12
#define bfwUART_PARAM_ADD_ENCODED_PARAMS    1
#define bfoUART_PARAM_DMA_EXTRA         13
#define bfwUART_PARAM_DMA_EXTRA         1
#define bfoUART_PARAM_FIFO_MODE         16
#define bfwUART_PARAM_FIFO_MODE         8

/**
 *
 * DESCRIPTION
 *  This structure comprises the uart harFMSHare parameters that affect
 *  the software driver.  This structure needs to be initialized with
 *  the correct values and be pointed to by the (FUartPs_pice).cc
 *  member of the relevant uart uart_pice structure.
 * SOURCE
 */
typedef struct _t_uart_param_ 
{
    int afce_mode;         /* automatic flow control*/
    int dma_extra;         /* FMSH_ahb_dmac compatibility signals*/
    int fifo_access;       /* programmable FIFO access mode*/
    int fifo_stat;         /* additional FIFO status registers*/
    int new_feat;          /* new v3 features (shadow registers, etc.)*/
    int shadow;            /* include shadow registers*/
    int sir_lp_mode;       /* low-power IrDA SIR mode*/
    int sir_mode;          /* serial infrared*/
    int thre_mode;         /* programmable thre interrupts*/
    
    UINT32 fifo_depth;   /* FIFO depth*/
} vxT_UART_PARAMS;

/**
 *
 *  This structure contains variables which relate to each individual
 *  uart instance.  Cumulatively, they can be thought of as the "state
 *  variables" for each distinct uart uart_pice.
 */
typedef struct _t_uart_chn_
{
    /*enum FUartPs_state state;           // uart driver state*/
    int state;           /* uart driver state*/
    
    UINT32 value_in_fcr;              /* FCR value (write only reg)*/
    UINT32 ier_save;                  /* saved IER value*/
    UINT32 lsr_save;                  /* saved line status errors*/
    
    /*FMSH_callback listener;              // user event listener*/
    /*FMSH_callback txCallback;            // user Tx callback function*/
    /*FMSH_callback rxCallback;            // user Rx callback function*/
    
    /*enum FMSH_callback_mode callbackMode;    // Rx callback mode*/
    int callbackMode;    /* Rx callback mode*/
    
    UINT32 *txBuffer;                 /* pointer to user Tx buffer*/
    UINT32 txHold;                    /* Tx holding register*/
    
    unsigned txIdx;                     /* Tx holding register index*/
    unsigned txLength;                  /* user Tx buffer length*/
    unsigned txRemain;                  /* chars left to send in buffer*/
    
    UINT32 *rxBuffer;                 /* pointer to user Rx buffer*/
    UINT32 rxHold;                    /* Rx holding register*/
    
    unsigned rxIdx;                     /* Rx holding register index*/
    unsigned rxLength;                  /* user Rx buffer length*/
    unsigned rxRemain;                  /* space left in buffer*/
    
    int rxAlign;                       /* is Rx buffer aligned?    */
    int fifos_enabled;                 /* are FIFOs enabled? (cached)*/
    
    /*enum FUartPs_tx_trigger txTrigger;  // saved user Tx trigger*/
    int txTrigger;  /* saved user Tx trigger*/
} vxT_UART_CHN;


/*
uart ctrl
*/
typedef struct _t_uart_ctrl_
{
    int  ctrl_x;  /* 0, 1 */

    const char *name;
    
    UINT16 devId;                /**< Unique ID of device */
    UINT32 cfgBaseAddr;            /**< APB Base address of the device */
    UINT32 input_clock;
    
    vxT_UART_PARAMS  tParam;
    vxT_UART_CHN     tChn;

    UINT32 devIP_ver;
    UINT32 devIP_type;      /* identify peripheral types:  */

    int status;
} vxT_UART_CTRL;


typedef struct _t_uart_
{
    int uart_x;
    
    vxT_UART_CTRL * pUartCtrl;

    int init_flag;
} vxT_UART;
    

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_VX_UART_H__ */

