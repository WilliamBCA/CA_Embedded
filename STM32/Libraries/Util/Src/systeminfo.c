/*
 * Generic functions used the the STM system
 */

#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "systemInfo.h"
#include "githash.h"
#include "HAL_otp.h"

// F4xx UID
#define ID1 *((unsigned long *) (UID_BASE))
#define ID2 *((unsigned long *) (UID_BASE + 4U))
#define ID3 *((unsigned long *) (UID_BASE + 8U))

static const char* mcuType()
{
    static char mcu[50] = { 0 }; // static to prevent allocate on stack.
    int len = 0;

    const DBGMCU_TypeDef* mcuType = DBGMCU;
    const uint16_t idCode = 0x00000FFF & mcuType->IDCODE;
    const uint16_t revCode = 0xFFFF & (mcuType->IDCODE >> 16);

    switch(idCode)
    {
    case 0x423: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xB/C");        break;
    case 0x433: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xD/E");        break;
    default:    len += snprintf(&mcu[len], sizeof(mcu) -len, "Unknown 0x%3X", idCode); break;
    }

    len += snprintf(&mcu[len], sizeof(mcu) -len, " Rev %x", revCode);

    return mcu;
}

static char* productType(uint8_t id)
{
    switch(id)
    {
    case AC_Board         :  return "AC Board";          break;
    case DC_Board         :  return "DC Board";          break;
    case Temperature      :  return "Temperature";       break;
    case Current          :  return "Current";           break;
    case FlowChip         :  return "FlowChip";          break;
    case humidityChip     :  return "humidityChip";      break;
    case Pressure         :  return "Pressure";          break;
    case SaltFlowBoard    :  return "Salt Flow Board";   break;
    case SaltLeak         :  return "SaltLeak";          break;
    case HotValvePumpBoard:  return "HotValvePumpBoard"; break;
    }
    return "NA";
}

const char* systemInfo()
{
    static char buf[600] = { 0 };
    BoardInfo info = { 0 };

    if (HAL_otpRead(&info) != OTP_SUCCESS)
    {
        info.otpVersion = 0; // Invalid OTP version
    }

    int len = 0;
    len  = snprintf(&buf[len], sizeof(buf) - len, "Serial Number: %lX%lX%lX\r\n", ID1, ID2, ID3);
    switch(info.otpVersion)
    {
    case OTP_VERSION_1:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n", productType(info.v1.boardType));
        break;
    case OTP_VERSION_2:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n", productType(info.v2.boardType));
        len += snprintf(&buf[len], sizeof(buf) - len, "Sub Product Type: %u\r\n", info.v2.subBoardType);
        break;
    default:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: NA\r\n");
        break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "MCU Family: %s\r\n", mcuType());
    len += snprintf(&buf[len], sizeof(buf) - len, "Software Version: %s\r\n", GIT_VERSION);
    len += snprintf(&buf[len], sizeof(buf) - len, "Compile Date: %s\r\n", GIT_DATE);
    len += snprintf(&buf[len], sizeof(buf) - len, "Git SHA %s\r\n", GIT_SHA);
    switch(info.otpVersion)
    {
    case OTP_VERSION_1:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d", info.v1.pcbVersion.major, info.v1.pcbVersion.minor);
        break;
    case OTP_VERSION_2:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d", info.v2.pcbVersion.major, info.v2.pcbVersion.minor);
        break;
    default:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: NA");
        break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "\r\n");

    return buf;
}

int getBoardInfo(BoardType *bdt, SubBoardType *sbdt)
{
    BoardInfo info = { 0 };
    if (HAL_otpRead(&info) != OTP_SUCCESS)
        return -1;

    if (info.otpVersion == OTP_VERSION_1)
    {
        if (bdt)  { *bdt  = info.v1.boardType; }
        if (sbdt) { *sbdt = 0; } // Sub board type is not included in version 1
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2)
    {
        if (bdt)  { *bdt  = info.v2.boardType; }
        if (sbdt) { *sbdt = info.v2.subBoardType; }
        return 0;
    }

    return -1; // New OTP version. i.e. this SW is to old.
}

int getPcbVersion(pcbVersion* ver)
{
    BoardInfo info = { 0 };
    if (ver == 0 || HAL_otpRead(&info) != OTP_SUCCESS)
        return -1;

    if (info.otpVersion == OTP_VERSION_1)
    {
        ver->major = info.v1.pcbVersion.major;
        ver->minor = info.v1.pcbVersion.minor;
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2)
    {
        ver->major = info.v2.pcbVersion.major;
        ver->minor = info.v2.pcbVersion.minor;
        return 0;
    }

    return -1; // New OTP version. i.e. this SW is to old.
}
