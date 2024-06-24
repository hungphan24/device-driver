/*
    - GPIO P8_10  - PIN68 = 2_3: 0x481A_C000 - 0x481A_CFFF
    - offset:  offset is calculated by byte, __iomem * = 4 byte, mỗi bước nhảy là 4 byte  --> /4
         GPIO_SETDATAOUT:    0x194   ON
         GPIO_CLEARDATAOUT:  0x190   OFF
         GPIO_OE             0x134   IN/OUT: 0: OUT
*/
#ifndef __LED_MODULE_H__
#define __LED_MODULE_H__

#define GPIO0_ADDR_BASE     0x44E07000
#define GPIO0_ADDR_END      0x44E07FFF
#define GPIO0_ADDR_SIZE     (GPIO0_ADDR_END - GPIO0_ADDR_BASE)

#define GPIO_OE_OFFSET			    0x134
#define GPIO_CLEARDATAOUT_OFFSET	0x190
#define GPIO_SETDATAOUT_OFFSET		0x194

#define GPIO0_30                    (1 << 30)       /* P9_11 */

#endif  /* __LED_MODULE_H__ */