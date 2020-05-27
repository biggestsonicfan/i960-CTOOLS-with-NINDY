/* cyt_test.h */


typedef struct
{
    char *mbox0_test;
    char *mbox1_test;
    char *mbox2_test;
    char *mbox3_test;
    char *mbox4_test;
    char *mbox5_test;
    char *mbox6_test;
    char *mbox7_test;
    char *bist_test;
    char *pci_mem_tests;
    char *pci_dma_tests;
    char *doorbell_test;
    char *local_tas_test;
    char *remote_tas_test;
} PCI_TEST_RESULTS;


#define TEST_FAILED	"Test Failed ***\n"
#define TEST_PASSED	"Test Passed\n"
#define TEST_NOTRUN	"Test Not Run\n"

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
