#include <linux/workqueues.h>

struct bch_async_checksum {
    struct work_struct work;
    struct completion done;
    struct bio_vec *bvecs;
    unsigned int nr_vecs;
    u32 *result;
    int status;
};

static void bch_checksum_work(struct work_struct *work)
{
    struct bch_async_checksum *ctx =
        container_of(work, struct bch_async_checksum, work);

    /* Example: simple Fletcher32 checksum */
    ctx->status = 0;
    *ctx->result = 0;

    for (unsigned int i = 0; i < ctx->nr_vecs; i++) {
        void *kaddr = kmap_local_page(ctx->bvecs[i].bv_page) +
                      ctx->bvecs[i].bv_offset;
        *ctx->result ^= crc32(~0, kaddr, ctx->bvecs[i].bv_len);
        kunmap_local(kaddr);
    }

    complete(&ctx->done);
}

int bch_checksum_async(struct bio_vec *bvecs,
                       unsigned int nr_vecs,
                       u32 *out)
{
    struct bch_async_checksum ctx;

    init_completion(&ctx.done);
    ctx.bvecs   = bvecs;
    ctx.nr_vecs = nr_vecs;
    ctx.result  = out;
    ctx.status  = 0;

    INIT_WORK(&ctx.work, bch_checksum_work);

    /* Queue on system workqueue (later: hardware offload) */
    schedule_work(&ctx.work);

    /* If you want to block here (sync semantics): */
    wait_for_completion(&ctx.done);

    return ctx.status;
}
