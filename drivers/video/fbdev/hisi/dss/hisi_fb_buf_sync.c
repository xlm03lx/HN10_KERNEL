/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#include "hisi_fb.h"
#define HISI_DSS_LAYERBUF_FREE	"hisifb%d-layerbuf-free"

////////////////////////////////////////////////////////////////////////////////
//
// layerbuffer handle, for online compose
//
static bool  hisi_check_parameter_valid(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return false;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return false;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL!\n");
		return false;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("ion_client is NULL!\n");
		return false;
	}

	return true;
}

int hisifb_layerbuf_lock(struct hisi_fb_data_type *hisifd,
	dss_overlay_t *pov_req, struct list_head *plock_list)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	int i = 0;
	int m = 0;
	struct hisifb_layerbuf *node = NULL;
	struct ion_handle *ionhnd = NULL;
	struct iommu_map_format iommu_format;
	bool add_tail = false;
	bool has_map_iommu;
	bool parameter_valid;

	parameter_valid = hisi_check_parameter_valid(hisifd, pov_req, plock_list);
	if (parameter_valid == false) {
		return -EINVAL;
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);
			add_tail = false;
			ionhnd = NULL;
			has_map_iommu = false;

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->img.shared_fd < 0)
				continue;

			if ((layer->img.phy_addr == 0) &&
				(layer->img.vir_addr == 0) &&
				(layer->img.afbc_payload_addr == 0)) {
				HISI_FB_ERR("fb%d, layer_idx%d, chn_idx%d, no buffer!\n",
					hisifd->index, layer->layer_idx, layer->chn_idx);
				continue;
			}

			if (layer->img.shared_fd >= 0) {
				ionhnd = ion_import_dma_buf(hisifd->ion_client, layer->img.shared_fd);
				if (IS_ERR(ionhnd)) {
					ionhnd = NULL;
					HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_import_dma_buf, shared_fd=%d!\n",
						hisifd->index, i, layer->img.shared_fd);
				} else {
					if (layer->img.mmu_enable == 1) {
						memset(&iommu_format, 0, sizeof(struct iommu_map_format));
						if (ion_map_iommu(hisifd->ion_client, ionhnd, &iommu_format)) {
							HISI_FB_ERR("fb%d, layer_idx%d, failed to ion_map_iommu, shared_fd=%d!\n",
								hisifd->index, i, layer->img.shared_fd);
						} else {
							has_map_iommu = true;
						}
					}

					add_tail = true;
				}
			}

			if (add_tail) {
				node = kzalloc(sizeof(struct hisifb_layerbuf), GFP_KERNEL);
				if (node == NULL) {
					HISI_FB_ERR("fb%d, layer_idx%d, failed to kzalloc!\n",
						hisifd->index, layer->layer_idx);

					if (ionhnd) {
						ion_free(hisifd->ion_client, ionhnd);
						ionhnd = NULL;
					}

					continue;
				}

				node->shared_fd = layer->img.shared_fd;
				node->frame_no = pov_req->frame_no;
				node->ion_handle = ionhnd;
				node->has_map_iommu = has_map_iommu;
				node->timeline = 0;
				//mmbuf
				node->mmbuf.addr = layer->img.mmbuf_base;
				node->mmbuf.size = layer->img.mmbuf_size;

				node->vir_addr = layer->img.vir_addr;
				node->chn_idx = layer->chn_idx;

				list_add_tail(&node->list_node, plock_list);

				if (g_debug_layerbuf_sync) {
					HISI_FB_INFO("fb%d, frame_no=%d, layer_idx(%d), shared_fd=%d, ion_handle=%pK, "
						"has_map_iommu=%d, timeline=%d, mmbuf(0x%x, %d).\n",
						hisifd->index, node->frame_no, i, node->shared_fd, node->ion_handle,
						node->has_map_iommu, node->timeline,
						node->mmbuf.addr, node->mmbuf.size);
				}
			}
		}
	}

	return 0;
}

void hisifb_layerbuf_flush(struct hisi_fb_data_type *hisifd,
	struct list_head *plock_list)
{
	struct hisifb_layerbuf *node, *_node_;
	unsigned long flags = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("ion_client is NULL");
		return;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL");
		return;
	}

	spin_lock_irqsave(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
	hisifd->buf_sync_ctrl.layerbuf_flushed = true;
	list_for_each_entry_safe(node, _node_, plock_list, list_node) {
		list_del(&node->list_node);
		list_add_tail(&node->list_node, &(hisifd->buf_sync_ctrl.layerbuf_list));
	}
	spin_unlock_irqrestore(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
}

void hisifb_layerbuf_unlock(struct hisi_fb_data_type *hisifd,
	struct list_head *pfree_list)
{
	struct hisifb_layerbuf *node, *_node_;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("hisifd->ion_client is NULL");
		return;
	}
	if (NULL == hisifd->mmbuf_gen_pool) {
		HISI_FB_ERR("hisifd->mmbuf_gen_pool is NULL");
		return;
	}
	if (NULL == pfree_list) {
		HISI_FB_ERR("pfree_list is NULL");
		return;
	}

	list_for_each_entry_safe(node, _node_, pfree_list, list_node) {
		list_del(&node->list_node);

		if (g_debug_layerbuf_sync) {
			HISI_FB_INFO("fb%d, frame_no=%d, share_fd=%d, ion_handle=%pK, has_map_iommu=%d, "
				"timeline=%d, mmbuf(0x%x, %d).vir_addr = 0x%llx, chn_idx = %d\n",
				hisifd->index, node->frame_no, node->shared_fd, node->ion_handle, node->has_map_iommu,
				node->timeline, node->mmbuf.addr, node->mmbuf.size, node->vir_addr, node->chn_idx);
		}

		node->timeline = 0;
		if (node->ion_handle) {
			if (node->has_map_iommu) {
				ion_unmap_iommu(hisifd->ion_client, node->ion_handle);
			}
			ion_free(hisifd->ion_client, node->ion_handle);
		}
		kfree(node);
	}
}

void hisifb_layerbuf_lock_exception(struct hisi_fb_data_type *hisifd,
	struct list_head *plock_list)
{
	unsigned long flags = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == plock_list) {
		HISI_FB_ERR("plock_list is NULL");
		return;
	}

	spin_lock_irqsave(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);
	hisifd->buf_sync_ctrl.layerbuf_flushed = false;
	spin_unlock_irqrestore(&(hisifd->buf_sync_ctrl.layerbuf_spinlock), flags);

	hisifb_layerbuf_unlock(hisifd, plock_list);
}

static void hisifb_layerbuf_unlock_work(struct work_struct *work)
{
	struct hisifb_buf_sync *pbuf_sync = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	unsigned long flags;
	struct hisifb_layerbuf *node, *_node_;
	struct list_head free_list;

	pbuf_sync = container_of(work, struct hisifb_buf_sync, free_layerbuf_work);
	if (NULL == pbuf_sync) {
		HISI_FB_ERR("pbuf_sync is NULL");
		return;
	}
	hisifd = container_of(pbuf_sync, struct hisi_fb_data_type, buf_sync_ctrl);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}
	if (NULL == hisifd->ion_client) {
		HISI_FB_ERR("hisifd->ion_client is NULL");
		return;
	}

	INIT_LIST_HEAD(&free_list);
	down(&pbuf_sync->layerbuf_sem);
	spin_lock_irqsave(&pbuf_sync->layerbuf_spinlock, flags);
	list_for_each_entry_safe(node, _node_, &pbuf_sync->layerbuf_list, list_node) {
		if (node->timeline >= 2) {
			list_del(&node->list_node);
			list_add_tail(&node->list_node, &free_list);
		}
	}
	spin_unlock_irqrestore(&pbuf_sync->layerbuf_spinlock, flags);
	up(&pbuf_sync->layerbuf_sem);
	hisifb_layerbuf_unlock(hisifd, &free_list);
}


////////////////////////////////////////////////////////////////////////////////
//
// buf sync fence
//
#define BUF_SYNC_TIMEOUT_MSEC	(10 * MSEC_PER_SEC)
#define BUF_SYNC_RELEASE_FENCE_NAME	"hisifb%d-release-fence"
#define BUF_SYNC_RETIRE_FENCE_NAME	"hisifb%d-retire-fence"
#define BUF_SYNC_TIMELINE_NAME	"hisifb%d-timeline"

int hisifb_buf_sync_create_fence(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	char fence_name[256] = {0};
	int release_fence_fd = -1;
	struct sync_fence *release_fence = NULL;
	struct sync_pt *release_fence_pt = NULL;
	unsigned int release_fence_val = 0;

	int retire_fence_fd = -1;
	struct sync_fence *retire_fence = NULL;
	struct sync_pt *retire_fence_pt = NULL;
	unsigned int retire_fence_val = 0;

	if (!hisifd) {
		HISI_FB_ERR("hisifd is NULL!\n");
		return -EINVAL;
	}

	if (!pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return -EINVAL;
	}
	pov_req->release_fence = -1;
	pov_req->retire_fence = -1;

	/* create release fence */
	release_fence_fd = get_unused_fd_flags(0);
	if (release_fence_fd < 0) {
		HISI_FB_ERR("failed to get_unused_fd for release_fence_fd!\n");
		return release_fence_fd;
	}

	release_fence_val = hisifd->buf_sync_ctrl.timeline_max + hisifd->buf_sync_ctrl.threshold + 1;
	release_fence_pt = sw_sync_pt_create(hisifd->buf_sync_ctrl.timeline, release_fence_val);
	if (release_fence_pt == NULL) {
		HISI_FB_ERR("failed to create release_fence_pt!\n");
		put_unused_fd((unsigned int)release_fence_fd);
		return -ENOMEM;
	}

	snprintf(fence_name, sizeof(fence_name), BUF_SYNC_RELEASE_FENCE_NAME, hisifd->index);
	release_fence = sync_fence_create(fence_name, release_fence_pt);
	if (release_fence == NULL) {
		HISI_FB_ERR("failed to create release_fence!\n");
		sync_pt_free(release_fence_pt);
		put_unused_fd((unsigned int)release_fence_fd);
		return -ENOMEM;
	}
	sync_fence_install(release_fence, release_fence_fd);
	pov_req->release_fence = release_fence_fd;

	/* create retire fence */
	retire_fence_fd = get_unused_fd_flags(0);
	if (retire_fence_fd < 0) {
		HISI_FB_ERR("failed to get_unused_fd for retire_fence_fd!\n");
		return retire_fence_fd;
	}

	retire_fence_val = release_fence_val + hisifd->buf_sync_ctrl.retire_threshold;
	retire_fence_pt = sw_sync_pt_create(hisifd->buf_sync_ctrl.timeline, retire_fence_val);
	if (retire_fence_pt == NULL) {
		HISI_FB_INFO("failed to create retire_fence_pt!\n");
		put_unused_fd((unsigned int)retire_fence_fd);
		return -ENOMEM;
	}

	snprintf(fence_name, sizeof(fence_name), BUF_SYNC_RETIRE_FENCE_NAME, hisifd->index);
	retire_fence = sync_fence_create(fence_name, retire_fence_pt);
	if (retire_fence == NULL) {
		HISI_FB_ERR("failed to create retire_fence!\n");
		sync_pt_free(retire_fence_pt);
		put_unused_fd((unsigned int)retire_fence_fd);
		return -ENOMEM;
	}
	sync_fence_install(retire_fence, retire_fence_fd);
	pov_req->retire_fence = retire_fence_fd;

	if (g_debug_fence_timeline) {
		HISI_FB_INFO("hisifb%d frame_no(%d) timeline_max(%d), release(%d),"
			"retire(%d), timeline(%d)!\n",
			hisifd->index, hisifd->ov_req.frame_no,
			hisifd->buf_sync_ctrl.timeline_max, release_fence_val,
			retire_fence_val, hisifd->buf_sync_ctrl.timeline->value);
	}

	return 0;
}

void hisifb_buf_sync_close_fence(dss_overlay_t *pov_req)
{
	if (!pov_req) {
		HISI_FB_ERR("pov_req is NULL!\n");
		return;
	}

	if (pov_req->release_fence >= 0) {
		sys_close(pov_req->release_fence);
		pov_req->release_fence = -1;
	}

	if (pov_req->retire_fence >= 0) {
		sys_close(pov_req->retire_fence);
		pov_req->retire_fence = -1;
	}
}

int hisifb_buf_sync_wait(int fence_fd)
{
	int ret = 0;
	struct sync_fence *fence = NULL;

	fence = sync_fence_fdget(fence_fd);
	if (fence == NULL){
		HISI_FB_ERR("fence_fd=%d, sync_fence_fdget failed!\n", fence_fd);
		return -EINVAL;
	}

	ret = sync_fence_wait(fence, BUF_SYNC_TIMEOUT_MSEC);
	if (ret < 0) {
		HISI_FB_ERR("Waiting on fence failed, fence_fd: %d, ret: %d.\n", fence_fd, ret);
	}
	sync_fence_put(fence);

	return ret;
}

int hisifb_buf_sync_handle_offline(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	dss_wb_layer_t *wb_layer = NULL;
	int i = 0;
	int m = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	if (pov_req->wb_enable) {
		for (i = 0; i < pov_req->wb_layer_nums; i++) {
			wb_layer = &(pov_req->wb_layer_infos[i]);

			if (wb_layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(wb_layer->acquire_fence);
			}
		}
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(layer->acquire_fence);
			}
		}
	}

	return 0;
}

int hisifb_buf_sync_handle(struct hisi_fb_data_type *hisifd, dss_overlay_t *pov_req)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	dss_overlay_block_t *pov_h_block = NULL;
	dss_layer_t *layer = NULL;
	int i = 0;
	int m = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	if (NULL == pov_req) {
		HISI_FB_ERR("pov_req is NULL");
		return -EINVAL;
	}

	pov_h_block_infos = (dss_overlay_block_t *)(pov_req->ov_block_infos_ptr);
	for (m = 0; m < pov_req->ov_block_nums; m++) {
		pov_h_block = &(pov_h_block_infos[m]);

		for (i = 0; i < pov_h_block->layer_nums; i++) {
			layer = &(pov_h_block->layer_infos[i]);

			if (layer->dst_rect.y < pov_h_block->ov_block_rect.y)
				continue;

			if (layer->acquire_fence >= 0) {
				hisifb_buf_sync_wait(layer->acquire_fence);
			}
		}
	}

	return 0;
}

void hisifb_buf_sync_signal(struct hisi_fb_data_type *hisifd)
{
	struct hisifb_layerbuf *node = NULL;
	struct hisifb_layerbuf *_node_ = NULL;
	int val = 0;

	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return;
	}

	spin_lock(&hisifd->buf_sync_ctrl.refresh_lock);
	if (hisifd->buf_sync_ctrl.refresh) {
		val = hisifd->buf_sync_ctrl.refresh;
		sw_sync_timeline_inc(hisifd->buf_sync_ctrl.timeline, val);
		hisifd->buf_sync_ctrl.timeline_max += val;
		hisifd->buf_sync_ctrl.refresh = 0;
	}
	spin_unlock(&hisifd->buf_sync_ctrl.refresh_lock);

	spin_lock(&(hisifd->buf_sync_ctrl.layerbuf_spinlock));
	list_for_each_entry_safe(node, _node_, &(hisifd->buf_sync_ctrl.layerbuf_list), list_node) {
		if (hisifd->buf_sync_ctrl.layerbuf_flushed) {
			node->timeline++;
		}
	}
	hisifd->buf_sync_ctrl.layerbuf_flushed = false;
	spin_unlock(&(hisifd->buf_sync_ctrl.layerbuf_spinlock));

	queue_work(hisifd->buf_sync_ctrl.free_layerbuf_queue, &(hisifd->buf_sync_ctrl.free_layerbuf_work));

	if (g_debug_fence_timeline) {
		HISI_FB_INFO("hisifb%d frame_no(%d) timeline_max(%d), timeline(%d)!\n",
			hisifd->index, hisifd->ov_req.frame_no, hisifd->buf_sync_ctrl.timeline_max,
			hisifd->buf_sync_ctrl.timeline->value);
	}
}

void hisifb_buf_sync_suspend(struct hisi_fb_data_type *hisifd)
{
	unsigned long flags;
	int val = 0;

	spin_lock_irqsave(&hisifd->buf_sync_ctrl.refresh_lock,flags);
	val = hisifd->buf_sync_ctrl.refresh + 1;
	sw_sync_timeline_inc(hisifd->buf_sync_ctrl.timeline, val);
	hisifd->buf_sync_ctrl.timeline_max += val;
	hisifd->buf_sync_ctrl.refresh = 0;
	spin_unlock_irqrestore(&hisifd->buf_sync_ctrl.refresh_lock,flags);
}

void hisifb_buf_sync_register(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char tmp_name[256] = {0};

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	hisifd->buf_sync_ctrl.fence_name = "dss-fence";

	hisifd->buf_sync_ctrl.threshold = 0;
	hisifd->buf_sync_ctrl.retire_threshold = 0;

	hisifd->buf_sync_ctrl.refresh = 0;
	hisifd->buf_sync_ctrl.timeline_max = 1;
	spin_lock_init(&hisifd->buf_sync_ctrl.refresh_lock);

	snprintf(tmp_name, sizeof(tmp_name), BUF_SYNC_TIMELINE_NAME, hisifd->index);
	hisifd->buf_sync_ctrl.timeline = sw_sync_timeline_create(tmp_name);
	if (hisifd->buf_sync_ctrl.timeline == NULL) {
		dev_err(&pdev->dev, "cannot create time line!");
		return ; /* -ENOMEM */
	}

	// handle free layerbuf
	spin_lock_init(&(hisifd->buf_sync_ctrl.layerbuf_spinlock));
	INIT_LIST_HEAD(&(hisifd->buf_sync_ctrl.layerbuf_list));
	hisifd->buf_sync_ctrl.layerbuf_flushed = false;
	sema_init(&(hisifd->buf_sync_ctrl.layerbuf_sem), 1);

	snprintf(tmp_name, sizeof(tmp_name), HISI_DSS_LAYERBUF_FREE, hisifd->index);
	INIT_WORK(&(hisifd->buf_sync_ctrl.free_layerbuf_work), hisifb_layerbuf_unlock_work);
	hisifd->buf_sync_ctrl.free_layerbuf_queue = create_singlethread_workqueue(tmp_name);
	if (!hisifd->buf_sync_ctrl.free_layerbuf_queue) {
		dev_err(&pdev->dev, "failed to create free_layerbuf_queue!\n");
		return ;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}

void hisifb_buf_sync_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		dev_err(&pdev->dev, "hisifd is NULL");
		return;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->buf_sync_ctrl.timeline) {
		sync_timeline_destroy((struct sync_timeline *)hisifd->buf_sync_ctrl.timeline);
		hisifd->buf_sync_ctrl.timeline = NULL;
	}

	if (hisifd->buf_sync_ctrl.free_layerbuf_queue) {
		destroy_workqueue(hisifd->buf_sync_ctrl.free_layerbuf_queue);
		hisifd->buf_sync_ctrl.free_layerbuf_queue = NULL;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
}
