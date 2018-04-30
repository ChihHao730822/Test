/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <mmc.h>
#include <av_func.h>



static int curr_device = -1;
#ifndef CONFIG_GENERIC_MMC
int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int dev;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (strcmp(argv[1], "init") == 0) {
		if (argc == 2) {
			if (curr_device < 0)
				dev = 1;
			else
				dev = curr_device;
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);
		} else {
			return CMD_RET_USAGE;
		}

		if (mmc_legacy_init(dev) != 0) {
			puts("No MMC card found\n");
			return 1;
		}

		curr_device = dev;
		printf("mmc%d is available\n", curr_device);
	} else if (strcmp(argv[1], "device") == 0) {
		if (argc == 2) {
			if (curr_device < 0) {
				puts("No MMC device available\n");
				return 1;
			}
		} else if (argc == 3) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);

#ifdef CONFIG_SYS_MMC_SET_DEV
			if (mmc_set_dev(dev) != 0)
				return 1;
#endif
			curr_device = dev;
		} else {
			return CMD_RET_USAGE;
		}

		printf("mmc%d is current device\n", curr_device);
	} else {
		return CMD_RET_USAGE;
	}

	return 0;
}

U_BOOT_CMD(
	mmc, 3, 1, do_mmc,
	"MMC sub-system",
	"init [dev] - init MMC sub system\n"
	"mmc device [dev] - show or set current device"
);
#else /* !CONFIG_GENERIC_MMC */

enum mmc_state {
	MMC_INVALID,
	MMC_READ,
	MMC_WRITE,
	MMC_ERASE,
};
static void print_mmcinfo(struct mmc *mmc)
{
	printf("Device: %s\n", mmc->name);
	printf("Manufacturer ID: %x\n", mmc->cid[0] >> 24);
	printf("OEM: %x\n", (mmc->cid[0] >> 8) & 0xffff);
	printf("Name: %c%c%c%c%c \n", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);

	printf("Tran Speed: %d\n", mmc->tran_speed);
	printf("Rd Block Len: %d\n", mmc->read_bl_len);

	printf("%s version %d.%d\n", IS_SD(mmc) ? "SD" : "MMC",
			(mmc->version >> 4) & 0xf, mmc->version & 0xf);

	printf("High Capacity: %s\n", mmc->high_capacity ? "Yes" : "No");
	puts("Capacity: ");
	print_size(mmc->capacity, "\n");

	printf("Bus Width: %d-bit %s\n", mmc->bus_width,
				mmc->ddr ? "DDR" : "SDR");
}

int do_mmcinfo (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mmc *mmc;
	int dev_num, err;

	if (argc == 2)
		dev_num = simple_strtoul(argv[1], NULL, 10);
	else if (curr_device < 0) {
		if (get_mmc_num() > 0) {
			curr_device = 0;
			dev_num = curr_device;
		}
		else {
			puts("No MMC device available\n");
			return 1;
		}
	}

	mmc = find_mmc_device(dev_num);

	if (mmc) {
		mmc->has_init = 0;

		err = mmc_init(mmc);
		if (err) {
			printf("no mmc device at slot %x\n", dev_num);
			return err;
		}

		print_mmcinfo(mmc);
		return 0;
	} else {
		printf("no mmc device at slot %x\n", dev_num);
		return 1;
	}
}

U_BOOT_CMD(
	mmcinfo, 2, 0, do_mmcinfo,
	"display MMC info",
	"mmcinfo [dev_num]\n"
	"    - device number of the device to dislay info of\n"
	""
);

int do_mmcops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum mmc_state state;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (curr_device < 0) {
		if (get_mmc_num() > 0)
			curr_device = 0;
		else {
			puts("No MMC device available\n");
			return 1;
		}
	}

	if (strcmp(argv[1], "rescan") == 0) {
		struct mmc *mmc = find_mmc_device(curr_device);

		if (!mmc) {
			printf("no mmc device at slot %x\n", curr_device);
			return 1;
		}

		mmc->has_init = 0;

		if (mmc_init(mmc))
			return 1;
		else
			return 0;
	} else if (strncmp(argv[1], "part", 4) == 0) {
		block_dev_desc_t *mmc_dev;
		struct mmc *mmc = find_mmc_device(curr_device);

		if (!mmc) {
			printf("no mmc device at slot %x\n", curr_device);
			return 1;
		}
		mmc_init(mmc);
		mmc_dev = mmc_get_dev(curr_device);
		if (mmc_dev != NULL &&
				mmc_dev->type != DEV_TYPE_UNKNOWN) {
			print_part(mmc_dev);
			return 0;
		}

		puts("get mmc type error!\n");
		return 1;
	} else if (strcmp(argv[1], "list") == 0) {
		print_mmc_devices('\n');
		return 0;
	} else if (strcmp(argv[1], "dev") == 0) {
		int dev, part = -1;
		struct mmc *mmc;

		if (argc == 2)
			dev = curr_device;
		else if (argc == 3)
			dev = simple_strtoul(argv[2], NULL, 10);
		else if (argc == 4) {
			dev = (int)simple_strtoul(argv[2], NULL, 10);
			part = (int)simple_strtoul(argv[3], NULL, 10);
			if (part > PART_ACCESS_MASK) {
				printf("#part_num shouldn't be larger"
					" than %d\n", PART_ACCESS_MASK);
				return 1;
			}
		} else
			return CMD_RET_USAGE;

		mmc = find_mmc_device(dev);
		if (!mmc) {
			printf("no mmc device at slot %x\n", dev);
			return 1;
		}

		mmc_init(mmc);
		if (part != -1) {
			int ret;
			if (mmc->part_config == MMCPART_NOAVAILABLE) {
				printf("Card doesn't support part_switch\n");
				return 1;
			}

			if (part != mmc->part_num) {
				ret = mmc_switch_part(dev, part);
				if (!ret)
					mmc->part_num = part;

				printf("switch to partions #%d, %s\n",
						part, (!ret) ? "OK" : "ERROR");
			}
		}
		curr_device = dev;
		if (mmc->part_config == MMCPART_NOAVAILABLE)
			printf("mmc%d is current device\n", curr_device);
		else
			printf("mmc%d(part %d) is current device\n",
				curr_device, mmc->part_num);

		return 0;
	}

	if (strcmp(argv[1], "read") == 0)
		state = MMC_READ;
	else if (strcmp(argv[1], "write") == 0)
		state = MMC_WRITE;
	else if (strcmp(argv[1], "erase") == 0)
		state = MMC_ERASE;
	else
		state = MMC_INVALID;

	if (state != MMC_INVALID) {
		int idx = 3;
		if (state == MMC_ERASE)
			idx = 4;
		int dev = simple_strtoul(argv[idx - 1], NULL, 10);
		struct mmc *mmc = find_mmc_device(dev);
		u32 blk, cnt, count, n;
		void *addr;
		int part, err;

		curr_device = dev;

		if (state != MMC_ERASE) {
			addr = (void *)simple_strtoul(argv[idx], NULL, 16);
			++idx;
		} else
			addr = 0;
		blk = simple_strtoul(argv[idx], NULL, 16);
		cnt = simple_strtoul(argv[idx + 1], NULL, 16);

		if (!mmc) {
			printf("no mmc device at slot %x\n", curr_device);
			return 1;
		}

		printf("\nMMC %s: dev # %d, block # %d, count %d ... ",
				argv[1], curr_device, blk, cnt);

		mmc_init(mmc);

		switch (state) {
		case MMC_READ:
			n = mmc->block_dev.block_read(curr_device, blk,
						      cnt, addr);
			/* flush cache after read */
			flush_cache((ulong)addr, cnt * 512); /* FIXME */
			break;
		case MMC_WRITE:
			n = mmc->block_dev.block_write(curr_device, blk,
						      cnt, addr);
			break;
		case MMC_ERASE:

			/* Select erase partition */
			if (strcmp(argv[2], "boot") == 0) {
				part = 0;
				/* Read Boot partition size. */
				count = mmc->boot_size_multi / mmc->read_bl_len;
			} else if (strcmp(argv[2], "user") == 0) {
				part = 1;
				/* Read User partition size. */
				count = mmc->capacity / mmc->read_bl_len;
			} else {
				part = 1;
				count = mmc->capacity / mmc->read_bl_len;
				printf("Default erase user partition\n");
			}

			/* If input counter is larger than max counter */
			if ((blk + cnt) > count) {
				cnt = (count - blk) - 1;
				printf("Block count is Too BIG!!\n");
			}

			/* If input counter is 0 */
			if (!cnt ) {
				cnt = (count - blk) - 1;
				printf("Erase all from %d block\n", blk);
			}

			if (part == 0) {
				err = emmc_boot_open(mmc);
				if (err)
					printf("eMMC OPEN Failed.!!\n");
			}

			n = mmc->block_dev.block_erase(curr_device, blk, cnt);

			if (part == 0) {
				err = emmc_boot_close(mmc);
				if (err)
					printf("eMMC CLOSE Failed.!!\n");
			}
			break;
		default:
			BUG();
		}

		printf("%d blocks %s: %s\n",
				n, argv[1], (n == cnt) ? "OK" : "ERROR");
		return (n == cnt) ? 0 : 1;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	mmc, 6, 1, do_mmcops,
	"MMC sub system",
	"mmc read [dev] addr blk# cnt\n"
	"mmc write [dev] addr blk# cnt\n"
	"mmc erase [boot | user] [dev] blk# cnt\n"
	"mmc rescan\n"
	"mmc part - lists available partition on current mmc device\n"
	"mmc dev [dev] [part] - show or set current mmc device [partition]\n"
	"mmc list - lists available devices");

int do_emmc(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rc = 0;
	u32 dev;

	switch (argc) {
	case 5:
		if (strcmp(argv[1], "partition") == 0) {
			dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);
			u32 bootsize = simple_strtoul(argv[3], NULL, 10);
			u32 rpmbsize = simple_strtoul(argv[4], NULL, 10);

			if (!mmc)
				rc = 1;

			rc = emmc_boot_partition_size_change(mmc, bootsize, rpmbsize);
			if (rc == 0) {
				printf("eMMC boot partition Size is %d MB.!!\n", bootsize);
				printf("eMMC RPMB partition Size is %d MB.!!\n", rpmbsize);
			} else {
				printf("eMMC boot partition Size change Failed.!!\n");
			}
		} else {
			printf("Usage:\n%s\n", cmdtp->usage);
			rc =1;
		}
		break;

	case 3:
		if (strcmp(argv[1], "open") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				rc = 1;

			rc = emmc_boot_open(mmc);

			if (rc == 0) {
			printf("eMMC OPEN Success.!!\n");
			printf("\t\t\t!!!Notice!!!\n");
			printf("!You must close eMMC boot Partition after all image writing!\n");
			printf("!eMMC boot partition has continuity at image writing time.!\n");
			printf("!So, Do not close boot partition, Before, all images is written.!\n");
			} else {
				printf("eMMC OPEN Failed.!!\n");
			}
		} else if (strcmp(argv[1], "close") == 0) {
			int dev = simple_strtoul(argv[2], NULL, 10);
			struct mmc *mmc = find_mmc_device(dev);

			if (!mmc)
				rc = 1;

			rc = emmc_boot_close(mmc);

			if (rc == 0) {
				printf("eMMC CLOSE Success.!!\n");
			} else {
				printf("eMMC CLOSE Failed.!!\n");
			}
		} else {
			printf("Usage:\n%s\n", cmdtp->usage);
			rc =1;
		}
		break;
	case 0:
	case 1:
	case 2:
	case 4:
	default:
		printf("Usage:\n%s\n", cmdtp->usage);
		rc = 1;
		break;
	}

	return rc;
}


U_BOOT_CMD(
	emmc,	5,	0,	do_emmc,
	"Open/Close eMMC boot Partition",
	"emmc open <device num> \n"
	"emmc close <device num> \n"
	"emmc partition <device num> <boot partiton size MB> <RPMB partition size MB>\n");

int do_av (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mmc *mmc;
	int dev_num, err,start,blkcnt;

	switch (argc) {
	case 7:
		 if (strcmp(argv[1], "vy") == 0) {
			int dev_num = simple_strtoul(argv[2], NULL, 10);
			int start = simple_strtoul(argv[3], NULL, 16);
			int blkcnt = simple_strtoul(argv[4], NULL, 10);
			int arg = simple_strtoul(argv[5], NULL, 16);
			int stop = simple_strtoul(argv[6], NULL, 10);			
			av_read_compare_512k(dev_num,start,blkcnt,arg,stop);
			return 0;		 	
		 	}
		break;
	case 6:
		 if (strcmp(argv[1], "write4k") == 0) {
			int dev_num = simple_strtoul(argv[2], NULL, 10);
			int start = simple_strtoul(argv[3], NULL, 16);
			int blkcnt = simple_strtoul(argv[4], NULL, 10);
			int arg = simple_strtoul(argv[5], NULL, 16);
			write_4k(dev_num,start,blkcnt,arg);
			return 0;
		 	}else if (strcmp(argv[1], "write64k") == 0) {
			int dev_num = simple_strtoul(argv[2], NULL, 10);
			int start = simple_strtoul(argv[3], NULL, 16);
			int blkcnt = simple_strtoul(argv[4], NULL, 10);
			int arg = simple_strtoul(argv[5], NULL, 16);
			write_64k(dev_num,start,blkcnt,arg);
			return 0;
		 	} else if (strcmp(argv[1], "write") == 0) {
			int dev_num = simple_strtoul(argv[2], NULL, 10);
			int start = simple_strtoul(argv[3], NULL, 16);
			int blkcnt = simple_strtoul(argv[4], NULL, 10);
			int arg = simple_strtoul(argv[5], NULL, 16);
			write_512k(dev_num,start,blkcnt,arg);
			return 0;
		 	} else if (strcmp(argv[1], "writem") == 0) {
			  int dev_num = simple_strtoul(argv[2], NULL, 10);
			  int start = simple_strtoul(argv[3], NULL, 16);
			  int blkcnt = simple_strtoul(argv[4], NULL, 10);
			  int arg = simple_strtoul(argv[5], NULL, 16);
			  write_100M(dev_num,start,blkcnt,arg);
			  return 0;// chunk size = 100M
		 	}else if (strcmp(argv[1], "vnd") == 0){
		 	  int dev_num = simple_strtoul(argv[2], NULL, 10);
			  int cmd_index = simple_strtoul(argv[3], NULL, 10);
			  int arg = simple_strtoul(argv[4], NULL, 16);
			  int response_type = simple_strtoul(argv[5], NULL, 10);
			  mmc = find_mmc_device(dev_num);
			  av_send_vendor_cmd(mmc,cmd_index, arg, response_type);
			  
		 	} else if (strcmp(argv[1], "vnd2") == 0){
		 	  int dev_num = simple_strtoul(argv[2], NULL, 10);
			  int cmd_index = simple_strtoul(argv[3], NULL, 10);
			  int arg = simple_strtoul(argv[4], NULL, 16);
			  int response_type = simple_strtoul(argv[5], NULL, 10);
			  mmc = find_mmc_device(dev_num);
			  av_send_vendor_cmd2(mmc,cmd_index, arg, response_type);
		 		}
		break;
	case 5:
		if (strcmp(argv[1], "read") == 0) {
	     int dev_num = simple_strtoul(argv[2], NULL, 10);
		 start = simple_strtoul(argv[3], NULL, 16);
		 blkcnt = simple_strtoul(argv[4], NULL, 10);
		 av_read_sector(dev_num,start,blkcnt);
		 }else if (strcmp(argv[1], "readxk") == 0) {
	     int chunksize = simple_strtoul(argv[2], NULL, 10);
		 start = simple_strtoul(argv[3], NULL, 16);
		 blkcnt = simple_strtoul(argv[4], NULL, 10);
		 av_read(chunksize,start,blkcnt);		 
		 
		 }else if (strcmp(argv[1], "pms") == 0) {
		  int p = simple_strtoul(argv[2], NULL, 10);
		  int m = simple_strtoul(argv[3], NULL, 10);
		  int s = simple_strtoul(argv[4], NULL, 10);
		  av_set_clock_pms(p,m,s);
		  
		 	}
	break;
	case 4:
		if(strcmp(argv[1], "clk") == 0){
			 dev_num = simple_strtoul(argv[2], NULL, 10);
			 int val = simple_strtoul(argv[3], NULL, 10);
			 mmc = find_mmc_device(dev_num);
			 clock_ctrl(mmc,val);			 
		}else if(strcmp(argv[1], "power") == 0){
			dev_num = simple_strtoul(argv[2], NULL, 10);
			int value = simple_strtoul(argv[3], NULL, 10);			 	 
			av_emmc_power_control(dev_num,value);
		}else if(strcmp(argv[1], "worsepattern") ==0){
			int speed = simple_strtoul(argv[2], NULL, 10);
			int size = simple_strtoul(argv[3], NULL, 10);
			Worse_Pattern_Test(speed,size);			  
		}
		break;
	case 3:
		if (strcmp(argv[1], "init") == 0) {	
		dev_num = simple_strtoul(argv[2], NULL, 10);
	    mmc = find_mmc_device(dev_num);
		
		if (av_emmc_init(mmc)){
			printf("init fail");
			}
		}else if (strcmp(argv[1], "info") == 0){
		         dev_num = simple_strtoul(argv[2], NULL, 10);
				 mmc = find_mmc_device(dev_num);
				 print_mmcinfo(mmc);
			}else if (strcmp(argv[1], "ratio") == 0){
				 dev_num = simple_strtoul(argv[2], NULL, 10);
				 exynos5_get_mmc_ratio1(dev_num);
				}else if(strcmp(argv[1], "ext") == 0){
			     dev_num = simple_strtoul(argv[2], NULL, 10);
				 mmc = find_mmc_device(dev_num);
				 ext(mmc);
					}else if(strcmp(argv[1], "hs400") == 0){
			     dev_num = simple_strtoul(argv[2], NULL, 10);
				 mmc = find_mmc_device(dev_num);
				 setHS400(mmc);				 
					}else if(strcmp(argv[1], "full_wr") == 0){
			     		dev_num = simple_strtoul(argv[2], NULL, 10);
				 		mmc = find_mmc_device(dev_num);				 
						 full_card_write_read_512k(mmc);
				 	}else if(strcmp(argv[1], "rand") == 0){
						int msec= simple_strtoul(argv[2], NULL, 10);
						random_gen(msec);						
					}else if(strcmp(argv[1], "fullcard_write_erase") ==0){
						int cmd38_arg= simple_strtoul(argv[2], NULL, 10);
						fullcard_write_erase(cmd38_arg);			  
				   }else if(strcmp(argv[1], "at") ==0){
						int shift = simple_strtoul(argv[2], NULL, 10);
						write_arg_test(shift);			  
		}else if(strcmp(argv[1], "tu") == 0){
			dev_num = 1;
			mmc = find_mmc_device(dev_num);
			int cardsize = simple_strtoul(argv[2], NULL, 10);
			tunnningPatten_wr(mmc,cardsize);
			
			}else if(strcmp(argv[1], "clkselect") == 0){			
			mmc = find_mmc_device(1);
			int clkphase = simple_strtoul(argv[2], NULL, 10);
			clkSelect(mmc,clkphase);
		}
		
		break;
	case 2:	
		if(strcmp(argv[1], "random_w") ==0){				
			  random_write();			  
		}else if(strcmp(argv[1], "fullcard_write") ==0){
				full_card_write();			  
		}else if(strcmp(argv[1], "infinite_wr") ==0){
				infinite_write_read();			  
		}else if(strcmp(argv[1], "tunning") == 0){
			dev_num = 1;
			mmc = find_mmc_device(dev_num);
	    	tuning(mmc);
		}else if(strcmp(argv[1], "endwrite") == 0){					
			card_end_part_write();
		}else if(strcmp(argv[1], "timming") == 0){					
			av_read_compare_512k(1,0,500,0,0);		
		}else if(strcmp(argv[1], "1") ==0){			
			Worse_Pattern_Test_All(1024);			  
		}else if(strcmp(argv[1], "2") == 0){		     
//			 mmc = find_mmc_device(1);
			 one();
		}else if(strcmp(argv[1], "3") == 0){		     
//			 mmc = find_mmc_device(1);
			 erase_all();				 
		}else if(strcmp(argv[1], "4") == 0){			
			mmc = find_mmc_device(1);			
			SDR25(mmc);
		}else if(strcmp(argv[1], "5") == 0){			
			mmc = find_mmc_device(1);			
			DDR50(mmc);
		}else if(strcmp(argv[1], "6") == 0){			
			mmc = find_mmc_device(1);
			setHS200(mmc);		
		}else if(strcmp(argv[1], "7") == 0){			
			mmc = find_mmc_device(1);
			setHS400(mmc);
		}else if(strcmp(argv[1], "8") == 0){			
			mmc = find_mmc_device(1);
			av_rest_n(mmc);
		}else if(strcmp(argv[1], "9") == 0){			
			mmc = find_mmc_device(1);
			infinite_write_read();			
		}
		else if(strcmp(argv[1], "10") == 0){			
			mmc = find_mmc_device(1);			
			infinite_read();
		}else if(strcmp(argv[1], "11") == 0){			
			mmc = find_mmc_device(1);			
	    	tuning(mmc);
		}
		
		break;
	case 1:
	case 0:	
	default:
		cmd_usage(cmdtp);
		break;
	
		}		    
    
	if (curr_device < 0) {
		if (get_mmc_num() > 0) {
			curr_device = 1;
			dev_num = curr_device;
		}
		else {
			puts("No MMC device available\n");
			return 1;
		}
	}

	
   
}

U_BOOT_CMD(
	av, 7, 1, do_av,
	"\n",
	"   \n"
	"   \n"		
	"  1	Worse_Pattern_Test(0x55AA)(0x00FF)\n"	
	"  2	For HS400 Device Data output timing measurement test \n"
	"  3	erase full card \n"	
	"  4	SDR25 \n"
	"  5	DDR25 \n"
	"  6	switch to HS200 \n"
	"  7	switch to HS400 \n"
	"  9    infinite write & read \n"
	" 10    infinite read (1/4 cardsize)\n"
	" 11	Tunning clock for HS200/HS400\n"
	"   \n"
	"	write4k <dev_num> <start_addr> <blkcnt> <pattern(4byte)>\n"
	"	write64k <dev_num> <star  t_addr> <blkcnt> <pattern(4byte)>\n"
	"	vnd <dev_num> <cmd_index> <arg> <response_type>\n"
	"	customer_fail <CMD38 ARG> /** random write/read/erase with random arg/size. **/\n"
	"	fullcard_write_erase <CMD38 ARG> /** write fullard,than erase fullcard with 1000sectors.**/\n"
	"	fullcard_write /** write fullcard. **/\n"
	"	infinite_wr/** infinite write & read fullcard. **/\n"
	"	verify <dev_num><start_addr><size(MB)><argument><(stop(1) or not (0))when compare fail> \n"
	"	worsepattern <speed(52/200/400)>/** Worse Pattern test. **/\n"	
	"	tu <cardsize> tunning pattern write & read test\n"
	"	clkselect<phase number> Select clock phase \n" 
);


////////////////Menu sample//////////////////////////////////

int inputSystem(int base)
{
	char c;
	char num[9];
	int var=0,i=0;
	
	do{
		  c = getc();
		  //printf("c :%d , ",c);
		  if(c == 8 || c == 127)//unicode '8' is backspace,127 is delete
		  {
		  
		  	if(i !=0)
		  	{		  	
				num[i]='0';
				i-=1;				
				printf("\b");
				printf(" ");
				printf("\b");				
		  	}
		  }else{
		  
		  	if(i<8)
		  	{
		  		printf("%c", c);
		  		num[i]=c;
		  		i++;
		  	}else{
		  	
		  		i=8;//Avoid to array out of range
		  	}
		  }
		
		}while(c != 13);//enter's unicode is 0x0d
	
		if (i==1 && c==13)//if input nothing and press enter
			return 999;	
	
	var = simple_strtoul(num, NULL, base);
	
	printf("\n");
	return var;
}


void second_menu_usage(void)  
{  
	printf("\n");
	printf("============================================\r\n");
	printf("[0] return to previous page\r\n");
    printf("[1] second_menu \r\n");    
	printf("============================================\r\n");
    printf("Enter your selection: ");  
}

void sec_menu_shell(void)  
{    
	int var;
	
    while (1)  
    {	 
	 second_menu_usage(); //show menu  
	 var = inputSystem(10);	 
		  
	  switch (var)  
	  {  
        case 1: 
        {  
            printf("sec func 1\n");
            break;  
        }		
		case 0:  
        {  
			printf("sec func 0\n");
            return;  
        } 
	  }   
    }  
 }


void main_menu_usage(void)  
{  
	printf("\n");
	printf(" ============================================\r\n");
	printf("|                                            |\r\n");
	printf(" ============================================\r\n");
	printf("[000] exit menu\r\n");
    printf("[001] Worse_Pattern_Test(0x55AA)(0x00FF00FF)\r\n");  
    printf("[002] For HS400 Device Data output timing measurement test\r\n");  
    printf("[003] Erase full card\r\n");
    printf("[004] tunning pattern write & read test\r\n");
    printf("[005] Write data (512k chunk size)\r\n");
    printf("[006] reserve\r\n");
    printf("[007] reserve\r\n");  
    printf("[008] reserve\r\n");  
    printf("[009] reserve\r\n");
	printf("[099] next page\r\n");
	printf("[119] u-boot reset\r\n");
	printf("============================================\r\n");
    printf("Enter your selection: ");  
}

void menu_shell(void)  
{
	int var = 0;
	char cmd_buf[200];
	int lastcmd = 999;
    while (1)
    {    	
        main_menu_usage(); //show menu
        
		var = inputSystem(10);
		if(var == 999 && lastcmd != 0)
			var = lastcmd;
		//printf("var=%d\n",var);
        switch (var)  
        {  
	case 1: 
	{  
	   Worse_Pattern_Test_All(1024);
	    break;  
	}  
	case 2: 
	{
	    one();
	    break;  
	}  
	case 3: 
	{
		erase_all();
		break;  
	}
	case 4:
		printf("case4\n");
		break;
	case 5:  
	{				
		writeArgInput(write_512k);
		break;  
	}
	case 118:
	{
		strcpy(cmd_buf, "fastboot");
		run_command(cmd_buf, 0);
	}
	case 119:
	{
		strcpy(cmd_buf, "reset");
		run_command(cmd_buf, 0);
	}
	case 99: 
	{  
		sec_menu_shell();
		break;  
	}
	case 0: 
	{  
		printf("Leave menu\n");
		return;  
	}
	default :
		break;
        }
	lastcmd = var;
    }  
 }


int do_menu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])  {  
    menu_shell();  
    return 0;  
}


U_BOOT_CMD(
    menu, 3, 0, do_menu,
    "menu - display a menu, to select the items to do something\n",
    " - display a menu, to select the items to do something"  
);



#endif
