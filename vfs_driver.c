/*---------------------------------------------------------
Project:	test vfs read/write
Purpose:	Single-file VFS read/write (FAT16 Only)
Author:		Ho-Jung Kim (godmode2k@hotmail.com)
Date:		Since February 21, 2013
File:		vfs_driver.c

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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vfs_driver.h"
// -------------------------------------------------------



//! Prototype 
// -------------------------------------------------------
void read_partition_table(FILE* fp) {
	{
		printf( "------------------------------\n" );
		printf( "read_partition_table()\n" );
		printf( "------------------------------\n" );
	}

	PartitionTable pt[4];
	int i;

	if ( !fp ) {
		fprintf( stderr, "[-] Open VFS file\n" );
		return;
	}

	fseek( fp, 0x1BE, SEEK_SET ); // go to partition table start
	fread( pt, sizeof(PartitionTable), 4, fp); // read all four entries
								
	for ( i = 0; i < 4; i++ ) {
		printf( "Partition %d, type %02X\n", i, pt[i].partition_type );
		printf( "  Start sector %08lX, %ld sectors long\n", 
				pt[i].start_sector, pt[i].length_sectors );
	}
}

void read_boot_sector_fat16(FILE* fp, PartitionTable pt[4], Fat16BootSector* pBS) {
	{
		printf( "------------------------------\n" );
		printf( "read_boot_sector_fat16()\n" );
		printf( "------------------------------\n" );
	}

	//PartitionTable pt[4];
	Fat16BootSector bs;
	int i;

	memset( (void*)&bs, 0x00, sizeof(Fat16BootSector) );

	if ( !fp ) {
		fprintf( stderr, "[-] Open VFS file\n" );
		return;
	}

	fseek( fp, 0x1BE, SEEK_SET ); // go to partition table start
	fread( pt, sizeof(PartitionTable), 4, fp); // read all four entries
								
	/*
	for ( i = 0; i < 4; i++ ) {
		printf( "Partition %d, type %02X\n", i, pt[i].partition_type );
		printf( "  Start sector %08X, %d sectors long\n", 
				pt[i].start_sector, pt[i].length_sectors );
	}
	*/

	for ( i = 0; i < 4; i++ ) {        
		if ( pt[i].partition_type == 0 ) {
			printf( "[+] FAT16 filesystem found from partition %d, type = 0 (empty/unused)\n", i );
		}

		if ( pt[i].partition_type == 4 || pt[i].partition_type == 6 ||
			pt[i].partition_type == 14 ) {
			printf( "[+] FAT16 filesystem found from partition %d\n", i );
			break;
		}
	}
    
	printf( "Result:\n" );
	if ( i == 4 ) {
		//fprintf( stderr, "[-] No FAT16 filesystem found, exiting...\n" );
		//return;
		fprintf( stderr, "[-] No FAT16 filesystem found, ignore...\n" );

		// One partition
		i = 0;
		fprintf( stderr, "Fixed set a partition number to 0(zero), start sector = %ld, length = %ld\n",
				pt[i].start_sector, pt[i].length_sectors );
	}


	fseek( fp, 512 * pt[i].start_sector, SEEK_SET );
	fread( &bs, sizeof(Fat16BootSector), 1, fp );

	printf( "FAT16 Boot Sector {\n" );
	printf( "  Jump code: %02X:%02X:%02X\n", bs.jmp[0], bs.jmp[1], bs.jmp[2] );
	printf( "  OEM code: [%.8s]\n", bs.oem );
	printf( "  sector_size: %d\n", bs.sector_size );
	printf( "  sectors_per_cluster: %d\n", bs.sectors_per_cluster );
	printf( "  reserved_sectors: %d\n", bs.reserved_sectors );
	printf( "  number_of_fats: %d\n", bs.number_of_fats );
	printf( "  root_dir_entries: %d\n", bs.root_dir_entries );
	printf( "  total_sectors_short: %d\n", bs.total_sectors_short );
	printf( "  media_descriptor: 0x%02X\n", bs.media_descriptor );
	printf( "  fat_size_sectors: %d\n", bs.fat_size_sectors );
	printf( "  sectors_per_track: %d\n", bs.sectors_per_track );
	printf( "  number_of_heads: %d\n", bs.number_of_heads );
	printf( "  hidden_sectors: %ld\n", bs.hidden_sectors );
	printf( "  total_sectors_long: %ld\n", bs.total_sectors_long );
	printf( "  drive_number: 0x%02X\n", bs.drive_number );
	printf( "  current_head: 0x%02X\n", bs.current_head );
	printf( "  boot_signature: 0x%02X\n", bs.boot_signature );
	printf( "  volume_id: 0x%08lX\n", bs.volume_id );
	printf( "  Volume label: [%.11s]\n", bs.volume_label );
	printf( "  Filesystem type: [%.8s]\n", bs.fs_type );
	printf( "  Boot sector signature: 0x%04X\n", bs.boot_sector_signature );
	printf( "}\n" );

	// FAT16 Entry
	{
		unsigned long fat_start, fat_end_1, fat_end_2;
		unsigned long root_start, data_start;

		// Calculate start offsets of FAT, root directory and data
		fat_start = ftell(fp) + (bs.reserved_sectors-1) * bs.sector_size;
		// or
		// fat_start = (bs.reserved_sectors) * bs.sector_size;
		root_start = fat_start + ((bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size);
		data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);

		fat_end_1 = fat_start + ((bs.fat_size_sectors * 1) * bs.sector_size);
		fat_end_2 = fat_end_1 + ((bs.fat_size_sectors * 1) * bs.sector_size);
		fat_end_1 -= 1;
		fat_end_2 -= 1;
		printf( "FAT Entry: FAT start at %08X(#1 %08X, #2 %08X), root dir at %08X, data at %08X\n", 
			fat_start, fat_end_1, fat_end_2, root_start, data_start );
	}

	// return
	memcpy( (void*)pBS, (void*)&bs, sizeof(Fat16BootSector) );
}

void calc_disk_unit(const unsigned long disk_size, char* pStrDiskUnit, const int len) {
	{
		printf( "------------------------------\n" );
		printf( "calc_disk_unit()\n" );
		printf( "------------------------------\n" );
	}

	// pStrDiskUnit[12]; 12 length means:
	// 		9999GB: 0000(4) + two decimal points(2) + Unit(2) + EOL(1) + padding(3)
	if ( !pStrDiskUnit || (len < 11) ) {
		printf( "[-] buffer is NULL or its length is too short (%d < 11)\n", len );
		return;
	}

	memset( (void*)pStrDiskUnit, 0x00, len );

	double res_disk_size = ((disk_size / 1024768) > 0) ?
		(double)(disk_size / 1024768.f) : ((disk_size / 1024) > 0) ?
		(double)(disk_size / 1024.f) : disk_size;
	snprintf( pStrDiskUnit, len, "%.2f %s", res_disk_size,
		((disk_size / 1024768) > 0) ? "MB" : ((disk_size / 1024) > 0) ? "KB" : "Bytes" );
}

void read_files_info_fat16(FILE* fp, const PartitionTable pt[4], const Fat16BootSector bs,
		const char* pStrPathFilename, const bool isDir) {
	{
		printf( "------------------------------\n" );
		printf( "read_files_info_fat16()\n" );
		printf( "------------------------------\n" );
	}

	unsigned long abs_file_position = 0;
	unsigned long abs_pos_root_dir_entry = 0;
	unsigned long abs_pos_data_entry = 0;
	int i;
	Fat16Entry fat16Entry;
	char strBufResult[255] = { 0 };
	unsigned long total_disk_size = 0;
	double allocated_disk_size = 0;
	double available_disk_size = 0;
	char allocated_disk_unit[12] = { 0, };	// 9999GB: 0000(4) + two decimal points(2) + Unit(2) + EOL(1) + padding(3)
	char available_disk_unit[12] = { 0, };

	if ( !fp ) {
		fprintf( stderr, "[-] Open VFS file\n" );
		return;
	}

	printf( "Now at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n", 
		ftell(fp), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats );


	// Disk Size and free disk space
	{
		if ( bs.total_sectors_short == 0 ) {
			total_disk_size = bs.total_sectors_long * bs.sector_size;
			printf( "Total size: %ld Bytes (%ld MB)\n", bs.total_sectors_long * bs.sector_size,
					(total_disk_size / 1024768) );
		}
		else {
			total_disk_size = bs.total_sectors_short * bs.sector_size;
			printf( "Total size: %d Bytes (%ld MB)\n", bs.total_sectors_short * bs.sector_size,
				 (total_disk_size / 1024768) );
		}

		// FAT Entry
		{
			unsigned long fat_start, fat_end_1, fat_end_2;
			unsigned long root_start, data_start;

			// Calculate start offsets of FAT, root directory and data
			fat_start = ftell(fp) + (bs.reserved_sectors-1) * bs.sector_size;
			// or
			// fat_start = (bs.reserved_sectors) * bs.sector_size;
			root_start = fat_start + ((bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size);
			data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);

			fat_end_1 = fat_start + ((bs.fat_size_sectors * 1) * bs.sector_size);
			fat_end_2 = fat_end_1 + ((bs.fat_size_sectors * 1) * bs.sector_size);
			fat_end_1 -= 1;
			fat_end_2 -= 1;
			//printf( "FAT Entry: FAT start at 0x%08X(#1 0x%08X, #2 0x%08X), root dir at 0x%08X, data at 0x%08X\n", 
			//	fat_start, fat_end_1, fat_end_2, root_start, data_start );

			// USE FAT16 #1 Entry
			{
				unsigned long fat_entry1_start, free_space, clusters, clusters_size;
				unsigned long get_entry;
				long fpPos = 0;
				bool found_reserved = false;

				fat_entry1_start = fat_start;
				clusters = 0;

				fseek( fp, fat_start, SEEK_SET );

				while ( fat_entry1_start != fat_end_1 ) {
					get_entry = 0;
					fpPos = ftell(fp);

					//printf( "Now at 0x%X\n", ftell(fp) );

					if ( found_reserved ) {
						fread( &get_entry, 2, 1, fp );
						fat_entry1_start += 2;
					}
					else {
						fread( &get_entry, 4, 1, fp );
						fat_entry1_start += 4;
					}

					//printf( "ENTRY = 0x%08X\n", get_entry );

					if ( get_entry == 0xFFFFFFF8 ) {
						//printf( "Reserved: 0x%8X\n", get_entry );
						found_reserved = true;
						continue;
					}

					if ( get_entry == 0x0000 ) {
						//printf( "End-of-Entry (0x%X)\n", fpPos );
						break;
					}

					clusters += 1;
				} // while()


				clusters_size = (clusters * (bs.sector_size * bs.sectors_per_cluster));
				//printf( "Now FAT #1 entry start = 0x%08X, FAT #1 entry end = 0x%08X, clusters = %ld\n",
				//		fat_entry1_start, fat_end_1, clusters );

				memset( (void*)&allocated_disk_unit, 0x00, sizeof(allocated_disk_unit) );
				memset( (void*)&available_disk_unit, 0x00, sizeof(available_disk_unit) );

				calc_disk_unit( clusters_size, &allocated_disk_unit, sizeof(allocated_disk_unit) );
				calc_disk_unit( (total_disk_size - clusters_size), &available_disk_unit, sizeof(available_disk_unit) );
				printf( "Allocated: %ld Bytes (%s)\n", clusters_size, allocated_disk_unit );
				printf( "Available: %ld Bytes (%s)\n", (total_disk_size - clusters_size), available_disk_unit );


				/*
				allocated_disk_size = ((clusters_size / 1024768) > 0) ?
					(double)(clusters_size / 1024768.f) : ((clusters_size / 1024) > 0) ?
					(double)(clusters_size / 1024.f) : clusters_size;
				available_disk_size = (((total_disk_size - clusters_size) / 1024768) > 0) ?
					(double)((total_disk_size - clusters_size) / 1024768.f) : (((total_disk_size - clusters_size) / 1024) > 0) ?
					(double)((total_disk_size - clusters_size) / 1024.f) : (total_disk_size - clusters_size);
				snprintf( allocated_disk_unit, sizeof(allocated_disk_unit), "%.2f %s", allocated_disk_size,
					((clusters_size / 1024768) > 0) ? "MB" : ((clusters_size / 1024) > 0) ? "KB" : "Bytes" );
				snprintf( available_disk_unit, sizeof(available_disk_unit), "%.2f %s", available_disk_size,
					(((total_disk_size - clusters_size) / 1024768) > 0) ?
						"MB" : (((total_disk_size - clusters_size) / 1024) > 0) ? "KB" : "Bytes" );
				printf( "Allocated: %ld Bytes (%s)\n", clusters_size, allocated_disk_unit );
				printf( "Available: %ld Bytes (%s)\n", (total_disk_size - clusters_size), available_disk_unit );
				*/


				/*
				// Bitwise operation
				// -----------------------------
				// 	Unit	|	r-shift (>>)
				// -----------------------------
				//	Byte	|	0
				//	KB		|	10 (Byte >> 10)	// 2^10 = 1,024
				//	MB		|	20 (KB >> 10)	// 2^20 = 1,048,576
				//	GB		|	30 (MB >> 10)	// 2^30 = 1,073,741,824
				//	...		|	...
				// -----------------------------
				memset( (void*)&allocated_disk_unit, 0x00, sizeof(allocated_disk_unit) );
				memset( (void*)&available_disk_unit, 0x00, sizeof(available_disk_unit) );


				allocated_disk_size = ((clusters_size >> 20) > 0) ?
					(double)(clusters_size >> 20) : ((clusters_size >> 10) > 0) ?
					(double)((clusters_size >> 10)) : clusters_size;
				available_disk_size = (((total_disk_size - clusters_size) >> 20) > 0) ?
					(double)((total_disk_size - clusters_size) >> 20) : (((total_disk_size - clusters_size) >> 10) > 0) ?
					(double)((total_disk_size - clusters_size) >> 10) : (total_disk_size - clusters_size);


				// Calculates remainder
				//allocated_disk_size = ((clusters_size >> 20) > 0) ?
				//	(double)((clusters_size >> 20) + 0.01*(clusters_size & 19)) : ((clusters_size >> 10) > 0) ?
				//		(double)(((clusters_size >> 10)) + 0.01*(clusters_size & 10)) : clusters_size;
				//available_disk_size = (((total_disk_size - clusters_size) >> 20) > 0) ?
				//	(double)(((total_disk_size - clusters_size) >> 20) + 0.01*((total_disk_size - clusters_size) & 19)) :
				//		(((total_disk_size - clusters_size) >> 10) > 0) ?
				//			(double)(((total_disk_size - clusters_size) >> 10) + 0.01*((total_disk_size - clusters_size) & 9) ) :
				//				(total_disk_size - clusters_size);

				snprintf( allocated_disk_unit, sizeof(allocated_disk_unit), "%.2f %s", allocated_disk_size,
					((clusters_size >> 20) > 0) ? "MB" : ((clusters_size >> 10) > 0) ? "KB" : "Bytes" );
				snprintf( available_disk_unit, sizeof(available_disk_unit), "%.2f %s", available_disk_size,
					(((total_disk_size - clusters_size) >> 20) > 0) ?
						"MB" : (((total_disk_size - clusters_size) >> 10) > 0) ? "KB" : "Bytes" );
				printf( "Allocated: %ld Bytes (%s)\n", clusters_size, allocated_disk_unit );
				printf( "Available: %ld Bytes (%s)\n", (total_disk_size - clusters_size), available_disk_unit );
				*/


				//! TEST: KB, MB only
				//printf( "Allocated: %ld Bytes (%ld %s)\n", clusters_size,
				//		((clusters_size / 1024768) > 0) ? (clusters_size / 1024768) : (clusters_size / 1024),
				//		((clusters_size / 1024768) > 0) ? "MB" : "KB" );
				//printf( "Available: %ld Bytes (%ld %s)\n", (total_disk_size - clusters_size),
				//	 (((total_disk_size - clusters_size) / 1024768) > 0) ? ((total_disk_size - clusters_size) / 1024768) : ((total_disk_size - clusters_size) / 1024),
				//	 (((total_disk_size - clusters_size) / 1024768) > 0) ? "MB" : "KB" );
			}
		}
	}
	


	// Rest position
	abs_file_position = ((512 * pt[i].start_sector) + sizeof(Fat16BootSector));
	fseek( fp, abs_file_position + ((bs.reserved_sectors-1 + bs.fat_size_sectors * bs.number_of_fats)
			* bs.sector_size), SEEK_SET );
	printf( "(Root Directory Entry) Now at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n", 
		ftell(fp), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats );

	abs_pos_root_dir_entry = ftell(fp);
	abs_pos_data_entry = (abs_pos_root_dir_entry + (bs.root_dir_entries * sizeof(Fat16Entry)));


	// File list
	printf( "Size | DateTime | Name\n" );
	for ( i = 0; i < bs.root_dir_entries; i++ ) {
		char strBufEntryName[255] = { 0 };
		char strBufDateTime[20] = { 0 };
		const Fat16Entry* pEntry = NULL;
		const unsigned long startFilePos = ftell( fp );

		fread( &fat16Entry, sizeof(Fat16Entry), 1, fp );
		pEntry = &fat16Entry;

		memset( (void*)&strBufResult, 0x00, sizeof(strBufResult) );
		memset( (void*)&strBufEntryName, 0x00, sizeof(strBufEntryName) );
		memset( (void*)&strBufDateTime, 0x00, sizeof(strBufDateTime) );

		switch ( pEntry->filename[0] ) {
			case 0x00:
				// Unused entry
				{
					//fprintf( stderr, "[-] Unused entry; SKIP\n" );
					return;
				}
				break;
			case 0xE5:
				{
					//printf( "Deleted file: [?%.7s.%.3s]\n", pEntry->filename+1, pEntry->ext );
					//return;
					continue;
				}
				break;
			case 0x05:
				// Filename is actually 0xE5
				{
					//printf( "File starting with 0xE5: [%c%.7s.%.3s]\n",
					//	0xE5, pEntry->filename+1, pEntry->ext );

					if ( strlen(pEntry->ext) > 0 ) {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%c%.7s.%.3s",
								0xE5, pEntry->filename+1, pEntry->ext );
					}
					else {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%c%.7s",
								0xE5, pEntry->filename+1 );
					}
				}
				break;
			case 0x2E:
				// Sub-Directory
				{
					printf( "Sub-Directory: [%.8s.%.3s]\n", pEntry->filename, pEntry->ext );
					//printf( "%.8s.%.3s/\n", pEntry->filename, pEntry->ext );

					if ( strlen(pEntry->ext) > 0 ) {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s.%.3s/",
								pEntry->filename, pEntry->ext );
					}
					else {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s/",
								pEntry->filename );
					}

					//! TODO: Reset seek to next sub-directory; Sub-directories
					// ...
					/*
					fseek( in, data_start + cluster_size * (cluster-2), SEEK_SET );
					printf( "-> data_start(%d) + cluster_size(%d) * (cluster(%d)-2) = %d(0x%X)\n",
							data_start, cluster_size, cluster, (data_start + cluster_size * (cluster-2)),
							(data_start + cluster_size * (cluster-2)) );
					printf( "-> Now data at 0x%X\n", ftell(in) );
					*/
				}
				break;
			default:
				// File
				{
					//printf( "File: [%.8s.%.3s]\n", pEntry->filename, pEntry->ext );
					//printf( "%.8s.%.3s\n", pEntry->filename, pEntry->ext );
					if ( strlen(pEntry->ext) > 0 ) {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s.%.3s",
								pEntry->filename, pEntry->ext );
					}
					else {
						snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s",
								pEntry->filename );
					}
				}
				break;
		}

		// Checks File Attributes
		/*
		[FAT16 File Attributes]
		------------------------------------
		value	Description
		------------------------------------
		0x01	Read-Only
		0x02	Hidden file
		0x04	System file
		0x08	Volume label
		----
		0x0F	LFN (Long Filename) | VFAT
		---------------------
		0x10	Sub-directory
		0x20	Archive flag
		----
		0x40	Not used; must be set to 0
		0x80	Not used; must be set to 0
		------------------------------------
		*/
		{
			// Ignore 0x00 and 0x0F
			const unsigned char bufFileAttribute[8] = { 0x01, 0x02, 0x04, 0x08,
								0x10, 0x20, 0x40, 0x80 };
			int i;
			bool retAttribute = false;
			for ( i = 0; i < sizeof(bufFileAttribute); i++ ) {
				if ( pEntry->attributes == bufFileAttribute[i] ) {
					retAttribute = true;

					// Sub-directory
					if ( bufFileAttribute[i] == 0x10 ) {
						if ( strlen(pEntry->ext) > 0 ) {
							snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s.%.3s/",
									pEntry->filename, pEntry->ext );
						}
						else {
							snprintf( strBufEntryName, sizeof(strBufEntryName), "%.8s/",
									pEntry->filename );
						}

						//! TODO: Reset seek; First Sub-directory
						// ...
						/*
						fseek( in, data_start + cluster_size * (cluster-2), SEEK_SET );
						printf( "-> data_start(%d) + cluster_size(%d) * (cluster(%d)-2) = %d(0x%X)\n",
								data_start, cluster_size, cluster, (data_start + cluster_size * (cluster-2)),
								(data_start + cluster_size * (cluster-2)) );
						printf( "-> Now data at 0x%X\n", ftell(in) );
						*/

					}

					break;
				}
			}

			if ( !retAttribute ) {
				//fprintf( stderr, "[-] File attributes not correct (0x%01X)\n",
				//		pEntry->attributes );
				continue;
			}
		}

		/*
		printf( "  Modified: %04d-%02d-%02d %02d:%02d.%02d | Start: [%04X] | Size: %ld\n", 
				1980 + (pEntry->modify_date >> 9),
				(pEntry->modify_date >> 5) & 0xF,
				pEntry->modify_date & 0x1F,
				(pEntry->modify_time >> 11),
				(pEntry->modify_time >> 5) & 0x3F,
				pEntry->modify_time & 0x1F,
				pEntry->starting_cluster,
				pEntry->file_size );
		*/
		snprintf( strBufDateTime, sizeof(strBufDateTime),
				"%04d-%02d-%02d %02d:%02d.%02d",
				1980 + (pEntry->modify_date >> 9),
				(pEntry->modify_date >> 5) & 0xF,
				pEntry->modify_date & 0x1F,
				(pEntry->modify_time >> 11),
				(pEntry->modify_time >> 5) & 0x3F,
				pEntry->modify_time & 0x1F ); 

		snprintf( strBufResult, sizeof(strBufResult), "Start(%08X) Cluster(%04X) %ld %s ",
				startFilePos, pEntry->starting_cluster, pEntry->file_size, strBufDateTime );
		strncat( (char*)&strBufResult, (char*)&strBufEntryName, sizeof(strBufEntryName) ); 
		printf( "%s\n", strBufResult );
	} // for()
}

static void __read_file_data_fat16(FILE* in, FILE* out, unsigned long fat_start,
		unsigned long data_start, unsigned long cluster_size, unsigned short cluster,
		unsigned long file_size) {
	{
		printf( "------------------------------\n" );
		printf( "__read_file_data_fat16()\n" );
		printf( "------------------------------\n" );
	}

	unsigned char buffer[4096];
	unsigned short cluster_orig = cluster;
	size_t bytes_read, bytes_to_read, file_left = file_size, cluster_left = cluster_size;

	// Go to first data cluster
	fseek( in, data_start + cluster_size * (cluster-2), SEEK_SET );

	printf( "-> data_start(%d) + cluster_size(%d) * (cluster(%d)-2) = %d(0x%X)\n",
			data_start, cluster_size, cluster, (data_start + cluster_size * (cluster-2)),
			(data_start + cluster_size * (cluster-2)) );
	printf( "-> Now data at 0x%X\n", ftell(in) );
	memset( (char*)&buffer, 0x00, sizeof(buffer) );
	out = fopen( "result.txt", "wb" );

	// Read until we run out of file or clusters
	//while ( file_left > 0 && cluster != 0xFFFF ) {
	while ( file_left > 0 ) {
		memset( (char*)&buffer, 0x00, sizeof(buffer) );
		bytes_to_read = sizeof( buffer );

		//printf( "bytes_to_read = %d, cluster_left = %d, file_left = %d\n",
		//		bytes_to_read, cluster_left, file_left );

		// don't read past the file or cluster end
		if ( bytes_to_read > file_left )
			bytes_to_read = file_left;
		if ( bytes_to_read > cluster_left )
			bytes_to_read = cluster_left;

		// read data from cluster, write to file
		bytes_read = fread( buffer, 1, bytes_to_read, in );
		fwrite( buffer, 1, bytes_read, out );
		printf( "Copied %d bytes\n", bytes_read );
		//printf( "Length = %d, Data = %s\n", strlen(buffer), buffer );

		// decrease byte counters for current cluster and whole file
		cluster_left -= bytes_read;
		file_left -= bytes_read;

		//printf( "cluster_left = %d, file_left = %d\n", cluster_left, file_left );

		// if we have read the whole cluster and read next cluster # from FAT
		if ( cluster_left == 0 ) {
			fseek( in, fat_start + cluster*2, SEEK_SET );
			fread( &cluster, 2, 1, in );

			/*
			[FAT16 Entry offset 0]
			-------------------------------------------------
			Cluster 0    1    | 2    3    4    5    6    x...
			-------------------------------------------------
					0001 0203 | 0405 0607 0809 0A0B 0C0D 0E0F
			-------------------------------------------------
			0000	F8FF FFFF | ffff ff07 ffff ffff ffff ff00 ................
			0010	0000 0000 | 0000 0000 0000 0000 0000 0000 ................
			####    #### #### | #### #### #### #### #### #### ................
			-------------------------------------------------

			[FAT16 Entry]
			----------------------------------
			FAT16			Description
			----------------------------------
			0000h			Unused cluster
			0001h			Reserved
			0002h ~ FFEFh	Next data cluster
			FFF0h ~ FFF6h	Reserved
			FFF7h			Detective cluster
			FFF8h - FFFFh	Last cluster
			----------------------------------
			*/


			/*
			//! TEST
			{
				//set below fseek() first
				//fseek( in, fat_start + ((cluster*2) - 4), SEEK_SET );
				unsigned char cls[8];
				int x;
				fread( &cls, 8, 1, in );
				printf( "End of cluster reached, next cluster %x\n", cls);
				for ( x = 0; x < 8; x++ )
					printf( "0x%x ", cls[x] );
				printf( "\n" );
			}
			*/

			// For one cluster only
			// Seek next data block (NOT next cluster)
			if ( (cluster == 0xFFFF) && (file_left > 0) ) {
				printf( "found 0xFFFF for just one cluster\n" );
				cluster = cluster_orig;	// NOTE: for one cluster only
				fseek( in, data_start + (cluster_size * (cluster_orig-2)) + ftell(out), SEEK_SET );
				cluster_left = cluster_size; // reset cluster byte counter

				//printf( "-> Now next data at 0x%X (fat_start(%d) + ((cluster(%d)*2)) = seek %d)\n",
				//		ftell(in), fat_start, cluster, (fat_start + cluster*2) );
				continue;
			}

			printf( "End of cluster reached, next cluster %d\n", cluster );
			fseek( in, data_start + (cluster_size * (cluster-2)), SEEK_SET );
			cluster_left = cluster_size; // reset cluster byte counter
		}
	}

	if ( out ) {
		fclose( out );
		out = NULL;
	}
}

void read_file_data_fat16(FILE* fp, const PartitionTable pt[4], const Fat16BootSector bs,
		const char* pStrFilename, const char* pStrExt) {
	{
		printf( "------------------------------\n" );
	 	printf( "read_file_data_fat16()\n" );
		printf( "------------------------------\n" );
	}

	int i;
	Fat16Entry fat16Entry;
	const Fat16Entry* pEntry = NULL;
	bool retAttribute = false;

	if ( !fp ) {
		fprintf( stderr, "[-] Open VFS file\n" );
		return;
	}

	if ( !pStrFilename ) {
		fprintf( stderr, "[-] Filename is NULL\n" );
		return;
	}


	printf( "Filename = %s, Ext = %s\n", pStrFilename, (pStrExt? pStrExt : "NULL") );

	fseek( fp, (bs.reserved_sectors-1 + bs.fat_size_sectors * bs.number_of_fats)
			* bs.sector_size, SEEK_SET );	// NOTE: MUST RESET; DOES NOT "SEEK_CUR"
	printf( "Now at 0x%X, sector size %d, FAT size %d sectors, %d FATs\n", 
		ftell(fp), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats );

	for ( i = 0; i < bs.root_dir_entries; i++ ) {
		const unsigned long startFilePos = ftell( fp );

		fread( &fat16Entry, sizeof(Fat16Entry), 1, fp );
		pEntry = &fat16Entry;
		retAttribute = false;

		switch ( pEntry->filename[0] ) {
			case 0x00:
				// Unused entry
				{
					//fprintf( stderr, "[-] Unused entry; SKIP\n" );
					//return;
				}
				break;
			case 0xE5:
				// Deleted
				{
					//printf( "Deleted file: [?%.7s.%.3s]\n", pEntry->filename+1, pEntry->ext );
					//return;
					continue;
				}
				break;
			case 0x05:
				// Filename is actually 0xE5
				{
					//printf( "File starting with 0xE5: [%c%.7s.%.3s]\n",
					//	0xE5, pEntry->filename+1, pEntry->ext );
				}
				break;
			case 0x2E:
				// Directory
				{
					//printf( "Directory: [%.8s.%.3s]\n", pEntry->filename, pEntry->ext );
					//printf( "%.8s.%.3s/\n", pEntry->filename, pEntry->ext );

					//fprintf( stderr, "[-] Directory\n" );
					continue;
				}
				break;
			default:
				// File
				{
					//printf( "File: [%.8s.%.3s]\n", pEtry->filename, pEntry->ext );
					//printf( "%.8s.%.3s\n", pEntry->filename, pEntry->ext );
				}
				break;
		}

		// Checks File Attributes
		{
			const unsigned char bufFileAttribute[8] = { 0x01, 0x02, 0x04, 0x08,
								0x10, 0x20, 0x40, 0x80 };
			int i;

			for ( i = 0; i < sizeof(bufFileAttribute); i++ ) {
				if ( pEntry->attributes == bufFileAttribute[i] ) {
					retAttribute = true;
					break;
				}
			}

			if ( retAttribute ) {
				//printf( "SRC: %.8s.%.3s | DST: %s.%s\n",
				//	pEntry->filename, pEntry->ext, pStrFilename, pStrExt );

				if ( !memcmp(pEntry->filename, pStrFilename, 8) ) {
					if ( !memcmp(pEntry->ext, pStrExt, 3) ) {
						printf( "Found file: %s.%s\n", pStrFilename, pStrExt );
						break;
					}
					else {
						retAttribute = false;
						continue;
					}
				}
				else {
					retAttribute = false;
					continue;
				}
			}
			else {
				//fprintf( stderr, "[-] File attributes not correct (0x%01X)\n",
				//		pEntry->attributes );
				continue;
			}
		}
	} // for()

	if ( retAttribute ) {
		unsigned long fat_start, root_start, data_start;
		int i;

		{
			for ( i = 0; i < 4; i++ ) {        
				if ( pt[i].partition_type == 0 ) {
					//printf( "[+] FAT16 filesystem found from partition %d, type = 0 (empty/unused)\n", i );
				}

				if ( pt[i].partition_type == 4 || pt[i].partition_type == 6 ||
					pt[i].partition_type == 14 ) {
					printf( "[+] FAT16 filesystem found from partition %d\n", i );
					break;
				}
			}
			
			if ( i == 4 ) {
				//fprintf( stderr, "[-] No FAT16 filesystem found, exiting...\n" );
				//return;
				fprintf( stderr, "[-] No FAT16 filesystem found, ignore...\n" );

				// One partition
				i = 0;
				fprintf( stderr, "Fixed set a partition number to 0(zero), start sector = %ld, length = %ld\n",
						pt[i].start_sector, pt[i].length_sectors );
			}
		}

		// Get boot sector for(ftell(fp))
		fseek( fp, 512 * pt[i].start_sector, SEEK_SET );
		fread( &bs, sizeof(Fat16BootSector), 1, fp );

		// Calculate start offsets of FAT, root directory and data
		fat_start = ftell(fp) + (bs.reserved_sectors-1) * bs.sector_size;
		// or
		// fat_start = (bs.reserved_sectors) * bs.sector_size;
		root_start = fat_start + ((bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size);
		data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);

		printf( "FAT start at %08X, root dir at %08X, data at %08X\n", 
			fat_start, root_start, data_start );

		printf( "cluster(%d) = sector_per_cluster(%d) * sector_size(%d)\n",
			(bs.sectors_per_cluster * bs.sector_size), bs.sectors_per_cluster, bs.sector_size );

		fseek( fp, root_start, SEEK_SET );
		__read_file_data_fat16( fp, NULL, fat_start, data_start, (bs.sectors_per_cluster * bs.sector_size),
				pEntry->starting_cluster, pEntry->file_size );
	}
	else {
		fprintf( stderr, "[-] No such file or directory\n" );
		return;
	}
}
// -------------------------------------------------------



#ifdef __REQ_MAIN_FUNCTION__
//! MAIN
// -------------------------------------------------------
int main(int argc, char* argv[]) {
	{
		printf( "------------------------------\n" );
		printf( "main()\n" );
		printf( "------------------------------\n" );
	}

	FILE* fpVFS = NULL;

	int i;
	PartitionTable pt[4];
	Fat16BootSector bs;


	if ( argc < 2 ) {
		fprintf( stderr, "Usage: %s [filename]\n", argv[0] );
		return -1;
	}

	fpVFS = fopen( argv[1], "rb" );

	if ( !fpVFS ) {
		fprintf( stderr, "main: [-] Open VFS file: %s\n", argv[1] );
		return -1;
	}

	for ( i = 0; i < 4; i++ ) {
		memset( (void*)&pt[i], 0x00, sizeof(PartitionTable) );
	}
	memset( (void*)&bs, 0x00, sizeof(Fat16BootSector) );

	//read_partition_table( fpVFS );
	read_boot_sector_fat16( fpVFS, pt, &bs );
	read_files_info_fat16( fpVFS, pt, bs, NULL, false );

	printf( "\nRoot directory read, now at 0x%X\n", ftell(fpVFS) );

	read_file_data_fat16( fpVFS, pt, bs, "README  ", "   " );
	//read_file_data_fat16( fpVFS, pt, bs, "README1 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README2 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README3 ", "TXT" );
	//read_file_data_fat16( fpVFS, pt, bs, "README4 ", "TXT" );

//! TEST
/*
	{
		unsigned long fat_start, root_start, data_start;

		// Fixed partition 0
		fseek( fpVFS, 512 * pt[0].start_sector, SEEK_SET );
		// performed in read_boot_sector_fat16()
		//fread( &bs, sizeof(Fat16BootSector), 1, fpVFS );

		// Calculate start offsets of FAT, root directory and data
		fat_start = ftell(fpVFS) + (bs.reserved_sectors-1) * bs.sector_size;
		root_start = fat_start + bs.fat_size_sectors * bs.number_of_fats * bs.sector_size;
		data_start = root_start + bs.root_dir_entries * sizeof(Fat16Entry);
		printf( "FAT start at %08X, root dir at %08X, data at %08X\n", 
			fat_start, root_start, data_start );

		//fseek( fpVFS, root_start, SEEK_SET );
		//__read_file_data_fat16( fpVFS, NULL, fat_start, data_start, bs.sectors_per_cluster * 
		//			bs.sector_size, entry.starting_cluster, entry.file_size );

	}
*/




	// Release
	if ( fpVFS ) {
		fclose( fpVFS );
		fpVFS = NULL;
	}


	return 0;
}
#endif	// #ifdef __REQ_MAIN_FUNCTION__
// -------------------------------------------------------

