#include "cyc9060.h"

int dmaTest (
   int unit,            /* Which DMA Controller 0 or 1 */
   long   *PAddr, 	/* PCI Start address of test */
   long   *LAddr, 	/* Local DRAM Start address of test */
   long   count         /* Byte count for test */
   );


#define DMA_BLK		0x5000		/* DMA transfers are 0x5000 bytes */
#define OK		0		/* DMA transfer OK */
#define TRANSFER_ERROR	-1		/* DMA transfer not OK */
#define LONGTIME	10000000	/* Transfer timeout */	
#define WRITE		0
#define READ		~WRITE

#define FAILED          1
#define PASSED          0
