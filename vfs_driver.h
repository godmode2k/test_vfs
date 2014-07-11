#ifndef __VFS_DRIVER_H__
#define __VFS_DRIVER_H__
/*---------------------------------------------------------
Project:	test vfs read/write
Purpose:	Single-file VFS read/write (FAT16 Only)
Author:		Ho-Jung Kim (godmode2k@hotmail.com)
Date:		Since February 21, 2013
File:		vfs_driver.h

License:
Last Modified Date: March 7, 2013
-----------------------------------------------------------
Note:
Reference:
	- http://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/
	- http://www.tavi.co.uk/phobos/fat.html
---------------------------------------------------------*/



//! Header and Definition
// -------------------------------------------------------
/*
	The partition type should be 4 for FAT16 partitions that
	are less than 32 MiB (MiB = 1024^2 bytes), 6 for over 32 MiB
	partitions and 14 for FAT16 partitions using LBA addressing.
	The Partition type:
	 4: less than 32 MiB
	 6: over 32 MiB
	 14: using LBA
*/
// Partition Table
typedef struct {
	unsigned char first_byte;
	unsigned char start_chs[3];
	unsigned char partition_type;
	unsigned char end_chs[3];
	unsigned long start_sector;
	unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

// Boot Sector
typedef struct {
	unsigned char jmp[3];
	// BPB {
	char oem[8];
	unsigned short sector_size;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char number_of_fats;
	unsigned short root_dir_entries;
	unsigned short total_sectors_short; // if zero, later field is used
	unsigned char media_descriptor;
	unsigned short fat_size_sectors;
	unsigned short sectors_per_track;
	unsigned short number_of_heads;
	unsigned long hidden_sectors;
	unsigned long total_sectors_long;

	unsigned char drive_number;
	unsigned char current_head;
	unsigned char boot_signature;
	unsigned long volume_id;
	char volume_label[11];
	char fs_type[8];
	// BPB }
	char boot_code[448];
	unsigned short boot_sector_signature;
} __attribute((packed)) Fat16BootSector;

typedef struct {
	unsigned char filename[8];
	unsigned char ext[3];
	unsigned char attributes;
	unsigned char reserved[10];
	unsigned short modify_time;
	unsigned short modify_date;
	unsigned short starting_cluster;
	unsigned long file_size;
} __attribute((packed)) Fat16Entry;

// -------------------------------------------------------



//! Prototype 
// -------------------------------------------------------
void calc_disk_unit(const unsigned long disk_size, char* pStrDiskUnit, const int len);

void read_partition_table(FILE* fp);
void read_boot_sector_fat16(FILE* fp, PartitionTable pt[4], Fat16BootSector* pBS);
void read_files_info_fat16(FILE* fp, const PartitionTable pt[4], const Fat16BootSector bs,
		const char* pStrPathFilename, const bool isDir);
void read_file_data_fat16(FILE* fp, const PartitionTable pt[4], const Fat16BootSector bs,
		const char* pStrFilename, const char* pStrExt);
// -------------------------------------------------------



#endif	// #ifdef__VFS_DRIVER_H__
