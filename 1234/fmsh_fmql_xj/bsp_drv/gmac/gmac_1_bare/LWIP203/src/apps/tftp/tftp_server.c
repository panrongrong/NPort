/**
* @defgroup tftp TFTP server
* @ingroup apps
*
* This is simple TFTP server for the lwIP raw API.
*/
#include "fmsh_devc_verify.h"
#include "lwip/apps/tftp_server.h"
#include "fmsh_gmac_lwip.h"
#include "fmsh_ps_parameters.h"
#include "fmsh_common_io.h"
#include "fmsh_gpio_mix.h"
#include "fmsh_gpio_lib.h"
#include "fmsh_gpio_private.h"
#include "ps_init_lwip.h"
#include "fmsh_devc_private.h"
#include "fmsh_dmac_lib.h"
/*#if LWIP_UDP*/

#define TFTP_BURST_PACKET 10

static u8 * rcv_tftp_space_ptr;
static u8 ate_tftp_rcv_err_code=0;
static u8 open_tst_lvl_flag=0;
static u8 open_usr_lvl_flag=0;
static u8 gm_test_status=0;
static u32 gm_pass_frame=0;
static u32 tftp_burst_rcv_length=0;
static u32 tftp_rcv_index=0;

extern FGpioPs_T inst_GPIOB;
extern FDmaPs_T *pDmac;
extern u8 gpio40;
extern u8 gpio42;

u32 src_addr[12]={0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000,
0x20000000};

/* file type */
typedef enum _rcv_file_type{
    bin_file=0,
    bit_file=1,
    gm_file=2
}ATE_FileType;
static ATE_FileType rcv_file_type;

#include "lwip/udp.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"

#include "fmsh_devc_verify.h"

#define TFTP_HEADER_LENGTH    4

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

enum tftp_error {
	TFTP_ERROR_FILE_NOT_FOUND    = 1,
	TFTP_ERROR_ACCESS_VIOLATION  = 2,
	TFTP_ERROR_DISK_FULL         = 3,
	TFTP_ERROR_ILLEGAL_OPERATION = 4,
	TFTP_ERROR_UNKNOWN_TRFR_ID   = 5,
	TFTP_ERROR_FILE_EXISTS       = 6,
	TFTP_ERROR_NO_SUCH_USER      = 7
};

#include <string.h>

struct tftp_state {
	const struct tftp_context *ctx;
	void *handle;
	struct pbuf *last_data;
	struct udp_pcb *upcb;
	ip_addr_t addr;
	u16_t port;
	int timer;
	int last_pkt;
	u16_t blknum;
	u8 retries;
	u8 mode_write;
};

static struct tftp_state tftp_state;

static void tftp_tmr(void* arg);

static char *cmp_type_bit="bit";
static char *cmp_type_bin="bin";
static char *cmp_type_gm=".gm";
static char *cmp_usrlvl="openusr";
static char *cmp_tstlvl="opentst";

static ATE_FileType identify_file_type(char *filename)
{
	u8 length;
	length=strlen(filename);
    if (strcmp((char *)(filename+length-3),cmp_type_bit)==0)
        return bit_file;
    if (strcmp((char *)(filename+length-3),cmp_type_bin)==0)
        return bin_file;
    if (strcmp((char *)(filename+length-3),cmp_type_gm)==0)
        return gm_file;
}
unsigned long pll_x32[]={  /*devc x32*/
    EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<3, 1<<3),  /*bypass force*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<0, 1<<0 ),  /*ARM PLL reset*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x007f0000U, 30<<16),  /*45*/
    EMIT_WRITE(SLCR_REG_BASE + 0x180, 1),  /*Enable ARM PLL LATCH*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<0, 0<<0 ), /*deassert ARM PLL reset*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<3, 0<<3 ), /*EXIT ARM PLL Bypass Force*/
    EMIT_EXIT(),
};

unsigned long pll_x16[]={ /* devc x16*/
    EMIT_WRITE(SLCR_REG_BASE + 0x008, 0xDF0D767BU),
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<3, 1<<3),  /*bypass force*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<0, 1<<0 ),  /*ARM PLL reset*/
    EMIT_MASKWRITE(SLCR_REG_BASE + 0x100, 0x007f0000U, 60<<16),  /*45*/
    EMIT_WRITE(SLCR_REG_BASE + 0x180, 1),  /*Enable ARM PLL LATCH*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<0, 0<<0 ), /*deassert ARM PLL reset*/
    EMIT_MASKWRITE (SLCR_REG_BASE + 0x100,1<<3, 0<<3 ), /*EXIT ARM PLL Bypass Force*/
    EMIT_EXIT(),
};

u8 FDevcPs_wrCheck(u32 far_addr,u32 wr_data,u32 golden_addr)
{
    u8 ret=0;
    u32 indx=0;
    /*u32 far_addr=(type<<23)|((tb&0x1)<<22)|(row<<17)|(col<<7)|min;*/
    u32 rdData[404]={0};
    u32 tmpData=0;
    u8 bTypeFlag=0;
    /*type=1*/
    if((far_addr&0x00800000)==0x00800000)
    {
          bTypeFlag=1;
    }

    ret=FDevcPs_wrFrameData(&g_DEVC,far_addr,wr_data);
    if(1==ret)
       return ret;

    FDevcPs_txDataSwap(&g_DEVC, bit_swap);
    FDevcPs_getConfigdata(&g_DEVC, rdData,  2* 101, far_addr, 0xaa55);
    devc_byte_swap((u32*)rdData,  2* 101);

    for(indx=0;indx<101;indx++)
    {
       tmpData=*((volatile unsigned int *)(golden_addr+4*indx));
       if(wr_data==0xFFFFFFFF)
       {
           if(((~tmpData)&rdData[101+indx])!=(~tmpData))
              ret=1;
       }
       else
       {
           if((tmpData|rdData[101+indx])!=tmpData)
            ret=1;
       }
    }

    return ret;
}

static void close_handle(void)
{
	tftp_state.port = 0;
	ip_addr_set_any(0, &tftp_state.addr);

	if(tftp_state.last_data != NULL) {
		pbuf_free(tftp_state.last_data);
		tftp_state.last_data = NULL;
	}

	sys_untimeout(tftp_tmr, NULL);

	if (tftp_state.handle) {
		tftp_state.ctx->close(tftp_state.handle);
		tftp_state.handle = NULL;
		LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: closing\n"));
	}
}

static void send_error(const ip_addr_t *addr, u16_t port, enum tftp_error code, const char *str)
{
	int str_length = strlen(str);
	struct pbuf* p;
	u16_t* payload;

	p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)(TFTP_HEADER_LENGTH + str_length + 1), PBUF_RAM);
	if(p == NULL) {
		return;
	}

	payload = (u16_t*) p->payload;
	payload[0] = PP_HTONS(TFTP_ERROR);
	payload[1] = lwip_htons(code);
	MEMCPY(&payload[2], str, str_length + 1);

	udp_sendto(tftp_state.upcb, p, addr, port);
	pbuf_free(p);
}

static void send_ack(u16_t blknum)
{
	struct pbuf* p;
	u16_t* payload;

	p = pbuf_alloc(PBUF_TRANSPORT, TFTP_HEADER_LENGTH, PBUF_RAM);
	if(p == NULL) {
		return;
	}
	payload = (u16_t*) p->payload;

	payload[0] = PP_HTONS(TFTP_ACK);
	payload[1] = lwip_htons(blknum);
	udp_sendto(tftp_state.upcb, p, &tftp_state.addr, tftp_state.port);
	pbuf_free(p);
}

static void resend_data(void)
{
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, tftp_state.last_data->len, PBUF_RAM);
	if(p == NULL) {
		return;
	}

	if(pbuf_copy(p, tftp_state.last_data) != ERR_OK) {
		pbuf_free(p);
		return;
	}

	udp_sendto(tftp_state.upcb, p, &tftp_state.addr, tftp_state.port);
	pbuf_free(p);
}

static void send_data(void)
{
	u16_t *payload;
	int ret;

	if(tftp_state.last_data != NULL) {
		pbuf_free(tftp_state.last_data);
	}

	tftp_state.last_data = pbuf_alloc(PBUF_TRANSPORT, TFTP_HEADER_LENGTH + TFTP_MAX_PAYLOAD_SIZE, PBUF_RAM);
	if(tftp_state.last_data == NULL) {
		return;
	}

	payload = (u16_t *) tftp_state.last_data->payload;
	payload[0] = PP_HTONS(TFTP_DATA);
	payload[1] = lwip_htons(tftp_state.blknum);

	ret = tftp_state.ctx->read(tftp_state.handle, &payload[2], TFTP_MAX_PAYLOAD_SIZE);
	if (ret < 0) {
		send_error(&tftp_state.addr, tftp_state.port, TFTP_ERROR_ACCESS_VIOLATION, "Error occured while reading the file.");
		close_handle();
		return;
	}

	pbuf_realloc(tftp_state.last_data, (u16_t)(TFTP_HEADER_LENGTH + ret));
	resend_data();
}

static void recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    /*gpio40=0;*/
    /* FGpioPs_writeData(&inst_GPIOB, (gpio40<<8)|(gpio42<<10)); ///debug*/
	u16_t *sbuf = (u16_t *) p->payload;
	int opcode;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(upcb);

	if (((tftp_state.port != 0) && (port != tftp_state.port)) ||
		(!ip_addr_isany_val(tftp_state.addr) && !ip_addr_cmp(&tftp_state.addr, addr))) {
			send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Only one connection at a time is supported");
			pbuf_free(p);
			tftp_reset(NULL);/**/
			return;
		}

	opcode = sbuf[0];

	tftp_state.last_pkt = tftp_state.timer;
	tftp_state.retries = 0;

	switch (opcode) {
	case PP_HTONS(TFTP_RRQ): /* fall through */
	case PP_HTONS(TFTP_WRQ):
		{
            /* clear SRAM exchange test resault */
            *(u32*)ATE_TFTP_EXCHANGE_OPCODE=0;
            *(u32*)(ATE_TFTP_EXCHANGE_OPCODE+4)=0;

            /* clear MIO status */
            FGpioPs_setDirection(&inst_GPIOB, 0xffffffff);
            FGpioPs_writeData(&inst_GPIOB, (1<<8)|(1<<9)|(1<<10)|(1<<11)); /* 1111*/

			const char tftp_null = 0;
			char filename[TFTP_MAX_FILENAME_LEN];
			char mode[TFTP_MAX_MODE_LEN];
			u16_t filename_end_offset;
			u16_t mode_end_offset;

			if(tftp_state.handle != NULL) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Only one connection at a time is supported");
				tftp_reset(NULL);/**/
				break;
			}

			sys_timeout(TFTP_TIMER_MSECS, tftp_tmr, NULL);

			/* find \0 in pbuf -> end of filename string */
			filename_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), 2);
			if((u16_t)(filename_end_offset-2) > sizeof(filename)) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Filename too long/not NULL terminated");
				tftp_reset(NULL);/**/
				break;
			}
			pbuf_copy_partial(p, filename, filename_end_offset-1, 2);
			do{
				int i=0;
				TRACE_OUT(TFTP_DEBUG_OUT,"tftp rcv file, file name : ");
				while(filename[i]!='\0'){
					TRACE_OUT(TFTP_DEBUG_OUT,"%c",filename[i]);
					i++;}
				TRACE_OUT(TFTP_DEBUG_OUT,"\n");
			}while(0);
            /*////////////////    ate lwip        //////////////////*/
            rcv_file_type=identify_file_type((char *)filename);
            if(rcv_file_type==bit_file)
            {
                /* close lvl shifter */
                FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);
                FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x838, 0x0);             /*close usr lvl*/
                FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x954, 0x0);             /*close tst lvl*/
                FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);     /*lock*/
                /* if debug by JTAG, sometimes fail */
                /*FDevcPs_fabricInit(&g_DEVC, FMSH_NON_SECURE_PCAP_WRITE);*/
                FDevcPs_Prog_B(&g_DEVC);
                /* need open lvl ? */
                u8 length=strlen(filename);
                if (strncmp((char *)(filename+length-11),cmp_usrlvl,7)==0)
                    open_usr_lvl_flag=1;
                if (strncmp((char *)(filename+length-11),cmp_tstlvl,7)==0)
                    open_tst_lvl_flag=1;
                rcv_tftp_space_ptr=(u8 *)ATE_TFTP_SRAM_SECTION_TWO_START;
            }
            if(rcv_file_type==gm_file)
            {
                gm_test_status=0;
                gm_pass_frame=0;
                FDevcPs_Prog_B(&g_DEVC);
                /*FDevcPs_fabricInit(&g_DEVC, FMSH_PCAP_READBACK);*/
                rcv_tftp_space_ptr=(u8 *)ATE_TFTP_SRAM_SECTION_TWO_START;
#if (DEVC_WIDTH!=32)
                ps_config_gmac(pll_x32);
#endif
            }
            if(rcv_file_type==bin_file){
                rcv_tftp_space_ptr=(u8 *)ATE_TFTP_BIN_START;
            }

			/* init */
            tftp_burst_rcv_length=0;
			ate_tftp_rcv_err_code=0;

			/* find \0 in pbuf -> end of mode string */
			mode_end_offset = pbuf_memfind(p, &tftp_null, sizeof(tftp_null), filename_end_offset+1);
			if((u16_t)(mode_end_offset-filename_end_offset) > sizeof(mode)) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Mode too long/not NULL terminated");
				tftp_reset(NULL);/**/
				break;
			}
			pbuf_copy_partial(p, mode, mode_end_offset-filename_end_offset, filename_end_offset+1);

			/* tftp_state.handle = tftp_state.ctx->open(filename, mode, opcode == PP_HTONS(TFTP_WRQ));*/
			do{/* debug*/
				tftp_state.handle=(void *)1;
			}while(0);

			tftp_state.blknum = 1;

			if (!tftp_state.handle) {
				send_error(addr, port, TFTP_ERROR_FILE_NOT_FOUND, "Unable to open requested file.");
				tftp_reset(NULL);/**/
				break;
			}

			LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: %s request from ", (opcode == PP_HTONS(TFTP_WRQ)) ? "write" : "read"));
			ip_addr_debug_print(TFTP_DEBUG | LWIP_DBG_STATE, addr);
			LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, (
													  " for '%s' mode '%s'\n", filename, mode));

			ip_addr_copy(tftp_state.addr, *addr);
			tftp_state.port = port;

			if (opcode == PP_HTONS(TFTP_WRQ)) {
				tftp_state.mode_write = 1;
				send_ack(0);
			} else {
				tftp_state.mode_write = 0;
				send_data();
			}

			break;
		}

	case PP_HTONS(TFTP_DATA):
		{
			u16_t blknum;

			if (tftp_state.handle == NULL) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "No connection");
				tftp_reset(NULL);/**/
				break;
			}

			if (tftp_state.mode_write != 1) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Not a write connection");
				tftp_reset(NULL);/**/
				break;
			}

			blknum = lwip_ntohs(sbuf[1]);
            /*printf("%d\r\n",blknum);*/
			pbuf_header(p, -TFTP_HEADER_LENGTH);


/*			if((u32_t)(rcv_tftp_space_ptr+(p->tot_len))>ATE_TFTP_SRAM_END)*/
/*			{*/
/*				ate_tftp_rcv_err_code=TFTP_ERROR_DISK_FULL;*/
/*				TRACE_OUT(TFTP_DEBUG_OUT, "Error: Space not enough!!!\r\n");*/
/*			}*/
/*			else*/
			{
                tftp_rcv_index++;
                u32 tmp_len=p->tot_len;
				if(rcv_file_type==bit_file)
				{/* bit */
#if (DEVC_WIDTH==32)
                    /*pbuf_copy_partial(p,(u8 *rcv_tftp_space_ptr,tmp_len,0);   ////x32*/
                    dma_done();
                    dma_cp((u32)(p->payload),(u32)rcv_tftp_space_ptr,tmp_len/2);
                    rcv_tftp_space_ptr=rcv_tftp_space_ptr+tmp_len;        /*/// x32*/
                    tftp_burst_rcv_length+=tmp_len;   /*/// x32*/
#elif (DEVC_WIDTH==16)
                    dma_done();
                    memset(rcv_tftp_space_ptr,tmp_len*2,0);
                    dma_cp((u32)(p->payload),(u32)rcv_tftp_space_ptr+2,tmp_len/2);
                    /*pbuf_copy_partial_devcx16_swap(p,rcv_tftp_space_ptr,tmp_len,0);   ////x16*/
                    rcv_tftp_space_ptr=rcv_tftp_space_ptr+tmp_len*2;    /*/// x16*/
                    tftp_burst_rcv_length+=tmp_len*2;   /*/// x16*/
#elif (DEVC_WIDTH==8)
                    dma_done();
                    memset(rcv_tftp_space_ptr,tmp_len*4,0);
                    dma_cp((u32)(p->payload),(u32)rcv_tftp_space_ptr+3,tmp_len);
                    /*pbuf_copy_partial_devcx16_swap(p,rcv_tftp_space_ptr,tmp_len,0);   ////x16*/
                    rcv_tftp_space_ptr=rcv_tftp_space_ptr+tmp_len*4;    /*/// x16*/
                    tftp_burst_rcv_length+=tmp_len*4;   /*/// x16*/
#endif
                    if(tftp_rcv_index>=TFTP_BURST_PACKET||tmp_len < TFTP_MAX_PAYLOAD_SIZE){
                        tftp_rcv_index=0;
                        send_ack(blknum);  /*/\\\\\\\\\//*/
                        /*printf("%ld\r\n",rcv_tftp_space_ptr);*/
                        rcv_tftp_space_ptr=(u8 *)ATE_TFTP_SRAM_SECTION_TWO_START;
                        /* devcfg */
                        /*ps_config_gmac(pll_down);   ///////////*/
                        /* make sure DMA done*/
                        dma_done();

                        FDevcPs_pcapLoadPartition(&g_DEVC,(u32*)rcv_tftp_space_ptr, (u32*)PCAP_WR_DATA_ADDR, tftp_burst_rcv_length / 4, tftp_burst_rcv_length / 4, FMSH_NON_SECURE_PCAP_WRITE);
                        /*ps_config_gmac(pll_up);   /////////////*/
                        /*send_ack(blknum);    ///////////////*/
                        tftp_burst_rcv_length=0;
                    }
                }
				if(rcv_file_type==bin_file)
				{/* bin */
					pbuf_copy_partial(p,rcv_tftp_space_ptr,tmp_len,0);
					rcv_tftp_space_ptr=rcv_tftp_space_ptr+tmp_len;
                    if(tftp_rcv_index>=TFTP_BURST_PACKET||tmp_len < TFTP_MAX_PAYLOAD_SIZE){
                        tftp_rcv_index=0;
                        send_ack(blknum);
                        /*rcv_tftp_space_ptr=(u8 *ATE_TFTP_BIN_START;*/
                        tftp_burst_rcv_length=0;
                    }
				}
                if(rcv_file_type==gm_file)
                {/* gm file */
#if (DEVC_WIDTH!=32)
                    pbuf_copy_partial(p,rcv_tftp_space_ptr,tmp_len,0);
#else
                    dma_done();
                    dma_cp((u32)(p->payload),(u32)rcv_tftp_space_ptr,tmp_len/2);
#endif
                    rcv_tftp_space_ptr=rcv_tftp_space_ptr+tmp_len;        /*/// x32*/
                    tftp_burst_rcv_length+=tmp_len;   /*/// x32*/

                    if(tftp_rcv_index>=TFTP_BURST_PACKET||tmp_len < TFTP_MAX_PAYLOAD_SIZE){
                        tftp_rcv_index=0;
                        send_ack(blknum); /*/\\\\\\\\\\\\\\\//*/
                        rcv_tftp_space_ptr=(u8 *)ATE_TFTP_SRAM_SECTION_TWO_START;
                        u32 total_frames=tftp_burst_rcv_length/TFTP_GM_FRAME_SIZE;
                        dma_done();
                        /*ps_config_gmac(pll_down); //////////////*/

                        u8 type;
                        u8 tb;
                        u8 row;
                        u16 col;
                        u8 min;
                        u8 wr01;
                        u32 far_addr;
                        u8 *data_p=rcv_tftp_space_ptr;
                        for(u32 frame_index=0;frame_index<total_frames;frame_index++)
                        {
                            u8 status=0;
                            type=*data_p;
                            tb=*(data_p+1);
                            row=*(data_p+2);
                            col=*(data_p+3)*256+(*(data_p+4));
                            min=*(data_p+5);
                            wr01=*(data_p+6);
                            data_p+=12;
                            far_addr=(type<<23)|(tb<<22)|(row<<17)|(col<<7)|min;
                            if(wr01==1){
                                /*FDevcPs_fabricInit(&g_DEVC, FMSH_PCAP_READBACK);*/
                                status=FDevcPs_wrCheck(far_addr,0xffffffff,(u32)data_p);
                            }
                            else{
                                /*FDevcPs_fabricInit(&g_DEVC, FMSH_PCAP_READBACK);*/
                                status=FDevcPs_wrCheck(far_addr,0,(u32)data_p);
                            }
                            data_p+=404;
                            if(0!=status){
                                gm_test_status|=1;
                            }
#if 0 /*debug*/
                            if(0!=status)
                                printf("f,type:%d,tb:%d,row:%d,col:%d,min:%d\r\n",type,tb,row,col,min);
#endif
                            if(0==gm_test_status){
                                gm_pass_frame++;
                            }
                        }

                        /*ps_config_gmac(pll_up);  //////////////*/
                        /*send_ack(blknum); //////////////*/
                        tftp_burst_rcv_length=0;
                    }
                }
			}

			/**/

			if (p->tot_len < TFTP_MAX_PAYLOAD_SIZE)
			{
				/* close_handle();*/
				TRACE_OUT(TFTP_DEBUG_OUT,"tftp finish, close handle.\n");/* debug*/
				tftp_state.handle = NULL;/* debug*/
				tftp_reset(NULL);/**/

                if(ate_tftp_rcv_err_code!=0)
                    break;
                /* finish bit */
				if(rcv_file_type==bit_file)
				{
                    FDevcPs_transfer(&g_DEVC, (void*)src_addr, 12, (u8 *)FMSH_DMA_INVALID_ADDRESS, 0, FMSH_NON_SECURE_PCAP_WRITE_DUMMMY);
                    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x008, 0xDF0D767BU);
                    if (open_tst_lvl_flag!=0){
                        FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x954, 0xf);             /*OPEN TST_LVL_SHFTR_EN*/
                        open_tst_lvl_flag=0;
                    }
                    if (open_usr_lvl_flag!=0){
                        FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x838, 0xf);             /*open usr lvl*/
                        open_usr_lvl_flag=0;
                    }
                    FMSH_WriteReg(FPS_SLCR_BASEADDR, 0x004, 0xDF0D767BU);     /*lock*/
                }

                /* boot bin! */
				if(rcv_file_type==bin_file)
				{
                    /* close */
                    FDmaPs_disable(pDmac);
                    /* go */
					ate_enet_boot(ATE_TFTP_BIN_START,0);
				}
                /* gm file */
                if(rcv_file_type==gm_file)
                {
                    if(0==gm_test_status)
                        FGpioPs_writeData(&inst_GPIOB, (1<<8)|(1<<10)|(1<<11));  /* 40 41 42 43 1011 */
                    else
                        FGpioPs_writeData(&inst_GPIOB, (1<<8)|(1<<10)); /* 40 41 42 43 1010 */
#if 1 /*debug*/
                    if(0!=gm_test_status)
                        printf("f %d\r\n",gm_pass_frame);
                    else
                        printf("p %d\r\n",gm_pass_frame);
#endif
#if (DEVC_WIDTH!=32)
                    ps_config_gmac(pll_x16);
#endif
                }

			}
			break;
		}
	case PP_HTONS(TFTP_ACK):
		{
			u16_t blknum;
			int lastpkt;

			if (tftp_state.handle == NULL) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "No connection");
				tftp_reset(NULL);/**/
				break;
			}

			if (tftp_state.mode_write != 0) {
				send_error(addr, port, TFTP_ERROR_ACCESS_VIOLATION, "Not a read connection");
				tftp_reset(NULL);/**/
				break;
			}

			blknum = lwip_ntohs(sbuf[1]);
			if (blknum != tftp_state.blknum) {
				send_error(addr, port, TFTP_ERROR_UNKNOWN_TRFR_ID, "Wrong block number");
				tftp_reset(NULL);/**/
				break;
			}

			lastpkt = 0;

			if (tftp_state.last_data != NULL) {
				lastpkt = tftp_state.last_data->tot_len != (TFTP_MAX_PAYLOAD_SIZE + TFTP_HEADER_LENGTH);
			}

			if (!lastpkt) {
				tftp_state.blknum++;
				send_data();
			} else {
				/* close_handle();*/
				tftp_state.handle = NULL;/* debug*/
			}

			break;
		}

	default:
        {
            send_error(addr, port, TFTP_ERROR_ILLEGAL_OPERATION, "Unknown operation");
            tftp_reset(NULL);/**/
            break;
        }
	}

	pbuf_free(p);
    /*gpio40=1;*/
    /*FGpioPs_writeData(&inst_GPIOB, (gpio40<<8)|(gpio42<<10)); ///debug*/
}

static void tftp_tmr(void* arg)
{
	LWIP_UNUSED_ARG(arg);

	tftp_state.timer++;

	if (tftp_state.handle == NULL) {
		return;
	}

	sys_timeout(TFTP_TIMER_MSECS, tftp_tmr, NULL);

	if ((tftp_state.timer - tftp_state.last_pkt) > (TFTP_TIMEOUT_MSECS / TFTP_TIMER_MSECS)) {
		if ((tftp_state.last_data != NULL) && (tftp_state.retries < TFTP_MAX_RETRIES)) {
			LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: timeout, retrying\n"));
			resend_data();
			tftp_state.retries++;
		} else {
			LWIP_DEBUGF(TFTP_DEBUG | LWIP_DBG_STATE, ("tftp: timeout\n"));
			close_handle();
		}
	}
}

/** @ingroup tftp
* Initialize TFTP server.
* @param ctx TFTP callback struct
*/
err_t tftp_init(const struct tftp_context *ctx)
{
	err_t ret;

	struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	if (pcb == NULL) {
		return ERR_MEM;
	}

	ret = udp_bind(pcb, IP_ANY_TYPE, TFTP_PORT);
	if (ret != ERR_OK) {
		udp_remove(pcb);
		return ret;
	}

	tftp_state.handle    = NULL;
	tftp_state.port      = 0;
	tftp_state.ctx       = NULL;/*ctx;*/
	tftp_state.timer     = 0;
	tftp_state.last_data = NULL;
	tftp_state.upcb      = pcb;

	udp_recv(pcb, recv, NULL);

	return ERR_OK;
}
err_t tftp_reset(const struct tftp_context *ctx)
{
	err_t ret;
	udp_disconnect(tftp_state.upcb);
	udp_remove(tftp_state.upcb);

	struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	if (pcb == NULL) {
		return ERR_MEM;
	}

	ret = udp_bind(pcb, IP_ANY_TYPE, TFTP_PORT);
	if (ret != ERR_OK) {
		udp_remove(pcb);
		return ret;
	}

	tftp_state.handle    = NULL;
	tftp_state.port      = 0;
	tftp_state.ctx       = NULL;/*ctx;*/
	tftp_state.timer     = 0;
	tftp_state.last_data = NULL;
	tftp_state.upcb      = pcb;

	udp_recv(pcb, recv, NULL);

	return ERR_OK;
}

/*#endif /* LWIP_UDP */
