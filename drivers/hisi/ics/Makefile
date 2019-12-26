ifeq ($(es_low_freq),true)
EXTRA_CFLAGS += -DCONFIG_ES_VDEC_LOW_FREQ
endif

ifneq ($(choose_variant),user)
EXTRA_CFLAGS += -DNPU_DEBUGFS_TEST_ENABLE
endif

obj-$(CONFIG_HISI_ICS_IPU) += cambricon_ipu.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_smmu_drv.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_clock.o
obj-$(CONFIG_HISI_ICS_IPU) += ipu_mntn.o
