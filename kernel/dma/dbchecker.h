#ifndef _KERNEL_DMA_DBCHECKER_H
#define _KERNEL_DMA_DBCHECKER_H

#include "direct.h"

#define DBCHECKER_BASE_ADDR 0x40000000
#define DBCHECKER_REG_SIZE 8 // 64bit
#define DBCHECKER_REG_NUM 8  // 8 registers

#define DBCHECKER_EN_OFFSET         0x00
#define DBCHECKER_CMD_OFFSET        0x08
#define DBCHECKER_RES_OFFSET        0x10
#define DBCHECKER_KEYL_OFFSET       0x18
#define DBCHECKER_KEYH_OFFSET       0x20
#define DBCHECKER_ERR_CNT_OFFSET    0x28
#define DBCHECKER_ERR_INFO_OFFSET   0x30
#define DBCHECKER_ERR_MTDT_OFFSET   0x38

#define DBCHECKER_BNDS_MAX_SIZE  0x10000 // 64KB
#define DBCHECKER_BNDL_MAX_SIZE  0x100000000 // 4GB

void dbchecker_init(void);
void dbchecker_enable(void);
void dbchecker_disable(void);
dma_addr_t dbchecker_alloc_mtdt(dma_addr_t addr, size_t size, enum dma_data_direction dir);
dma_addr_t dbchecker_free_mtdt(dma_addr_t addr);
int dbchecker_err_handler(void);
#endif