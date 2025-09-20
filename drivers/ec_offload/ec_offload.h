#ifndef _EC_OFFLOAD_H
#define _EC_OFFLOAD_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/workqueue.h>

struct ec_req;
struct ec_dev;

/* Callback type: ctx, status (0 = ok), req pointer */
typedef void (*ec_callback_t)(void *ctx, int status, struct ec_req *req);

struct ec_req {
    /* Erasure coding parameters */
    unsigned k, m;            /* data blocks, parity blocks */
    unsigned block_size;      /* size of each block */

    u8 **data_bufs;           /* array of data block pointers */
    u8 **parity_bufs;         /* array of parity block pointers */

    /* Context for completion */
    void *ctx;
    ec_callback_t cb;

    /* Internal driver state */
    struct work_struct work;
    struct ec_dev *edev;
    struct list_head list;
};

struct ec_dev {
    struct device *dev;
    struct workqueue_struct *wq;
    spinlock_t lock;
    struct list_head pending;
};

/* API */
struct ec_dev *ec_offload_register(struct device *parent);
void ec_offload_unregister(struct ec_dev *edev);

int ec_submit_request(struct ec_dev *edev, struct ec_req *req);

#endif
