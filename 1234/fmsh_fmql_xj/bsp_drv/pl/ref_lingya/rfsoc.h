#ifndef RFSOC_H_
#define RFSOC_H_

#define AD_READ		(0 << 15)
#define AD_WRITE    (1 << 15)
#define AD_CNT(x)	((((x) - 1) & 0x7) << 12)
#define AD_ADDR(x)	((x) & 0x7FF)

int32_t spi_init(uint8_t id_no, uint8_t clk_pha, uint8_t clk_pol);
int32_t rfsoc_init(void);
int32_t rfsoc_spi_loop(uint8_t id_no, uint8_t *tx, uint8_t *rx, uint8_t len);
int32_t rfsoc_spi_write(uint8_t id_no, uint32_t reg, uint32_t val);
int32_t rfsoc_spi_read(uint8_t id_no, uint32_t reg, uint32_t *val);
int32_t rfsoc_spi_cfmwrt(uint8_t id_no, uint32_t reg, uint32_t val);
int32_t rfsoc_ctrl(uint32_t val);

extern uint32_t g_reg_addr_val_cfg[][3];
extern uint32_t g_reg_addr_val_cnt;


#endif  