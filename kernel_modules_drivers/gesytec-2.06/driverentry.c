
static int lpp_init_one (struct pci_dev *pdev, const struct pci_device_id *ent)
    {
    DEV *dev;
    int rc;
    uint minor;
    unsigned long plx_base;

    dbg("%s(): pci_dev->vendor=%04X device=%04X\n  pci_device_id->vendor=%04X device=%04X",
        __FUNCTION__, 
        pdev->vendor, pdev->device,
        ent->vendor, ent->device);
    //return -ENODEV;
	
    dev = kmalloc (sizeof(DEV), GFP_KERNEL);
	if (!dev)  return -ENOMEM;
    memset (dev, 0x00, sizeof (*dev));

	
    /*  Enable tasklet for the device */
	tasklet_init(&dev->Dpc, DpcForIsr, (unsigned long) dev);

    /* enable device (incl. PCI PM wakeup), and bus-mastering */
	rc = pci_enable_device(pdev);
    dbg1("pci_enable_device()=%d dev=%p", rc, dev);
    if (rc)  goto err_out_free;

	rc = pci_set_mwi(pdev);
    dbg1("pci_set_mwi()=%d", rc);
	if (rc)  goto err_out_disable;

	rc = pci_request_regions(pdev, DRIVER_MOD_NAME);
    dbg1("pci_request_regions()=%d", rc);
	if (rc)  goto err_out_mwi;

	if (pdev->irq < 2) 
        {
		rc = -EIO;
		printk(KERN_ERR PFX "invalid irq (%d) for pci dev %s\n",
		       pdev->irq, pdev->slot_name);
		goto err_out_res;
        }

	plx_base     = pci_resource_start (pdev, 1);
    dev->PlxIntCsrPort = plx_base + P9050_INTCSR;
    dev->cport   = pci_resource_start (pdev, 2);
    dev->rport   = pci_resource_start (pdev, 3);
    dev->wport   = dev->rport;
    dev->wtcport = pci_resource_start (pdev, 5);
    dev->irq     = pdev->irq;

/*
	pio_end = pci_resource_end (pdev, 0);
	pio_flags = pci_resource_flags (pdev, 0);
	pio_len = pci_resource_len (pdev, 0);
*/

//	rc = register_netdev(dev);
//	if (rc)  goto err_out_iomap;


    /* select a "subminor" number (part of a minor number) */
    down (&minor_table_mutex);
    for (minor = MAX_LPCS; minor < MAX_DEVICES; ++minor) 
        {
        if (minor_table[minor] == NULL)
            break;
        }
    if (minor >= MAX_DEVICES) 
        {
        info ("Too many devices plugged in, can not handle this device.");
        rc = -EINVAL;
        goto err_minor_table;
        }

    dev->minor = minor;
    minor_table[minor] = dev;

    pci_set_drvdata(pdev, dev);
#ifdef CONFIG_DEVFS_FS
	devfs_mk_cdev(MKDEV(major, minor),
			S_IFCHR | S_IRUGO | S_IWUGO,
			"lpc/%d", minor);
#endif
    up (&minor_table_mutex);

    dbg1("%s(): minor=%d return 0",
        __FUNCTION__, 
        minor);
    return 0;


err_minor_table:
    up (&minor_table_mutex);
//err_out_iomap:
//	iounmap(regs);
err_out_res:
	pci_release_regions(pdev);
err_out_mwi:
	pci_clear_mwi(pdev);
err_out_disable:
	pci_disable_device(pdev);
err_out_free:
    tasklet_kill(&dev->Dpc);
    kfree(dev);
    dbg1("%s(): return %d",
        __FUNCTION__, 
        rc);
	return rc;
    }

static void lpp_remove_one (struct pci_dev *pdev)
    {
	DEV *dev = pci_get_drvdata(pdev);
    dbg("%s(): dev=%08X pci_dev->vendor=%04X device=%04X",
        __FUNCTION__, (uint)dev, 
        pdev->vendor, pdev->device);
	if (!dev)  BUG();

    down (&minor_table_mutex);
#ifdef CONFIG_DEVFS_FS
	devfs_remove("lpc/%d", dev->minor);
#endif
    minor_table[dev->minor] = NULL;
//	unregister_netdev(dev);
//	iounmap(cp->regs);
	pci_release_regions(pdev);
	pci_clear_mwi(pdev);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
    tasklet_kill(&dev->Dpc);
    kfree(dev);

    info(DRIVER_DESC " " DRIVER_DEV_NAME "%d now disconnected", dev->minor);
    up (&minor_table_mutex);
    }


#ifdef CONFIG_PM
static int lpp_suspend (struct pci_dev *pdev, u32 state)
    {
	return 0;
    }

static int lpp_resume (struct pci_dev *pdev)
    {
	return 0;
    }
#endif /* CONFIG_PM */


#ifdef MODULE

static int __init module_lpc_init(void) 
    {
    int i, val;
    char c;
    
//    if (!DbgInit())  return -ENOMEM;
    info(DRIVER_DESC " " DRIVER_VERSION);

    int ret = register_chrdev(major, DRIVER_DEV_NAME, &lpc_fops);
    if (ret < 0) 
        {
        err("register_chrdev(major=%d, \"" DRIVER_DEV_NAME "\")=%d -> failed", major, ret);
//        DbgExit();
        return ret;
        }
    if (major == 0)  major = ret;  // dynamic major

#ifdef CONFIG_DEVFS_FS
#warning CONFIG_DEVFS_FS not tested
	ret = devfs_mk_dir("lpc");
    dbg1("major=%d   devfs_mk_dir()=%08X", major, ret);
#else
#warning Info: CONFIG_DEVFS_FS not used
#endif

    ret = pci_module_init(&lpp_driver);
    
    if( isa ) 
        {
        i = 0;
        while( *isa && (i < MAX_LPCS*4) ) 
            {
            c = *isa++;
            val = -1;
            if((c >= '0') && (c <= '9')) val = c - '0';
            if((c >= 'A') && (c <= 'F')) val = c + 10 - 'A';
            if((c >= 'a') && (c <= 'f')) val = c + 10 - 'a';
            if(val >= 0) 
                {
                lpcs[i >> 2] = (lpcs[i >> 2] << 4) | val;
                i++;
                }
            }
        }

    dbg("Loading module " DRIVER_MOD_NAME " " DRIVER_VERSION "  pci_module_init()=%d", ret);
    if (!lpc_init())  return 0;
    if (ret)
        {
        unregister_chrdev( major, DRIVER_DEV_NAME );
#ifdef CONFIG_DEVFS_FS
        devfs_remove("lpc");
#endif
//        DbgExit();
        }
    return ret;
    }

static void __exit module_lpc_exit(void) 
    {
	pci_unregister_driver(&lpp_driver);

#ifdef CHECK_ISA_AT_LOAD_TIME
    int i;
    
    for( i=0; i<MAX_LPCS; i++ )
        {
        if( lpcs[i] ) release_region(lpcs[i] & 0x0FFF, 4);
        }
#endif
    unregister_chrdev( major, DRIVER_DEV_NAME );
#ifdef CONFIG_DEVFS_FS
	devfs_remove("lpc");
#endif
    info("Unloading module " DRIVER_MOD_NAME " "DRIVER_VERSION);
//    DbgExit();
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

module_init(module_lpc_init);
module_exit(module_lpc_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
MODULE_INFO(supported, "external");
#endif

#endif

#endif  // #ifdef MODULE
