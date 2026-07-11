/* FatFs media adapter for the PSoC SD-over-SPI driver. */
#include "ff.h"
#include "diskio.h"
#include "sd_spi.h"

static DSTATUS g_disk_status = STA_NOINIT;

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0u) {
        return STA_NOINIT;
    }
    if (!sd_spi_present() && !sd_spi_init()) {
        g_disk_status = (DSTATUS)(STA_NOINIT | STA_NODISK);
        return g_disk_status;
    }
    g_disk_status = 0u;
    return g_disk_status;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0u) {
        return STA_NOINIT;
    }
    if (!sd_spi_present()) {
        g_disk_status = (DSTATUS)(STA_NOINIT | STA_NODISK);
    }
    return g_disk_status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    UINT i;
    if (pdrv != 0u || buff == 0 || count == 0u || disk_status(pdrv) != 0u) {
        return RES_PARERR;
    }
    for (i = 0u; i < count; i++) {
        if (!sd_spi_read_block((uint32)(sector + (LBA_t)i), buff)) {
            return RES_ERROR;
        }
        buff += SD_BLOCK_BYTES;
    }
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    UINT i;
    if (pdrv != 0u || buff == 0 || count == 0u || disk_status(pdrv) != 0u) {
        return RES_PARERR;
    }
    for (i = 0u; i < count; i++) {
        if (!sd_spi_write_block((uint32)(sector + (LBA_t)i), buff)) {
            return RES_ERROR;
        }
        buff += SD_BLOCK_BYTES;
    }
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0u || disk_status(pdrv) != 0u) {
        return RES_NOTRDY;
    }
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK; /* sd_spi_write_block() is synchronous. */
        case GET_SECTOR_COUNT:
            if (buff == 0 || sd_spi_sector_count() == 0u) return RES_ERROR;
            *(LBA_t *)buff = (LBA_t)sd_spi_sector_count();
            return RES_OK;
        case GET_SECTOR_SIZE:
            if (buff == 0) return RES_PARERR;
            *(WORD *)buff = (WORD)SD_BLOCK_BYTES;
            return RES_OK;
        case GET_BLOCK_SIZE:
            if (buff == 0) return RES_PARERR;
            *(DWORD *)buff = 1u;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

