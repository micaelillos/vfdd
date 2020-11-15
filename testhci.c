/*
 * testhci.c - dirty and temporarly test the state of bluetooth hci for dotled
 * 
 * include LICENSE
 */
#include <stdint.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <appclass.h>

//#include <bluetooth/hci.h>

#define HCIGETDEVINFO	_IOR('H', 211, int)

#define BTPROTO_HCI	1

/* HCI device flags */
enum {
        HCI_UP,
        HCI_INIT,
        HCI_RUNNING,

        HCI_PSCAN,
        HCI_ISCAN,
        HCI_AUTH,
        HCI_ENCRYPT,
        HCI_INQUIRY,

        HCI_RAW,
};

struct hci_dev_stats {
        uint32_t err_rx;
        uint32_t err_tx;
        uint32_t cmd_tx;
        uint32_t evt_rx;
        uint32_t acl_tx;
        uint32_t acl_rx;
        uint32_t sco_tx;
        uint32_t sco_rx;
        uint32_t byte_rx;
        uint32_t byte_tx;
};

/* BD Address */
typedef struct {
        uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

static inline int hci_test_bit(int nr, void *addr)
{
        return *((uint32_t *) addr + (nr >> 5)) & (1 << (nr & 31));
}

struct hci_dev_info {
        uint16_t dev_id;
        char     name[8];

        bdaddr_t bdaddr;

        uint32_t flags;
        uint8_t  type;

        uint8_t  features[8];

        uint32_t pkt_type;
        uint32_t link_policy;
        uint32_t link_mode;

        uint16_t acl_mtu;
        uint16_t acl_pkts;
        uint16_t sco_mtu;
        uint16_t sco_pkts;

        struct   hci_dev_stats stat;
};

static struct hci_dev_info di;

int dotled_test_bluetooth( AppClass *data, void *user_data );
int dotled_test_bluetooth( AppClass *data, void *user_data )
{
   int ctl;
   int ret = 0;
   
   /* Open HCI socket  */
   if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
      msg_error("Can't open HCI socket.");
      return 0;
   }
   if (ioctl(ctl, (int) HCIGETDEVINFO, (void *) &di)) {
      msg_error("Can't get device info");
      goto finish;
   }

   if ( hci_test_bit(HCI_UP, &di.flags)){
      ret = 1;
   }
finish:
   close(ctl);
   return ret;
}
