#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "ec_offload.h"

/* --- Mock EC encoder (toy parity = XOR of data blocks) --- */
static void mock_ec_encode(struct ec_req *req)
{
    unsigned i, j;

    for (j = 0; j < req->m; j++) {
        memset(req->parity_bufs[j], 0, req->block_size);

        for (i = 0; i < req->k; i++) {
            unsigned b;
            for (b = 0; b < req->block_size; b++)
                req->parity_bufs[j][b] ^= req->data_bufs[i][b];
        }
    }
}

/* --- Workqueue completion handler --- */
static void ec_work_handler(struct work_struct *work)
{
    struct ec_req *req = container_of(work, struct ec_req, work);

    /* Pretend hardware took time, then do software EC */
    mock_ec_encode(req);

    /* Notify bcachefs via callback */
    if (req->cb)
        req->cb(req->ctx, 0, req);
}

/* --- Submission path --- */
int ec_submit_request(struct ec_dev *edev, struct ec_req *req)
{
    req->edev = edev;
    INIT_WORK(&req->work, ec_work_handler);

    spin_lock(&edev->lock);
    list_add_tail(&req->list, &edev->pending);
    spin_unlock(&edev->lock);

    queue_work(edev->wq, &req->work);

    return 0;
}
EXPORT_SYMBOL_GPL(ec_submit_request);

/* --- Register/unregister --- */
struct ec_dev *ec_offload_register(struct device *parent)
{
    struct ec_dev *edev;

    edev = kzalloc(sizeof(*edev), GFP_KERNEL);
    if (!edev)
        return NULL;

    edev->dev = parent;
    spin_lock_init(&edev->lock);
    INIT_LIST_HEAD(&edev->pending);

    edev->wq = alloc_workqueue("ec_offload_wq",
                               WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
    if (!edev->wq) {
        kfree(edev);
        return NULL;
    }

    pr_info("ec_offload: mock EC device registered\n");
    return edev;
}
EXPORT_SYMBOL_GPL(ec_offload_register);

void ec_offload_unregister(struct ec_dev *edev)
{
    if (!edev)
        return;

    destroy_workqueue(edev->wq);
    kfree(edev);

    pr_info("ec_offload: mock EC device unregistered\n");
}
EXPORT_SYMBOL_GPL(ec_offload_unregister);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranav Jha <pranavj765@gmail.com>");
MODULE_DESCRIPTION("Driver that offloads checksum");
