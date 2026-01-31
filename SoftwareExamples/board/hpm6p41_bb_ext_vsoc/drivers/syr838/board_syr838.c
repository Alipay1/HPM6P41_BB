#include "board_syr838.h"
#include "hpm_i2c.h"

#include "def_rtt_printf.h"

#define SYR838_I2C_ADDR          (0x41U)   /* 7-bit address: 0x41h:contentReference[oaicite:3]{index=3} */

#define SYR838_REG_VSEL0         (0x00U)   /* VSEL0 register address:contentReference[oaicite:4]{index=4} */
#define SYR838_REG_VSEL1         (0x01U)   /* VSEL1 register address:contentReference[oaicite:5]{index=5} */
#define SYR838_REG_PGOOD         (0x05U)   /* PGOOD register address:contentReference[oaicite:6]{index=6} */

#define SYR838_NSEL_MASK         (0x3FU)   /* NSEL[5:0] controls VOUT code:contentReference[oaicite:7]{index=7} */

#define LOG(args...) printf(args)

static hpm_i2c_context_t * syr838_i2c_context;

hpm_stat_t board_syr838_reg_i2c_context(void * ptr){
  if(ptr == NULL){
    return 1;
  }
  syr838_i2c_context =( hpm_i2c_context_t *) ptr;
  return 0;
}

static hpm_stat_t syr838_read_reg(uint8_t reg, uint8_t *val)
{
    return hpm_i2c_master_addr_read_blocking(syr838_i2c_context,
                                             SYR838_I2C_ADDR,
                                             reg, 1,
                                             val, 1,
                                             0xFF);
}

static hpm_stat_t syr838_write_reg(uint8_t reg, uint8_t val)
{
    return hpm_i2c_master_addr_write_blocking(syr838_i2c_context,
                                              SYR838_I2C_ADDR,
                                              reg, 1,
                                              &val, 1,
                                              0xFF);
}
/* VOUT = 0.7125V + NSEL*12.5mV, NSEL=0..63 (0x3F):contentReference[oaicite:8]{index=8} */
static uint8_t syr838_uv_to_nsel(uint32_t vout_uv, uint32_t *actual_uv)
{
    /* clamp to supported range: 712500uV..1500000uV */
    if (vout_uv < 712500U)  vout_uv = 712500U;
    if (vout_uv > 1500000U) vout_uv = 1500000U;

    /* round to nearest step (12.5mV = 12500uV) */
    uint32_t code = (vout_uv - 712500U + 6250U) / 12500U;
    if (code > 0x3FU) code = 0x3FU;

    if (actual_uv) {
        *actual_uv = 712500U + code * 12500U;
    }
    return (uint8_t)code;
}

/* write BOTH VSEL0 & VSEL1 so it works regardless of VSEL pin level */
hpm_stat_t syr838_set_vout_uv(uint32_t vout_uv)
{
    hpm_stat_t st;
    uint32_t actual_uv = 0;
    uint8_t nsel = syr838_uv_to_nsel(vout_uv, &actual_uv);

    for (uint8_t reg = SYR838_REG_VSEL0; reg <= SYR838_REG_VSEL1; reg++) {
        uint8_t vsel = 0;

        /* read-modify-write: keep BUCK_EN/MODE bits, only update NSEL[5:0]:contentReference[oaicite:9]{index=9} */
        do { st = syr838_read_reg(reg, &vsel); } while (st != status_success);

        vsel = (uint8_t)((vsel & ~SYR838_NSEL_MASK) | (nsel & SYR838_NSEL_MASK));

        do { st = syr838_write_reg(reg, vsel); } while (st != status_success);
    }

    /* optional: read PGOOD[7] to see if buck enabled & soft-start done:contentReference[oaicite:10]{index=10} */
    uint8_t pgood = 0;
    st = syr838_read_reg(SYR838_REG_PGOOD, &pgood);

    LOG("SYR838 set VOUT: req=%u uV, nsel=0x%02X, actual=%u uV, PGOOD=%u\r\n",
        (unsigned)vout_uv,
        (unsigned)nsel,
        (unsigned)actual_uv,
        (unsigned)((pgood >> 7) & 0x1));

    return status_success;
}