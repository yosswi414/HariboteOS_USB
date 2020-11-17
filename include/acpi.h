#include "general.h"

struct RSDPtr {
    byte Signature[8];
    byte CheckSum;
    byte OemID[6];
    byte Revision;
    dword *RsdtAddress;
};

struct FACP {
    byte Signature[4];
    dword Length;
    byte unneded1[40 - 8];
    dword *DSDT;
    byte unneded2[48 - 44];
    dword *SMI_CMD;
    byte ACPI_ENABLE;
    byte ACPI_DISABLE;
    byte unneded3[64 - 54];
    dword *PM1a_CNT_BLK;
    dword *PM1b_CNT_BLK;
    byte unneded4[89 - 72];
    byte PM1_CNT_LEN;
};

// check if the given address has a valid header
unsigned int *acpiCheckRSDPtr(unsigned int *ptr);

// finds the acpi header and returns the address of the rsdt
unsigned int *acpiGetRSDPtr(void);

// checks for a given header and validates checksum
int acpiCheckHeader(unsigned int *ptr, char *sig);

int acpiEnable(void);

//
// bytecode of the \_S5 object
// -----------------------------------------
//        | (optional) |    |    |    |
// NameOP | \          | _  | S  | 5  | _
// 08     | 5A         | 5F | 53 | 35 | 5F
//
// -----------------------------------------------------------------------------------------------------------
//           |           |              | ( SLP_TYPa   ) | ( SLP_TYPb   ) | ( Reserved   ) | (Reserved    )
// PackageOP | PkgLength | NumElements  | byteprefix Num | byteprefix Num | byteprefix Num | byteprefix Num
// 12        | 0A        | 04           | 0A         05  | 0A          05 | 0A         05  | 0A         05
//
//----this-structure-was-also-seen----------------------
// PackageOP | PkgLength | NumElements |
// 12        | 06        | 04          | 00 00 00 00
//
// (Pkglength bit 6-7 encode additional PkgLength bytes [shouldn't be the case here])
//
int initAcpi(void);

void acpiPowerOff(void);